#include "challenge_window.h"
#include "trial_engine.h"
#include "trial_history.h"
#include "storage.h"
#include "summary_window.h"
#include "bw_action_bar.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_status_left, *s_status_right;
static TextLayer *s_eq_line1, *s_eq_line2;
static TextLayer *s_timer;
static BWActionBar *s_action;

static const FactGroup *s_fg;
static TrialQueue s_queue;
static uint64_t s_trial_start_ms;
static AppTimer *s_tick_timer;

static void schedule_tick(void);

static uint64_t now_ms(void) {
  time_t s; uint16_t ms;
  time_ms(&s, &ms);
  return (uint64_t)s * 1000ull + ms;
}

static void format_mmss(uint16_t total_s, char *out, size_t cap) {
  uint16_t m = total_s / 60;
  uint16_t s = total_s % 60;
  /* U+23F1 stopwatch is supported by Pebble's bitmap font; fall back to text "T" if not. */
  snprintf(out, cap, "%u:%02u", m, s);
}

static void render_current_question(void) {
  if (!s_fg) return;
  if (s_fg->category == CATEGORY_SKIP) {
    static char l1[16], l2[16];
    snprintf(l1, sizeof(l1), "Skip count");
    snprintf(l2, sizeof(l2), "by %u", s_fg->skip_by);
    text_layer_set_text(s_eq_line1, l1);
    text_layer_set_text(s_eq_line2, l2);
    return;
  }

  Question *qn = &s_queue.questions[s_queue.position];
  uint8_t form = trial_engine_pick_form(qn);
  static char l1[8], l2[8];
  uint16_t c = (uint16_t)qn->a * qn->b;
  /* Use ASCII 'x' and '/' — Pebble system fonts don't include U+00D7 / U+00F7. */
  if (qn->op == OP_MULT) {
    switch (form) {
      case 1: snprintf(l1, sizeof(l1), "?x%u", qn->b);
              snprintf(l2, sizeof(l2), "=%u", c); break;
      case 2: snprintf(l1, sizeof(l1), "%ux?", qn->a);
              snprintf(l2, sizeof(l2), "=%u", c); break;
      case 3: snprintf(l1, sizeof(l1), "%u=", c);
              snprintf(l2, sizeof(l2), "?x%u", qn->b); break;
      default: snprintf(l1, sizeof(l1), "%u=", c);
               snprintf(l2, sizeof(l2), "%ux?", qn->a); break;
    }
  } else {  /* OP_DIV */
    switch (form) {
      case 5: snprintf(l1, sizeof(l1), "?/%u", qn->a);
              snprintf(l2, sizeof(l2), "=%u", qn->b); break;
      case 6: snprintf(l1, sizeof(l1), "%u/?", c);
              snprintf(l2, sizeof(l2), "=%u", qn->b); break;
      case 7: snprintf(l1, sizeof(l1), "%u=", qn->b);
              snprintf(l2, sizeof(l2), "?/%u", qn->a); break;
      default: snprintf(l1, sizeof(l1), "%u=", qn->b);
               snprintf(l2, sizeof(l2), "%u/?", c); break;
    }
  }
  text_layer_set_text(s_eq_line1, l1);
  text_layer_set_text(s_eq_line2, l2);
}

static void render_status_right(void) {
  if (!s_fg) return;
  static char buf[12];
  if (s_fg->category == CATEGORY_SKIP) {
    buf[0] = '\0';
  } else {
    snprintf(buf, sizeof(buf), "%u/%u", s_queue.answered + 1, s_queue.total);
  }
  text_layer_set_text(s_status_right, buf);
}

static void render_timer(void) {
  uint64_t elapsed_ms = now_ms() - s_trial_start_ms;
  static char buf[16];
  format_mmss((uint16_t)(elapsed_ms / 1000ull), buf, sizeof(buf));
  text_layer_set_text(s_timer, buf);
}

static void tick_cb(void *data) {
  (void)data;
  s_tick_timer = NULL;
  render_timer();
  schedule_tick();
}

static void schedule_tick(void) {
  s_tick_timer = app_timer_register(1000, tick_cb, NULL);
}

static void cancel_tick(void) {
  if (s_tick_timer) { app_timer_cancel(s_tick_timer); s_tick_timer = NULL; }
}

