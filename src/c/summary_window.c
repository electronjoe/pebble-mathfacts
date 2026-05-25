#include "summary_window.h"
/* Stub — replaced in Task 14. */

static Window *s_window;

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  TextLayer *tl = text_layer_create(layer_get_bounds(root));
  text_layer_set_text(tl, "Summary\n(coming soon)");
  text_layer_set_text_alignment(tl, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(tl));
  window_set_user_data(window, tl);
}

static void window_unload(Window *window) {
  text_layer_destroy((TextLayer *)window_get_user_data(window));
  window_destroy(s_window);
  s_window = NULL;
}

void summary_window_push(uint8_t fg_id, uint16_t duration_s) {
  (void)fg_id; (void)duration_s;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load, .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
