#include "trial_history.h"
#include <string.h>

uint8_t trial_history_decode(const uint8_t *buf, uint16_t buf_len,
                             TrialRecord *out, uint8_t out_cap) {
  uint8_t n = 0;
  uint16_t i = 0;
  while (i + TRIAL_RECORD_BYTES <= buf_len && n < out_cap) {
    memcpy(&out[n], buf + i, TRIAL_RECORD_BYTES);
    i += TRIAL_RECORD_BYTES;
    n++;
  }
  return n;
}

uint16_t trial_history_encode(const TrialRecord *in, uint8_t in_count,
                              uint8_t *buf, uint16_t buf_cap) {
  uint16_t needed = (uint16_t)in_count * TRIAL_RECORD_BYTES;
  if (needed > buf_cap) needed = (buf_cap / TRIAL_RECORD_BYTES) * TRIAL_RECORD_BYTES;
  memcpy(buf, in, needed);
  return needed;
}

uint8_t trial_history_insert(TrialRecord *arr, uint8_t count, TrialRecord newest) {
  uint8_t new_count = (count < TRIAL_HISTORY_CAPACITY) ? count + 1 : TRIAL_HISTORY_CAPACITY;
  uint8_t shift = new_count - 1;
  if (shift > 0) {
    memmove(&arr[1], &arr[0], shift * sizeof(TrialRecord));
  }
  arr[0] = newest;
  return new_count;
}
