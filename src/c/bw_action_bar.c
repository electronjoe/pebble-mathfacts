#include "bw_action_bar.h"
#include <stdlib.h>

#define BAR_W 20

struct BWActionBar {
  Layer *layer;
  BWIconKind up, sel, down;
};

/* Draw an icon centered at (cx, cy). White on the assumed-black background. */
static void draw_icon(GContext *ctx, int16_t cx, int16_t cy, BWIconKind kind) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);

  switch (kind) {
    case BW_ICON_NONE:
      break;

    case BW_ICON_CANCEL:
      /* X: two crossed diagonals 10 px tall */
      graphics_draw_line(ctx, GPoint(cx - 5, cy - 5), GPoint(cx + 5, cy + 5));
      graphics_draw_line(ctx, GPoint(cx + 5, cy - 5), GPoint(cx - 5, cy + 5));
      /* Make the strokes 2 px thick by drawing parallel lines. */
      graphics_draw_line(ctx, GPoint(cx - 5, cy - 4), GPoint(cx + 5, cy + 6));
      graphics_draw_line(ctx, GPoint(cx + 5, cy - 4), GPoint(cx - 5, cy + 6));
      break;

    case BW_ICON_CHECK:
      /* Checkmark: short diagonal up-right, then longer down-right */
      graphics_draw_line(ctx, GPoint(cx - 5, cy),     GPoint(cx - 1, cy + 4));
      graphics_draw_line(ctx, GPoint(cx - 1, cy + 4), GPoint(cx + 5, cy - 4));
      graphics_draw_line(ctx, GPoint(cx - 5, cy + 1), GPoint(cx - 1, cy + 5));
      graphics_draw_line(ctx, GPoint(cx - 1, cy + 5), GPoint(cx + 5, cy - 3));
      break;

    case BW_ICON_SKIP:
      /* Right-arrow: horizontal shaft + > head */
      graphics_draw_line(ctx, GPoint(cx - 6, cy), GPoint(cx + 5, cy));
      graphics_draw_line(ctx, GPoint(cx - 6, cy + 1), GPoint(cx + 5, cy + 1));
      graphics_draw_line(ctx, GPoint(cx + 1, cy - 4), GPoint(cx + 5, cy));
      graphics_draw_line(ctx, GPoint(cx + 1, cy + 5), GPoint(cx + 5, cy + 1));
      break;

    case BW_ICON_CHEVRON_UP:
      graphics_draw_line(ctx, GPoint(cx - 5, cy + 2), GPoint(cx, cy - 3));
      graphics_draw_line(ctx, GPoint(cx, cy - 3),     GPoint(cx + 5, cy + 2));
      graphics_draw_line(ctx, GPoint(cx - 5, cy + 3), GPoint(cx, cy - 2));
      graphics_draw_line(ctx, GPoint(cx, cy - 2),     GPoint(cx + 5, cy + 3));
      break;

    case BW_ICON_CHEVRON_DOWN:
      graphics_draw_line(ctx, GPoint(cx - 5, cy - 2), GPoint(cx, cy + 3));
      graphics_draw_line(ctx, GPoint(cx, cy + 3),     GPoint(cx + 5, cy - 2));
      graphics_draw_line(ctx, GPoint(cx - 5, cy - 3), GPoint(cx, cy + 2));
      graphics_draw_line(ctx, GPoint(cx, cy + 2),     GPoint(cx + 5, cy - 3));
      break;
  }
}

static void update_proc(Layer *layer, GContext *ctx) {
  BWActionBar *bar = (BWActionBar *)layer_get_data(layer);
  GRect b = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  /* Three icon slots: align with the three physical buttons. The action bar
   * spans y=0..(h-1); icons centered at 1/6, 1/2, 5/6 of the bar height. */
  int16_t cx = b.size.w / 2;
  int16_t y_top = b.size.h / 6;
  int16_t y_mid = b.size.h / 2;
  int16_t y_bot = b.size.h - (b.size.h / 6);

  draw_icon(ctx, cx, y_top, bar->up);
  draw_icon(ctx, cx, y_mid, bar->sel);
  draw_icon(ctx, cx, y_bot, bar->down);
}

BWActionBar *bw_action_bar_create(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);

  Layer *layer = layer_create_with_data(GRect(b.size.w - BAR_W, 16, BAR_W, b.size.h - 16),
                                        sizeof(BWActionBar));
  BWActionBar *bar = (BWActionBar *)layer_get_data(layer);
  bar->layer = layer;
  bar->up = bar->sel = bar->down = BW_ICON_NONE;
  layer_set_update_proc(layer, update_proc);
  layer_add_child(root, layer);
  return bar;
}

void bw_action_bar_destroy(BWActionBar *bar) {
  if (!bar) return;
  layer_destroy(bar->layer);
}

void bw_action_bar_set_icons(BWActionBar *bar,
                             BWIconKind up, BWIconKind select, BWIconKind down) {
  if (!bar) return;
  bar->up = up;
  bar->sel = select;
  bar->down = down;
  layer_mark_dirty(bar->layer);
}
