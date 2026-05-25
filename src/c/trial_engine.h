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