static void finish_trial(void) {
  cancel_tick();
  uint64_t elapsed_ms = now_ms() - s_trial_start_ms;
  uint16_t duration_s = (uint16_t)(elapsed_ms / 1000ull);
  summary_window_push(s_fg->id, duration_s);
  window_stack_remove(s_window, false);
}

static void cancel_trial(void) {
  cancel_tick();
  /* FG Selection already removed itself when it pushed us; popping just
   * Challenge therefore lands on Home. */
  window_stack_remove(s_window, false);
}

static void up_click(ClickRecognizerRef rec, void *ctx) {
  (void)rec; (void)ctx;
  cancel_trial();
}

static void select_click(ClickRecognizerRef rec, void *ctx) {
  (void)rec; (void)ctx;
  if (s_fg->category == CATEGORY_SKIP) {
    finish_trial();
    return;
  }
  trial_engine_advance_correct(&s_queue);
  if (trial_engine_is_complete(&s_queue)) {
    finish_trial();
  } else {
    render_status_right();
    render_current_question();
  }
}

static void down_click(ClickRecognizerRef rec, void *ctx) {
  (void)rec; (void)ctx;
  if (s_fg->category == CATEGORY_SKIP) return;
  trial_engine_skip(&s_queue);
  if (trial_engine_is_complete(&s_queue)) {
    finish_trial();
  } else {
    render_status_right();
    render_current_question();
  }
}

static void click_config(void *ctx) {
  (void)ctx;
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);

  s_status = status_bar_layer_create();
  status_bar_layer_set_colors(s_status, GColorBlack, GColorWhite);
  layer_add_child(root, status_bar_layer_get_layer(s_status));

  s_status_left = text_layer_create(GRect(4, 0, 90, 16));
  text_layer_set_background_color(s_status_left, GColorClear);
  text_layer_set_text_color(s_status_left, GColorWhite);
  text_layer_set_font(s_status_left, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_status_left, s_fg->status_label);
  layer_add_child(root, text_layer_get_layer(s_status_left));

  s_status_right = text_layer_create(GRect(b.size.w - 60, 0, 56, 16));
  text_layer_set_background_color(s_status_right, GColorClear);
  text_layer_set_text_color(s_status_right, GColorWhite);
  text_layer_set_text_alignment(s_status_right, GTextAlignmentRight);
  text_layer_set_font(s_status_right, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root, text_layer_get_layer(s_status_right));

  GFont big = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_eq_line1 = text_layer_create(GRect(0, 40, b.size.w - 20, 36));
  text_layer_set_background_color(s_eq_line1, GColorClear);
  text_layer_set_text_alignment(s_eq_line1, GTextAlignmentCenter);
  text_layer_set_font(s_eq_line1, big);
  layer_add_child(root, text_layer_get_layer(s_eq_line1));

  s_eq_line2 = text_layer_create(GRect(0, 78, b.size.w - 20, 36));
  text_layer_set_background_color(s_eq_line2, GColorClear);
  text_layer_set_text_alignment(s_eq_line2, GTextAlignmentCenter);
  text_layer_set_font(s_eq_line2, big);
  layer_add_child(root, text_layer_get_layer(s_eq_line2));

  s_timer = text_layer_create(GRect(0, 130, b.size.w - 20, 20));
  text_layer_set_background_color(s_timer, GColorClear);
  text_layer_set_text_alignment(s_timer, GTextAlignmentCenter);
  text_layer_set_font(s_timer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root, text_layer_get_layer(s_timer));

  s_action = bw_action_bar_create(window);
  BWIconKind down_icon = (s_fg->category == CATEGORY_SKIP) ? BW_ICON_NONE : BW_ICON_SKIP;
  bw_action_bar_set_icons(s_action, BW_ICON_CANCEL, BW_ICON_CHECK, down_icon);

  render_status_right();
  render_current_question();
  render_timer();
  schedule_tick();
}

static void window_unload(Window *window) {
  (void)window;
  cancel_tick();
  bw_action_bar_destroy(s_action);
  text_layer_destroy(s_timer);
  text_layer_destroy(s_eq_line2);
  text_layer_destroy(s_eq_line1);
  text_layer_destroy(s_status_right);
  text_layer_destroy(s_status_left);
  status_bar_layer_destroy(s_status);
  window_destroy(s_window);
  s_window = NULL;
}

void challenge_window_push(uint8_t fg_id) {
  s_fg = fact_group_at(fg_id);
  if (!s_fg) return;
  trial_engine_build(&s_queue, s_fg);
  s_trial_start_ms = now_ms();

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}
