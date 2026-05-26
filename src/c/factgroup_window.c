#include "factgroup_window.h"
#include "storage.h"
#include "trial_history.h"
#include "challenge_window.h"
#include <stdio.h>

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_status_label;
static MenuLayer *s_menu;
static FactCategory s_cat;

static uint16_t get_num_rows(struct MenuLayer *menu, uint16_t section, void *ctx) {
  (void)menu; (void)section; (void)ctx;
  return fact_group_count_in_category(s_cat);
}

static int16_t get_cell_height(struct MenuLayer *menu, MenuIndex *idx, void *ctx) {
  (void)menu; (void)idx; (void)ctx;
  return 32;
}

static void format_time(uint16_t duration_s, char *out, size_t cap) {
  uint16_t m = duration_s / 60;
  uint16_t s = duration_s % 60;
  snprintf(out, cap, "%u:%02u", m, s);
}

static void draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *idx, void *cb_ctx) {
  (void)cb_ctx;
  uint8_t fg_id = fact_group_nth_in_category(s_cat, (uint8_t)idx->row);
  const FactGroup *fg = fact_group_at(fg_id);
  if (!fg) return;

  bool highlighted = menu_cell_layer_is_highlighted(cell_layer);
  graphics_context_set_text_color(ctx, highlighted ? GColorWhite : GColorBlack);

  GRect b = layer_get_bounds(cell_layer);
  graphics_draw_text(ctx, fg->short_name,
    fonts_get_system_font(highlighted ? FONT_KEY_GOTHIC_14_BOLD : FONT_KEY_GOTHIC_14),
    GRect(4, 0, b.size.w - 8, 16),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  char sub[24];
  uint8_t count = storage_trial_count(fg_id);
  if (count == 0) {
    snprintf(sub, sizeof(sub), "no data yet");
  } else {
    char timebuf[8];
    format_time(storage_best_duration(fg_id), timebuf, sizeof(timebuf));
    snprintf(sub, sizeof(sub), "%s, %u trials", timebuf, count);
  }
  graphics_draw_text(ctx, sub,
    fonts_get_system_font(FONT_KEY_GOTHIC_14),
    GRect(4, 14, b.size.w - 8, 16),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void select_click(struct MenuLayer *menu, MenuIndex *idx, void *ctx) {
  (void)menu; (void)ctx;
  uint8_t fg_id = fact_group_nth_in_category(s_cat, (uint8_t)idx->row);
  challenge_window_push(fg_id);
  /* Remove FG Selection so cancel/finish/back from Challenge or Summary all
   * land on Home rather than back on this list. */
  window_stack_remove(s_window, false);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);

  s_status = status_bar_layer_create();
  status_bar_layer_set_colors(s_status, GColorBlack, GColorWhite);
  layer_add_child(root, status_bar_layer_get_layer(s_status));

  s_status_label = text_layer_create(GRect(b.size.w - 80, 0, 76, 16));
  text_layer_set_background_color(s_status_label, GColorClear);
  text_layer_set_text_color(s_status_label, GColorWhite);
  text_layer_set_text_alignment(s_status_label, GTextAlignmentRight);
  text_layer_set_font(s_status_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_status_label, "Pick group");
  layer_add_child(root, text_layer_get_layer(s_status_label));

  s_menu = menu_layer_create(GRect(0, 16, b.size.w, b.size.h - 16));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_num_rows,
    .get_cell_height = get_cell_height,
    .draw_row = draw_row,
    .select_click = select_click,
  });
  menu_layer_set_click_config_onto_window(s_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void window_unload(Window *window) {
  (void)window;
  menu_layer_destroy(s_menu);
  text_layer_destroy(s_status_label);
  status_bar_layer_destroy(s_status);
  window_destroy(s_window);
  s_window = NULL;
}

void factgroup_window_push(FactCategory cat) {
  s_cat = cat;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
