#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
  uint32_t timestamp_utc;
  uint16_t duration_s;
} TrialRecord;

#define TRIAL_RECORD_BYTES 6
#define TRIAL_HISTORY_CAPACITY 42

uint8_t trial_history_decode(const uint8_t *buf, uint16_t buf_len,
                             TrialRecord *out, uint8_t out_cap);

uint16_t trial_history_encode(const TrialRecord *in, uint8_t in_count,
                              uint8_t *buf, uint16_t buf_cap);

uint8_t trial_history_insert(TrialRecord *arr, uint8_t count, TrialRecord newest);
