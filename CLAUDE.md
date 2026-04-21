# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A C program (`syswatch.c`) that prints a three-line system status snapshot: host/uptime, CPU+memory, and disk usage — color-coded by configurable thresholds.

## Building

```sh
make
```

The Makefile always links statically (`-static`). It prefers `musl-gcc` if found, otherwise falls back to `gcc` (which will emit a harmless linker warning about `getpwuid` with glibc). Run the result with:

```sh
./syswatch
```

No runtime dependencies beyond libc. All data read directly from `/proc` and `/sys`.

## Thresholds

Configured as `#define` constants at the top of `syswatch.c`:

| Constant    | Default |
|-------------|---------|
| `CPU_WARN`  | 70%     |
| `MEM_WARN`  | 80%     |
| `DISK_WARN` | 85%     |

Yellow is triggered at 85% of the warn threshold; red at or above it.

## Style constraints

- No third-party libraries; all system data read from `/proc` or via `statvfs()`; CPU sampled over a 200ms window with all other reads overlapped inside it
- All logic stays inlined in `syswatch.c`; no helper scripts or config files

## Git commits

Always include Claude as a co-author:

```
Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>
```
