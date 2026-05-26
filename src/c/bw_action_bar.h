#pragma once
#include <pebble.h>

/* A tiny B&W action-bar replacement for Pebble's action_bar_layer, used
 * because that layer requires GBitmap icons (we don't bundle any) and the
 * Pebble system fonts don't render the unicode glyphs we want. This draws
 * icon shapes (X / check / arrows / chevrons) directly via graphics primitives. */

typedef enum {
  BW_ICON_NONE = 0,
  BW_ICON_CANCEL,       /* X */
  BW_ICON_CHECK,        /* checkmark */
  BW_ICON_SKIP,         /* right-arrow */
  BW_ICON_CHEVRON_UP,
  BW_ICON_CHEVRON_DOWN,
} BWIconKind;

typedef struct BWActionBar BWActionBar;

/* Create a 20-px-wide black bar on the right edge of `window`'s root layer,
 * starting just below the status bar (y=16). The bar is added to the window. */
BWActionBar *bw_action_bar_create(Window *window);

void bw_action_bar_destroy(BWActionBar *bar);

/* Set which icon to draw for each of the three buttons (top=up, middle=select,
 * bottom=down). Triggers a redraw. */
void bw_action_bar_set_icons(BWActionBar *bar,
                             BWIconKind up, BWIconKind select, BWIconKind down);
