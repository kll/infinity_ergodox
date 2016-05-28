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
#include <math.h>
#include <stdio.h>
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <glad/glad.h>
#pragma GCC diagnostic warning "-Wstrict-prototypes"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "keyboard_data.h"
#include "shader.h"

static GDisplay* lcd;
static GDisplay* led;
static GDisplay* temp;
static GDisplay* lightmap;
static font_t font;
static GLFWwindow* window;

typedef struct {
    GLuint program_id;
    GLuint view_projection_location;
    GLuint keyboard_position_location;
    GLuint element_color_location;
    GLuint texture_sampler_location;
}program_t;

static program_t keyboard_program;
static program_t lcd_program;
static program_t* current_program;

static GLuint keyboard_vertex_buffer;
static GLuint key_inner_vertex_buffer;
static GLuint key_inner_vertex_buffer_size;
static GLuint key_outer_vertex_buffer;
static GLuint key_outer_vertex_buffer_size;
static GLuint lcd_vertex_buffer;
static GLuint lcd_uv_buffer;

static GLuint lcd_texture;


static const int num_keys = sizeof(keys) / sizeof(keyinfo_t);
void error_callback(int error, const char* description)
{
    (void)error;
    fputs(description, stderr);
}

GDisplay* get_lcd_display(void) {
    return lcd;
}

GDisplay* get_led_display(void) {
    return led;
}
#define lightmap_size 128
float min_intensity = 5.0f / 256.0f;

static color_t intensity_map[256][lightmap_size][lightmap_size];

