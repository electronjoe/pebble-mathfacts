# MathFacts — Design Spec

**Date:** 2026-05-25
**Target hardware:** Pebble Flint (Pebble Time 2 Duo), 144×168 pixels, color hardware but rendered black-and-white by design.
**SDK:** Pebble SDK 3, watchapp (not watchface).
**App UUID:** `dae32bd3-d94d-4f59-9c28-92d59749ed3c`
**App display name:** `MathFacts`

## 1. Purpose

A timed math-fact drilling tool for the developer's own kids, used at home under supervision. The Pebble runs each "trial" (one drill of one fact group), records the elapsed time on-device, and shows a 120-day history graph so the kid can see their times improve.

Storage is **watch-only** — no PebbleKit JS, no phone companion. The graph on the Home screen is the only data view.

## 2. Scope

### In scope (v1)

- Three Fact Group Categories: Skip Counting, Multiplication, Multiplication & Division.
- Sixteen Fact Groups total (see §4).
- Trial timing, history storage (≈42 trials per fact group), 120-day historical line graph.
- Black-and-white UI, Flint-only build target.
- Clear-all-data hidden behind a 5-second long-press of Select on the Home screen.

### Out of scope (v1)

- Phone sync / data export / cloud backup.
- Additional categories (addition, subtraction, fractions, etc.).
- Per-fact accuracy tracking (we only record total trial duration, not per-question time or correctness).
- Multi-platform builds (aplite/basalt/chalk/diorite/emery/gabbro removed from `targetPlatforms`).
- Sound effects.
- Multiple user profiles.

## 3. UI screens

All mockups live in `.superpowers/brainstorm/<session>/content/` and are referenced inline below. Five screens total; navigation is a window stack.

```
   Home  ──●──▶  Fact Group Select  ──●──▶  Challenge  ──complete──▶  Summary  ──●──▶  Home
    │                                          │
    │                                       cancel (▲)
    │                                          │
    └─── 5s ●hold ──▶  Clear Data ──●──▶  Home  ◀──────────────────────┘
```

### 3.1 Home UI

`home-ui-bw-v2.html`

- **Status bar (y=0..15, 16 px):** `MathFacts` (left), current category abbreviation (right): `Skip`, `Mult`, or `M&D`.
- **Graph plot (y=20..136, ≈117 px):** Multi-series line graph, X-axis = days (120-day window ending today, or shorter if no older data exists), Y-axis = trial duration in seconds.
  - Y-axis scaling: from best (min) to worst (max) trial duration across all fact groups in the current category, **restricted to trials whose timestamp falls inside the visible X window**.
  - One line per fact group within the current category.
  - Series differentiated by line pattern in fixed order — within a category, the fact groups are listed in ID order (see §4) and assigned patterns: 1st = solid, 2nd = dashed, 3rd = dotted, 4th = dash-dot. (Skip Counting has 8 groups; patterns cycle: 5th = solid, 6th = dashed, etc. Acceptable because Skip Counting will not have 8 lines all overlapping in practice — most users drill 1–3 at a time.)
  - Each line is labeled at its right edge with a short tag:
    - Multiplication / M&D: `2-3`, `4-5`, `6-7`, `8-9`.
    - Skip Counting: the integer alone — `2`, `3`, …, `9`.
  - Daily best only: a "day" is a calendar day in **local time** (`localtime_r` on `time(NULL)`). If multiple trials exist on the same local day for a fact group, only the lowest-duration trial contributes a point.
- **X-axis labels (y=137..148, 12 px):** `120d` (left edge), `today` (right edge).
- **Control hint (y=150..167, 18 px):** `▲▼ category   ● select`.
- **Empty state:** if no fact groups in the category have any trials yet, render the plot area with the gridlines and a centered "no data yet" message (Gothic 14).

**Controls:**
- `▲` / `▼` cycles the active Fact Group Category. Persisted across app launches (last viewed category remembered in a dedicated persist key).
- `●` (Select) — single click pushes the Fact Group Selection screen.
- `●` (Select) — hold ≥ 5 s pushes the Clear Data screen (subtle vibration tick at the 5-second mark).
- Back button — exits the app (system default behavior on the bottom window).

### 3.2 Fact Group Selection UI

`factgroup-select-v2.html`

Standard Pebble `menu_layer` with one section.

- **Status bar:** category short-name on the left (`Mult`, `Skip Count`, `M&D`), `Pick group` on the right.
- **Each row:**
  - Title line (Gothic 14, bold when highlighted): fact group name (e.g. `2-3 × 1-10`, `By 4`).
  - Subtitle line (Gothic 10): best time (`0:42`) followed by `· N trials`, or `no data yet` when N=0. The word "best" is **not** rendered — the time alone implies it.
