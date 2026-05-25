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
  /* Skipped question now sits at the tail (index 20 after the original 20). */
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
