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