- **Highlight model:** standard inverted row. `▲` / `▼` moves highlight. `●` enters Challenge UI for the highlighted fact group.
- **Scroll affordance:** Up-chevron at top of list when content extends above; down-chevron at bottom when content extends below. Provided by `menu_layer` natively.
- **Back button:** pops back to Home UI.

### 3.3 Challenge UI

`challenge-ui-v2.html`

All three category challenge screens share chrome:

- **Status bar:** fact group abbrev on the left (`Mult 2-3`, `Skip Count By 7`, `M&D 6-7`); progress indicator `n/N` on the right (`5/20`, `12/40`, etc.). Skip Counting omits the n/N counter — it's a single-question trial.
  - **Progress semantics:** `N` is fixed for the duration of the trial (20 for Mult, 40 for M&D) — it does **not** grow when the student skips. `n` is the number of questions that have been answered with `●` (correct/done); skipping does not increment `n`. So a trial that has been skipped twice will show `n/N` plateau at the skip point, then advance again when the student returns to the skipped questions at the end.
- **Big challenge text:** Gothic 28 bold, two lines centered between y=70 and y=120 in the content area (left of action bar).
- **Timer:** Gothic 14 below the challenge text, `⏱ MM:SS` updated on the `TickTimerService` second tick.
- **Action bar (right 20 px):** icons for the three hardware buttons, aligned vertically with each button.

**Variant: Skip Counting (`skip` category)**

- Challenge text: prompt of the form `Skip count / by N` on two lines, plus a smaller `to 100` line below.
- Action bar:
  - `▲` (top): `✕` cancel
  - `●` (middle): `✓` done
  - `▼` (bottom): unused (no icon rendered)

Pressing `●` records `duration_s` and transitions to Challenge Summary.

**Variant: Multiplication (`mult` category)**

- Challenge text: equation split across two lines per the **alignment rule** below.
- Action bar:
  - `▲`: `✕` cancel
  - `●`: `✓` correct, advance to next question
  - `▼`: `→` skip — append current question to end of queue, advance

After question N is answered, transition to Challenge Summary. Trial considered complete when all 20 originally-generated questions have been answered (skipped ones included).

**Variant: Multiplication & Division (`multdiv` category)**

Identical to Multiplication, but with 40 questions, division formats allowed.

**Equation alignment rule (Mult and M&D)**

For an equation of the form `A op B = C` with exactly one of A/B/C replaced by `?`:

- The side of the equation containing `?` is rendered intact on its own line, no spaces around the operator (e.g. `?×2`, `2×?`, `42÷?`, `?÷2`).
- The other side is the number alone fused with the `=` sign on its own line (e.g. `=6`, `7=`).
- The "answer-on-the-left" forms (`6=…`) and "answer-on-the-right" forms (`…=6`) split as illustrated in `challenge-ui-v2.html`.
- Font: Gothic 28 bold, centered. Maximum expression line is 4 characters (e.g. `42÷?`), which fits within the 124 px content width.

**Cancel behavior**

Pressing `▲` (cancel) at any point during a Challenge:

- Unsubscribes `TickTimerService`.
- Frees the question queue.
- Discards all trial state — nothing is written to persist.
- Pops back to the Home UI directly (skips the Summary screen).

### 3.4 Challenge Summary UI

`summary-ui.html`

Reached on trial completion. Cursor-highlight menu pattern.

- **Status bar:** `Trial done` (left), fact group abbrev (right).
- **Result time:** Gothic 22 bold, centered, format `M:SS`.
- **Comparison chart (y≈50..120):** up to 4 vertical bars showing this trial plus the previous up-to-3 trials of the same fact group. The current trial is filled black; previous trials are outlined.
  - Each bar labeled with its `M:SS` time underneath.
  - Tags below the time labels: `-3 -2 -1 now`.
  - Bar heights are scaled so the slowest bar fills the chart area; shorter = faster.
  - If fewer than 3 priors exist, render only what's available (e.g. 1 prior + now = 2 bars).
- **Option list (y=130..167):** `Store` (top), `Discard` (bottom).
- **Highlight:** default on `Store`. `▲` / `▼` toggles, `●` confirms.
- **Action bar:** `▲` chevron up, `●` check, `▼` chevron down.

**Outcomes:**
- `Store` → append `{timestamp, duration_s}` to the fact group's persist key (drop oldest if at capacity), then pop back to Home UI.
- `Discard` → no write, pop back to Home UI.
- Back button → equivalent to Discard.

### 3.5 Clear Data UI

`clear-data-v2.html`

Reached by holding `●` on Home UI for ≥ 5 s. Cursor-highlight menu pattern.

- **Status bar (inverted):** `⚠ Clear all data?` centered.
- **Warning body:** three centered lines, Gothic 12: `Erase every` / `stored trial.` / `Cannot be undone.` (last line bold).
- **Option list:** `Cancel` (top), `Clear data` (bottom).
- **Default highlight:** `Cancel`.
- **Action bar:** `▲` chevron up, `●` check, `▼` chevron down.

