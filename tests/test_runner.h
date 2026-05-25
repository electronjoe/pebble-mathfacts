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
