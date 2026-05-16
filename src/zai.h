#ifndef ZAI_H
#define ZAI_H

#include "zai_types.h"

/* #############################################################################
 * # [SECTION] Platform Mouse Input
 * #############################################################################
 */
typedef enum zai_platform_input_mouse_keys
{
    /* basic mouse keys */
    ZAI_MOUSE_KEY_LEFT,
    ZAI_MOUSE_KEY_MIDDLE,
    ZAI_MOUSE_KEY_RIGHT,

    /* padding */
    ZAI_MOUSE_KEY_UNUSED1,
    ZAI_MOUSE_KEY_UNUSED2,
    ZAI_MOUSE_KEY_UNUSED3,
    ZAI_MOUSE_KEY_UNUSED4,
    ZAI_MOUSE_KEY_UNUSED5,

    ZAI_MOUSE_KEYS_COUNT

} zai_platform_input_mouse_keys;

typedef struct zai_platform_input_mouse
{
    i32 dx;     /* Relative movement delta for x  */
    i32 dy;     /* Relative movement delta for y  */
    i32 x;      /* Mouse position on screen for x */
    i32 y;      /* Mouse position on screen for y */
    f32 scroll; /* Mouse scroll wheel delta       */

    u8 keys_is_down[ZAI_MOUSE_KEYS_COUNT];
    u8 keys_was_down[ZAI_MOUSE_KEYS_COUNT];

} zai_platform_input_mouse;

/* #############################################################################
 * # [SECTION] Platform Keyboard Input
 * #############################################################################
 */
typedef enum zai_platform_input_keyboard_keys
{
    /* numbers */
    ZAI_KEYBOARD_KEY_0,
    ZAI_KEYBOARD_KEY_1,
    ZAI_KEYBOARD_KEY_2,
    ZAI_KEYBOARD_KEY_3,
    ZAI_KEYBOARD_KEY_4,
    ZAI_KEYBOARD_KEY_5,
    ZAI_KEYBOARD_KEY_6,
    ZAI_KEYBOARD_KEY_7,
    ZAI_KEYBOARD_KEY_8,
    ZAI_KEYBOARD_KEY_9,

    /* letters */
    ZAI_KEYBOARD_KEY_A,
    ZAI_KEYBOARD_KEY_B,
    ZAI_KEYBOARD_KEY_C,
    ZAI_KEYBOARD_KEY_D,
    ZAI_KEYBOARD_KEY_E,
    ZAI_KEYBOARD_KEY_F,
    ZAI_KEYBOARD_KEY_G,
    ZAI_KEYBOARD_KEY_H,
    ZAI_KEYBOARD_KEY_I,
    ZAI_KEYBOARD_KEY_J,
    ZAI_KEYBOARD_KEY_K,
    ZAI_KEYBOARD_KEY_L,
    ZAI_KEYBOARD_KEY_M,
    ZAI_KEYBOARD_KEY_N,
    ZAI_KEYBOARD_KEY_O,
    ZAI_KEYBOARD_KEY_P,
    ZAI_KEYBOARD_KEY_Q,
    ZAI_KEYBOARD_KEY_R,
    ZAI_KEYBOARD_KEY_S,
    ZAI_KEYBOARD_KEY_T,
    ZAI_KEYBOARD_KEY_U,
    ZAI_KEYBOARD_KEY_V,
    ZAI_KEYBOARD_KEY_W,
    ZAI_KEYBOARD_KEY_X,
    ZAI_KEYBOARD_KEY_Y,
    ZAI_KEYBOARD_KEY_Z,

    /* function keys */
    ZAI_KEYBOARD_KEY_F1,
    ZAI_KEYBOARD_KEY_F2,
    ZAI_KEYBOARD_KEY_F3,
    ZAI_KEYBOARD_KEY_F4,
    ZAI_KEYBOARD_KEY_F5,
    ZAI_KEYBOARD_KEY_F6,
    ZAI_KEYBOARD_KEY_F7,
    ZAI_KEYBOARD_KEY_F8,
    ZAI_KEYBOARD_KEY_F9,
    ZAI_KEYBOARD_KEY_F10,
    ZAI_KEYBOARD_KEY_F11,
    ZAI_KEYBOARD_KEY_F12,

    /* control keys */
    ZAI_KEYBOARD_KEY_CONTROL,
    ZAI_KEYBOARD_KEY_RETURN,
    ZAI_KEYBOARD_KEY_SPACE,
    ZAI_KEYBOARD_KEY_SHIFT,
    ZAI_KEYBOARD_KEY_TAB,
    ZAI_KEYBOARD_KEY_ALT,
    ZAI_KEYBOARD_KEY_ESCAPE,

    /* movement keys */
    ZAI_KEYBOARD_KEY_LEFT,
    ZAI_KEYBOARD_KEY_UP,
    ZAI_KEYBOARD_KEY_RIGHT,
    ZAI_KEYBOARD_KEY_DOWN,

    /* others */
    ZAI_KEYBOARD_KEY_PLUS,
    ZAI_KEYBOARD_KEY_MINUS,

    /* padding */
    ZAI_KEYBOARD_KEY_UNUSED1,
    ZAI_KEYBOARD_KEY_UNUSED2,
    ZAI_KEYBOARD_KEY_UNUSED3,

    ZAI_KEYBORD_KEYS_COUNT

} zai_platform_input_keyboard_keys;

