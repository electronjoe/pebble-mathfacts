#include <pebble.h>
#include <string.h>
#include "storage.h"

uint8_t storage_load_history(uint8_t fg_id, TrialRecord *out, uint8_t out_cap) {
  if (fg_id >= FACT_GROUP_COUNT || !out) return 0;
  uint8_t buf[TRIAL_RECORD_BYTES * TRIAL_HISTORY_CAPACITY];
  int read = persist_read_data(fg_id, buf, sizeof(buf));
  if (read <= 0) return 0;
  return trial_history_decode(buf, (uint16_t)read, out, out_cap);
}

void storage_append_trial(uint8_t fg_id, TrialRecord newest) {
  if (fg_id >= FACT_GROUP_COUNT) return;
  TrialRecord arr[TRIAL_HISTORY_CAPACITY];
  uint8_t count = storage_load_history(fg_id, arr, TRIAL_HISTORY_CAPACITY);
  count = trial_history_insert(arr, count, newest);
  uint8_t buf[TRIAL_RECORD_BYTES * TRIAL_HISTORY_CAPACITY];
  uint16_t written = trial_history_encode(arr, count, buf, sizeof(buf));
  persist_write_data(fg_id, buf, written);
}

void storage_clear_all(void) {
  for (uint8_t i = 0; i < FACT_GROUP_COUNT; i++) {
    persist_delete(i);
  }
  persist_delete(STORAGE_KEY_LAST_CATEGORY);
}

FactCategory storage_load_last_category(void) {
  if (!persist_exists(STORAGE_KEY_LAST_CATEGORY)) return CATEGORY_SKIP;
  int v = persist_read_int(STORAGE_KEY_LAST_CATEGORY);
  if (v < 0 || v >= CATEGORY_COUNT) return CATEGORY_SKIP;
  return (FactCategory)v;
}

void storage_save_last_category(FactCategory cat) {
  persist_write_int(STORAGE_KEY_LAST_CATEGORY, (int32_t)cat);
}

uint16_t storage_best_duration(uint8_t fg_id) {
  TrialRecord arr[TRIAL_HISTORY_CAPACITY];
  uint8_t n = storage_load_history(fg_id, arr, TRIAL_HISTORY_CAPACITY);
  uint16_t best = 0xFFFF;
  for (uint8_t i = 0; i < n; i++) {
    if (arr[i].duration_s < best) best = arr[i].duration_s;
  }
  return (best == 0xFFFF) ? 0 : best;
}

uint8_t storage_trial_count(uint8_t fg_id) {
  TrialRecord arr[TRIAL_HISTORY_CAPACITY];
  return storage_load_history(fg_id, arr, TRIAL_HISTORY_CAPACITY);
}