float hue2rgb (float p, float q, float t){
    if(t < 0.0f) t += 1.0f;
    if(t > 1.0f) t -= 1.0f;
    if(t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
    if(t < 1.0f/2.0f) return q;
    if(t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
    return p;
}

color_t hslToRgb(float h, float s, float l){
    float r, g, b;
    h = h / 360.0f;

    if(s == 0){
        r = g = b = l; // achromatic
    }else{
        float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
        float p = 2.0f * l - q;
        r = hue2rgb(p, q, h + 1.0f/3.0f);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.0f/3.0f);
    }
    return RGB2COLOR((int)roundf(r * 255), (int)roundf(g * 255), (int)roundf(b * 255));
}


static void create_keyboard_vertex_buffer(void) {
    glGenBuffers(1, &keyboard_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, keyboard_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(keyboard_vertex_data), keyboard_vertex_data, GL_STATIC_DRAW);
}

static GLfloat* create_quad(GLfloat* out, point* points) {
    *out++ = points[0].x;
    *out++ = points[0].y;
    *out++ = points[1].x;
    *out++ = points[1].y;
    *out++ = points[3].x;
    *out++ = points[3].y;
    *out++ = points[1].x;
    *out++ = points[1].y;
    *out++ = points[2].x;
    *out++ = points[2].y;
    *out++ = points[3].x;
    *out++ = points[3].y;
    return out;
}

static GLfloat* create_quad_from_box(GLfloat* out, GLfloat left, GLfloat top, GLfloat width, GLfloat height) {
    point p[] = {
            {left, top},
            {left + width, top},
            {left + width, top + height},
            {left, top + height}
    };
    return create_quad(out, p);
}

static void create_key_vertex_buffers(void) {
    GLfloat outer_vertex_data[2 * num_keys * 6];
    GLfloat* outer_vertex = outer_vertex_data;
    GLfloat inner_vertex_data[2 * num_keys * 6];
    GLfloat* inner_vertex = inner_vertex_data;
    const float keycap_border = keycap_size - keycap_inner_size;

    for (int i=0;i<num_keys;i++) {
        if (keys[i].size == 0)
            continue;
        MatrixFloat2D mat;
        gmiscMatrixFloat2DApplyRotation(&mat, NULL, keys[i].rot);
        gmiscMatrixFloat2DApplyTranslation(&mat, &mat, keys[i].pos.x, keys[i].pos.y);

        float width = keycap_size  * keys[i].size;
        float height = keycap_size;
        float half_width = width / 2.0f;
        float half_height = height / 2.0f;
        point points[] = {
            {-half_width, -half_height},
            {half_width, -half_height},
            {half_width, half_height},
            {-half_width, half_height},
        };
        gmiscMatrixFloat2DApplyToPoints(points, points, &mat, 4);
        outer_vertex = create_quad(outer_vertex, points);

        width -= keycap_border;
        height -= keycap_border;
        half_width = width / 2.0f;
        half_height = height / 2.0f;
        point points2[] = {
            {-half_width, -half_height},
            {half_width, -half_height},
            {half_width, half_height},
            {-half_width, half_height}
        };
        gmiscMatrixFloat2DApplyToPoints(points2, points2, &mat, 4);
        inner_vertex = create_quad(inner_vertex, points2);
    }
    key_outer_vertex_buffer_size = outer_vertex - outer_vertex_data;
    glGenBuffers(1, &key_outer_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, key_outer_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, key_outer_vertex_buffer_size * sizeof(GLfloat), outer_vertex_data, GL_STATIC_DRAW);

    key_inner_vertex_buffer_size = inner_vertex - inner_vertex_data;
    glGenBuffers(1, &key_inner_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, key_inner_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, key_inner_vertex_buffer_size * sizeof(GLfloat), inner_vertex_data, GL_STATIC_DRAW);
}

static void create_lcd_vertex_buffer(void) {
    GLfloat vertex_data[2 * 6 * 6];
    GLfloat* vertex = vertex_data;

    // The whole transparent area
    vertex = create_quad_from_box(vertex, lcd_pos.x, lcd_pos.y, lcd_size.x, lcd_size.y);
    // The LCD lit area
    vertex = create_quad_from_box(vertex,
        lcd_pos.x + lcd_transparent_border_width,
        lcd_pos.y + lcd_transparent_border_width,
        lcd_size.x - 2 * lcd_transparent_border_width,
        lcd_lit_area_height
    );
    // The black box at the bottom
    vertex = create_quad_from_box(vertex,
        lcd_pos.x + lcd_transparent_border_width,
        lcd_pos.y + lcd_transparent_border_width + lcd_lit_area_height,
        lcd_size.x - 2 * lcd_transparent_border_width,
        lcd_size.y - 2 * lcd_transparent_border_width - lcd_lit_area_height
    );
    // The black border
    vertex = create_quad_from_box(vertex,
        lcd_pos.x + lcd_transparent_border_width,
        lcd_pos.y + lcd_transparent_border_width,
        lcd_size.x - 2 * lcd_transparent_border_width,
        lcd_lit_area_height - lcd_black_border_to_black_box_dist
    );
    // The LCD lit area again, but inside the black border only
    int inner_lit_area_height = lcd_lit_area_height - 2 * lcd_black_border_width - lcd_black_border_to_black_box_dist;
    vertex = create_quad_from_box(vertex,
        lcd_pos.x + lcd_transparent_border_width + lcd_black_border_width,
        lcd_pos.y + lcd_transparent_border_width + lcd_black_border_width,
        lcd_size.x - 2 * lcd_transparent_border_width - 2 * lcd_black_border_width,
        inner_lit_area_height
    );
    // The main LCD pixel area
    vertex = create_quad_from_box(vertex,
        lcd_pos.x + lcd_size.x / 2 - lcd_pixel_area_size.x / 2,
        lcd_pos.y + lcd_transparent_border_width + lcd_black_border_width +
            inner_lit_area_height / 2  - lcd_pixel_area_size.y / 2,
        lcd_pixel_area_size.x,
        lcd_pixel_area_size.y
    );

    glGenBuffers(1, &lcd_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, lcd_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);

    glGenBuffers(1, &lcd_uv_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, lcd_uv_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lcd_uv_data), lcd_uv_data, GL_STATIC_DRAW);
}

