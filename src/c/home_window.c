#include "home_window.h"
#include "storage.h"
#include "graph_data.h"
#include "trial_history.h"
#include <time.h>

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_category_label;
static TextLayer *s_hint_label;
static Layer *s_graph_layer;
static FactCategory s_current_category;

#define GRAPH_X_MIN 18
#define GRAPH_X_MAX 142
#define GRAPH_Y_TOP 20
#define GRAPH_Y_BOTTOM 136

static const GraphRect GRAPH_RECT = {
  .x_min = GRAPH_X_MIN, .x_max = GRAPH_X_MAX,
  .y_top = GRAPH_Y_TOP, .y_bottom = GRAPH_Y_BOTTOM,
};

static const char *category_short_name(FactCategory c) {
  switch (c) {
    case CATEGORY_SKIP:    return "Skip";
    case CATEGORY_MULT:    return "Mult";
    case CATEGORY_MULTDIV: return "M&D";
    default:               return "?";
  }
}

/* Patterns: 0 = solid, 1 = dashed, 2 = dotted, 3 = dash-dot.
 * Drawn pixel-by-pixel using an 8-pixel phase cycle. */
static void draw_styled_line(GContext *ctx, GPoint a, GPoint b, uint8_t pattern_idx) {
  int dx = b.x - a.x; int dy = b.y - a.y;
  int adx = dx < 0 ? -dx : dx;
  int ady = dy < 0 ? -dy : dy;
  int steps = adx > ady ? adx : ady;
  if (steps == 0) { graphics_draw_pixel(ctx, a); return; }
  for (int i = 0; i <= steps; i++) {
    bool draw = true;
    int phase = i & 7;
    switch (pattern_idx & 3) {
      case 0: draw = true; break;
      case 1: draw = (phase < 4); break;
      case 2: draw = (phase & 1) == 0; break;
      case 3: draw = (phase < 3) || phase == 5; break;
    }
    if (draw) {
      int16_t x = (int16_t)(a.x + (dx * i) / steps);
      int16_t y = (int16_t)(a.y + (dy * i) / steps);
      graphics_draw_pixel(ctx, GPoint(x, y));
    }
  }
}

static void draw_axis(GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, GPoint(GRAPH_X_MIN, GRAPH_Y_TOP), GPoint(GRAPH_X_MIN, GRAPH_Y_BOTTOM));
  graphics_draw_line(ctx, GPoint(GRAPH_X_MIN, GRAPH_Y_BOTTOM), GPoint(GRAPH_X_MAX, GRAPH_Y_BOTTOM));
  graphics_draw_text(ctx, "120d",
    fonts_get_system_font(FONT_KEY_GOTHIC_14),
    GRect(GRAPH_X_MIN - 2, GRAPH_Y_BOTTOM + 1, 30, 14),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, "today",
    fonts_get_system_font(FONT_KEY_GOTHIC_14),
    GRect(GRAPH_X_MAX - 32, GRAPH_Y_BOTTOM + 1, 32, 14),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
}

static void draw_one_series(GContext *ctx, uint8_t fg_id, uint16_t best_all, uint16_t worst_all,
                            uint8_t pattern_idx) {
  TrialRecord arr[TRIAL_HISTORY_CAPACITY];
  uint8_t n = storage_load_history(fg_id, arr, TRIAL_HISTORY_CAPACITY);
  if (n == 0) return;

  GraphPoint pts[GRAPH_MAX_POINTS];
  uint32_t today_utc = (uint32_t)time(NULL);
  uint8_t m = graph_data_daily_best(arr, n, today_utc, pts, GRAPH_MAX_POINTS);
  if (m == 0) return;

  GPoint prev = GPoint(
    graph_data_pixel_x(pts[0].days_ago, &GRAPH_RECT),
    graph_data_pixel_y(pts[0].duration_s, best_all, worst_all, &GRAPH_RECT)
  );
  for (uint8_t i = 1; i < m; i++) {
    GPoint next = GPoint(
      graph_data_pixel_x(pts[i].days_ago, &GRAPH_RECT),
      graph_data_pixel_y(pts[i].duration_s, best_all, worst_all, &GRAPH_RECT)
    );
    draw_styled_line(ctx, prev, next, pattern_idx);
    prev = next;
  }

  const FactGroup *fg = fact_group_at(fg_id);
  if (fg) {
    graphics_draw_text(ctx, fg->graph_label,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(prev.x - 16, prev.y - 7, 18, 14),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  }
}

static void update_proc(Layer *layer, GContext *ctx) {
  (void)layer;
  graphics_context_set_text_color(ctx, GColorBlack);
  draw_axis(ctx);

  uint16_t best_all = 0xFFFF, worst_all = 0;
  for (uint8_t i = 0; i < FACT_GROUP_COUNT; i++) {
    const FactGroup *fg = fact_group_at(i);
    if (fg->category != s_current_category) continue;
    TrialRecord arr[TRIAL_HISTORY_CAPACITY];
    uint8_t n = storage_load_history(i, arr, TRIAL_HISTORY_CAPACITY);
    for (uint8_t j = 0; j < n; j++) {
      if (arr[j].duration_s < best_all)  best_all  = arr[j].duration_s;
      if (arr[j].duration_s > worst_all) worst_all = arr[j].duration_s;
    }
  }

  if (best_all == 0xFFFF) {
    graphics_draw_text(ctx, "no data yet",
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(0, GRAPH_Y_TOP + 40, 144, 18),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    return;
  }

  uint8_t within_idx = 0;
  for (uint8_t i = 0; i < FACT_GROUP_COUNT; i++) {
    const FactGroup *fg = fact_group_at(i);
    if (fg->category != s_current_category) continue;
    draw_one_series(ctx, i, best_all, worst_all, within_idx);
    within_idx++;
  }
}

static void apply_category(FactCategory c) {
  s_current_category = c;
  text_layer_set_text(s_category_label, category_short_name(c));
  storage_save_last_category(c);
  if (s_graph_layer) layer_mark_dirty(s_graph_layer);
}

static void up_click(ClickRecognizerRef rec, void *ctx)   { (void)rec; (void)ctx;
  apply_category((FactCategory)(((int)s_current_category + CATEGORY_COUNT - 1) % CATEGORY_COUNT));
}
static void down_click(ClickRecognizerRef rec, void *ctx) { (void)rec; (void)ctx;
  apply_category((FactCategory)(((int)s_current_category + 1) % CATEGORY_COUNT));
}
static void click_config(void *ctx) { (void)ctx;
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

  s_graph_layer = layer_create(GRect(0, 0, b.size.w, 150));
  layer_set_update_proc(s_graph_layer, update_proc);
  layer_add_child(root, s_graph_layer);

  s_hint_label = text_layer_create(GRect(0, 150, b.size.w, 18));
  text_layer_set_text(s_hint_label, "\xE2\x96\xB2\xE2\x96\xBC category   \xE2\x97\x8F select");
  text_layer_set_text_alignment(s_hint_label, GTextAlignmentCenter);
  text_layer_set_font(s_hint_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root, text_layer_get_layer(s_hint_label));

  apply_category(storage_load_last_category());
}

static void window_unload(Window *window) {
  (void)window;
  layer_destroy(s_graph_layer);
  text_layer_destroy(s_hint_label);
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
