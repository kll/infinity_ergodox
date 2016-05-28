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

static GDisplay* lcd;
static GDisplay* led;
static GDisplay* temp;
static GDisplay* lightmap;
static font_t font;
static GLFWwindow* window;
static GLuint keyboard_shader;
static GLuint lcd_shader;

typedef struct {
    GLuint view_projection_location;
    GLuint keyboard_position_location;
    GLuint element_color_location;
    GLuint texture_sampler_location;
}locations_t;

static locations_t keyboard_locations;
static locations_t lcd_locations;
static locations_t* locations;

static GLuint keyboard_vertex_buffer;
static GLuint key_inner_vertex_buffer;
static GLuint key_inner_vertex_buffer_size;
static GLuint key_outer_vertex_buffer;
static GLuint key_outer_vertex_buffer_size;
static GLuint lcd_vertex_buffer;
static GLuint lcd_uv_buffer;

static GLuint lcd_texture;

typedef struct {
    point pos;
    float size;
    float rot;
} keyinfo_t;

static const GLfloat keyboard_vertex_data[] = {
    25, 0,
    210, 0,
    242, 7,
    334, 50,
    366, 57,
    593, 57,
    619, 83,
    619, 333,
    633, 356,
    772, 421,
    784, 454,
    678, 678,
    646, 690,
    325, 543,
    25, 543,
    0, 517,
    0, 25,
};

keyinfo_t keys[] = {
     {{71.12f, 160.85f}, 1.5f, 0.0f},
     {{166.37f, 160.85f}, 1.0f, 0.0f},
     {{242.57f, 148.15f}, 1.0f, 0.0f},
     {{318.77f, 141.8f}, 1.0f, 0.0f},
     {{394.97f, 148.16f}, 1.0f, 0.0f},
     {{471.17f, 153.24f}, 1.0f, 0.0f},
     {{547.37f, 153.24f}, 1.0f, 0.0f},

     {{71.12f, 237.05f}, 1.5f, 0.0f},
     {{166.37f, 237.05f}, 1.0f, 0.0f},
     {{242.57f, 224.35f}, 1.0f, 0.0f},
     {{318.77f, 218.0f}, 1.0f, 0.0f},
     {{394.97f, 224.36f}, 1.0f, 0.0f},
     {{471.17f, 229.44f}, 1.0f, 0.0f},
     {{547.37f, 248.49f}, 1.5f, 270.0f},

     {{71.12f, 313.25f}, 1.5f, 0.0f},
     {{166.37f, 313.25f}, 1.0f, 0.0f},
     {{242.57f, 300.55f}, 1.0f, 0.0f},
     {{318.77f, 294.2f}, 1.0f, 0.0f},
     {{394.97f, 300.56f}, 1.0f, 0.0f},
     {{471.17f, 305.44f}, 1.0f, 0.0f},
     {{0, 0}, 0, 0},

     {{71.12f, 389.45f}, 1.5f, 0.0f},
     {{166.37f, 389.45f}, 1.0f, 0.0f},
     {{242.57f, 376.65f}, 1.0f, 0.0f},
     {{318.77f, 370.4f}, 1.0f, 0.0f},
     {{394.97f, 376.76f}, 1.0f, 0.0f},
     {{471.17f, 381.64f}, 1.0f, 0.0f},
     {{547.37f, 362.79f}, 1.5f, 270.0f},

     {{90.17f, 465.65f}, 1.0f, 0.0f},
     {{166.37f, 465.65f}, 1.0f, 0.0f},
     {{242.57f, 452.85f}, 1.0f, 0.0f},
     {{318.77f, 446.6f}, 1.0f, 0.0f},
     {{394.97f, 452.96f}, 1.0f, 0.0f},
     {{629.09f, 433.73f}, 1.0f, 65.0f},
     {{698.15f, 465.94f}, 1.0f, 65.0f},

     {{0, 0}, 0, 0},
     {{0, 0}, 0, 0},
     {{0, 0}, 0, 0},
     {{0, 0}, 0, 0},
     {{0, 0}, 0, 0},
     {{0, 0}, 0, 0},
     {{665.94f, 535.0f}, 1.0f, 65.0f},

     {{0, 0}, 0, 0},
     {{0, 0}, 0, 0},
     {{0, 0}, 0, 0},
     {{0, 0}, 0, 0},
     {{512.72f, 505.11f}, 2.0f, 245.0f},
     {{580.78f, 537.32f}, 2.0f, 245.0f},
     {{633.73f, 604.06f}, 1.0f, 65.0f},
};

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

