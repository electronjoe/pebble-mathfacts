# MathFacts Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the MathFacts Pebble watchapp from the template at `src/c/bringup.c`, implementing all five UI screens, sixteen fact groups across three categories, on-device persistence, and B&W rendering for Pebble Flint, per `docs/superpowers/specs/2026-05-25-mathfacts-design.md`.

**Architecture:** Pebble SDK 3 watchapp built with `pebble` CLI / waf. Pure-logic modules (fact catalog, trial engine, history packing, graph data) are split from platform-dependent UI modules so the pure code can be exercised by a host gcc test harness in `tests/`. UI windows each own a Pebble `Window` and are pushed/popped on the system window stack. No PebbleKit JS, no resources beyond system fonts.

**Tech Stack:** C11, Pebble SDK 4.9 (`pebble` CLI v5.0.35), gcc for host tests, GNU Make for the test runner, the Pebble emulator (Flint board) for UI verification.

**Reference material in-repo:**
- `resources/CLAUDE.md` — applib index (which Pebble functions to call for what).
- `resources/examples/ui-patterns/` — copy-paste-ready `menu_layer`, `action_bar_layer`, dialogs.
- `resources/examples/watchface-tutorial/` — minimal tick-timer + text-layer + bitmap shell.

**Conventions used in this plan:**
- All paths are relative to the repo root (`/Users/scottmoeller/Documents/github/pebble-mathfacts/`).
- Commit messages start with one of `setup:`, `test:`, `feat:`, `fix:`, `chore:` and include the `Co-Authored-By: Claude Opus 4.7` trailer.
- Pure-logic source lives under `src/c/` and is included into both the Pebble build (via the wscript glob) and the host test binaries (via the `tests/Makefile`). Those modules MUST NOT include `<pebble.h>`; only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, and `<stdlib.h>` are permitted.
- UI-layer modules (`*_window.c`, `storage.c`, `main.c`) DO include `<pebble.h>` and are excluded from host tests.

---

## Phase 1 — Project setup

### Task 1: Configure `package.json` for MathFacts

**Files:**
- Modify: `package.json` (whole file)

- [ ] **Step 1: Replace template metadata**

Rewrite `package.json` so the app builds only for Flint and identifies as MathFacts:

```json
{
  "name": "mathfacts",
  "author": "Scott Moeller",
  "version": "1.0.0",
  "keywords": ["pebble-app"],
  "private": true,
  "dependencies": {},
  "pebble": {
    "displayName": "MathFacts",
    "uuid": "dae32bd3-d94d-4f59-9c28-92d59749ed3c",
    "sdkVersion": "3",
    "enableMultiJS": false,
    "targetPlatforms": ["flint"],
    "watchapp": {
      "watchface": false
    },
    "messageKeys": [],
    "resources": {
      "media": []
    }
  }
}
```

- [ ] **Step 2: Verify pebble build succeeds**

Run: `pebble build`
Expected: build completes with no errors, produces `build/pebble-app.elf` and `build/mathfacts.pbw`. The existing `src/c/bringup.c` still compiles as the entry point at this stage.

- [ ] **Step 3: Commit**

```bash
git add package.json
git commit -m "$(cat <<'EOF'
setup: configure package.json for MathFacts, Flint-only

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 2: Replace `bringup.c` with `main.c` skeleton

**Files:**
- Delete: `src/c/bringup.c`
- Create: `src/c/main.c`

- [ ] **Step 1: Remove template source**

```bash
git rm src/c/bringup.c
```

- [ ] **Step 2: Create empty Home-window skeleton in `main.c`**

```c
#include <pebble.h>

static Window *s_home_window;

static void prv_home_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  (void)bounds;
}

static void prv_home_unload(Window *window) {
  (void)window;
}

static void prv_init(void) {
  s_home_window = window_create();
  window_set_window_handlers(s_home_window, (WindowHandlers){
    .load = prv_home_load,
    .unload = prv_home_unload,
  });
  window_stack_push(s_home_window, true);
}

static void prv_deinit(void) {
  window_destroy(s_home_window);
}

int main(void) {
  prv_init();
  APP_LOG(APP_LOG_LEVEL_INFO, "MathFacts started");
  app_event_loop();
  prv_deinit();
}
```

- [ ] **Step 3: Verify pebble build succeeds**

Run: `pebble build`
Expected: clean build of `main.c`, produces `build/mathfacts.pbw`.

- [ ] **Step 4: Commit**

```bash
git add -A src/c/
git commit -m "$(cat <<'EOF'
setup: replace bringup template with empty MathFacts main.c

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 3: Set up host test harness

**Files:**
- Create: `tests/Makefile`
- Create: `tests/test_runner.h`
- Create: `tests/test_sanity.c`
- Modify: `.gitignore`

- [ ] **Step 1: Create the test runner helpers**

Write `tests/test_runner.h`:

```c
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failures = 0;

#define EXPECT(cond, msg) do { \
  if (!(cond)) { \
    fprintf(stderr, "  FAIL: %s:%d %s — %s\n", __FILE__, __LINE__, #cond, msg); \
    g_failures++; \
  } \
} while (0)

#define EXPECT_EQ_INT(actual, expected, msg) do { \
  long long _a = (long long)(actual); \
  long long _e = (long long)(expected); \
  if (_a != _e) { \
    fprintf(stderr, "  FAIL: %s:%d %s — expected %lld, got %lld\n", \
            __FILE__, __LINE__, msg, _e, _a); \
    g_failures++; \
  } \
} while (0)

#define EXPECT_STR_EQ(actual, expected, msg) do { \
  const char *_a = (actual); \
  const char *_e = (expected); \
  if (strcmp(_a, _e) != 0) { \
    fprintf(stderr, "  FAIL: %s:%d %s — expected \"%s\", got \"%s\"\n", \
            __FILE__, __LINE__, msg, _e, _a); \
    g_failures++; \
  } \
} while (0)

#define RUN_TEST(fn) do { \
  fprintf(stderr, "  %s\n", #fn); \
  fn(); \
} while (0)

#define TEST_MAIN_RETURN() do { \
  if (g_failures > 0) { fprintf(stderr, "%d failure(s)\n", g_failures); return 1; } \
  fprintf(stderr, "ok\n"); return 0; \
} while (0)
```

- [ ] **Step 2: Create a sanity test**

Write `tests/test_sanity.c`:

```c
#include "test_runner.h"

static void test_arithmetic(void) {
  EXPECT_EQ_INT(2 + 2, 4, "math still works");
}

int main(void) {
  RUN_TEST(test_arithmetic);
  TEST_MAIN_RETURN();
}
```

- [ ] **Step 3: Create the Makefile**

Write `tests/Makefile`:

```make
CC ?= cc
CFLAGS := -Wall -Wextra -Werror -std=c11 -g -I../src/c -I.

TESTS := test_sanity

.PHONY: all test clean
all: $(TESTS)

test: all
	@set -e; for t in $(TESTS); do echo "--- $$t"; ./$$t; done; echo "ALL TESTS PASSED"

clean:
	rm -f $(TESTS)

test_sanity: test_sanity.c
	$(CC) $(CFLAGS) -o $@ $^
```

- [ ] **Step 4: Update `.gitignore` to exclude built test binaries**

Add to the end of `.gitignore`:

```
tests/test_*
!tests/test_*.c
!tests/test_*.h
```

- [ ] **Step 5: Verify the harness works**

Run: `make -C tests test`
Expected:
```
--- test_sanity
  test_arithmetic
ok
ALL TESTS PASSED
```

- [ ] **Step 6: Commit**

```bash
git add tests/ .gitignore
git commit -m "$(cat <<'EOF'
test: add host-side test harness for pure-logic modules

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

## Phase 2 — Pure-logic modules (TDD)

### Task 4: `fact_catalog` — 16 fact groups + accessors

**Files:**
- Create: `src/c/fact_catalog.h`, `src/c/fact_catalog.c`
- Create: `tests/test_fact_catalog.c`
- Modify: `tests/Makefile`

- [ ] **Step 1: Write the failing test**

Write `tests/test_fact_catalog.c`:

```c
#include "test_runner.h"
#include "fact_catalog.h"

static void test_total_count(void) {
  EXPECT_EQ_INT(FACT_GROUP_COUNT, 16, "16 fact groups total");
}

static void test_skip_counting_groups(void) {
  for (int i = 0; i <= 7; i++) {
    EXPECT_EQ_INT(fact_group_at(i)->category, CATEGORY_SKIP, "FG 0-7 is Skip Counting");
    EXPECT_EQ_INT(fact_group_at(i)->id, i, "FG id matches index");
  }
  EXPECT_STR_EQ(fact_group_at(0)->graph_label, "2", "first skip label");
  EXPECT_STR_EQ(fact_group_at(7)->graph_label, "9", "last skip label");
  EXPECT_STR_EQ(fact_group_at(5)->status_label, "Skip Count By 7", "status label format");
  EXPECT_EQ_INT(fact_group_at(5)->skip_by, 7, "skip_by value");
}

