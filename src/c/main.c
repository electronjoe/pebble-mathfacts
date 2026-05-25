#include <pebble.h>

static Window *s_home_window;

static void prv_home_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  (void)bounds;
}

static void prv_home_unload(Window *window) {
  (void)window;
}

static void prv_init(void) {
  s_home_window = window_create();
  window_set_window_handlers(s_home_window, (WindowHandlers){
    .load = prv_home_load,
    .unload = prv_home_unload,
  });
  window_stack_push(s_home_window, true);
}

static void prv_deinit(void) {
  window_destroy(s_home_window);
}

int main(void) {
  prv_init();
  APP_LOG(APP_LOG_LEVEL_INFO, "MathFacts started");
  app_event_loop();
  prv_deinit();
}