void print_compile_status(const char* file, GLuint id) {
    GLint result = GL_FALSE;
    int info_log_length;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
    if ( info_log_length > 0 ){
        char* error_message = malloc(info_log_length + 1);
        glGetShaderInfoLog(id, info_log_length, NULL, error_message);
        printf("%s %s\n", file, error_message);
        free(error_message);
    }
}

void print_link_status(GLuint id) {
    GLint result = GL_FALSE;
    int info_log_length;
    glGetProgramiv(id, GL_LINK_STATUS, &result);
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
    if ( info_log_length > 0 ){
        char* error_message = malloc(info_log_length + 1);
        glGetShaderInfoLog(id, info_log_length, NULL, error_message);
        printf("%s\n", error_message);
        free(error_message);
    }
}

GLuint load_shader(const char* path) {
    FILE* f = fopen( path, "r");
    fseek( f, 0, SEEK_END );
    int filesize = ftell( f );
    rewind( f );
    char* buffer = (char*)malloc(filesize+1);
    fread(buffer, 1, filesize, f);
    buffer[filesize] = '\0';
    fclose(f);

    GLenum type = 0;
    if (strstr(path, "vertexshader")) {
       type = GL_VERTEX_SHADER;
    }
    else if(strstr(path, "fragmentshader")) {
        type = GL_FRAGMENT_SHADER;
    }
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, (const char**)&buffer, NULL);
    glCompileShader(id);

    print_compile_status(path, id);

    free(buffer);
    // Check Vertex Shader

    return id;
}

GLuint load_program(const char* vertex_shader, const char* fragment_shader) {
    GLuint vertex_shader_id = load_shader(vertex_shader);
    GLuint fragment_shader_id = load_shader(fragment_shader);
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);
    print_link_status(program_id);
    glDetachShader(program_id, vertex_shader_id);
    glDetachShader(program_id, fragment_shader_id);

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    return program_id;
}

void create_keyboard_vertex_buffer(void) {
    glGenBuffers(1, &keyboard_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, keyboard_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(keyboard_vertex_data), keyboard_vertex_data, GL_STATIC_DRAW);
}

GLfloat* create_quad(GLfloat* out, point* points) {
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

GLfloat* create_quad_from_box(GLfloat* out, GLfloat left, GLfloat top, GLfloat width, GLfloat height) {
    point p[] = {
            {left, top},
            {left + width, top},
            {left + width, top + height},
            {left, top + height}
    };
    return create_quad(out, p);
}

void create_key_vertex_buffers(void) {
    const float keycap_size = 73.66f; // 7.25 * 2.54 * 4
    const float keycap_inner_size = 50.8f;
    const float keycap_border = keycap_size - keycap_inner_size;

    GLfloat outer_vertex_data[2 * num_keys * 6];
    GLfloat* outer_vertex = outer_vertex_data;
    GLfloat inner_vertex_data[2 * num_keys * 6];
    GLfloat* inner_vertex = inner_vertex_data;

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

void create_lcd_vertex_buffer(void) {
    int lcd_x = 32;
    int lcd_y = 19;
    GLfloat vertex_data[2 * 6 * 6];
    GLfloat* vertex = vertex_data;

    vertex = create_quad_from_box(vertex, lcd_x, lcd_y, 173, 102);
    vertex = create_quad_from_box(vertex, 4 + lcd_x, 4 + lcd_y, 165, 67);
    vertex = create_quad_from_box(vertex, 4 + lcd_x, 72 + lcd_y, 165, 28);
    vertex = create_quad_from_box(vertex, 6 + lcd_x, 6 + lcd_y, 161, 63);
    vertex = create_quad_from_box(vertex, 4 + lcd_x, 71 + lcd_y, 165, 1);
    vertex = create_quad_from_box(vertex, 23 + lcd_x, 21 + lcd_y, 128, 32);

    glGenBuffers(1, &lcd_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, lcd_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);


    static const GLfloat uv_data[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
    };
    glGenBuffers(1, &lcd_uv_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, lcd_uv_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv_data), uv_data, GL_STATIC_DRAW);
}

void create_lcd_texture(void) {
    gdispGClear(lcd, White);
    glGenTextures(1, &lcd_texture);
    glBindTexture(GL_TEXTURE_2D, lcd_texture);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, 128, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, gdispPixmapGetBits(lcd));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
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
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);
    keyboard_shader=load_program("keyboard.vertexshader", "keyboard.fragmentshader");
    lcd_shader = load_program("lcd.vertexshader", "lcd.fragmentshader");

    create_keyboard_vertex_buffer();
    create_key_vertex_buffers();
    create_lcd_vertex_buffer();


    keyboard_locations.view_projection_location = glGetUniformLocation(keyboard_shader, "view_projection");
    keyboard_locations.keyboard_position_location = glGetUniformLocation(keyboard_shader, "keyboard_location");
    keyboard_locations.element_color_location = glGetUniformLocation(keyboard_shader, "element_color");

    lcd_locations.view_projection_location = glGetUniformLocation(lcd_shader, "view_projection");
    lcd_locations.keyboard_position_location = glGetUniformLocation(lcd_shader, "keyboard_location");
    lcd_locations.texture_sampler_location = glGetUniformLocation(lcd_shader, "texture_sampler");

    gfxInit();
    lcd = gdispPixmapCreate(128, 32);
    led = gdispPixmapCreate(7, 7);
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
    glUniformMatrix4fv(locations->view_projection_location, 1, GL_FALSE, view_projection);
}

