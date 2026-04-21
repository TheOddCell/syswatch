# syswatch

A minimal POSIX shell system status snapshot. Prints a single line each for host info, CPU/memory, and disk usage — color-coded by threshold.

```
ThinkPad X1 Carbon odd@hostname 2026-04-20 20:16:34 up 3 days
CPU 23% [firefox 4%] MEM 61%
DISK / 48% /home 72%
```

Colors go green → yellow → red as usage approaches and exceeds the warn threshold.

## Usage

```sh
sh syswatch.sh
```

## Thresholds

Edit the top of `syswatch.sh`:

| Variable    | Default |
|-------------|---------|
| `CPU_WARN`  | 70%     |
| `MEM_WARN`  | 80%     |
| `DISK_WARN` | 85%     |

## Dependencies

`vmstat`, `ps`, `free`, `df` — present on any standard Linux system.

## License

BSD Zero Clause — see `LICENSE`.
