/*
Fred Sundvik <fsundvik@gmail.com>

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

#include "gfx.h"
#include "visualizer.h"

static GDisplay* lcd;
static GDisplay* led;

GDisplay* get_lcd_display(void) {
    return lcd;
}

GDisplay* get_led_display(void) {
    return led;
}

int main(void) {

    gfxInit();
    lcd = gdispPixmapCreate(128, 32);
    led = gdispPixmapCreate(7, 7);
    gdispSetDisplay(lcd);

    // Initialize and clear the display
    visualizer_init();

    uint8_t default_layer_state = 0;
    uint8_t layer_state = 0;
    uint8_t leds = 0;

    while(TRUE) {
        visualizer_update(default_layer_state, layer_state, leds);
        gfxSleepMilliseconds(1);
    }
}
