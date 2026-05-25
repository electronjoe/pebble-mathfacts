#include "home_window.h"
#include "storage.h"

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_category_label;
static TextLayer *s_empty_label;
static FactCategory s_current_category;

static const char *category_short_name(FactCategory c) {
  switch (c) {
    case CATEGORY_SKIP:    return "Skip";
    case CATEGORY_MULT:    return "Mult";
    case CATEGORY_MULTDIV: return "M&D";
    default:               return "?";
  }
}

static void apply_category(FactCategory c) {
  s_current_category = c;
  text_layer_set_text(s_category_label, category_short_name(c));
  storage_save_last_category(c);
}

static void up_click(ClickRecognizerRef rec, void *ctx) {
  (void)rec; (void)ctx;
  apply_category((FactCategory)(((int)s_current_category + CATEGORY_COUNT - 1) % CATEGORY_COUNT));
}

static void down_click(ClickRecognizerRef rec, void *ctx) {
  (void)rec; (void)ctx;
  apply_category((FactCategory)(((int)s_current_category + 1) % CATEGORY_COUNT));
}

static void click_config(void *ctx) {
  (void)ctx;
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);

  s_status = status_bar_layer_create();
  status_bar_layer_set_colors(s_status, GColorBlack, GColorWhite);
  layer_add_child(root, status_bar_layer_get_layer(s_status));

  s_category_label = text_layer_create(GRect(b.size.w - 50, 0, 46, 16));
  text_layer_set_background_color(s_category_label, GColorClear);
  text_layer_set_text_color(s_category_label, GColorWhite);
  text_layer_set_text_alignment(s_category_label, GTextAlignmentRight);
  text_layer_set_font(s_category_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root, text_layer_get_layer(s_category_label));

  s_empty_label = text_layer_create(GRect(0, b.size.h / 2 - 8, b.size.w, 16));
  text_layer_set_text(s_empty_label, "no data yet");
  text_layer_set_text_alignment(s_empty_label, GTextAlignmentCenter);
  text_layer_set_font(s_empty_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root, text_layer_get_layer(s_empty_label));

  apply_category(storage_load_last_category());
}

static void window_unload(Window *window) {
  (void)window;
  text_layer_destroy(s_empty_label);
  text_layer_destroy(s_category_label);
  status_bar_layer_destroy(s_status);
}

void home_window_push(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}

void home_window_destroy(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}
