#include "summary_window.h"
#include "storage.h"
#include "trial_history.h"
#include <stdio.h>
#include <time.h>

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_status_right;
static TextLayer *s_result_label;
static Layer *s_bars_layer;
static TextLayer *s_store_label, *s_discard_label;
static ActionBarLayer *s_action;

static uint8_t s_fg_id;
static uint16_t s_duration_s;

typedef enum { SUMMARY_STORE = 0, SUMMARY_DISCARD = 1 } SummaryChoice;
static SummaryChoice s_choice;

static void format_mmss(uint16_t s, char *out, size_t cap) {
  snprintf(out, cap, "%u:%02u", s / 60, s % 60);
}

static void draw_bars(Layer *layer, GContext *ctx) {
  (void)layer;
  TrialRecord arr[TRIAL_HISTORY_CAPACITY];
  uint8_t n = storage_load_history(s_fg_id, arr, TRIAL_HISTORY_CAPACITY);
  uint16_t bars_s[4];
  uint8_t bar_count = 0;

  for (uint8_t i = 0; i < n && bar_count < 3; i++) {
    bars_s[2 - bar_count] = arr[i].duration_s;
    bar_count++;
  }
  if (bar_count > 0) {
    uint16_t tmp[3];
    for (uint8_t i = 0; i < bar_count; i++) tmp[i] = bars_s[3 - bar_count + i];
    for (uint8_t i = 0; i < bar_count; i++) bars_s[i] = tmp[i];
  }
  bars_s[bar_count] = s_duration_s;
  bar_count++;

  uint16_t worst = 0;
  for (uint8_t i = 0; i < bar_count; i++) if (bars_s[i] > worst) worst = bars_s[i];
  if (worst == 0) worst = 1;

  const int16_t y_top = 30, y_bot = 60;
  const int16_t span_y = y_bot - y_top;
  const int16_t x_base = 22;
  const int16_t bar_w = 18, bar_gap = 6;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_text_color(ctx, GColorBlack);

  for (uint8_t i = 0; i < bar_count; i++) {
    int16_t h = (int16_t)((int32_t)span_y * bars_s[i] / worst);
    int16_t x = x_base + i * (bar_w + bar_gap);
    GRect r = GRect(x, y_bot - h, bar_w, h);
    bool is_now = (i == bar_count - 1);
    if (is_now) {
      graphics_fill_rect(ctx, r, 0, GCornerNone);
    } else {
      graphics_draw_rect(ctx, r);
    }
    char timebuf[8];
    format_mmss(bars_s[i], timebuf, sizeof(timebuf));
    graphics_draw_text(ctx, timebuf,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(x - 4, y_bot + 2, bar_w + 8, 14),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    const char *tag = is_now ? "now" :
      (i == bar_count - 2 ? "-1" : i == bar_count - 3 ? "-2" : "-3");
    graphics_draw_text(ctx, tag,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(x - 4, y_bot + 14, bar_w + 8, 14),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
}

static void update_highlight(void) {
  text_layer_set_background_color(s_store_label,   s_choice == SUMMARY_STORE   ? GColorBlack : GColorWhite);
  text_layer_set_text_color(s_store_label,         s_choice == SUMMARY_STORE   ? GColorWhite : GColorBlack);
  text_layer_set_background_color(s_discard_label, s_choice == SUMMARY_DISCARD ? GColorBlack : GColorWhite);
  text_layer_set_text_color(s_discard_label,       s_choice == SUMMARY_DISCARD ? GColorWhite : GColorBlack);
}

static void up_click(ClickRecognizerRef rec, void *ctx)   { (void)rec; (void)ctx;
  s_choice = SUMMARY_STORE; update_highlight();
}
static void down_click(ClickRecognizerRef rec, void *ctx) { (void)rec; (void)ctx;
  s_choice = SUMMARY_DISCARD; update_highlight();
}
static void select_click(ClickRecognizerRef rec, void *ctx) { (void)rec; (void)ctx;
  if (s_choice == SUMMARY_STORE) {
    TrialRecord t = { .timestamp_utc = (uint32_t)time(NULL), .duration_s = s_duration_s };
    storage_append_trial(s_fg_id, t);
  }
  window_stack_remove(s_window, false);
}

static void click_config(void *ctx) { (void)ctx;
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);

  s_status = status_bar_layer_create();
  status_bar_layer_set_colors(s_status, GColorBlack, GColorWhite);
  layer_add_child(root, status_bar_layer_get_layer(s_status));

  s_status_right = text_layer_create(GRect(4, 0, b.size.w - 28, 16));
  text_layer_set_background_color(s_status_right, GColorClear);
  text_layer_set_text_color(s_status_right, GColorWhite);
  text_layer_set_font(s_status_right, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_status_right, "Trial done");
  layer_add_child(root, text_layer_get_layer(s_status_right));

  static char result_buf[8];
  format_mmss(s_duration_s, result_buf, sizeof(result_buf));
  s_result_label = text_layer_create(GRect(0, 16, b.size.w - 20, 24));
  text_layer_set_background_color(s_result_label, GColorClear);
  text_layer_set_text_alignment(s_result_label, GTextAlignmentCenter);
  text_layer_set_font(s_result_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(s_result_label, result_buf);
  layer_add_child(root, text_layer_get_layer(s_result_label));

  s_bars_layer = layer_create(GRect(0, 40, b.size.w - 20, 60));
  layer_set_update_proc(s_bars_layer, draw_bars);
  layer_add_child(root, s_bars_layer);

  s_store_label = text_layer_create(GRect(4, 110, b.size.w - 28, 22));
  text_layer_set_text(s_store_label, "Store");
  text_layer_set_text_alignment(s_store_label, GTextAlignmentCenter);
  text_layer_set_font(s_store_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(root, text_layer_get_layer(s_store_label));

  s_discard_label = text_layer_create(GRect(4, 138, b.size.w - 28, 22));
  text_layer_set_text(s_discard_label, "Discard");
  text_layer_set_text_alignment(s_discard_label, GTextAlignmentCenter);
  text_layer_set_font(s_discard_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(root, text_layer_get_layer(s_discard_label));

  s_action = action_bar_layer_create();
  action_bar_layer_set_background_color(s_action, GColorBlack);
  action_bar_layer_add_to_window(s_action, window);

  s_choice = SUMMARY_STORE;
  update_highlight();
}

static void window_unload(Window *window) {
  (void)window;
  action_bar_layer_destroy(s_action);
  text_layer_destroy(s_discard_label);
  text_layer_destroy(s_store_label);
  layer_destroy(s_bars_layer);
  text_layer_destroy(s_result_label);
  text_layer_destroy(s_status_right);
  status_bar_layer_destroy(s_status);
  window_destroy(s_window);
  s_window = NULL;
}

void summary_window_push(uint8_t fg_id, uint16_t duration_s) {
  s_fg_id = fg_id;
  s_duration_s = duration_s;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}