static void setup_keyboard_location(int keyboard_x, int keyboard_y) {
    const GLfloat matrix[4*4] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        keyboard_x, keyboard_y, 0.0f, 1.0f
    };
    glUniformMatrix4fv(locations->keyboard_position_location, 1, GL_FALSE, matrix);
}

void draw_main_keyboard_area(void) {
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
    glUniform3f(locations->element_color_location, 0xDA / 255.0f, 0xDA / 255.0f, 0xDA / 255.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(keyboard_vertex_data) / sizeof(GLfloat) / 2);
    glDisableVertexAttribArray(0);
}

void draw_leds(int keyboard_x, int keyboard_y) {

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

        int row = i / 7;
        int col = i - row * 7;
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

void draw_triangles_with_offset(GLuint vertex_buffer, GLuint vertex_buffer_size, GLuint offset, GLuint r, GLuint g, GLuint b) {
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
    glUniform3f(locations->element_color_location, r / 255.0f, g / 255.0f, b / 255.0f);
    glDrawArrays(GL_TRIANGLES, 0, vertex_buffer_size);
    glDisableVertexAttribArray(0);
}

void draw_lcd_texture(int keyboard_x, int keyboard_y, GLuint offset) {
    glUseProgram(lcd_shader);
    locations = &lcd_locations;
    glDisable(GL_DEPTH_TEST);
    setup_view_projection();
    setup_keyboard_location(keyboard_x, keyboard_y);

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
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 32, GL_RGBA, GL_UNSIGNED_BYTE, gdispPixmapGetBits(lcd));
    glUniform1i(locations->texture_sampler_location, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void draw_triangles(GLuint vertex_buffer, GLuint vertex_buffer_size, GLuint r, GLuint g, GLuint b) {
    draw_triangles_with_offset(vertex_buffer, vertex_buffer_size, 0, r, g, b);
}

void draw_keycaps(void) {
    draw_triangles(key_outer_vertex_buffer, key_outer_vertex_buffer_size, 0, 0, 0);
    draw_triangles(key_inner_vertex_buffer, key_inner_vertex_buffer_size, 0x20, 0x20, 0x20);
}

void draw_lcd(int keyboard_x, int keyboard_y) {
    draw_triangles(lcd_vertex_buffer, 6, 0, 256, 0);
    draw_triangles_with_offset(lcd_vertex_buffer, 6, 6, 0, 0, 0);
    draw_triangles_with_offset(lcd_vertex_buffer, 6, 12, 0, 0, 0);
    draw_triangles_with_offset(lcd_vertex_buffer, 6, 18, 0, 128, 0);
    draw_triangles_with_offset(lcd_vertex_buffer, 6, 24, 0, 128, 0);
    draw_lcd_texture(keyboard_x, keyboard_y, 30);
}

void draw_emulator(void) {
    int keyboard_x = 10;
    int keyboard_y = 10;
    systemticks_t start_draw = gfxSystemTicks();

    if (!glfwGetCurrentContext()) {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
    }

    gdispSetDisplay(temp);
    glClearColor(0x8B / 255.0f, 0x45 / 255.0f, 0x13 / 255.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(keyboard_shader);
    locations = &keyboard_locations;
    setup_view_projection();
    setup_keyboard_location(keyboard_x, keyboard_y);

    draw_main_keyboard_area();
    systemticks_t after_draw_keyboard = gfxSystemTicks();

    draw_keycaps();
    systemticks_t after_draw_keycaps = gfxSystemTicks();

    draw_leds(keyboard_x, keyboard_y);
    systemticks_t after_draw_leds = gfxSystemTicks();

    draw_lcd(keyboard_x, keyboard_y);
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