**Outcomes:**
- `Cancel` (or back button) → pop back to Home.
- `Clear data` → call `persist_delete` for every app persist key, pop back to Home; Home re-renders empty.

## 4. Fact Group Catalog

Total: 16 fact groups across 3 categories.

### 4.1 Skip Counting (8 groups)

Each fact group is a single recitation challenge — the student counts orally aloud. The app shows the prompt and times the recitation; `●` indicates done.

| ID | Name   | Prompt (lines)                 |
|----|--------|---------------------------------|
| 0  | By 2   | `Skip count` / `by 2` / `to 100` |
| 1  | By 3   | `Skip count` / `by 3` / `to 100` |
| 2  | By 4   | `Skip count` / `by 4` / `to 100` |
| 3  | By 5   | `Skip count` / `by 5` / `to 100` |
| 4  | By 6   | `Skip count` / `by 6` / `to 100` |
| 5  | By 7   | `Skip count` / `by 7` / `to 100` |
| 6  | By 8   | `Skip count` / `by 8` / `to 100` |
| 7  | By 9   | `Skip count` / `by 9` / `to 100` |

The student recites multiples up to and including the largest multiple ≤ 100. The app does **not** validate the recitation — it only times it.

### 4.2 Multiplication (4 groups, 20 facts each)

Each fact group's underlying fact set is the Cartesian product `{A range} × {1..10}`. Trial = each underlying fact appears exactly once as a Challenge Question (20 questions), in randomized order with a per-question randomized display format.

| ID | Name        | Underlying facts                            |
|----|-------------|---------------------------------------------|
| 8  | 2-3 × 1-10  | { 2,3 } × { 1..10 } = 20 facts              |
| 9  | 4-5 × 1-10  | { 4,5 } × { 1..10 } = 20 facts              |
| 10 | 6-7 × 1-10  | { 6,7 } × { 1..10 } = 20 facts              |
| 11 | 8-9 × 1-10  | { 8,9 } × { 1..10 } = 20 facts              |

### 4.3 Multiplication & Division (4 groups, 40 facts each)

Same underlying multiplication fact sets as §4.2. Trial = each underlying fact appears exactly twice — once as a multiplication question, once as a division question — for 40 total questions. Order randomized, display format randomized per question.

| ID | Name        | Trial size |
|----|-------------|------------|
| 12 | 2-3 ×÷ 1-10 | 40         |
| 13 | 4-5 ×÷ 1-10 | 40         |
| 14 | 6-7 ×÷ 1-10 | 40         |
| 15 | 8-9 ×÷ 1-10 | 40         |

### 4.4 Display formats

For an underlying multiplication fact `A × B = C`:

| Form | Visual (line 1 / line 2) |
|------|--------------------------|
| 1    | `?×B` / `=C`             |
| 2    | `A×?` / `=C`             |
| 3    | `C=` / `?×B`             |
| 4    | `C=` / `A×?`             |

For the same fact rephrased as division `C ÷ A = B` (or `C ÷ B = A`):

| Form | Visual (line 1 / line 2) |
|------|--------------------------|
| 5    | `?÷A` / `=B`             |
| 6    | `C÷?` / `=B`             |
| 7    | `B=` / `?÷A`             |
| 8    | `B=` / `C÷?`             |

For each question, the app picks one of the 4 forms for that question's operation uniformly at random.

**M&D queue construction:** for an M&D trial, the 40-question queue is built deterministically at trial start as 20 multiplication entries (one per underlying fact) plus 20 division entries (one per underlying fact), then shuffled once. The operation (mult vs div) is therefore decided at queue build time, not per-presentation, but the **display form** (which slot is `?`, which side `=` is on) is picked at each render — so a skipped-and-requeued question may be redisplayed in a different visual form.

## 5. Data model

### 5.1 Trial record (packed)

```c
typedef struct __attribute__((packed)) {
  uint32_t timestamp_utc;   // time(NULL) at moment of Store
  uint16_t duration_s;      // whole seconds elapsed during trial
} TrialRecord;              // 6 bytes
```

### 5.2 Persist layout

| Key range            | Contents                                                 |
|----------------------|----------------------------------------------------------|
| `0..15` (one per FG) | `TrialRecord[]` packed array, ordered newest-first, up to ~42 entries (256 B / 6 B). On Store: prepend; if `count == capacity`, drop the oldest first. |
| `100`                | `uint8_t` — last viewed Fact Group Category index (0=Skip, 1=Mult, 2=M&D). Default 0. |

Per-FG arrays are loaded into RAM with `persist_read_data` when the corresponding screen is opened; modifications are written back with `persist_write_data` on Store.

### 5.3 Capacity math

