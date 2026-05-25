#include "test_runner.h"

static void test_arithmetic(void) {
  EXPECT_EQ_INT(2 + 2, 4, "math still works");
}

int main(void) {
  RUN_TEST(test_arithmetic);
  TEST_MAIN_RETURN();
}
