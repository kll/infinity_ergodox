/*
Copyright 2016 Fred Sundvik <fsundvik@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "keyboard.h"
#include "action_layer.h"
#include "visualizer.h"
#include "host.h"
#include "hook.h"
#include "usb_main.h"
#include "suspend.h"
#include "serial_link/system/driver.h"
#include "chibios.h"

static host_driver_configuration_t driver_configuration = {
    .num_drivers = 2,
	.connection_delay = 50,
	.connection_timeout = 0,
	.try_connect_all = false,
    .drivers = {
        &chibios_usb_driver,
        &serial_link_driver,
    }
};

host_driver_configuration_t* hook_get_driver_configuration(void) {
    return &driver_configuration;
}

void hook_early_init(void) {
    visualizer_init();
}

void hook_keyboard_loop(void) {
    visualizer_update(default_layer_state, layer_state, host_keyboard_leds());
}

void hook_usb_suspend_entry(void) {
    visualizer_suspend();
}

void hook_usb_wakeup(void) {
    visualizer_resume();
}

void hook_usb_suspend_loop(void) {
    visualizer_update(default_layer_state, layer_state, host_keyboard_leds());
    /* Do this in the suspended state */
    suspend_power_down(); // on AVR this deep sleeps for 15ms
}

