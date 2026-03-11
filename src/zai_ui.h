#ifndef ZAI_UI_H
#define ZAI_UI_H

#include "zai_types.h"

/* #############################################################################
 * # [SECTION] Immediate UI
 * #############################################################################
 */
#ifndef ZAI_UI_MAX_STACK
#define ZAI_UI_MAX_STACK 8 /* Maximum nested panel elements */
#endif

typedef struct zai_rect
{
    u32 x;
    u32 y;
    u32 w;
    u32 h;

} zai_rect;

typedef struct zai_ui_context
{
    u16 mouse_x;
    u16 mouse_y;
    u16 mouse_x_prev;
    u16 mouse_y_prev;

    u32 cursor_y;
    u16 padding;

    u16 active_id;

    u8 mouse_left_is_down;
    u8 mouse_right_is_down;

    u8 mouse_left_was_down;  /* internal */
    u8 mouse_right_was_down; /* internal */

    u8 mouse_pressed;
    u8 mouse_released;

    f32 scale;

    i32 stack_ptr;
    zai_rect stack[ZAI_UI_MAX_STACK];

} zai_ui_context;

typedef enum zai_ui_state
{
    ZAI_UI_STATE_IDLE = 0,
    ZAI_UI_STATE_HOVER = (1 << 0),   /* Mouse is over the rect */
    ZAI_UI_STATE_PRESSED = (1 << 1), /* Mouse just went down this frame */
    ZAI_UI_STATE_HELD = (1 << 2),    /* Mouse is currently down */
    ZAI_UI_STATE_RELEASED = (1 << 3) /* Mouse was released (clicked) */

} zai_ui_state;

typedef struct zai_ui_result
{
    u32 x;
    u32 y;
    u32 w;
    u32 h;

    u8 state;

} zai_ui_result;

ZAI_API ZAI_INLINE zai_ui_result zai_ui_result_init(u32 x, u32 y, u32 w, u32 h, u8 state)
{
    zai_ui_result result;

    result.x = x;
    result.y = y;
    result.w = w;
    result.h = h;
    result.state = state;

    return result;
}

ZAI_API ZAI_INLINE zai_ui_result zai_ui_internal_process(zai_ui_context *ctx, u16 id, u32 x, u32 y, u32 w, u32 h)
{
    zai_ui_result res = {0};

    f32 scale = ctx->scale > 0.0f ? ctx->scale : 1.0f;

    u16 mouse_x = ctx->mouse_x;
    u16 mouse_y = ctx->mouse_y;
    u16 active_id = ctx->active_id;
    u16 pad = (u16)((f32)ctx->padding * scale);

    u8 is_over;

    if (!x && !y && ctx->stack_ptr)
    {
        zai_rect *panel = ctx->stack + ctx->stack_ptr - 1;
        u16 pad2 = pad << 1;

        res.x = panel->x + pad;
        res.y = panel->y + ctx->cursor_y + pad;

        res.w = w ? (u32)((f32)w * scale) : (panel->w - pad2);
        res.h = (u32)((f32)h * scale);

        ctx->cursor_y += res.h + pad;
    }
    else
    {
        res.x = (u32)((f32)x * scale);
        res.y = (u32)((f32)y * scale);
        res.w = (u32)((f32)w * scale);
        res.h = (u32)((f32)h * scale);
    }

    if (ctx->stack_ptr)
    {
        zai_rect *panel = ctx->stack + ctx->stack_ptr - 1;

        /* clang-format off */
        if (res.y + res.h    < panel->y) return res;
        if (res.y > panel->y + panel->h) return res;
        if (res.x + res.w    < panel->x) return res;
        if (res.x > panel->x + panel->w) return res;
        /* clang-format on */
    }

    if (active_id && active_id != id)
    {
        return res;
    }

    is_over = mouse_x >= res.x && mouse_y >= res.y && (mouse_x - res.x) <= res.w && (mouse_y - res.y) <= res.h;

    if (is_over)
    {
        res.state |= ZAI_UI_STATE_HOVER;

        if (!active_id && ctx->mouse_pressed)
        {
            ctx->active_id = id;
            res.state |= ZAI_UI_STATE_PRESSED;
        }
    }

    if (ctx->active_id == id)
    {
        res.state |= ZAI_UI_STATE_HELD;

        if (ctx->mouse_released)
        {
            ctx->active_id = 0;

            if (is_over)
            {
                res.state |= ZAI_UI_STATE_RELEASED;
            }
        }
    }

    return res;
}

