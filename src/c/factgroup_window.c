#include "factgroup_window.h"

static Window *s_window;
static FactCategory s_cat;

static void window_load(Window *window) {
  (void)s_cat;
  Layer *root = window_get_root_layer(window);
  TextLayer *tl = text_layer_create(layer_get_bounds(root));
  text_layer_set_text(tl, "Fact group\n(coming soon)");
  text_layer_set_text_alignment(tl, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(tl));
  window_set_user_data(window, tl);
}
static void window_unload(Window *window) {
  text_layer_destroy((TextLayer *)window_get_user_data(window));
  window_destroy(s_window);
  s_window = NULL;
}

void factgroup_window_push(FactCategory cat) {
  s_cat = cat;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load, .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
