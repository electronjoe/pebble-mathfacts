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