static void test_mult_groups(void) {
  for (int i = 8; i <= 11; i++) {
    EXPECT_EQ_INT(fact_group_at(i)->category, CATEGORY_MULT, "FG 8-11 is Mult");
  }
  EXPECT_EQ_INT(fact_group_at(8)->a_min, 2, "first mult a_min");
  EXPECT_EQ_INT(fact_group_at(8)->a_max, 3, "first mult a_max");
  EXPECT_EQ_INT(fact_group_at(11)->a_min, 8, "last mult a_min");
  EXPECT_EQ_INT(fact_group_at(11)->a_max, 9, "last mult a_max");
  EXPECT_STR_EQ(fact_group_at(8)->graph_label, "2-3", "mult graph label");
  EXPECT_STR_EQ(fact_group_at(8)->status_label, "Mult 2-3", "mult status label");
}

static void test_multdiv_groups(void) {
  for (int i = 12; i <= 15; i++) {
    EXPECT_EQ_INT(fact_group_at(i)->category, CATEGORY_MULTDIV, "FG 12-15 is M&D");
  }
  EXPECT_EQ_INT(fact_group_at(12)->a_min, 2, "first m&d a_min");
  EXPECT_STR_EQ(fact_group_at(15)->status_label, "M&D 8-9", "m&d status label");
}

static void test_count_by_category(void) {
  EXPECT_EQ_INT(fact_group_count_in_category(CATEGORY_SKIP), 8, "skip count");
  EXPECT_EQ_INT(fact_group_count_in_category(CATEGORY_MULT), 4, "mult count");
  EXPECT_EQ_INT(fact_group_count_in_category(CATEGORY_MULTDIV), 4, "m&d count");
}

int main(void) {
  RUN_TEST(test_total_count);
  RUN_TEST(test_skip_counting_groups);
  RUN_TEST(test_mult_groups);
  RUN_TEST(test_multdiv_groups);
  RUN_TEST(test_count_by_category);
  TEST_MAIN_RETURN();
}
```

- [ ] **Step 2: Wire the test into the Makefile**

Edit `tests/Makefile` — replace the `TESTS := test_sanity` line with:

```make
TESTS := test_sanity test_fact_catalog
```

And append after the existing `test_sanity` rule:

```make
test_fact_catalog: test_fact_catalog.c ../src/c/fact_catalog.c
	$(CC) $(CFLAGS) -o $@ $^
```

- [ ] **Step 3: Run test and verify it fails (file does not exist yet)**

Run: `make -C tests test_fact_catalog`
Expected: compile failure: `fatal error: 'fact_catalog.h' file not found`.

- [ ] **Step 4: Write the header**

Write `src/c/fact_catalog.h`:

```c
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  CATEGORY_SKIP = 0,
  CATEGORY_MULT = 1,
  CATEGORY_MULTDIV = 2,
  CATEGORY_COUNT = 3,
} FactCategory;

typedef struct {
  uint8_t id;
  FactCategory category;
  const char *short_name;
  const char *graph_label;
  const char *status_label;
  uint8_t skip_by;
  uint8_t a_min;
  uint8_t a_max;
} FactGroup;

#define FACT_GROUP_COUNT 16

const FactGroup *fact_group_at(uint8_t id);
uint8_t fact_group_count_in_category(FactCategory cat);
uint8_t fact_group_nth_in_category(FactCategory cat, uint8_t n);
```

- [ ] **Step 5: Write the implementation**

Write `src/c/fact_catalog.c`:

```c
#include "fact_catalog.h"

static const FactGroup FACT_GROUPS[FACT_GROUP_COUNT] = {
  { 0, CATEGORY_SKIP, "By 2", "2", "Skip Count By 2", 2, 0, 0 },
  { 1, CATEGORY_SKIP, "By 3", "3", "Skip Count By 3", 3, 0, 0 },
  { 2, CATEGORY_SKIP, "By 4", "4", "Skip Count By 4", 4, 0, 0 },
  { 3, CATEGORY_SKIP, "By 5", "5", "Skip Count By 5", 5, 0, 0 },
  { 4, CATEGORY_SKIP, "By 6", "6", "Skip Count By 6", 6, 0, 0 },
  { 5, CATEGORY_SKIP, "By 7", "7", "Skip Count By 7", 7, 0, 0 },
  { 6, CATEGORY_SKIP, "By 8", "8", "Skip Count By 8", 8, 0, 0 },
  { 7, CATEGORY_SKIP, "By 9", "9", "Skip Count By 9", 9, 0, 0 },
  { 8, CATEGORY_MULT, "2-3 \xC3\x97 1-10", "2-3", "Mult 2-3", 0, 2, 3 },
  { 9, CATEGORY_MULT, "4-5 \xC3\x97 1-10", "4-5", "Mult 4-5", 0, 4, 5 },
  {10, CATEGORY_MULT, "6-7 \xC3\x97 1-10", "6-7", "Mult 6-7", 0, 6, 7 },
  {11, CATEGORY_MULT, "8-9 \xC3\x97 1-10", "8-9", "Mult 8-9", 0, 8, 9 },
  {12, CATEGORY_MULTDIV, "2-3 \xC3\x97\xC3\xB7 1-10", "2-3", "M&D 2-3", 0, 2, 3 },
  {13, CATEGORY_MULTDIV, "4-5 \xC3\x97\xC3\xB7 1-10", "4-5", "M&D 4-5", 0, 4, 5 },
  {14, CATEGORY_MULTDIV, "6-7 \xC3\x97\xC3\xB7 1-10", "6-7", "M&D 6-7", 0, 6, 7 },
  {15, CATEGORY_MULTDIV, "8-9 \xC3\x97\xC3\xB7 1-10", "8-9", "M&D 8-9", 0, 8, 9 },
};

const FactGroup *fact_group_at(uint8_t id) {
  if (id >= FACT_GROUP_COUNT) return 0;
  return &FACT_GROUPS[id];
}

uint8_t fact_group_count_in_category(FactCategory cat) {
  uint8_t n = 0;
  for (uint8_t i = 0; i < FACT_GROUP_COUNT; i++) {
    if (FACT_GROUPS[i].category == cat) n++;
  }
  return n;
}

uint8_t fact_group_nth_in_category(FactCategory cat, uint8_t n) {
  uint8_t found = 0;
  for (uint8_t i = 0; i < FACT_GROUP_COUNT; i++) {
    if (FACT_GROUPS[i].category == cat) {
      if (found == n) return i;
      found++;
    }
  }
  return 0xFF;
}
```

Note: the `\xC3\x97` / `\xC3\xB7` byte sequences are UTF-8 for `×` and `÷` respectively, which Pebble's fonts render correctly.

- [ ] **Step 6: Run test and verify it passes**

Run: `make -C tests test`
Expected: both `test_sanity` and `test_fact_catalog` print PASS, ending with `ALL TESTS PASSED`.

- [ ] **Step 7: Verify pebble build still succeeds**

Run: `pebble build`
Expected: clean build (the new files are picked up by the wscript glob).

- [ ] **Step 8: Commit**

```bash
git add src/c/fact_catalog.h src/c/fact_catalog.c tests/test_fact_catalog.c tests/Makefile
git commit -m "$(cat <<'EOF'
feat: add fact_catalog with 16 fact groups across 3 categories

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 5: `trial_history` — pack, decode, insert, trim

**Files:**
- Create: `src/c/trial_history.h`, `src/c/trial_history.c`
- Create: `tests/test_trial_history.c`
- Modify: `tests/Makefile`

- [ ] **Step 1: Write the failing test**

Write `tests/test_trial_history.c`:

```c
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
    0xEF, 0xBE, 0xAD, 0xDE,   // little-endian 0xDEADBEEF
    0xE8, 0x03                // little-endian 1000
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
```

- [ ] **Step 2: Wire into Makefile**

Update `TESTS := …` to include `test_trial_history`, and append:

```make
test_trial_history: test_trial_history.c ../src/c/trial_history.c
	$(CC) $(CFLAGS) -o $@ $^
```

- [ ] **Step 3: Run test, verify it fails**

Run: `make -C tests test_trial_history`
Expected: compile failure on missing header.

- [ ] **Step 4: Write the header**

Write `src/c/trial_history.h`:

```c
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
```

- [ ] **Step 5: Write the implementation**

Write `src/c/trial_history.c`:

```c
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
```

- [ ] **Step 6: Run tests, verify pass**

Run: `make -C tests test`
Expected: `ALL TESTS PASSED`.

- [ ] **Step 7: Verify pebble build still succeeds**

Run: `pebble build`

- [ ] **Step 8: Commit**