static void create_lcd_texture(void) {
    gdispGClear(lcd, White);
    glGenTextures(1, &lcd_texture);
    glBindTexture(GL_TEXTURE_2D, lcd_texture);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, 128, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, gdispPixmapGetBits(lcd));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void setup_variables(void) {
    GLuint program_id = keyboard_program.program_id;
    keyboard_program.view_projection_location = glGetUniformLocation(program_id, "view_projection");
    keyboard_program.keyboard_position_location = glGetUniformLocation(program_id, "keyboard_location");
    keyboard_program.element_color_location = glGetUniformLocation(program_id, "element_color");

    program_id = lcd_program.program_id;
    lcd_program.view_projection_location = glGetUniformLocation(program_id, "view_projection");
    lcd_program.keyboard_position_location = glGetUniformLocation(program_id, "keyboard_location");
    lcd_program.texture_sampler_location = glGetUniformLocation(program_id, "texture_sampler");
    lcd_program.element_color_location = glGetUniformLocation(program_id, "element_color");
}

int main(void) {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(GDISP_SCREEN_WIDTH, GDISP_SCREEN_HEIGHT,
            "Ergodox Emulator", NULL, NULL);
    if(!window) {
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    if(!gladLoadGL()) {
        exit(EXIT_FAILURE);
    };
    printf("OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);
    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);
    keyboard_program.program_id = load_program("keyboard.vertexshader", "keyboard.fragmentshader");
    lcd_program.program_id = load_program("lcd.vertexshader", "lcd.fragmentshader");
    setup_variables();

    create_keyboard_vertex_buffer();
    create_key_vertex_buffers();
    create_lcd_vertex_buffer();

    gfxInit();
    lcd = gdispPixmapCreate(lcd_pixel_area_size.x, lcd_pixel_area_size.y);
    led = gdispPixmapCreate(led_size.x, led_size.y);
    temp = gdispPixmapCreate(GDISP_SCREEN_WIDTH, GDISP_SCREEN_HEIGHT);

    create_lcd_texture();
    glfwMakeContextCurrent(NULL);

    lightmap = gdispPixmapCreate(lightmap_size * 8, lightmap_size * 8);

    font = gdispOpenFont("DejaVuSansBold12");

    const int half_lightmap = lightmap_size / 2;

    float alpha = 0.001f;
    float radius = lightmap_size / 2;
    float beta = 1.0f / (radius * radius * min_intensity);
    for(int i=0;i<256;i++) {
        for (int j = 0;j < lightmap_size;j++) {
            for (int k=0;k < lightmap_size;k++) {
                float a = j - half_lightmap;
                float b = k - half_lightmap;
                float dist = a*a + b*b;
                dist = sqrtf(dist);
                float att = 1.0f / (1.0f + alpha*dist + beta*dist*dist);
                float new_i = i * att;
                color_t led_color = hslToRgb(200, 0.75f, new_i / 255.0f);
                if (new_i < min_intensity * 255.0f) {
                   led_color = RGB2COLOR(0, 0, 0);
                }
                intensity_map[i][j][k] = led_color;
            }
        }
    }

    gdispSetDisplay(lcd);

    // Initialize and clear the display
    visualizer_init();

    uint8_t default_layer_state = 0;
    uint8_t layer_state = 0;
    uint8_t leds = 0;

    while (!glfwWindowShouldClose(window)) {
        visualizer_update(default_layer_state, layer_state, leds);
        gfxSleepMilliseconds(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}

void lcd_backlight_hal_init(void) {
}

void lcd_backlight_hal_color(uint16_t r, uint16_t g, uint16_t b) {
    (void)r;
    (void)g;
    (void)b;
}

static inline int add_color_component(int a, int b) {
    int res = a + b;
    return res > 255 ? 255 : res;
}

static inline int add_colors(color_t a, color_t b) {
    int red = add_color_component(RED_OF(a), RED_OF(b));
    int green = add_color_component(GREEN_OF(a), GREEN_OF(b));
    int blue = add_color_component(BLUE_OF(a), BLUE_OF(b));
    return RGB2COLOR(red, green, blue);
}

static void setup_view_projection(void) {
    const GLfloat left = 0;
    const GLfloat right = GDISP_SCREEN_WIDTH;
    const GLfloat top = 0;
    const GLfloat bottom = GDISP_SCREEN_HEIGHT;
    const GLfloat far_val = 10.0f;
    const GLfloat near_val = -10.0f;
    const GLfloat a = 2.0f / (right-left);
    const GLfloat b = -(right + left) / (right - left);
    const GLfloat c = 2.0f / (top - bottom);
    const GLfloat d = -(top + bottom) / (top - bottom);
    const GLfloat e = -2.0f / (far_val - near_val);
    const GLfloat f = -(far_val + near_val) / (far_val - near_val);

    const GLfloat view_projection[4*4] = {
        a,    0.0f, 0.0f, 0.0f,
        0.0f, c,    0.0f, 0.0f,
        0.0f, 0.0f, e,    0.0f,
        b,    d,    f,    1.0f
    };
    glUniformMatrix4fv(current_program->view_projection_location, 1, GL_FALSE, view_projection);
}

static void setup_keyboard_location(int keyboard_x, int keyboard_y) {
    const GLfloat matrix[4*4] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        keyboard_x, keyboard_y, 0.0f, 1.0f
    };
    glUniformMatrix4fv(current_program->keyboard_position_location, 1, GL_FALSE, matrix);
}

static void use_program(program_t* program) {
    glUseProgram(program->program_id);
    current_program = program;
    setup_view_projection();
    setup_keyboard_location(keyboard_pos.x, keyboard_pos.y);
    glDisable(GL_DEPTH_TEST);
}

static void draw_main_keyboard_area(void) {
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, keyboard_vertex_buffer);
    glVertexAttribPointer(
       0,                  // attribute
       2,                  // size
       GL_FLOAT,           // type
       GL_FALSE,           // normalized?
       0,                  // stride
       (void*)0            // array buffer offset
    );
    float r = RED_OF(main_keyboard_area_color) / 255.0f;
    float g = GREEN_OF(main_keyboard_area_color) / 255.0f;
    float b = BLUE_OF(main_keyboard_area_color) / 255.0f;
    glUniform3f(current_program->element_color_location, r, g, b);
    glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(keyboard_vertex_data) / sizeof(GLfloat) / 2);
    glDisableVertexAttribArray(0);
}