/* #############################################################################
 * # [SECTION] Immediate UI Elements
 * #############################################################################
 */
ZAI_API ZAI_INLINE void zai_ui_begin(zai_ui_context *ctx)
{
    ctx->mouse_pressed = ctx->mouse_left_is_down && !ctx->mouse_left_was_down;
    ctx->mouse_released = !ctx->mouse_left_is_down && ctx->mouse_left_was_down;
}

ZAI_API ZAI_INLINE void zai_ui_end(zai_ui_context *ctx)
{
    ctx->mouse_x_prev = ctx->mouse_x;
    ctx->mouse_y_prev = ctx->mouse_y;
    ctx->mouse_left_was_down = ctx->mouse_left_is_down;
    ctx->mouse_right_was_down = ctx->mouse_right_is_down;
}

ZAI_API ZAI_INLINE zai_ui_result zai_ui_panel_begin(zai_ui_context *ctx, u32 x, u32 y, u32 w, u32 h)
{
    zai_ui_result res = {0};

    if (ctx->stack_ptr < ZAI_UI_MAX_STACK)
    {
        zai_rect *r = ctx->stack + ctx->stack_ptr++;
        f32 s = ctx->scale > 0.0f ? ctx->scale : 1.0f;

        r->x = (u32)((f32)x * s);
        r->y = (u32)((f32)y * s);
        r->w = (u32)((f32)w * s);
        r->h = (u32)((f32)h * s);

        ctx->cursor_y = 0;

        res.x = r->x;
        res.y = r->y;
        res.w = r->w;
        res.h = r->h;
        res.state = ZAI_UI_STATE_IDLE;
    }

    return res;
}

ZAI_API ZAI_INLINE void zai_ui_panel_end(zai_ui_context *ctx)
{
    if (ctx->stack_ptr > 0)
    {
        zai_rect *p = ctx->stack + ctx->stack_ptr - 1;
        u32 panel_h = p->h;

        ctx->stack_ptr--;

        if (ctx->stack_ptr)
        {
            ctx->cursor_y += panel_h + ctx->padding;
        }
    }
}

ZAI_API ZAI_INLINE zai_ui_result zai_ui_button(zai_ui_context *ctx, u16 id, u32 x, u32 y, u32 w, u32 h)
{
    return zai_ui_internal_process(ctx, id, x, y, w, h);
}

ZAI_API ZAI_INLINE zai_ui_result zai_ui_checkbox(zai_ui_context *ctx, u16 id, u32 x, u32 y, u32 w, u32 h, u8 *is_checked)
{
    zai_ui_result res = zai_ui_internal_process(ctx, id, x, y, w, h);

    if (res.state & ZAI_UI_STATE_RELEASED)
    {
        *is_checked = !(*is_checked);
    }

    return res;
}

ZAI_API ZAI_INLINE zai_ui_result zai_ui_drag_header(zai_ui_context *ctx, u16 id, u32 *win_x, u32 *win_y, u32 win_w, u32 win_h)
{
    zai_ui_result res = zai_ui_internal_process(ctx, id, *win_x, *win_y, win_w, win_h);

    if (res.state & ZAI_UI_STATE_HELD)
    {
        f32 s = ctx->scale > 0.0f ? ctx->scale : 1.0f;

        i32 screen_dx = (i32)ctx->mouse_x - (i32)ctx->mouse_x_prev;
        i32 screen_dy = (i32)ctx->mouse_y - (i32)ctx->mouse_y_prev;

        i32 nx = (i32)(*win_x) + (i32)((f32)screen_dx / s);
        i32 ny = (i32)(*win_y) + (i32)((f32)screen_dy / s);

        i32 vx;
        i32 vy;

        *win_x = (u32)(nx < 0 ? 0 : nx);
        *win_y = (u32)(ny < 0 ? 0 : ny);

        vx = (i32)res.x + screen_dx;
        vy = (i32)res.y + screen_dy;

        res.x = (u32)(vx < 0 ? 0 : vx);
        res.y = (u32)(vy < 0 ? 0 : vy);
    }

    return res;
}

