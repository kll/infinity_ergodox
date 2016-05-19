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

typedef struct {
    point pos;
    float size;
} key_t;

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

    key_t keys[] = {
         {{71.12f, 160.85f}, 1.5f},
         {{166.37f, 160.85f}, 1.0f},
         {{242.57f, 148.15f}, 1.0f},
         {{318.77f, 141.8f}, 1.0f},
         {{394.97f, 148.16f}, 1.0f},
         {{471.17f, 153.24f}, 1.0f},
         {{547.37f, 153.24f}, 1.0f},

         {{71.12f, 237.05f}, 1.5f},
         {{166.37f, 237.05f}, 1.0f},
         {{242.57f, 224.35f}, 1.0f},
         {{318.77f, 218.0f}, 1.0f},
         {{394.97f, 224.36f}, 1.0f},
         {{471.17f, 229.44f}, 1.0f},
         {{547.37f, 248.49f}, 1.0f},

         {{71.12f, 313.25f}, 1.5f},
         {{166.37f, 313.25f}, 1.0f},
         {{242.57f, 300.55f}, 1.0f},
         {{318.77f, 294.2f}, 1.0f},
         {{394.97f, 300.56f}, 1.0f},
         {{471.17f, 305.44f}, 1.0f},

         {{71.12f, 389.45f}, 1.5f},
         {{166.37f, 389.45f}, 1.0f},
         {{242.57f, 376.65f}, 1.0f},
         {{318.77f, 370.4f}, 1.0f},
         {{394.97f, 376.76f}, 1.0f},
         {{471.17f, 381.64f}, 1.0f},
         {{547.37f, 362.79f}, 1.0f},

         {{90.17f, 465.65f}, 1.0f},
         {{166.37f, 465.65f}, 1.0f},
         {{242.57f, 452.85f}, 1.0f},
         {{318.77f, 446.6f}, 1.0f},
         {{394.97f, 452.96f}, 1.0f},
         {{629.09f, 443.73f}, 1.0f},
         {{698.15f, 477.77f}, 1.0f},

         {{665.94f, 535.0f}, 1.0f},

         {{499.89f, 505.11f}, 2.0f},
         {{580.78f, 537.32f}, 2.0f},
         {{633.735f, 604.06f}, 1.0f},

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

    const int num_keys = sizeof(keys) / sizeof(key_t);

    int keycap_size = 73.66f; // 7.25 * 2.54 * 4
    int keycap_inner_size = 50.8f; // 7.25 * 2.54 * 4

    for (int i=0;i<num_keys;i++) {
        int mid_x = keys[i].pos.x;
        int mid_y = keys[i].pos.y;
        int width = keycap_size  * keys[i].size;
        int x = mid_x - width / 2;
        int y = mid_y - keycap_size / 2;
        gdispFillArea(x, y, width, keycap_size, Black);
        width = keycap_inner_size  * keys[i].size;
        x = mid_x - width / 2;
        y = mid_y - keycap_inner_size / 2;
        gdispFillArea(x, y, width, keycap_inner_size, HTML2COLOR(0x202020));
    }

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
