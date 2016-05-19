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
static GDisplay* temp;

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
    temp = gdispPixmapCreate(GDISP_SCREEN_WIDTH, GDISP_SCREEN_HEIGHT);
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

void draw_emulator(void) {
    gdispSetDisplay(temp);
    gdispClear(HTML2COLOR(0x8B4513));
    point points[] = {
        {26, 0},
        {211, 0},
        {244, 7},
        {337, 50},
        {369, 58},
        {598, 58},
        {624, 83},
        {624, 336},
        {638, 359},
        {778, 424},
        {790, 458},
        {685, 683},
        {651, 695},
        {338, 550},
        {328, 547},
        {26, 547},
        {0, 522},
        {0, 26},
    };
    // Main keyboard area
    gdispFillConvexPoly(10, 10, points, sizeof(points) / sizeof(point), HTML2COLOR(0xDADADA));
    // The LCD area
    gdispFillArea(36, 19, 173, 102, Green);
    // The black border in the LCD screen
    gdispDrawBox(40, 23, 165, 67, Black);
    gdispDrawBox(41, 24, 163, 65, Black);
    // The black area at the bottom of the LCD
    gdispFillArea(40, 91, 165, 28, Black);
    // The actual LCD screen contents
    gdispBlitArea(58, 38, 128, 32, gdispPixmapGetBits(lcd));
    // The leds
    gdispBlitArea(10, 200, 7, 7, gdispPixmapGetBits(led));

    gdispFlush();
    gdispSetDisplay(gdispGetDisplay(0));
    gdispBlitArea(0, 0, GDISP_SCREEN_WIDTH, GDISP_SCREEN_HEIGHT, gdispPixmapGetBits(temp));
    gdispFlush();
    gdispSetDisplay(lcd);
}
