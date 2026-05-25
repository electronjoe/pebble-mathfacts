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