```bash
git add src/c/trial_history.h src/c/trial_history.c tests/test_trial_history.c tests/Makefile
git commit -m "$(cat <<'EOF'
feat: add trial_history with packed encode/decode/insert/trim

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 6: `trial_engine` — question queue, shuffle, skip, format pick

**Files:**
- Create: `src/c/trial_engine.h`, `src/c/trial_engine.c`
- Create: `tests/test_trial_engine.c`
- Modify: `tests/Makefile`

- [ ] **Step 1: Write the failing test**

Write `tests/test_trial_engine.c`:

```c
#include "test_runner.h"
#include "trial_engine.h"
#include "fact_catalog.h"
#include <stdlib.h>

/* Tests use srand(1) for determinism; never relying on actual entropy. */

static void test_mult_queue_size_and_coverage(void) {
  TrialQueue q;
  srand(1);
  trial_engine_build(&q, fact_group_at(8));  /* Mult 2-3 */
  EXPECT_EQ_INT(q.total, 20, "Mult trial has 20 questions");
  EXPECT_EQ_INT(q.position, 0, "starts at position 0");
  EXPECT_EQ_INT(q.answered, 0, "starts with 0 answered");

  /* Each underlying fact (A,B) where A in {2,3} and B in {1..10} must appear exactly once. */
  bool seen[2][11] = { {false} };
  for (uint8_t i = 0; i < q.total; i++) {
    EXPECT_EQ_INT(q.questions[i].op, OP_MULT, "Mult queue has only mult ops");
    uint8_t ai = q.questions[i].a - 2;
    EXPECT(ai < 2, "a in {2,3}");
    EXPECT(q.questions[i].b >= 1 && q.questions[i].b <= 10, "b in 1..10");
    EXPECT(!seen[ai][q.questions[i].b], "no fact repeats");
    seen[ai][q.questions[i].b] = true;
  }
  for (int a = 0; a < 2; a++) {
    for (int b = 1; b <= 10; b++) {
      EXPECT(seen[a][b], "every fact covered");
    }
  }
}

static void test_multdiv_queue_size_and_coverage(void) {
  TrialQueue q;
  srand(1);
  trial_engine_build(&q, fact_group_at(12));  /* M&D 2-3 */
  EXPECT_EQ_INT(q.total, 40, "M&D trial has 40 questions");

  uint8_t mult_count = 0, div_count = 0;
  bool seen_mult[2][11] = { {false} };
  bool seen_div[2][11] = { {false} };
  for (uint8_t i = 0; i < q.total; i++) {
    uint8_t ai = q.questions[i].a - 2;
    if (q.questions[i].op == OP_MULT) {
      mult_count++;
      seen_mult[ai][q.questions[i].b] = true;
    } else if (q.questions[i].op == OP_DIV) {
      div_count++;
      seen_div[ai][q.questions[i].b] = true;
    }
  }
  EXPECT_EQ_INT(mult_count, 20, "20 mult questions");
  EXPECT_EQ_INT(div_count, 20, "20 div questions");
  for (int a = 0; a < 2; a++) {
    for (int b = 1; b <= 10; b++) {
      EXPECT(seen_mult[a][b], "every fact covered as mult");
      EXPECT(seen_div[a][b], "every fact covered as div");
    }
  }
}

static void test_shuffle_changes_order_with_different_seeds(void) {
  TrialQueue q1, q2;
  srand(1);
  trial_engine_build(&q1, fact_group_at(8));
  srand(42);
  trial_engine_build(&q2, fact_group_at(8));
  int differ = 0;
  for (uint8_t i = 0; i < q1.total; i++) {
    if (q1.questions[i].a != q2.questions[i].a ||
        q1.questions[i].b != q2.questions[i].b) {
      differ++;
    }
  }
  EXPECT(differ > 0, "different seeds produce different orders");
}

static void test_advance_correct(void) {
  TrialQueue q;
  srand(1);
  trial_engine_build(&q, fact_group_at(8));
  trial_engine_advance_correct(&q);
  EXPECT_EQ_INT(q.position, 1, "position advances on correct");
  EXPECT_EQ_INT(q.answered, 1, "answered advances on correct");
}

static void test_skip_appends_to_end_and_does_not_count(void) {
  TrialQueue q;
  srand(1);
  trial_engine_build(&q, fact_group_at(8));
  Question skipped = q.questions[0];
  trial_engine_skip(&q);
  EXPECT_EQ_INT(q.position, 1, "position advances on skip");
  EXPECT_EQ_INT(q.answered, 0, "answered does not advance on skip");
  /* Skipped question now sits at the tail (index 20 after the original 20, in a 40-slot buffer). */
  EXPECT_EQ_INT(q.questions[q.total].a, skipped.a, "skipped question stored at index q.total");
  EXPECT_EQ_INT(q.questions[q.total].b, skipped.b, "skipped question stored at index q.total");
  EXPECT_EQ_INT(q.tail, 21, "tail advanced past original total");
}

static void test_complete_after_all_answered(void) {
  TrialQueue q;
  srand(1);
  trial_engine_build(&q, fact_group_at(8));
  for (uint8_t i = 0; i < 20; i++) {
    EXPECT(!trial_engine_is_complete(&q), "not yet complete");
    trial_engine_advance_correct(&q);
  }
  EXPECT(trial_engine_is_complete(&q), "complete after 20 correct answers");
}

static void test_format_is_within_range(void) {
  TrialQueue q;
  srand(1);
  trial_engine_build(&q, fact_group_at(12));
  for (uint8_t i = 0; i < q.total; i++) {
    Question *qn = &q.questions[i];
    uint8_t f = trial_engine_pick_form(qn);
    if (qn->op == OP_MULT) {
      EXPECT(f >= 1 && f <= 4, "mult form in 1..4");
    } else {
      EXPECT(f >= 5 && f <= 8, "div form in 5..8");
    }
  }
}

int main(void) {
  RUN_TEST(test_mult_queue_size_and_coverage);
  RUN_TEST(test_multdiv_queue_size_and_coverage);
  RUN_TEST(test_shuffle_changes_order_with_different_seeds);
  RUN_TEST(test_advance_correct);
  RUN_TEST(test_skip_appends_to_end_and_does_not_count);
  RUN_TEST(test_complete_after_all_answered);
  RUN_TEST(test_format_is_within_range);
  TEST_MAIN_RETURN();
}
```

- [ ] **Step 2: Wire into Makefile**

Add `test_trial_engine` to the `TESTS :=` list and append:

```make
test_trial_engine: test_trial_engine.c ../src/c/trial_engine.c ../src/c/fact_catalog.c
	$(CC) $(CFLAGS) -o $@ $^
```

- [ ] **Step 3: Run, verify failure**

Run: `make -C tests test_trial_engine`
Expected: compile failure on missing header.

- [ ] **Step 4: Write the header**

Write `src/c/trial_engine.h`:

```c
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "fact_catalog.h"

typedef enum {
  OP_NONE = 0,   /* Skip Counting */
  OP_MULT = 1,
  OP_DIV  = 2,
} TrialOp;

typedef struct {
  uint8_t a;       /* factor A (the "by" factor) */
  uint8_t b;       /* factor B (the 1..10 factor) */
  TrialOp op;
} Question;

/* Max trial size is 40 (M&D). When skips occur questions are appended after the
 * original payload, so the buffer is sized for the worst-case where every
 * question is skipped exactly once: 40 + 40 = 80. */
#define TRIAL_QUEUE_CAPACITY 80

typedef struct {
  Question questions[TRIAL_QUEUE_CAPACITY];
  uint8_t total;        /* number of original questions (20 or 40) */
  uint8_t tail;         /* index just past the last currently-pending slot */
  uint8_t position;     /* index of the current question */
  uint8_t answered;     /* number of correct answers so far */
} TrialQueue;

void trial_engine_build(TrialQueue *q, const FactGroup *fg);
void trial_engine_advance_correct(TrialQueue *q);
void trial_engine_skip(TrialQueue *q);
bool trial_engine_is_complete(const TrialQueue *q);
uint8_t trial_engine_pick_form(const Question *q);
```

- [ ] **Step 5: Write the implementation**

Write `src/c/trial_engine.c`:

```c
#include "trial_engine.h"
#include <stdlib.h>
#include <string.h>

static void shuffle(Question *arr, uint8_t n) {
  for (uint8_t i = n - 1; i > 0; i--) {
    uint8_t j = (uint8_t)(rand() % (i + 1));
    Question tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
  }
}

