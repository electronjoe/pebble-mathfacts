#include "test_runner.h"
#include "graph_data.h"
#include "trial_history.h"

/* Helper: build a TrialRecord at a synthetic UTC midnight + offset_s. */
static TrialRecord rec(uint32_t day_index, uint16_t duration_s) {
  TrialRecord r = { .timestamp_utc = day_index * 86400u + 60u, .duration_s = duration_s };
  return r;
}

static void test_empty_input_produces_no_points(void) {
  GraphPoint pts[GRAPH_MAX_POINTS];
  uint8_t n = graph_data_daily_best(NULL, 0, 1000u, pts, GRAPH_MAX_POINTS);
  EXPECT_EQ_INT(n, 0, "no input → no points");
}

static void test_daily_best_picks_lowest(void) {
  TrialRecord input[] = {
    rec(10, 90),  /* day 10, 90 s */
    rec(10, 70),  /* day 10, 70 s — best for day 10 */
    rec(10, 80),  /* day 10, 80 s */
    rec( 9, 100), /* day 9, 100 s */
  };
  GraphPoint pts[GRAPH_MAX_POINTS];
  uint32_t today_utc = 11u * 86400u;
  uint8_t n = graph_data_daily_best(input, 4, today_utc, pts, GRAPH_MAX_POINTS);
  EXPECT_EQ_INT(n, 2, "two distinct days");
  EXPECT_EQ_INT(pts[0].days_ago, 2, "earlier day comes first");
  EXPECT_EQ_INT(pts[0].duration_s, 100, "day 9 best = 100");
  EXPECT_EQ_INT(pts[1].days_ago, 1, "later day next");
  EXPECT_EQ_INT(pts[1].duration_s, 70, "day 10 best = 70");
}

static void test_records_outside_window_dropped(void) {
  TrialRecord input[] = {
    rec( 0, 50),    /* very old */
    rec(150, 40),   /* still old, beyond 120-day window */
    rec(155, 60),
  };
  GraphPoint pts[GRAPH_MAX_POINTS];
  uint32_t today_utc = 300u * 86400u;
  uint8_t n = graph_data_daily_best(input, 3, today_utc, pts, GRAPH_MAX_POINTS);
  EXPECT_EQ_INT(n, 0, "all records too old, none included");
}

static void test_pixel_x_maps_window(void) {
  /* days_ago=120 → x = x_min ; days_ago=0 → x = x_max */
  GraphRect r = { .x_min = 20, .x_max = 140, .y_top = 20, .y_bottom = 136 };
  EXPECT_EQ_INT(graph_data_pixel_x(120, &r), 20, "oldest at x_min");
  EXPECT_EQ_INT(graph_data_pixel_x(0,   &r), 140, "today at x_max");
  EXPECT_EQ_INT(graph_data_pixel_x(60,  &r), 80,  "midpoint");
}

static void test_pixel_y_maps_inverted_range(void) {
  /* fastest (min duration) → y_top ; slowest (max) → y_bottom */
  GraphRect r = { .x_min = 20, .x_max = 140, .y_top = 20, .y_bottom = 136 };
  EXPECT_EQ_INT(graph_data_pixel_y(30, 30, 90, &r),  20, "best time at y_top");
  EXPECT_EQ_INT(graph_data_pixel_y(90, 30, 90, &r), 136, "worst time at y_bottom");
  EXPECT_EQ_INT(graph_data_pixel_y(60, 30, 90, &r),  78, "midpoint");
}

static void test_pixel_y_equal_range_defaults_center(void) {
  GraphRect r = { .x_min = 20, .x_max = 140, .y_top = 20, .y_bottom = 136 };
  /* All trials same duration → divisor would be zero; expect center. */
  EXPECT_EQ_INT(graph_data_pixel_y(45, 45, 45, &r), 78, "flat range → center");
}

int main(void) {
  RUN_TEST(test_empty_input_produces_no_points);
  RUN_TEST(test_daily_best_picks_lowest);
  RUN_TEST(test_records_outside_window_dropped);
  RUN_TEST(test_pixel_x_maps_window);
  RUN_TEST(test_pixel_y_maps_inverted_range);
  RUN_TEST(test_pixel_y_equal_range_defaults_center);
  TEST_MAIN_RETURN();
}