ZAI_API ZAI_INLINE zai_ui_result zai_ui_radio(zai_ui_context *ctx, u16 id, u32 x, u32 y, u32 size, i32 *current_val, i32 radio_val)
{
    zai_ui_result res = zai_ui_internal_process(ctx, id, x, y, size, size);

    if (res.state & ZAI_UI_STATE_RELEASED)
    {
        *current_val = radio_val;
    }

    return res;
}

ZAI_API ZAI_INLINE zai_ui_result zai_ui_slider_range(zai_ui_context *ctx, u16 id, u32 x, u32 y, u32 w, u32 h, f32 *val, f32 min, f32 max)
{
    zai_ui_result res = zai_ui_internal_process(ctx, id, x, y, w, h);

    if ((res.state & ZAI_UI_STATE_PRESSED) || (res.state & ZAI_UI_STATE_HELD))
    {
        f32 t = (f32)((i32)ctx->mouse_x - (i32)res.x) / (f32)res.w;
        t = (t < 0.0f) ? 0.0f : t;
        t = (t > 1.0f) ? 1.0f : t;

        *val = min + t * (max - min);
    }

    return res;
}

ZAI_API ZAI_INLINE zai_ui_result zai_ui_slider(zai_ui_context *ctx, u16 id, u32 x, u32 y, u32 w, u32 h, f32 *val)
{
    return zai_ui_slider_range(ctx, id, x, y, w, h, val, 0.0f, 1.0f);
}

ZAI_API ZAI_INLINE zai_ui_result zai_ui_slider_int(zai_ui_context *ctx, u16 id, u32 x, u32 y, u32 w, u32 h, i32 *val, i32 min, i32 max, u32 step)
{
    zai_ui_result res = zai_ui_internal_process(ctx, id, x, y, w, h);

    if ((res.state & ZAI_UI_STATE_PRESSED) || (res.state & ZAI_UI_STATE_HELD))
    {
        i32 range = max - min;
        i32 value;

        f32 t = (f32)((i32)ctx->mouse_x - (i32)res.x) / (f32)res.w;
        t = (t < 0.0f) ? 0.0f : t;
        t = (t > 1.0f) ? 1.0f : t;

        value = min + (i32)(t * (f32)range);

        /* snap to step */
        if (step > 1)
        {
            i32 remainder = (value - min) % (i32)step;
            value -= remainder;
        }

        value = (value < min) ? min : value;
        value = (value > max) ? max : value;

        *val = value;
    }

    return res;
}

/* #############################################################################
 * # [SECTION] Immediate UI Instances for Rendering
 * #############################################################################
 */
typedef struct zai_ui_render_instance
{
    f32 x, y, w, h;
    f32 r, g, b, a;

} zai_ui_render_instance;

#ifndef ZAI_UI_MAX_RENDER_INSTANCES
#define ZAI_UI_MAX_RENDER_INSTANCES 64
#endif

static zai_ui_render_instance zai_ui_render_instances[ZAI_UI_MAX_RENDER_INSTANCES];
static u32 zai_ui_render_instances_count = 0;

ZAI_API ZAI_INLINE void zai_ui_render_instance_push(zai_ui_result res, f32 r, f32 g, f32 b, f32 a)
{
    if (zai_ui_render_instances_count < ZAI_UI_MAX_RENDER_INSTANCES)
    {
        zai_ui_render_instance *inst = &zai_ui_render_instances[zai_ui_render_instances_count++];
        inst->x = (f32)res.x;
        inst->y = (f32)res.y;
        inst->w = (f32)res.w;
        inst->h = (f32)res.h;
        inst->r = r;
        inst->g = g;
        inst->b = b;
        inst->a = a;
    }
}

#endif /* ZAI_UI_H */
