/*
 * Copyright © 2016 Canonical Ltd.
 * Copyright © 2018 Gemian
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 * Modified by: Adam Boardman <adamboardman@gmail.com>
 */

#include "dbus_bus.h"

#include "src/adapters/dbus_connection_handle.h"
#include "src/adapters/dbus_event_loop.h"
#include "src/adapters/scoped_g_error.h"
#include "src/adapters/x11_display.h"

#include "duration_of.h"
#include "fake_log.h"
#include "fake_shared.h"
#include "spin_wait.h"
#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <tests/common/fake_exec.h>

namespace rt = repowerd::test;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

char const* const x11_display_service_introspection = R"(
<node>
  <interface name='org.thinkglobally.Gemian.Display'>
    <method name='TurnOn'>
      <arg type='s' name='filter' direction='in'/>
    </method>
    <method name='TurnOff'>
      <arg type='s' name='filter' direction='in'/>
    </method>
    <property name='ActiveOutputs' type='(ii)' access='read'/>
  </interface>
</node>)";

class FakeX11DisplayDBusService
{
public:
    FakeX11DisplayDBusService(std::string const& bus_address) : dbus_connection{bus_address}, dbus_event_loop{"FakeX11Display"} {
        dbus_connection.request_name("org.thinkglobally.Gemian.Display");
        x11_display_handler_registation = dbus_event_loop.register_object_handler(
                dbus_connection,
                "/org/thinkglobally/Gemian/Display",
                x11_display_service_introspection,
                [this](
                        GDBusConnection *connection,
                        gchar const *sender,
                        gchar const *object_path,
                        gchar const *interface_name,
                        gchar const *method_name,
                        GVariant *parameters,
                        GDBusMethodInvocation *invocation) {
                    dbus_method_call(
                            connection, sender, object_path, interface_name,
                            method_name, parameters, invocation);
                });
    }

    void emit_active_outputs(int internal, int external)
    {
        this->internal = internal;
        this->external = external;

        g_dbus_connection_emit_signal(
                dbus_connection,
                nullptr,
                "/org/thinkglobally/Gemian/Display",
                "org.freedesktop.DBus.Properties",
                "PropertiesChanged",
                g_variant_new_parsed(
                        "(@s 'org.thinkglobally.Gemian.Display',"
                        " @a{sv} { 'ActiveOutputs' : <(%i,%i)> },"
                        " @as [])",
                        internal,
                        external),
                nullptr);
    }

    struct MockDBusCalls
    {
        MOCK_METHOD1(turn_on, void(std::string const&));
        MOCK_METHOD1(turn_off, void(std::string const&));
    };

    testing::NiceMock<MockDBusCalls> mock_dbus_calls;

private:
    void dbus_method_call(
            GDBusConnection* /*connection*/,
            gchar const* /*sender*/,
            gchar const* /*object_path*/,
            gchar const* /*interface_name*/,
            gchar const* method_name_cstr,
            GVariant* parameters,
            GDBusMethodInvocation* invocation)
    {
        std::string const method_name{method_name_cstr ? method_name_cstr : ""};
        GVariant* reply{nullptr};

        if (method_name == "TurnOn")
        {
            char const* filter{""};
            g_variant_get(parameters, "(&s)", &filter);

            mock_dbus_calls.turn_on(filter);
        }
        else if (method_name == "TurnOff")
        {
            char const* filter{""};
            g_variant_get(parameters, "(&s)", &filter);

            mock_dbus_calls.turn_off(filter);
        }
        else if (method_name == "Get")
        {
            reply = g_variant_new_parsed("(<(%i,%i)>,)", internal.load(), external.load());
        }
        printf("method_name: %s", method_name.c_str());

        g_dbus_method_invocation_return_value(invocation, reply);
    }

    repowerd::DBusConnectionHandle dbus_connection;
    repowerd::DBusEventLoop dbus_event_loop;
    repowerd::HandlerRegistration x11_display_handler_registation;
    std::atomic<int> internal{0};
    std::atomic<int> external{0};
    std::string name{};
};

struct AX11Display : testing::Test
{
    void wait_for_have_environment(repowerd::X11Display& _x11_display, std::string value)
    {
        auto const result = rt::spin_wait_for_condition_or_timeout(
            [&] { return _x11_display.active_username() == value; },
                default_timeout);
        if (!result)
        {
            throw std::runtime_error(
                    "Timeout while waiting for active_username=" + value);
        }
    }

    rt::DBusBus bus;
    rt::FakeLog fake_log;
    rt::FakeExec fake_exec;
    repowerd::X11Display x11_display{
        rt::fake_shared(fake_log),
        rt::fake_shared(fake_exec),
        bus.address()};

    std::chrono::seconds const default_timeout{3};
};

}

TEST_F(AX11Display, turn_on_request)
{
    x11_display.set_active_username("gemini");
    x11_display.turn_on(repowerd::DisplayPowerControlFilter::all);

    EXPECT_TRUE(fake_log.contains_line({"turn_on"}));
    EXPECT_TRUE(fake_exec.contains_line({"/bin/su - gemini -c \"DISPLAY=:0 xrandr --output hwcomposer --auto; DISPLAY=:0 xset dpms force on\""}));
}

TEST_F(AX11Display, turn_off_lid_open_request)
{
    x11_display.set_active_username("gemini");
    x11_display.turn_off(repowerd::DisplayPowerControlFilter::all, false);

    EXPECT_TRUE(fake_log.contains_line({"turn_off"}));
    EXPECT_TRUE(fake_exec.contains_line({"/bin/su - gemini -c \"DISPLAY=:0 xset dpms force off\""}));
}

TEST_F(AX11Display, turn_off_lid_closed_request)
{
    x11_display.set_active_username("gemini");
    x11_display.turn_off(repowerd::DisplayPowerControlFilter::all, true);

    EXPECT_TRUE(fake_log.contains_line({"turn_off"}));
    EXPECT_TRUE(fake_exec.contains_line({"/bin/su - gemini -c \"DISPLAY=:0 xrandr --output hwcomposer --off; DISPLAY=:0 xset dpms force off\""}));
}
