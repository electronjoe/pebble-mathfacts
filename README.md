# bringup

A Pebble watchapp/watchface written in C using the Pebble SDK.

## Cloning

This repo uses a git submodule for [PebbleOS](https://github.com/coredevices/PebbleOS) at `resources/PebbleOS`. After cloning, initialize it:

```sh
git clone --recurse-submodules <this-repo-url>
# or, if already cloned without --recurse-submodules:
git submodule update --init --recursive
```

To pull submodule updates later:

```sh
git submodule update --remote --merge
```

## Building & running

```sh
pebble build                          # build for all targetPlatforms
pebble install --emulator flint       # install on the pebble time duo emulator
```

### Driving the emulator from the command line

```sh
pebble screenshot --emulator flint out.png   # save framebuffer to PNG
pebble emu-button --emulator flint click up
pebble emu-button --emulator flint click down
pebble emu-button --emulator flint click select
pebble emu-button --emulator flint click back
pebble emu-button --emulator flint push select   # hold (pair with release)
pebble emu-button --emulator flint release select
pebble logs --emulator flint                  # tail APP_LOG output
pebble kill                                   # stop emulator + pypkjs
```

### To Watch over Cloud

- Pebble app on phone, settines -> General -> Sign in Pebble Account /w Google

```sh
pebble login                          # Log in to Google account
pebble install --cloudpebble          # Install to phone
```

## Target platforms

`targetPlatforms` in `package.json` controls which watches you build for. The
modern Pebble hardware is **emery** (Pebble Time 2), **gabbro** (Pebble Round
2), and **flint** (Pebble 2 Duo); the original Pebble platforms (aplite,
basalt, chalk, diorite) are included by default for backwards compatibility.

## Project layout

```
src/c/           C source for the watchapp
src/pkjs/        PebbleKit JS (phone-side) source, if any
worker_src/c/    Background worker source, if any
resources/       Images, fonts, and other bundled resources
package.json     Project metadata (UUID, platforms, resources, message keys)
wscript          Build rules — usually no need to edit
```

By default this project is configured as a watchapp. To make it a watchface,
set `pebble.watchapp.watchface` to `true` in `package.json`.

## Documentation

Full SDK docs, tutorials, and API reference: <https://developer.repebble.com>