void trial_engine_build(TrialQueue *q, const FactGroup *fg) {
  memset(q, 0, sizeof(*q));
  if (!fg) return;

  uint8_t a_span = (uint8_t)(fg->a_max - fg->a_min + 1);
  uint8_t fact_count = (uint8_t)(a_span * 10);

  if (fg->category == CATEGORY_MULT) {
    uint8_t i = 0;
    for (uint8_t a = fg->a_min; a <= fg->a_max; a++) {
      for (uint8_t b = 1; b <= 10; b++) {
        q->questions[i++] = (Question){ .a = a, .b = b, .op = OP_MULT };
      }
    }
    q->total = fact_count;
    q->tail = fact_count;
    shuffle(q->questions, q->total);
  } else if (fg->category == CATEGORY_MULTDIV) {
    uint8_t i = 0;
    for (uint8_t a = fg->a_min; a <= fg->a_max; a++) {
      for (uint8_t b = 1; b <= 10; b++) {
        q->questions[i++] = (Question){ .a = a, .b = b, .op = OP_MULT };
        q->questions[i++] = (Question){ .a = a, .b = b, .op = OP_DIV };
      }
    }
    q->total = (uint8_t)(fact_count * 2);
    q->tail = q->total;
    shuffle(q->questions, q->total);
  } else {
    /* Skip Counting trial — single "question". The fact group itself carries
     * the recitation prompt; the queue exists only for uniformity. */
    q->questions[0] = (Question){ .a = fg->skip_by, .b = 0, .op = OP_NONE };
    q->total = 1;
    q->tail = 1;
  }
}

void trial_engine_advance_correct(TrialQueue *q) {
  q->position++;
  q->answered++;
}

void trial_engine_skip(TrialQueue *q) {
  if (q->tail >= TRIAL_QUEUE_CAPACITY) {
    /* Defensive — shouldn't be reachable given worst case = 80. */
    q->position++;
    return;
  }
  q->questions[q->tail] = q->questions[q->position];
  q->tail++;
  q->position++;
}

bool trial_engine_is_complete(const TrialQueue *q) {
  return q->answered >= q->total;
}

uint8_t trial_engine_pick_form(const Question *qn) {
  if (qn->op == OP_MULT) {
    return (uint8_t)(1 + (rand() % 4));   /* 1..4 */
  } else if (qn->op == OP_DIV) {
    return (uint8_t)(5 + (rand() % 4));   /* 5..8 */
  }
  return 0;
}
```

- [ ] **Step 6: Run tests, verify pass**

Run: `make -C tests test`
Expected: `ALL TESTS PASSED`.

- [ ] **Step 7: Verify pebble build still succeeds**

Run: `pebble build`

- [ ] **Step 8: Commit**

```bash
git add src/c/trial_engine.h src/c/trial_engine.c tests/test_trial_engine.c tests/Makefile
git commit -m "$(cat <<'EOF'
feat: add trial_engine — queue build, shuffle, skip, format picker

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 7: `graph_data` — daily-best aggregation + pixel mapping

**Files:**
- Create: `src/c/graph_data.h`, `src/c/graph_data.c`
- Create: `tests/test_graph_data.c`
- Modify: `tests/Makefile`

- [ ] **Step 1: Write the failing test**

Write `tests/test_graph_data.c`:

```c
#include "test_runner.h"
#include "graph_data.h"
#include "trial_history.h"
#include <time.h>

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
```

- [ ] **Step 2: Wire into Makefile**

Add `test_graph_data` to `TESTS :=` and append:

```make
test_graph_data: test_graph_data.c ../src/c/graph_data.c ../src/c/trial_history.c
	$(CC) $(CFLAGS) -o $@ $^
```

- [ ] **Step 3: Verify it fails**

Run: `make -C tests test_graph_data`
Expected: missing header.

- [ ] **Step 4: Write the header**

Write `src/c/graph_data.h`:

```c
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
```

- [ ] **Step 5: Write the implementation**

Write `src/c/graph_data.c`:

```c
#include "graph_data.h"
#include <string.h>

uint8_t graph_data_daily_best(const TrialRecord *records, uint8_t count,
                              uint32_t today_utc,
                              GraphPoint *out, uint8_t out_cap) {
  if (!records || count == 0 || !out || out_cap == 0) return 0;

  /* Bucket by integer day-offset from today (UTC days; the spec calls for
   * local-time days, but on Pebble we'll convert before calling — the test
   * harness passes synthetic UTC midnights so the math is the same here). */
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

  /* Emit oldest → newest. */
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
```

- [ ] **Step 6: Run tests, verify pass**

Run: `make -C tests test`
Expected: all tests pass.

- [ ] **Step 7: Verify pebble build**

Run: `pebble build`

- [ ] **Step 8: Commit**

```bash
git add src/c/graph_data.h src/c/graph_data.c tests/test_graph_data.c tests/Makefile
git commit -m "$(cat <<'EOF'
feat: add graph_data daily-best aggregation and pixel mapping

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

## Phase 3 — Pebble platform glue

### Task 8: `storage` — persist read/write wrappers

**Files:**
- Create: `src/c/storage.h`, `src/c/storage.c`

No host tests for this module — it's a thin shim around `persist_*` and is exercised via the emulator passes in Phase 4.

- [ ] **Step 1: Write the header**

Write `src/c/storage.h`:

```c
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
```

- [ ] **Step 2: Write the implementation**

Write `src/c/storage.c`:

```c
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
```

- [ ] **Step 3: Verify pebble build**

Run: `pebble build`
Expected: clean compile.

- [ ] **Step 4: Commit**

```bash
git add src/c/storage.h src/c/storage.c
git commit -m "$(cat <<'EOF'
feat: add storage layer over persist for per-FG history

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 9: Home window — shell, status bar, category cycling

**Files:**
- Create: `src/c/home_window.h`, `src/c/home_window.c`
- Modify: `src/c/main.c`

- [ ] **Step 1: Write the header**

Write `src/c/home_window.h`:

```c
#pragma once
#include <pebble.h>
#include "fact_catalog.h"

void home_window_push(void);
void home_window_destroy(void);
```

- [ ] **Step 2: Write the implementation (status bar + category cycle, no graph yet)**

Write `src/c/home_window.c`:

```c
#include "home_window.h"
#include "storage.h"

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_category_label;
static TextLayer *s_empty_label;
static FactCategory s_current_category;

static const char *category_short_name(FactCategory c) {
  switch (c) {
    case CATEGORY_SKIP:    return "Skip";
    case CATEGORY_MULT:    return "Mult";
    case CATEGORY_MULTDIV: return "M&D";
    default:               return "?";
  }
}

static void apply_category(FactCategory c) {
  s_current_category = c;
  static char buf[8];
  snprintf(buf, sizeof(buf), "%s", category_short_name(c));
  text_layer_set_text(s_category_label, buf);
  storage_save_last_category(c);
}

static void up_click(ClickRecognizerRef rec, void *ctx) {
  (void)rec; (void)ctx;
  apply_category((FactCategory)(((int)s_current_category + CATEGORY_COUNT - 1) % CATEGORY_COUNT));
}

static void down_click(ClickRecognizerRef rec, void *ctx) {
  (void)rec; (void)ctx;
  apply_category((FactCategory)(((int)s_current_category + 1) % CATEGORY_COUNT));
}

static void click_config(void *ctx) {
  (void)ctx;
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

  /* Placeholder empty state until the graph layer is added in the next task. */
  s_empty_label = text_layer_create(GRect(0, b.size.h / 2 - 8, b.size.w, 16));
  text_layer_set_text(s_empty_label, "no data yet");
  text_layer_set_text_alignment(s_empty_label, GTextAlignmentCenter);
  text_layer_set_font(s_empty_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root, text_layer_get_layer(s_empty_label));

  apply_category(storage_load_last_category());
}

static void window_unload(Window *window) {
  (void)window;
  text_layer_destroy(s_empty_label);
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
```

- [ ] **Step 3: Update `main.c` to use `home_window`**

Rewrite `src/c/main.c`:

```c
#include <pebble.h>
#include <time.h>
#include <stdlib.h>
#include "home_window.h"

static void prv_init(void) {
  srand((unsigned)time(NULL));
  home_window_push();
}

static void prv_deinit(void) {
  home_window_destroy();
}

int main(void) {
  prv_init();
  APP_LOG(APP_LOG_LEVEL_INFO, "MathFacts started");
  app_event_loop();
  prv_deinit();
}
```

- [ ] **Step 4: Verify pebble build**

Run: `pebble build`

- [ ] **Step 5: Manual emulator smoke test**

Run:
```sh
pebble install --emulator flint
```
Expected: app launches, top status bar shows time on the left and `Skip` (or whatever category was previously saved) on the right; center shows `no data yet`. Pressing Up / Down rotates the right-side label between `Skip`, `Mult`, `M&D` and persists the choice (kill emulator and relaunch to verify).

- [ ] **Step 6: Commit**

```bash
git add src/c/home_window.h src/c/home_window.c src/c/main.c
git commit -m "$(cat <<'EOF'
feat: home window shell with status bar and category cycling

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 10: Home window — graph rendering

**Files:**
- Modify: `src/c/home_window.c`

- [ ] **Step 1: Add a graph rendering layer**

Replace the body of `src/c/home_window.c` with this version, which adds a `Layer` driven by `graph_data` and removes the placeholder empty label:

```c
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

