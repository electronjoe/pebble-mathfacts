#pragma once
#include <stdint.h>
#include "trial_history.h"

#define GRAPH_WINDOW_DAYS 120
#define GRAPH_MAX_POINTS  GRAPH_WINDOW_DAYS

typedef struct {
  uint8_t  days_ago;     /* 0..GRAPH_WINDOW_DAYS, 0 = today */
  uint16_t duration_s;
} GraphPoint;

typedef struct {
  int16_t x_min;
  int16_t x_max;
  int16_t y_top;
  int16_t y_bottom;
} GraphRect;

/* Aggregate `records` (newest-first or any order) into one point per local
 * day (using `today_utc` as a reference midnight). Records older than
 * GRAPH_WINDOW_DAYS are dropped. The output is sorted oldest → newest.
 * Returns number of points written. */
uint8_t graph_data_daily_best(const TrialRecord *records, uint8_t count,
                              uint32_t today_utc,
                              GraphPoint *out, uint8_t out_cap);

/* Convert a days_ago value to an x pixel inside `rect`. days_ago == 0 maps to
 * x_max; days_ago == GRAPH_WINDOW_DAYS maps to x_min. */
int16_t graph_data_pixel_x(uint8_t days_ago, const GraphRect *rect);

/* Convert a duration to a y pixel inside `rect`. The duration `best` maps to
 * y_top; `worst` maps to y_bottom. When best == worst the function returns
 * the vertical center of the rect. */
int16_t graph_data_pixel_y(uint16_t duration_s, uint16_t best, uint16_t worst,
                           const GraphRect *rect);
