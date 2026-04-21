# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A single POSIX sh script (`syswatch.sh`) that prints a three-line system status snapshot: host/uptime, CPU+memory, and disk usage — color-coded by configurable thresholds.

## Running

```sh
sh syswatch.sh
```

No build step. Dependencies: `vmstat`, `ps`, `free`, `df` (standard Linux utilities).

## Thresholds

Configured as variables at the top of `syswatch.sh`:

| Variable    | Default |
|-------------|---------|
| `CPU_WARN`  | 70%     |
| `MEM_WARN`  | 80%     |
| `DISK_WARN` | 85%     |

Yellow is triggered at 85% of the warn threshold; red at or above it.

## Style constraints

- POSIX sh only (`#!/bin/sh`, `set -eu`) — no bashisms
- All logic stays inlined in `syswatch.sh`; no helper scripts or config files
- Color codes via `printf '%b\n'`, not `echo -e`

## Git commits

Always include Claude as a co-author:

```
Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>
```
