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
