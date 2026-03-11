#ifndef ZAI_H
#define ZAI_H

#include "zai_types.h"

/* #############################################################################
 * # [SECTION] Platform Input
 * #############################################################################
 */
#define ZAI_INPUT_KEYS_COUNT 256

typedef struct zai_platform_input_mouse
{
    i32 mouse_dx; /* Relative movement delta for x  */
    i32 mouse_dy; /* Relative movement delta for y  */
    i32 mouse_x;  /* Mouse position on screen for x */
    i32 mouse_y;  /* Mouse position on screen for y */
    f32 mouse_scroll;
    u8 mouse_left_is_down;
    u8 mouse_left_was_down;
    u8 mouse_right_is_down;
    u8 mouse_right_was_down;

} zai_platform_input_mouse;

typedef struct zai_platform_input_keyboard
{
    u8 keys_is_down[ZAI_INPUT_KEYS_COUNT];
    u8 keys_was_down[ZAI_INPUT_KEYS_COUNT];

} zai_platform_input_keyboard;

typedef struct zai_platform_input_controller
{
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
    u32 window_width;
    u32 window_height;

    f32 window_clear_color_r;
    f32 window_clear_color_g;
    f32 window_clear_color_b;
    f32 window_clear_color_a;

} zai_platform_window;

/* #############################################################################
 * # [SECTION] Platform API
 * #############################################################################
 */
typedef u8 (*zai_platform_api_io_print)(s8 *string);
typedef u8 (*zai_platform_api_io_file_size)(s8 *filename, u32 *file_size);
typedef u8 (*zai_platform_api_io_file_read)(s8 *filename, u8 *buffer, u32 buffer_size);

typedef struct zai_platform_api
{
    zai_platform_api_io_print io_print;
    zai_platform_api_io_file_size io_file_size;
    zai_platform_api_io_file_read io_file_read;

} zai_platform_api;

/* #############################################################################
 * # [SECTION] Main entry point (zai_update)
 * #############################################################################
 */
u8 zai_update_stub(zai_platform_api *api, zai_platform_window *window, zai_platform_input *input)
{
    api->io_print("[zai][error] No 'zai_update' function has been set! Using 'zai_update_stub'!\n");
    api->io_print("[zai][error] Define the following function in your code:\n\n");
    api->io_print("  u8 zai_update(zai_platform_api *api, zai_platform_window *window, zai_platform_input *input)\n  {\n  /* your code */\n  }\n\n");
    api->io_print("[zai][error]\n");

    window->window_clear_color_r = 1.0f;
    window->window_clear_color_g = 0.0f;
    window->window_clear_color_b = 0.0f;

    return 0;
}

typedef u8 (*zai_update_function)(
    zai_platform_api *api,       /* Platform provided functinos */
    zai_platform_window *window, /* Platform window informatino */
    zai_platform_input *input    /* Platform input */
);

static zai_update_function zai_update = zai_update_stub;

#endif /* ZAI_H */