- Per-key cap: 256 B / 6 B per record = **42 trials per fact group**.
- 16 FGs × 256 B = 4096 B; plus 1 B for last-category key = ~4 KB total worst-case usage, well under the 1 MiB per-app persist budget.

### 5.4 Sample-rate considerations for the Home graph

The graph shows daily-best only. With 42 trials stored per FG, a kid doing one trial per day per FG has ~42 days of history; doing several trials per day still consumes 42 trial slots quickly. This is acceptable for v1 (the user accepts the constraint of ≤42-day-worth of detailed history on heavy-use days); future versions may compact to daily-best on disk.

## 6. Random number generation

- Seed once at app init with `srand(time(NULL))`.
- Question shuffle: in-place Fisher-Yates on the question array.
- Display format pick: `rand() % N` where N is the number of valid forms.

## 7. Timer

- At first question render of a trial: `s_trial_start_ms = time_ms(NULL, NULL)` (Pebble `time_ms` returns ms-precision time).
- Subscribe `TickTimerService` at `SECOND_UNIT` to update the on-screen `⏱` display.
- On trial completion: `duration_s = (time_ms(NULL, NULL) - s_trial_start_ms) / 1000`. Round down so a 1:29.9 trial reads `1:29`.
- Unsubscribe `TickTimerService` on window unload.

## 8. App architecture

### 8.1 Module layout (`src/c/`)

```
main.c              — app entry, init/deinit, top-level window push
home_window.{c,h}   — Home UI window (graph rendering, category cycle)
factgroup_window.{c,h}  — Fact Group Selection (menu_layer)
challenge_window.{c,h}  — Challenge UI (all three variants behind a config)
summary_window.{c,h}    — Challenge Summary
clear_window.{c,h}      — Clear Data confirm
fact_catalog.{c,h}      — fact group registry (names, sizes, prompt text, category)
trial_engine.{c,h}      — generate question queue, advance, skip-to-end, timer
storage.{c,h}           — persist read/write, trim-on-write, daily-best helpers
graph.{c,h}             — pure rendering: takes per-FG TrialRecord arrays + draws lines
```

### 8.2 Window stack lifecycle

| Window pushed by | Pops to |
|------------------|---------|
| `prv_init` | (root — back exits app) |
| Home `●` short | Fact Group Select |
| FG Select `●` | Challenge |
| Challenge `●` last question | Summary |
| Summary `●` (any choice) | Home |
| Home `●` long-press 5 s | Clear Data |
| Clear `●` (any choice) | Home |

Pebble's window stack handles all back-button pop transitions automatically.

### 8.3 Long-press handling on Home

`window_single_click_subscribe(BUTTON_ID_SELECT, …)` and `window_long_click_subscribe(BUTTON_ID_SELECT, 5000, hold_down_handler, NULL)` are both registered on the Home window. Pebble's click system fires the single-click handler if the press releases before 5 s, and the long-click handler if the press is held to 5 s. Only one fires per press. The long-click handler triggers a `vibes_short_pulse()` for tactile confirmation, then pushes the Clear Data window.

## 9. Build & target platform

- `package.json`:
  - `displayName`: `MathFacts`
  - `uuid`: `dae32bd3-d94d-4f59-9c28-92d59749ed3c`
  - `targetPlatforms`: `["flint"]`
  - `messageKeys`: empty (no PebbleKit JS).
  - `resources.media`: empty for v1 (no custom images or fonts; system fonts only).

## 10. Constraints & non-functional notes

- **Memory:** all trial arrays for a single FG fit in 256 B; transient question queue for M&D worst case = 40 questions × ~6 B/queue-entry = 240 B. App heap budget on Flint is generous; this design uses well under 4 KB of runtime state.
- **Battery:** TickTimerService at second-rate is the only continuous timer; backlight is not forced (rely on user wrist movement to wake). No accelerometer, BT, or HRM subscriptions.
- **B&W rendering:** every layer uses only `GColorBlack` and `GColorWhite`. No greyscale, no color even though Flint hardware supports color.
- **Accessibility:** font sizes are all Gothic 14 or larger except for the 10-px subtitle on Fact Group Selection rows. Status bars use Gothic 11. All chosen to be legible on a 144×168 screen.

## 11. Memory of brainstorming decisions

The following preferences were captured to persistent project memory and apply to all future work:

- All mockups rendered in B&W with realistic 144×168 fonts. (`project-mathfacts-bw`)
- All Up/Down/Select menus use cursor-highlight pattern; safe option highlighted by default. (`feedback-menu-pattern`)

## 12. Open questions (deferred)

- Should we compact older trials to "daily best only" on-disk to extend history beyond ~42 trials per FG? Deferred — current limit is acceptable for v1.
- Should the graph show a trend line / median across fact groups? Deferred.
- Future: per-question reaction time within a trial. Deferred — would change data model.