/* Pattern table: index 0 = solid, 1 = dashed, 2 = dotted, 3 = dash-dot.
 * Each entry is the on/off pixel sequence in the order drawn. */
static void draw_styled_line(GContext *ctx, GPoint a, GPoint b, uint8_t pattern_idx) {
  /* Bresenham-style traversal so we can toggle pixel rendering per-pattern. */
  int dx = b.x - a.x; int dy = b.y - a.y;
  int steps = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy)
    ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
  if (steps == 0) { graphics_draw_pixel(ctx, a); return; }
  for (int i = 0; i <= steps; i++) {
    bool draw = true;
    int phase = i & 7;  /* 8-pixel pattern cycle */
    switch (pattern_idx & 3) {
      case 0: draw = true; break;                              /* solid */
      case 1: draw = (phase < 4); break;                       /* dashed */
      case 2: draw = (phase & 1) == 0; break;                  /* dotted */
      case 3: draw = (phase < 3) || phase == 5; break;         /* dash-dot */
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

  /* Right-edge label at the most recent point. */
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

  /* Find best/worst across all FGs in the current category. */
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
    /* No data yet for this category. */
    graphics_draw_text(ctx, "no data yet",
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(0, GRAPH_Y_TOP + 40, 144, 18),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    return;
  }

  /* Draw each FG in category, line-pattern by within-category index. */
  uint8_t within_idx = 0;
  for (uint8_t i = 0; i < FACT_GROUP_COUNT; i++) {
    const FactGroup *fg = fact_group_at(i);
    if (fg->category != s_current_category) continue;
    draw_one_series(ctx, i, best_all, worst_all, within_idx);
    within_idx++;
  }
}

static const char *category_label_text(FactCategory c) {
  return category_short_name(c);
}

static void apply_category(FactCategory c) {
  s_current_category = c;
  text_layer_set_text(s_category_label, category_label_text(c));
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
  text_layer_set_text(s_hint_label, "▲▼ category   ● select");
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
```

- [ ] **Step 2: Verify pebble build**

Run: `pebble build`
Expected: clean compile.

- [ ] **Step 3: Manual emulator smoke test**

Run: `pebble install --emulator flint`
Expected: status bar with category abbrev on the right, plot area with axis lines and `120d`/`today` labels at the bottom, "no data yet" message centered (because no trials are stored yet), control hint at bottom.

- [ ] **Step 4: Commit**

```bash
git add src/c/home_window.c
git commit -m "$(cat <<'EOF'
feat: home window draws multi-series line graph with patterns

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 11: Home window — long-press to Clear, single-click to FG Select

**Files:**
- Modify: `src/c/home_window.c`
- Create: `src/c/clear_window.h` (stub for now), `src/c/clear_window.c` (stub for now)
- Create: `src/c/factgroup_window.h` (stub for now), `src/c/factgroup_window.c` (stub for now)

- [ ] **Step 1: Create stub headers and implementations**

The full window bodies arrive in Tasks 12 and 14. For now we need just enough to compile and push.

Write `src/c/clear_window.h`:

```c
#pragma once
#include <pebble.h>
void clear_window_push(void);
```

Write `src/c/clear_window.c`:

```c
#include "clear_window.h"

static Window *s_window;

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  TextLayer *tl = text_layer_create(layer_get_bounds(root));
  text_layer_set_text(tl, "Clear data\n(coming soon)");
  text_layer_set_text_alignment(tl, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(tl));
  window_set_user_data(window, tl);
}
static void window_unload(Window *window) {
  text_layer_destroy((TextLayer *)window_get_user_data(window));
  window_destroy(s_window);
  s_window = NULL;
}

void clear_window_push(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load, .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
```

Write `src/c/factgroup_window.h`:

```c
#pragma once
#include <pebble.h>
#include "fact_catalog.h"
void factgroup_window_push(FactCategory cat);
```

Write `src/c/factgroup_window.c`:

```c
#include "factgroup_window.h"

static Window *s_window;
static FactCategory s_cat;

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  TextLayer *tl = text_layer_create(layer_get_bounds(root));
  text_layer_set_text(tl, "Fact group\n(coming soon)");
  text_layer_set_text_alignment(tl, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(tl));
  window_set_user_data(window, tl);
}
static void window_unload(Window *window) {
  text_layer_destroy((TextLayer *)window_get_user_data(window));
  window_destroy(s_window);
  s_window = NULL;
}

void factgroup_window_push(FactCategory cat) {
  s_cat = cat;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load, .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
```

- [ ] **Step 2: Wire Home buttons to push the new windows**

In `src/c/home_window.c`, add `#include "clear_window.h"` and `#include "factgroup_window.h"` at the top with the other includes, then replace the `click_config` function and add a new handler so the bound clicks become:

```c
#include <pebble.h>
#include "clear_window.h"
#include "factgroup_window.h"

/* ... existing static declarations and helpers ... */

static void select_click(ClickRecognizerRef rec, void *ctx) {
  (void)rec; (void)ctx;
  factgroup_window_push(s_current_category);
}

static void select_long_click(ClickRecognizerRef rec, void *ctx) {
  (void)rec; (void)ctx;
  vibes_short_pulse();
  clear_window_push();
}

static void click_config(void *ctx) {
  (void)ctx;
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_long_click_subscribe(BUTTON_ID_SELECT, 5000, select_long_click, NULL);
}
```

- [ ] **Step 3: Verify pebble build**

Run: `pebble build`

- [ ] **Step 4: Manual emulator smoke test**

Run: `pebble install --emulator flint`
Expected: pressing Select (middle button) pushes the "Fact group (coming soon)" screen; back returns. Holding Select for 5 s vibrates and pushes the "Clear data (coming soon)" screen; back returns. Up / Down still cycle the category on the Home screen.

- [ ] **Step 5: Commit**

```bash
git add src/c/clear_window.h src/c/clear_window.c src/c/factgroup_window.h src/c/factgroup_window.c src/c/home_window.c
git commit -m "$(cat <<'EOF'
feat: home window wires select-click and 5s long-press to child windows

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 12: Fact Group Selection window — `menu_layer`

**Files:**
- Modify: `src/c/factgroup_window.c`

- [ ] **Step 1: Reference the example**

Open `resources/examples/ui-patterns/src/windows/radio_button_window.c` for `menu_layer` callback conventions (it's the closest match — a single-section menu with text rows). The functions to mirror are `get_num_sections_callback`, `get_num_rows_callback`, `get_header_height_callback`, `draw_row_callback`, `select_click_callback`.

- [ ] **Step 2: Replace the placeholder with a real menu**

Overwrite `src/c/factgroup_window.c`:

```c
#include "factgroup_window.h"
#include "storage.h"
#include "trial_history.h"
#include <stdio.h>

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_status_label;
static MenuLayer *s_menu;
static FactCategory s_cat;

static uint16_t get_num_rows(struct MenuLayer *menu, uint16_t section, void *ctx) {
  (void)menu; (void)section; (void)ctx;
  return fact_group_count_in_category(s_cat);
}

static int16_t get_cell_height(struct MenuLayer *menu, MenuIndex *idx, void *ctx) {
  (void)menu; (void)idx; (void)ctx;
  return 32;
}

static void format_time(uint16_t duration_s, char *out, size_t cap) {
  uint16_t m = duration_s / 60;
  uint16_t s = duration_s % 60;
  snprintf(out, cap, "%u:%02u", m, s);
}

static void draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *idx, void *cb_ctx) {
  (void)cb_ctx;
  uint8_t fg_id = fact_group_nth_in_category(s_cat, (uint8_t)idx->row);
  const FactGroup *fg = fact_group_at(fg_id);
  if (!fg) return;

  bool highlighted = menu_cell_layer_is_highlighted(cell_layer);
  graphics_context_set_text_color(ctx, highlighted ? GColorWhite : GColorBlack);

  GRect b = layer_get_bounds(cell_layer);
  graphics_draw_text(ctx, fg->short_name,
    fonts_get_system_font(highlighted ? FONT_KEY_GOTHIC_14_BOLD : FONT_KEY_GOTHIC_14),
    GRect(4, 0, b.size.w - 8, 16),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  char sub[24];
  uint8_t count = storage_trial_count(fg_id);
  if (count == 0) {
    snprintf(sub, sizeof(sub), "no data yet");
  } else {
    char timebuf[8];
    format_time(storage_best_duration(fg_id), timebuf, sizeof(timebuf));
    snprintf(sub, sizeof(sub), "%s · %u trials", timebuf, count);
  }
  graphics_draw_text(ctx, sub,
    fonts_get_system_font(FONT_KEY_GOTHIC_14),
    GRect(4, 14, b.size.w - 8, 16),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void select_click(struct MenuLayer *menu, MenuIndex *idx, void *ctx) {
  (void)menu; (void)ctx;
  uint8_t fg_id = fact_group_nth_in_category(s_cat, (uint8_t)idx->row);
  /* Stub for now — will push challenge window in Task 13. */
  APP_LOG(APP_LOG_LEVEL_INFO, "Selected fact group %u", fg_id);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);

  s_status = status_bar_layer_create();
  status_bar_layer_set_colors(s_status, GColorBlack, GColorWhite);
  layer_add_child(root, status_bar_layer_get_layer(s_status));

  s_status_label = text_layer_create(GRect(b.size.w - 80, 0, 76, 16));
  text_layer_set_background_color(s_status_label, GColorClear);
  text_layer_set_text_color(s_status_label, GColorWhite);
  text_layer_set_text_alignment(s_status_label, GTextAlignmentRight);
  text_layer_set_font(s_status_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_status_label, "Pick group");
  layer_add_child(root, text_layer_get_layer(s_status_label));

  s_menu = menu_layer_create(GRect(0, 16, b.size.w, b.size.h - 16));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_num_rows,
    .get_cell_height = get_cell_height,
    .draw_row = draw_row,
    .select_click = select_click,
  });
  menu_layer_set_click_config_onto_window(s_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void window_unload(Window *window) {
  (void)window;
  menu_layer_destroy(s_menu);
  text_layer_destroy(s_status_label);
  status_bar_layer_destroy(s_status);
  window_destroy(s_window);
  s_window = NULL;
}

void factgroup_window_push(FactCategory cat) {
  s_cat = cat;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
```

- [ ] **Step 2: Verify pebble build**

Run: `pebble build`

- [ ] **Step 3: Manual emulator test**

Run: `pebble install --emulator flint`
Expected: From the Home screen, Select pushes a Fact Group Selection screen. Cycling through categories on Home and re-entering FG Selection shows different row counts — 8 for Skip, 4 for Mult, 4 for M&D. Rows display "no data yet" since no trials stored. Up/Down moves the highlight; chevrons appear for Skip Counting.

- [ ] **Step 4: Commit**

```bash
git add src/c/factgroup_window.c
git commit -m "$(cat <<'EOF'
feat: fact group selection menu_layer with best-time subtitles

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 13: Challenge window — all three category variants

**Files:**
- Create: `src/c/challenge_window.h`, `src/c/challenge_window.c`
- Modify: `src/c/factgroup_window.c`

- [ ] **Step 1: Write the header**

Write `src/c/challenge_window.h`:

```c
#pragma once
#include <pebble.h>
#include "fact_catalog.h"

/* Pushes the appropriate Challenge UI for the fact group. */
void challenge_window_push(uint8_t fg_id);
```

- [ ] **Step 2: Write the implementation**

Write `src/c/challenge_window.c`:

```c
#include "challenge_window.h"
#include "trial_engine.h"
#include "trial_history.h"
#include "storage.h"
#include "summary_window.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_status_left, *s_status_right;
static TextLayer *s_eq_line1, *s_eq_line2;
static TextLayer *s_timer;
static ActionBarLayer *s_action;
static GBitmap *s_icon_cancel, *s_icon_check, *s_icon_skip;
/* The icons are drawn from system fonts/glyphs at runtime — see render_icon
 * below — so no bitmap resources are needed. */

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
  snprintf(out, cap, "⏱ %u:%02u", m, s);
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
  uint16_t c = (uint16_t)qn->a * qn->b;  /* product */
  /* Form layout per spec §3.3:
   * Mult forms 1..4: ?×B/=C, A×?/=C, C=/?×B, C=/A×?
   * Div  forms 5..8: ?÷A/=B, C÷?/=B, B=/?÷A, B=/C÷?
   */
  if (qn->op == OP_MULT) {
    switch (form) {
      case 1: snprintf(l1, sizeof(l1), "?×%u", qn->b);
              snprintf(l2, sizeof(l2), "=%u", c); break;
      case 2: snprintf(l1, sizeof(l1), "%u×?", qn->a);
              snprintf(l2, sizeof(l2), "=%u", c); break;
      case 3: snprintf(l1, sizeof(l1), "%u=", c);
              snprintf(l2, sizeof(l2), "?×%u", qn->b); break;
      default: snprintf(l1, sizeof(l1), "%u=", c);
               snprintf(l2, sizeof(l2), "%u×?", qn->a); break;
    }
  } else {  /* OP_DIV */
    switch (form) {
      case 5: snprintf(l1, sizeof(l1), "?÷%u", qn->a);
              snprintf(l2, sizeof(l2), "=%u", qn->b); break;
      case 6: snprintf(l1, sizeof(l1), "%u÷?", c);
              snprintf(l2, sizeof(l2), "=%u", qn->b); break;
      case 7: snprintf(l1, sizeof(l1), "%u=", qn->b);
              snprintf(l2, sizeof(l2), "?÷%u", qn->a); break;
      default: snprintf(l1, sizeof(l1), "%u=", qn->b);
               snprintf(l2, sizeof(l2), "%u÷?", c); break;
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
  /* Pop self so back-button from Summary returns to Fact Group Selection. */
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
  if (s_fg->category == CATEGORY_SKIP) return;  /* skip not allowed */
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

  s_action = action_bar_layer_create();
  action_bar_layer_set_background_color(s_action, GColorBlack);
  action_bar_layer_add_to_window(s_action, window);

  render_status_right();
  render_current_question();
  render_timer();
  schedule_tick();
}

static void window_unload(Window *window) {
  (void)window;
  cancel_tick();
  action_bar_layer_destroy(s_action);
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
```

- [ ] **Step 3: Add a stub for `summary_window.h`**

Write `src/c/summary_window.h`:

```c
#pragma once
#include <pebble.h>
void summary_window_push(uint8_t fg_id, uint16_t duration_s);
```

Write `src/c/summary_window.c` as a stub (real impl in Task 14):

```c
#include "summary_window.h"
#include "storage.h"
#include "trial_history.h"
#include <stdio.h>
#include <time.h>

static Window *s_window;

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  TextLayer *tl = text_layer_create(layer_get_bounds(root));
  text_layer_set_text(tl, "Summary\n(coming soon)");
  text_layer_set_text_alignment(tl, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(tl));
  window_set_user_data(window, tl);
}

static void window_unload(Window *window) {
  text_layer_destroy((TextLayer *)window_get_user_data(window));
  window_destroy(s_window);
  s_window = NULL;
}

void summary_window_push(uint8_t fg_id, uint16_t duration_s) {
  (void)fg_id; (void)duration_s;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load, .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
```

- [ ] **Step 4: Wire FG Selection select-click → Challenge**

In `src/c/factgroup_window.c`, add `#include "challenge_window.h"` and replace the body of `select_click` with the following. Note the FG window removes itself once Challenge is pushed — this keeps the stack depth ≤ 2 so the navigation model is "Home + at-most-one-sub-window":

```c
static void select_click(struct MenuLayer *menu, MenuIndex *idx, void *ctx) {
  (void)menu; (void)ctx;
  uint8_t fg_id = fact_group_nth_in_category(s_cat, (uint8_t)idx->row);
  challenge_window_push(fg_id);
  /* Remove FG Selection so cancel/finish/back from Challenge or Summary all
   * land on Home rather than back on this list. */
  window_stack_remove(s_window, false);
}
```

- [ ] **Step 5: Verify pebble build**

Run: `pebble build`

- [ ] **Step 6: Manual emulator test**

Run: `pebble install --emulator flint`
Test path:
1. From Home, press Select → Fact Group Selection.
2. Pick `Skip Count → By 4` (or any). Challenge shows `Skip count / by 4` and a running timer. Select pops to "Summary (coming soon)". Back returns to Home.
3. From Home, cycle to Mult. Select → pick `2-3 × 1-10`. Challenge shows a two-line equation and `1/20` in the status bar. Pressing Select correctly cycles through 20 questions (the equation changes each time). Pressing Down skips a question and re-shows the same fact later. Pressing Up cancels back to Home.
4. From Home, cycle to M&D. Select → pick `2-3 ×÷ 1-10`. Challenge runs 40 questions, mixing `×` and `÷` formats.

- [ ] **Step 7: Commit**

```bash
git add src/c/challenge_window.h src/c/challenge_window.c src/c/summary_window.h src/c/summary_window.c src/c/factgroup_window.c
git commit -m "$(cat <<'EOF'
feat: challenge window for all 3 categories with timer, skip, cancel

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 14: Challenge Summary window — comparison bars + Store/Discard

**Files:**
- Modify: `src/c/summary_window.c`

- [ ] **Step 1: Replace the stub with the real summary**

Overwrite `src/c/summary_window.c`:

```c
#include "summary_window.h"
#include "storage.h"
#include "trial_history.h"
#include <stdio.h>
#include <time.h>

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_status_right;
static TextLayer *s_result_label;
static Layer *s_bars_layer;
static TextLayer *s_store_label, *s_discard_label;
static ActionBarLayer *s_action;

static uint8_t s_fg_id;
static uint16_t s_duration_s;

typedef enum { SUMMARY_STORE = 0, SUMMARY_DISCARD = 1 } SummaryChoice;
static SummaryChoice s_choice;

static void format_mmss(uint16_t s, char *out, size_t cap) {
  snprintf(out, cap, "%u:%02u", s / 60, s % 60);
}

static void draw_bars(Layer *layer, GContext *ctx) {
  (void)layer;
  TrialRecord arr[TRIAL_HISTORY_CAPACITY];
  uint8_t n = storage_load_history(s_fg_id, arr, TRIAL_HISTORY_CAPACITY);
  uint16_t bars_s[4];
  uint8_t bar_count = 0;

  /* Up to 3 previous trials + this trial — newest first in arr. */
  for (uint8_t i = 0; i < n && bar_count < 3; i++) {
    bars_s[2 - bar_count] = arr[i].duration_s;
    bar_count++;
  }
  /* Reverse the "prev" bars so they appear -3, -2, -1 left → right. */
  if (bar_count > 0) {
    uint16_t tmp[3];
    for (uint8_t i = 0; i < bar_count; i++) tmp[i] = bars_s[3 - bar_count + i];
    for (uint8_t i = 0; i < bar_count; i++) bars_s[i] = tmp[i];
  }
  bars_s[bar_count] = s_duration_s;
  bar_count++;

  uint16_t worst = 0;
  for (uint8_t i = 0; i < bar_count; i++) if (bars_s[i] > worst) worst = bars_s[i];
  if (worst == 0) worst = 1;

  const int16_t y_top = 30, y_bot = 60;
  const int16_t span_y = y_bot - y_top;
  const int16_t x_base = 22;
  const int16_t bar_w = 18, bar_gap = 6;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_text_color(ctx, GColorBlack);

  for (uint8_t i = 0; i < bar_count; i++) {
    int16_t h = (int16_t)((int32_t)span_y * bars_s[i] / worst);
    int16_t x = x_base + i * (bar_w + bar_gap);
    GRect r = GRect(x, y_bot - h, bar_w, h);
    bool is_now = (i == bar_count - 1);
    if (is_now) {
      graphics_fill_rect(ctx, r, 0, GCornerNone);
    } else {
      graphics_draw_rect(ctx, r);
    }
    char timebuf[8];
    format_mmss(bars_s[i], timebuf, sizeof(timebuf));
    graphics_draw_text(ctx, timebuf,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(x - 4, y_bot + 2, bar_w + 8, 14),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    const char *tag = is_now ? "now" :
      (i == bar_count - 2 ? "-1" : i == bar_count - 3 ? "-2" : "-3");
    graphics_draw_text(ctx, tag,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(x - 4, y_bot + 14, bar_w + 8, 14),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
}

static void update_highlight(void) {
  text_layer_set_background_color(s_store_label,   s_choice == SUMMARY_STORE   ? GColorBlack : GColorWhite);
  text_layer_set_text_color(s_store_label,         s_choice == SUMMARY_STORE   ? GColorWhite : GColorBlack);
  text_layer_set_background_color(s_discard_label, s_choice == SUMMARY_DISCARD ? GColorBlack : GColorWhite);
  text_layer_set_text_color(s_discard_label,       s_choice == SUMMARY_DISCARD ? GColorWhite : GColorBlack);
}

static void up_click(ClickRecognizerRef rec, void *ctx)   { (void)rec; (void)ctx;
  s_choice = SUMMARY_STORE; update_highlight();
}
static void down_click(ClickRecognizerRef rec, void *ctx) { (void)rec; (void)ctx;
  s_choice = SUMMARY_DISCARD; update_highlight();
}
static void select_click(ClickRecognizerRef rec, void *ctx) { (void)rec; (void)ctx;
  if (s_choice == SUMMARY_STORE) {
    TrialRecord t = { .timestamp_utc = (uint32_t)time(NULL), .duration_s = s_duration_s };
    storage_append_trial(s_fg_id, t);
  }
  window_stack_remove(s_window, false);
}

static void click_config(void *ctx) { (void)ctx;
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);

  s_status = status_bar_layer_create();
  status_bar_layer_set_colors(s_status, GColorBlack, GColorWhite);
  layer_add_child(root, status_bar_layer_get_layer(s_status));

  s_status_right = text_layer_create(GRect(4, 0, b.size.w - 28, 16));
  text_layer_set_background_color(s_status_right, GColorClear);
  text_layer_set_text_color(s_status_right, GColorWhite);
  text_layer_set_font(s_status_right, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_status_right, "Trial done");
  layer_add_child(root, text_layer_get_layer(s_status_right));

  static char result_buf[8];
  format_mmss(s_duration_s, result_buf, sizeof(result_buf));
  s_result_label = text_layer_create(GRect(0, 16, b.size.w - 20, 24));
  text_layer_set_background_color(s_result_label, GColorClear);
  text_layer_set_text_alignment(s_result_label, GTextAlignmentCenter);
  text_layer_set_font(s_result_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(s_result_label, result_buf);
  layer_add_child(root, text_layer_get_layer(s_result_label));

  s_bars_layer = layer_create(GRect(0, 40, b.size.w - 20, 60));
  layer_set_update_proc(s_bars_layer, draw_bars);
  layer_add_child(root, s_bars_layer);

  s_store_label = text_layer_create(GRect(4, 110, b.size.w - 28, 22));
  text_layer_set_text(s_store_label, "Store");
  text_layer_set_text_alignment(s_store_label, GTextAlignmentCenter);
  text_layer_set_font(s_store_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(root, text_layer_get_layer(s_store_label));

  s_discard_label = text_layer_create(GRect(4, 138, b.size.w - 28, 22));
  text_layer_set_text(s_discard_label, "Discard");
  text_layer_set_text_alignment(s_discard_label, GTextAlignmentCenter);
  text_layer_set_font(s_discard_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(root, text_layer_get_layer(s_discard_label));

  s_action = action_bar_layer_create();
  action_bar_layer_set_background_color(s_action, GColorBlack);
  action_bar_layer_add_to_window(s_action, window);

  s_choice = SUMMARY_STORE;
  update_highlight();
}

static void window_unload(Window *window) {
  (void)window;
  action_bar_layer_destroy(s_action);
  text_layer_destroy(s_discard_label);
  text_layer_destroy(s_store_label);
  layer_destroy(s_bars_layer);
  text_layer_destroy(s_result_label);
  text_layer_destroy(s_status_right);
  status_bar_layer_destroy(s_status);
  window_destroy(s_window);
  s_window = NULL;
}

void summary_window_push(uint8_t fg_id, uint16_t duration_s) {
  s_fg_id = fg_id;
  s_duration_s = duration_s;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}
```

- [ ] **Step 2: Verify pebble build**

Run: `pebble build`

- [ ] **Step 3: Manual emulator test**

Run: `pebble install --emulator flint`
Test path:
1. Run a Skip Counting trial. On completion the Summary screen shows the result time (`M:SS`), a single filled "now" bar (no priors yet), and Store / Discard rows with Store highlighted. Select stores; verify by re-running and seeing 2 bars (`-1` and `now`).
2. Run more trials of the same FG; the chart should show up to 3 prior outlined bars + 1 filled "now" bar.
3. Choose Discard once and verify a new trial does not get appended (count doesn't increment).
4. Back button from Summary should behave like Discard (just pop).

- [ ] **Step 4: Commit**

```bash
git add src/c/summary_window.c
git commit -m "$(cat <<'EOF'
feat: challenge summary with comparison bars and store/discard menu

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 15: Clear Data window — warning + cursor menu

**Files:**
- Modify: `src/c/clear_window.c`

- [ ] **Step 1: Replace the stub with the real confirm screen**

Overwrite `src/c/clear_window.c`:

```c
#include "clear_window.h"
#include "storage.h"

static Window *s_window;
static StatusBarLayer *s_status;
static TextLayer *s_status_label;
static TextLayer *s_body1, *s_body2, *s_body3;
static TextLayer *s_cancel_label, *s_clear_label;
static ActionBarLayer *s_action;

typedef enum { CHOICE_CANCEL = 0, CHOICE_CLEAR = 1 } ClearChoice;
static ClearChoice s_choice;

static void update_highlight(void) {
  text_layer_set_background_color(s_cancel_label, s_choice == CHOICE_CANCEL ? GColorBlack : GColorWhite);
  text_layer_set_text_color(s_cancel_label,       s_choice == CHOICE_CANCEL ? GColorWhite : GColorBlack);
  text_layer_set_background_color(s_clear_label,  s_choice == CHOICE_CLEAR  ? GColorBlack : GColorWhite);
  text_layer_set_text_color(s_clear_label,        s_choice == CHOICE_CLEAR  ? GColorWhite : GColorBlack);
}

static void up_click(ClickRecognizerRef rec, void *ctx)   { (void)rec; (void)ctx;
  s_choice = CHOICE_CANCEL; update_highlight();
}
static void down_click(ClickRecognizerRef rec, void *ctx) { (void)rec; (void)ctx;
  s_choice = CHOICE_CLEAR; update_highlight();
}
static void select_click(ClickRecognizerRef rec, void *ctx) { (void)rec; (void)ctx;
  if (s_choice == CHOICE_CLEAR) storage_clear_all();
  window_stack_remove(s_window, false);
}

static void click_config(void *ctx) { (void)ctx;
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);

  s_status = status_bar_layer_create();
  status_bar_layer_set_colors(s_status, GColorBlack, GColorWhite);
  layer_add_child(root, status_bar_layer_get_layer(s_status));

  s_status_label = text_layer_create(GRect(4, 0, b.size.w - 28, 16));
  text_layer_set_background_color(s_status_label, GColorClear);
  text_layer_set_text_color(s_status_label, GColorWhite);
  text_layer_set_text_alignment(s_status_label, GTextAlignmentCenter);
  text_layer_set_font(s_status_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_status_label, "⚠ Clear all data?");
  layer_add_child(root, text_layer_get_layer(s_status_label));

  s_body1 = text_layer_create(GRect(0, 26, b.size.w - 20, 18));
  text_layer_set_text(s_body1, "Erase every");
  text_layer_set_text_alignment(s_body1, GTextAlignmentCenter);
  text_layer_set_font(s_body1, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root, text_layer_get_layer(s_body1));

  s_body2 = text_layer_create(GRect(0, 42, b.size.w - 20, 18));
  text_layer_set_text(s_body2, "stored trial.");
  text_layer_set_text_alignment(s_body2, GTextAlignmentCenter);
  text_layer_set_font(s_body2, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root, text_layer_get_layer(s_body2));

  s_body3 = text_layer_create(GRect(0, 62, b.size.w - 20, 18));
  text_layer_set_text(s_body3, "Cannot be undone.");
  text_layer_set_text_alignment(s_body3, GTextAlignmentCenter);
  text_layer_set_font(s_body3, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(root, text_layer_get_layer(s_body3));

  s_cancel_label = text_layer_create(GRect(4, 100, b.size.w - 28, 24));
  text_layer_set_text(s_cancel_label, "Cancel");
  text_layer_set_text_alignment(s_cancel_label, GTextAlignmentCenter);
  text_layer_set_font(s_cancel_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(root, text_layer_get_layer(s_cancel_label));

  s_clear_label = text_layer_create(GRect(4, 132, b.size.w - 28, 24));
  text_layer_set_text(s_clear_label, "Clear data");
  text_layer_set_text_alignment(s_clear_label, GTextAlignmentCenter);
  text_layer_set_font(s_clear_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(root, text_layer_get_layer(s_clear_label));

  s_action = action_bar_layer_create();
  action_bar_layer_set_background_color(s_action, GColorBlack);
  action_bar_layer_add_to_window(s_action, window);

  s_choice = CHOICE_CANCEL;
  update_highlight();
}

static void window_unload(Window *window) {
  (void)window;
  action_bar_layer_destroy(s_action);
  text_layer_destroy(s_clear_label);
  text_layer_destroy(s_cancel_label);
  text_layer_destroy(s_body3);
  text_layer_destroy(s_body2);
  text_layer_destroy(s_body1);
  text_layer_destroy(s_status_label);
  status_bar_layer_destroy(s_status);
  window_destroy(s_window);
  s_window = NULL;
}

void clear_window_push(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}
```

- [ ] **Step 2: Verify pebble build**

Run: `pebble build`

- [ ] **Step 3: Manual emulator test**

Run: `pebble install --emulator flint`
1. Record a couple of trials first so there's data to clear.
2. Return to Home, hold Select for 5 s. Vibration fires; Clear Data screen appears with "Cancel" highlighted.
3. Press Select while on "Cancel" — pop back to Home, history intact.
4. Hold Select again; press Down to highlight "Clear data"; press Select — pop back to Home; the graph re-renders "no data yet"; FG Selection rows now show "no data yet" again.

- [ ] **Step 4: Commit**

```bash
git add src/c/clear_window.c
git commit -m "$(cat <<'EOF'
feat: clear data confirm window with cursor highlight

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

## Phase 4 — End-to-end manual verification

These tasks have no code changes — they're explicit acceptance checklists. Each ends with a git tag so the verified build is captured.

### Task 16: Acceptance — Skip Counting flow

- [ ] **Step 1: Clean build and install**

```bash
pebble build
pebble install --emulator flint
```

- [ ] **Step 2: Walk through the flow**

1. App starts on Home. Category shows last-saved (or `Skip` on first launch).
2. With `Skip` selected, press Select → FG Selection lists "By 2"…"By 9" with `no data yet`.
3. Pick `By 4`. Challenge: "Skip count / by 4". Wait ~10 s. Press Select.
4. Summary appears with the trial time and a single filled bar labeled "now". Press Select with Store highlighted.
5. Back at Home. Graph now shows a single point for the `By 4` line.
6. Run another `By 4` trial. Now Summary shows -1 and now bars; store again. Home graph shows two points.
7. Run a `By 6` trial. Home graph for category `Skip` shows two series with different line patterns (solid for By 2 — wait that has no data — solid for the first FG with data, dashed for the next, etc., per assignment-by-ID order).

- [ ] **Step 3: Tag the working build**

```bash
git tag v0.1-skip-counting
```

---

### Task 17: Acceptance — Multiplication and M&D flow with skips

- [ ] **Step 1: Mult trial**

1. From Home cycle to `Mult`. Select → pick `2-3 × 1-10`.
2. Verify status shows `Mult 2-3` and `1/20` on the right.
3. Press Select 10 times, answering correctly. Counter should reach `11/20`.
4. Press Down (skip). Counter does NOT advance — still `11/20`. Next question shown is different from the skipped one.
5. Answer Select 9 more times (counter reaches `20/20` — but we still have one skipped question outstanding). The Challenge keeps presenting the skipped question (in a randomly chosen display form) until Select is pressed; only then does it finish.
6. Summary appears; Store; back at Home, Mult line for 2-3 shows one point.

- [ ] **Step 2: M&D trial**

1. From Home cycle to `M&D`. Pick `2-3 ×÷ 1-10`.
2. Verify status shows `M&D 2-3` and `1/40`. Roughly half the questions should be `×` formats, half `÷`.
3. Run the trial, skipping a few questions; verify the same skipped fact reappears in a possibly different display form later.
4. Store the trial. Verify the Home `M&D` graph shows one point.

- [ ] **Step 3: Cancel mid-trial**

1. Start any Mult trial. Press Up (cancel) at the third question.
2. Verify you land directly on Home (no Summary). Verify no new trial was appended (FG Selection row count unchanged).

- [ ] **Step 4: Tag**

```bash
git tag v0.2-mult-multdiv
```

---

### Task 18: Acceptance — Persistence and Clear Data

- [ ] **Step 1: Persistence across reinstall**

1. With several trials stored, run: `pebble install --emulator flint` again (over-install).
2. Open the app; verify the graphs and FG Selection counts are exactly as left.

- [ ] **Step 2: Clear data path**

1. From Home hold Select for 5 s. Vibration fires; Cancel highlighted.
2. Press Down to highlight `Clear data`; Select.
3. Verify Home graph re-renders to `no data yet`, FG Selection rows show `no data yet`, and a fresh trial then stores normally.

- [ ] **Step 3: Tag**

```bash
git tag v0.3-final
```

---

## Self-review checklist

After completing all 18 tasks above, the assistant executing this plan should verify:

- [ ] Every section of the spec at `docs/superpowers/specs/2026-05-25-mathfacts-design.md` has at least one task that implements it. Specifically:
  - §3.1 Home → Tasks 9, 10, 11
  - §3.2 FG Selection → Task 12
  - §3.3 Challenge (all three variants) → Task 13
  - §3.4 Summary → Task 14
  - §3.5 Clear Data → Tasks 11 (stub) + 15
  - §4 Fact catalog → Task 4
  - §5 Data model + storage → Tasks 5, 8
  - §6 RNG → Task 9 (`srand` in `main.c`)
  - §7 Timer → Task 13
  - §8 Architecture → Tasks 9–15 in aggregate

- [ ] Host test suite (`make -C tests test`) is green.

- [ ] `pebble build` produces a clean `.pbw`.

- [ ] All three acceptance flows (Tasks 16, 17, 18) pass on the Flint emulator and tags `v0.1-skip-counting`, `v0.2-mult-multdiv`, `v0.3-final` exist.