static void draw_leds(int keyboard_x, int keyboard_y) {

    //memset(final_intensity, 0, sizeof(final_intensity));
    for (int i=0;i<num_keys;i++) {
        if (keys[i].size == 0)
            continue;
        MatrixFloat2D rot;
        gmiscMatrixFloat2DApplyRotation(&rot, NULL, keys[i].rot);
        int mid_x = keys[i].pos.x + keyboard_x;
        int mid_y = keys[i].pos.y + keyboard_y;
        point points[] = {
            {0, -20}
        };
        gmiscMatrixFloat2DApplyToPoints(points, points, &rot, 1);

        int row = i / led_size.x;
        int col = i - row * led_size.x;
        int luma = LUMA_OF(gdispGGetPixelColor(led, col, row));
        //color_t led_color = hslToRgb(200, 0.75f, 0.4f + 0.35f * luma / 255.0f);
        int led_x = points[0].x + mid_x;
        int led_y = points[0].y + mid_y;

        int start_x = led_x - lightmap_size / 2;
        int start_y = led_y - lightmap_size / 2;
        for (int a = 0; a < lightmap_size; ++a) {
            int y = start_y + a;
            for (int b = 0; b < lightmap_size; ++b) {
                int x = start_x + b;
                color_t src_color = gdispGetPixelColor(x, y);
                color_t led_color = intensity_map[luma][a][b];
                color_t final_color = add_colors(led_color, src_color);
                gdispDrawPixel(x, y, final_color);
            }
        }
    }
}

static void draw_triangles_with_offset(GLuint vertex_buffer, GLuint vertex_buffer_size, GLuint offset, color_t color) {
    offset = offset * sizeof(GLfloat) * 2;
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glVertexAttribPointer(
       0,                  // attribute
       2,                  // size
       GL_FLOAT,           // type
       GL_FALSE,           // normalized?
       0,                  // stride
       (void*)(intptr_t)offset           // array buffer offset
    );
    float r = RED_OF(color) / 255.0f;
    float g = GREEN_OF(color) / 255.0f;
    float b = BLUE_OF(color) / 255.0f;
    glUniform3f(current_program->element_color_location, r, g, b);
    glDrawArrays(GL_TRIANGLES, 0, vertex_buffer_size);
    glDisableVertexAttribArray(0);
}

