#pragma once
#include <pebble.h>
#include "fact_catalog.h"

/* Pushes the appropriate Challenge UI for the fact group. */
void challenge_window_push(uint8_t fg_id);
