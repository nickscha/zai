#define ZAI_APPLICATION
#include "zai.h"

void zai_update(zai_platform_state *platform_state)
{
    zai_platform_input_keyboard *keyboard = &platform_state->input.keyboard;

    if (!platform_state->application_initialized)
    {
        platform_state->api.io_print("Hello from application!\n");

        platform_state->window.title = "Hello from application!\n";
        platform_state->window.title_changed = 1;

        platform_state->window.clear_color_r = 0.1f;
        platform_state->window.clear_color_g = 0.6f;
        platform_state->window.clear_color_b = 0.1f;
        platform_state->window.clear_color_changed = 1;

        platform_state->application_initialized = 1;
    }

    if (keyboard->keys_is_down[ZAI_KEYBOARD_KEY_W])
    {
        platform_state->window.clear_color_r += 0.05f;
        platform_state->window.clear_color_changed = 1;
    }

    if (keyboard->keys_is_down[ZAI_KEYBOARD_KEY_S])
    {
        platform_state->window.clear_color_r -= 0.05f;
        platform_state->window.clear_color_changed = 1;
    }
}

#ifdef _WIN32
#ifdef __clang__
#elif __GNUC__
__attribute((externally_visible))
#endif
i32 DllMainCRTStartup(void)
{
    return 1;
}
#endif