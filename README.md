# dfzygisk

Zygisk module that injects frida-gadget into `com.garena.game.df`
**before** `libUE4.so` is loaded, so Lua bytecode loading can be captured
from the very first instruction.

## Why

The game decrypts its Lua hotfix bundle during engine init (< 1s) and never
keeps the plaintext resident. Attaching Frida later always misses it.

Zygisk's `postAppSpecialize` runs in the forked app process before any app
code executes — that is the only viable injection point.

## Layout

```
src/module.cpp   Zygisk module: dlopen libgadget.so in postAppSpecialize
src/zygisk.hpp   official Zygisk API v5 header
magisk/          Magisk module files (module.prop / service.sh / ...)
CMakeLists.txt   builds libzygisk.so for arm64-v8a
```

## Build

Pushing to `main` triggers GitHub Actions, artifact: `dfzygisk-module`.

## Install

1. Flash `dfzygisk.zip` in Magisk, reboot
2. Put `frida-gadget` as `/data/adb/modules/dfzygisk/lib64/libgadget.so`
3. Put its config as `/data/adb/modules/dfzygisk/lib64/libgadget.config.so`
4. Launch the game, connect: `frida -H 127.0.0.1:27043 -f com.garena.game.df`
