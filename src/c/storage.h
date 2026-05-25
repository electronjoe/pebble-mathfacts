#pragma once
#include <stdint.h>
#include "trial_history.h"
#include "fact_catalog.h"

/* Persist keys 0..15 hold per-FG histories (one TrialRecord array each).
 * Persist key 100 holds the last-viewed category (uint8_t). */
#define STORAGE_KEY_LAST_CATEGORY 100

/* Read the history for fact group `fg_id` into `out`. Returns number of
 * records read; 0 if none stored or buffer too small. */
uint8_t storage_load_history(uint8_t fg_id, TrialRecord *out, uint8_t out_cap);

/* Append a new trial, dropping the oldest if at capacity. */
void storage_append_trial(uint8_t fg_id, TrialRecord newest);

/* Delete all per-FG history keys and the last-category key. */
void storage_clear_all(void);

/* Last-viewed Fact Category accessors. Defaults to CATEGORY_SKIP if unset. */
FactCategory storage_load_last_category(void);
void storage_save_last_category(FactCategory cat);

/* Convenience: best (lowest) duration across all stored trials for `fg_id`,
 * or 0 if none. */
uint16_t storage_best_duration(uint8_t fg_id);

/* Convenience: trial count across all stored trials for `fg_id`. */
uint8_t storage_trial_count(uint8_t fg_id);
