CC ?= cc
CFLAGS := -Wall -Wextra -Werror -std=c11 -g -I../src/c -I.

TESTS := test_sanity test_fact_catalog test_trial_history test_trial_engine

.PHONY: all test clean
all: $(TESTS)

test: all
	@set -e; for t in $(TESTS); do echo "--- $$t"; ./$$t; done; echo "ALL TESTS PASSED"

clean:
	rm -f $(TESTS)

test_sanity: test_sanity.c
	$(CC) $(CFLAGS) -o $@ $^

test_fact_catalog: test_fact_catalog.c ../src/c/fact_catalog.c
	$(CC) $(CFLAGS) -o $@ $^

test_trial_history: test_trial_history.c ../src/c/trial_history.c
	$(CC) $(CFLAGS) -o $@ $^

test_trial_engine: test_trial_engine.c ../src/c/trial_engine.c ../src/c/fact_catalog.c
	$(CC) $(CFLAGS) -o $@ $^
