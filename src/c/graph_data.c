#include "graph_data.h"
#include <string.h>

uint8_t graph_data_daily_best(const TrialRecord *records, uint8_t count,
                              uint32_t today_utc,
                              GraphPoint *out, uint8_t out_cap) {
  if (!records || count == 0 || !out || out_cap == 0) return 0;

  uint16_t best_for_day[GRAPH_WINDOW_DAYS + 1];
  for (int i = 0; i <= GRAPH_WINDOW_DAYS; i++) best_for_day[i] = 0xFFFF;

  uint32_t today_day = today_utc / 86400u;
  for (uint8_t i = 0; i < count; i++) {
    uint32_t rec_day = records[i].timestamp_utc / 86400u;
    if (rec_day > today_day) continue;
    uint32_t age = today_day - rec_day;
    if (age > GRAPH_WINDOW_DAYS) continue;
    if (records[i].duration_s < best_for_day[age]) {
      best_for_day[age] = records[i].duration_s;
    }
  }

  uint8_t n = 0;
  for (int age = GRAPH_WINDOW_DAYS; age >= 0 && n < out_cap; age--) {
    if (best_for_day[age] != 0xFFFF) {
      out[n].days_ago = (uint8_t)age;
      out[n].duration_s = best_for_day[age];
      n++;
    }
  }
  return n;
}

int16_t graph_data_pixel_x(uint8_t days_ago, const GraphRect *rect) {
  int32_t span = rect->x_max - rect->x_min;
  int32_t frac = (int32_t)(GRAPH_WINDOW_DAYS - days_ago) * span;
  return (int16_t)(rect->x_min + frac / GRAPH_WINDOW_DAYS);
}

int16_t graph_data_pixel_y(uint16_t duration_s, uint16_t best, uint16_t worst,
                           const GraphRect *rect) {
  int32_t span_y = rect->y_bottom - rect->y_top;
  if (worst == best) {
    return (int16_t)(rect->y_top + span_y / 2);
  }
  int32_t span_d = (int32_t)worst - (int32_t)best;
  int32_t frac = ((int32_t)duration_s - (int32_t)best) * span_y;
  return (int16_t)(rect->y_top + frac / span_d);
}
