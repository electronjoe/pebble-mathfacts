# MathFacts

MathFacts is a Pebble watchapp for timed math-fact practice. It runs each drill
on the watch, records the elapsed time locally, and shows recent performance on
a simple black-and-white history graph.

The app is intentionally watch-only: no PebbleKit JS, no phone companion, no
cloud sync, and no data export in v1.

## App design

MathFacts is built for Pebble Flint with a black-and-white UI. The current
design is documented in:

```sh
docs/superpowers/specs/2026-05-25-mathfacts-design.md
```

The app has five screens:

- Home: category selector and 120-day daily-best history graph.
- Fact Group Select: menu of fact groups with best time and trial count.
- Challenge: timed drill flow with done, skip, and cancel controls.
- Summary: trial result with comparison bars and Store/Discard choice.
- Clear Data: hidden confirmation screen reached by holding Select for 5 seconds
  on Home.

## Practice content

There are 16 fact groups across three categories:

- Skip Counting: By 2 through By 9, one recitation timed to 100.
- Multiplication: `2-3`, `4-5`, `6-7`, and `8-9` times `1..10`, 20 questions per
  trial.
- Multiplication & Division: the same fact groups with multiplication and
  division forms, 40 questions per trial.

Multiplication and division trials randomize the question order. Skipped
questions are appended to the end of the current queue and must still be
completed before the trial can finish.

## Controls

- Home:
  - Up/Down cycles the active category.
  - Select opens Fact Group Select.
  - Hold Select for 5 seconds opens Clear Data.
  - Back exits the app.
- Fact Group Select:
  - Up/Down moves the highlighted fact group.
  - Select starts a challenge.
  - Back returns Home.
- Challenge:
  - Up cancels and discards the in-progress trial.
  - Select marks the prompt done/correct.
  - Down skips multiplication and division questions.
- Summary and Clear Data:
  - Up/Down changes the highlighted option.
  - Select confirms.
  - Back discards/cancels.

## Data storage

Trial history is stored with Pebble persist keys only on the watch.

- Keys `0..15`: one packed trial-history array per fact group.
- Key `100`: the last viewed category.
- Each trial record stores `timestamp_utc` and `duration_s`.
- Each fact group stores about 42 trials, newest first.
- The Home graph uses daily-best records in local time.

Clearing data deletes every app persist key.

## Cloning

This repo uses a git submodule for
[PebbleOS](https://github.com/coredevices/PebbleOS) at `resources/PebbleOS`.
After cloning, initialize it:

```sh
git clone --recurse-submodules <this-repo-url>
# or, if already cloned without --recurse-submodules:
git submodule update --init --recursive
```

To pull submodule updates later:

```sh
git submodule update --remote --merge
```

## Building and running

The package is configured as a Pebble SDK 3 watchapp targeting Flint only:

```json
"targetPlatforms": ["flint"]
```

Build and install:

```sh
pebble build
pebble install --emulator flint
```

Useful emulator commands:

```sh
pebble screenshot --emulator flint out.png
pebble emu-button --emulator flint click up
pebble emu-button --emulator flint click down
pebble emu-button --emulator flint click select
pebble emu-button --emulator flint click back
pebble emu-button --emulator flint push select
pebble emu-button --emulator flint release select
pebble logs --emulator flint
pebble kill
```

Install to a watch over CloudPebble:

```sh
pebble login
pebble install --cloudpebble
```

## Tests

Pure logic modules have a host-side test harness:

```sh
make -C tests
```

## Known issue: Flint emulator install hangs on SDK 4.9.169

`pebble install --emulator flint` can hang partway through the resource transfer
on SDK 4.9.169 with pebble-tool 5.0.35. The emulator opens and the install
status appears, then libpebble2 waits for the final resource install response
until the host prints `App install failed.`

The suspected root cause is the pre-fix Flint firmware ROM bundled with that SDK
release. Relevant fixes landed in
[coredevices/PebbleOS#1324](https://github.com/coredevices/PebbleOS/pull/1324)
on 2026-05-19 and should ship with a later Pebble SDK. Reinstalling pebble-tool
or running `pebble sdk install latest` will not help while 4.9.169 is still the
latest SDK, because the bundled firmware ROM is unchanged.

The `resources/PebbleOS` submodule in this repo is already past the fix, so one
local workaround is to build the Flint firmware from the submodule and replace
the SDK's bundled `qemu_micro_flash.bin`.

## Project layout

```text
src/c/main.c              App entry point
src/c/home_window.*       Home graph and category cycling
src/c/factgroup_window.*  Fact Group Select menu
src/c/challenge_window.*  Challenge flow
src/c/summary_window.*    Store/Discard summary
src/c/clear_window.*      Clear Data confirmation
src/c/fact_catalog.*      Fact group registry
src/c/trial_engine.*      Question queue and trial state
src/c/trial_history.*     Packed trial record helpers
src/c/storage.*           Persist read/write helpers
src/c/graph_data.*        Daily-best graph aggregation
tests/                    Host-side pure-logic tests
resources/PebbleOS/       PebbleOS submodule
package.json              Pebble package metadata
wscript                   Pebble build rules
```

## Documentation

Pebble SDK docs, tutorials, and API reference: <https://developer.repebble.com>