/*
    Key Pressed:   keys_is_down[ZAI_KEYBOARD_KEY_A] && !keys_was_down[ZAI_KEYBOARD_KEY_A]
    Key Released: !keys_is_down[ZAI_KEYBOARD_KEY_A] &&  keys_was_down[ZAI_KEYBOARD_KEY_A]
*/
typedef struct zai_platform_input_keyboard
{
    u8 keys_is_down[ZAI_KEYBORD_KEYS_COUNT];
    u8 keys_was_down[ZAI_KEYBORD_KEYS_COUNT];

} zai_platform_input_keyboard;

/* #############################################################################
 * # [SECTION] Platform Controller Input
 * #############################################################################
 */
typedef struct zai_platform_input_controller
{
    u8 connected;

    u8 button_a;
    u8 button_b;
    u8 button_x;
    u8 button_y;
    u8 shoulder_left;
    u8 shoulder_right;
    u8 trigger_left;
    u8 trigger_right;
    u8 dpad_up;
    u8 dpad_down;
    u8 dpad_left;
    u8 dpad_right;
    u8 stick_left;
    u8 stick_right;
    u8 start;
    u8 back;
    f32 stick_left_x;
    f32 stick_left_y;
    f32 stick_right_x;
    f32 stick_right_y;
    f32 trigger_left_value;
    f32 trigger_right_value;

} zai_platform_input_controller;

/* #############################################################################
 * # [SECTION] Platform Input
 * #############################################################################
 */
typedef struct zai_platform_input
{
    zai_platform_input_mouse mouse;
    zai_platform_input_keyboard keyboard;
    zai_platform_input_controller controller;

} zai_platform_input;

/* #############################################################################
 * # [SECTION] Platform Window
 * #############################################################################
 */
typedef struct zai_platform_window
{
    u8 size_changed;
    u8 clear_color_changed;
    u8 title_changed;

    u8 minimized;

    u32 width;
    u32 height;

    f32 clear_color_r;
    f32 clear_color_g;
    f32 clear_color_b;
    f32 clear_color_a;

    s8 *title;

} zai_platform_window;

/* #############################################################################
 * # [SECTION] Platform API
 * #############################################################################
 */
typedef u8 (*zai_platform_api_io_print)(s8 *string);
typedef u8 (*zai_platform_api_io_file_size)(s8 *filename, u32 *file_size);
typedef u8 (*zai_platform_api_io_file_read)(s8 *filename, u8 *buffer, u32 buffer_size);
typedef u8 (*zai_platform_api_io_file_write)(s8 *filename, u8 *buffer, u32 buffer_size);

typedef struct zai_platform_api
{
    zai_platform_api_io_print io_print;
    zai_platform_api_io_file_size io_file_size;
    zai_platform_api_io_file_read io_file_read;
    zai_platform_api_io_file_write io_file_write;

} zai_platform_api;

/* #############################################################################
 * # [SECTION] Platform Memory
 * #############################################################################
 */
typedef struct zai_platform_memory
{
    u8 *data;
    u32 size;

} zai_platform_memory;

/* #############################################################################
 * # [SECTION] Platform Timing
 * #############################################################################
 */
typedef struct zai_platform_timing
{
    i32 frame_count;       /* Frames processed count               */
    f64 frame_rate;        /* Frame Rate per second                */
    u32 frame_rate_target; /* Targeted Frame rate per second       */
    f64 frame_rate_raw;    /* Frame Rate per second raw (no cap)   */
    f64 time_elapsed;      /* Total elapsed time in seconds        */
    f64 time_delta;        /* Current render frame time in seconds */

} zai_platform_timing;

/* #############################################################################
 * # [SECTION] Main entry point (zai_update)
 * #############################################################################
 */
typedef struct zai_platform_state
{
    zai_platform_api api;       /* Platform provided functions */
    zai_platform_window window; /* Platform window information */
    zai_platform_input input;   /* Platform input */
    zai_platform_memory memory; /* Platform memory */
    zai_platform_timing timing; /* Platform timing */

    u8 running;
    u8 application_initialized;

} zai_platform_state;

#ifdef ZAI_APPLICATION
void zai_update(zai_platform_state *platform_state);
#else

ZAI_API ZAI_INLINE void zai_update_stub(zai_platform_state *platform_state)
{
    platform_state->api.io_print("[zai][error] No 'zai_update' function has been set! Using 'zai_update_stub'!\n");
    platform_state->api.io_print("[zai][error] Define the following function in your code:\n\n");
    platform_state->api.io_print("  u8 zai_update(zai_platform_state *platform_state)\n  {\n    /* your code */\n  }\n\n");
    platform_state->api.io_print("[zai][error]\n");

    platform_state->window.clear_color_r = 0.9f;
    platform_state->window.clear_color_g = 0.1f;
    platform_state->window.clear_color_b = 0.1f;
    platform_state->window.clear_color_changed = 1;

    platform_state->window.title = "[zai][error] No 'zai_update' function has been set! Using 'zai_update_stub'!\n";
    platform_state->window.title_changed = 1;
}

typedef void (*zai_update_function)(zai_platform_state *platform_state);

static zai_update_function zai_update = zai_update_stub;

#endif

#endif /* ZAI_H */
