#include "clear_window.h"
#include "storage.h"
#include "bw_action_bar.h"

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_status_label;
static TextLayer *s_body1, *s_body2, *s_body3;
static TextLayer *s_cancel_label, *s_clear_label;
static BWActionBar *s_action;

typedef enum { CHOICE_CANCEL = 0, CHOICE_CLEAR = 1 } ClearChoice;
static ClearChoice s_choice;

static void update_highlight(void) {
  text_layer_set_background_color(s_cancel_label, s_choice == CHOICE_CANCEL ? GColorBlack : GColorWhite);
  text_layer_set_text_color(s_cancel_label,       s_choice == CHOICE_CANCEL ? GColorWhite : GColorBlack);
  text_layer_set_background_color(s_clear_label,  s_choice == CHOICE_CLEAR  ? GColorBlack : GColorWhite);
  text_layer_set_text_color(s_clear_label,        s_choice == CHOICE_CLEAR  ? GColorWhite : GColorBlack);
}

static void up_click(ClickRecognizerRef rec, void *ctx)   { (void)rec; (void)ctx;
  s_choice = CHOICE_CANCEL; update_highlight();
}
static void down_click(ClickRecognizerRef rec, void *ctx) { (void)rec; (void)ctx;
  s_choice = CHOICE_CLEAR; update_highlight();
}
static void select_click(ClickRecognizerRef rec, void *ctx) { (void)rec; (void)ctx;
  if (s_choice == CHOICE_CLEAR) storage_clear_all();
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

  s_status_label = text_layer_create(GRect(4, 0, b.size.w - 28, 16));
  text_layer_set_background_color(s_status_label, GColorClear);
  text_layer_set_text_color(s_status_label, GColorWhite);
  text_layer_set_text_alignment(s_status_label, GTextAlignmentCenter);
  text_layer_set_font(s_status_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_status_label, "Clear all data?");
  layer_add_child(root, text_layer_get_layer(s_status_label));

  s_body1 = text_layer_create(GRect(0, 26, b.size.w - 20, 18));
  text_layer_set_text(s_body1, "Erase every");
  text_layer_set_text_alignment(s_body1, GTextAlignmentCenter);
  text_layer_set_font(s_body1, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root, text_layer_get_layer(s_body1));

  s_body2 = text_layer_create(GRect(0, 42, b.size.w - 20, 18));
  text_layer_set_text(s_body2, "stored trial.");
  text_layer_set_text_alignment(s_body2, GTextAlignmentCenter);
  text_layer_set_font(s_body2, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root, text_layer_get_layer(s_body2));

  s_body3 = text_layer_create(GRect(0, 62, b.size.w - 20, 18));
  text_layer_set_text(s_body3, "Cannot be undone.");
  text_layer_set_text_alignment(s_body3, GTextAlignmentCenter);
  text_layer_set_font(s_body3, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(root, text_layer_get_layer(s_body3));

  s_cancel_label = text_layer_create(GRect(4, 100, b.size.w - 28, 24));
  text_layer_set_text(s_cancel_label, "Cancel");
  text_layer_set_text_alignment(s_cancel_label, GTextAlignmentCenter);
  text_layer_set_font(s_cancel_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(root, text_layer_get_layer(s_cancel_label));

  s_clear_label = text_layer_create(GRect(4, 132, b.size.w - 28, 24));
  text_layer_set_text(s_clear_label, "Clear data");
  text_layer_set_text_alignment(s_clear_label, GTextAlignmentCenter);
  text_layer_set_font(s_clear_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(root, text_layer_get_layer(s_clear_label));

  s_action = bw_action_bar_create(window);
  bw_action_bar_set_icons(s_action, BW_ICON_CHEVRON_UP, BW_ICON_CHECK, BW_ICON_CHEVRON_DOWN);

  s_choice = CHOICE_CANCEL;
  update_highlight();
}

static void window_unload(Window *window) {
  (void)window;
  bw_action_bar_destroy(s_action);
  text_layer_destroy(s_clear_label);
  text_layer_destroy(s_cancel_label);
  text_layer_destroy(s_body3);
  text_layer_destroy(s_body2);
  text_layer_destroy(s_body1);
  text_layer_destroy(s_status_label);
  status_bar_layer_destroy(s_status);
  window_destroy(s_window);
  s_window = NULL;
}

void clear_window_push(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}
