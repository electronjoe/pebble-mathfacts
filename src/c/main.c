#include <pebble.h>
#include <time.h>
#include <stdlib.h>
#include "home_window.h"

static void prv_init(void) {
  srand((unsigned)time(NULL));
  home_window_push();
}

static void prv_deinit(void) {
  home_window_destroy();
}

int main(void) {
  prv_init();
  APP_LOG(APP_LOG_LEVEL_INFO, "MathFacts started");
  app_event_loop();
  prv_deinit();
}
