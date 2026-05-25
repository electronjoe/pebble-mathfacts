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
