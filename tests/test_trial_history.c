#include "test_runner.h"
#include "trial_history.h"
#include <string.h>

static void test_record_size(void) {
  EXPECT_EQ_INT(sizeof(TrialRecord), 6, "TrialRecord is 6 bytes");
}

static void test_capacity(void) {
  EXPECT_EQ_INT(TRIAL_HISTORY_CAPACITY, 42, "capacity is 42 records");
}

static void test_decode_empty(void) {
  uint8_t buf[1] = {0};
  TrialRecord out[TRIAL_HISTORY_CAPACITY];
  uint8_t n = trial_history_decode(buf, 0, out, TRIAL_HISTORY_CAPACITY);
  EXPECT_EQ_INT(n, 0, "empty buffer decodes to 0 records");
}

static void test_decode_one(void) {
  uint8_t buf[6] = {
    0xEF, 0xBE, 0xAD, 0xDE,   /* little-endian 0xDEADBEEF */
    0xE8, 0x03                /* little-endian 1000 */
  };
  TrialRecord out[TRIAL_HISTORY_CAPACITY];
  uint8_t n = trial_history_decode(buf, sizeof(buf), out, TRIAL_HISTORY_CAPACITY);
  EXPECT_EQ_INT(n, 1, "decoded 1 record");
  EXPECT_EQ_INT(out[0].timestamp_utc, 0xDEADBEEF, "timestamp decoded");
  EXPECT_EQ_INT(out[0].duration_s, 1000, "duration decoded");
}

static void test_encode_roundtrip(void) {
  TrialRecord in[3] = {
    { .timestamp_utc = 1700000000, .duration_s = 90 },
    { .timestamp_utc = 1700001000, .duration_s = 80 },
    { .timestamp_utc = 1700002000, .duration_s = 70 },
  };
  uint8_t buf[256];
  uint16_t written = trial_history_encode(in, 3, buf, sizeof(buf));
  EXPECT_EQ_INT(written, 18, "encoded 3 records = 18 bytes");

  TrialRecord out[TRIAL_HISTORY_CAPACITY];
  uint8_t n = trial_history_decode(buf, written, out, TRIAL_HISTORY_CAPACITY);
  EXPECT_EQ_INT(n, 3, "decoded 3 records");
  EXPECT_EQ_INT(out[2].timestamp_utc, 1700002000, "third timestamp preserved");
  EXPECT_EQ_INT(out[2].duration_s, 70, "third duration preserved");
}

static void test_insert_prepends(void) {
  TrialRecord arr[TRIAL_HISTORY_CAPACITY] = {
    { .timestamp_utc = 100, .duration_s = 30 },
    { .timestamp_utc =  50, .duration_s = 40 },
  };
  uint8_t count = 2;
  TrialRecord newest = { .timestamp_utc = 200, .duration_s = 25 };
  count = trial_history_insert(arr, count, newest);
  EXPECT_EQ_INT(count, 3, "count grows to 3");
  EXPECT_EQ_INT(arr[0].timestamp_utc, 200, "newest at index 0");
  EXPECT_EQ_INT(arr[1].timestamp_utc, 100, "previous newest at index 1");
  EXPECT_EQ_INT(arr[2].timestamp_utc,  50, "oldest at index 2");
}

static void test_insert_trims_at_capacity(void) {
  TrialRecord arr[TRIAL_HISTORY_CAPACITY];
  for (uint8_t i = 0; i < TRIAL_HISTORY_CAPACITY; i++) {
    arr[i].timestamp_utc = 1000 - i;
    arr[i].duration_s = i;
  }
  TrialRecord newest = { .timestamp_utc = 5000, .duration_s = 99 };
  uint8_t count = trial_history_insert(arr, TRIAL_HISTORY_CAPACITY, newest);
  EXPECT_EQ_INT(count, TRIAL_HISTORY_CAPACITY, "count stays at capacity");
  EXPECT_EQ_INT(arr[0].timestamp_utc, 5000, "new record at front");
  EXPECT_EQ_INT(arr[TRIAL_HISTORY_CAPACITY-1].timestamp_utc, 1000 - (TRIAL_HISTORY_CAPACITY-2),
                "oldest record dropped");
}

int main(void) {
  RUN_TEST(test_record_size);
  RUN_TEST(test_capacity);
  RUN_TEST(test_decode_empty);
  RUN_TEST(test_decode_one);
  RUN_TEST(test_encode_roundtrip);
  RUN_TEST(test_insert_prepends);
  RUN_TEST(test_insert_trims_at_capacity);
  TEST_MAIN_RETURN();
}