static void draw_lcd_texture(GLuint offset, color_t color) {
    use_program(&lcd_program);

    offset = offset * sizeof(GLfloat) * 2;
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, lcd_vertex_buffer);
    glVertexAttribPointer(
       0,                  // attribute
       2,                  // size
       GL_FLOAT,           // type
       GL_FALSE,           // normalized?
       0,                  // stride
       (void*)(intptr_t)offset           // array buffer offset
    );
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, lcd_uv_buffer);
    glVertexAttribPointer(
        1,                                // attribute.
        2,                                // size : U+V => 2
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
    );
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lcd_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lcd_pixel_area_size.x, lcd_pixel_area_size.y, GL_RGBA, GL_UNSIGNED_BYTE, gdispPixmapGetBits(lcd));
    glUniform1i(current_program->texture_sampler_location, 0);
    float r = RED_OF(color) / 255.0f;
    float g = GREEN_OF(color) / 255.0f;
    float b = BLUE_OF(color) / 255.0f;
    glUniform3f(current_program->element_color_location, r, g, b);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void draw_triangles(GLuint vertex_buffer, GLuint vertex_buffer_size, color_t color) {
    draw_triangles_with_offset(vertex_buffer, vertex_buffer_size, 0, color);
}

static void draw_keycaps(void) {
    draw_triangles(key_outer_vertex_buffer, key_outer_vertex_buffer_size, keycap_outer_color);
    draw_triangles(key_inner_vertex_buffer, key_inner_vertex_buffer_size, keycap_inner_color);
}

static void draw_lcd(void) {
    // The whole transparent area
    draw_triangles(lcd_vertex_buffer, 6, lcd_transparent_color);
    // The LCD lit area
    draw_triangles_with_offset(lcd_vertex_buffer, 6, 6, lcd_lit_color);
    // The black box at the bottom
    draw_triangles_with_offset(lcd_vertex_buffer, 6, 12, lcd_black_color);
    // The black border
    draw_triangles_with_offset(lcd_vertex_buffer, 6, 18, lcd_black_color);
    // The LCD lit area again, but inside the black border only
    draw_triangles_with_offset(lcd_vertex_buffer, 6, 24, lcd_lit_color);
    draw_lcd_texture(30, lcd_pixel_area_color);
}

void draw_emulator(void) {
    systemticks_t start_draw = gfxSystemTicks();

    if (!glfwGetCurrentContext()) {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
    }

    gdispSetDisplay(temp);
    glClearColor(
       RED_OF(background_color) / 255.0f,
       GREEN_OF(background_color) / 255.0f,
       BLUE_OF(background_color) / 255.0f,
       1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    use_program(&keyboard_program);

    draw_main_keyboard_area();
    systemticks_t after_draw_keyboard = gfxSystemTicks();

    draw_keycaps();
    systemticks_t after_draw_keycaps = gfxSystemTicks();

    draw_leds(keyboard_pos.x, keyboard_pos.y);
    systemticks_t after_draw_leds = gfxSystemTicks();

    draw_lcd();
    systemticks_t after_draw_lcd = gfxSystemTicks();

    gdispFlush();
    systemticks_t after_flush = gfxSystemTicks();
    systemticks_t total_time = after_flush - start_draw;
    char buffer[256];
    sprintf(buffer, "Frame time %i", total_time);
    gdispDrawString(0, 700, buffer, font, White);
    sprintf(buffer, "Keyboard %i, keycaps %i, leds %i, lcd %i",
            after_draw_keyboard - start_draw,
            after_draw_keycaps - after_draw_keyboard,
            after_draw_leds - after_draw_keycaps,
            after_draw_lcd - after_draw_leds
            );
    gdispDrawString(0, 715, buffer, font, White);
    gdispSetDisplay(gdispGetDisplay(0));
    gdispBlitArea(0, 0, GDISP_SCREEN_WIDTH, GDISP_SCREEN_HEIGHT, gdispPixmapGetBits(temp));
    systemticks_t after_blit = gfxSystemTicks();
    sprintf(buffer, "Frame time with flush %i", after_blit - start_draw);
    gdispDrawString(0, 730, buffer, font, White);
    gdispFlush();
    gdispSetDisplay(lcd);
    glfwSwapBuffers(window);
}
