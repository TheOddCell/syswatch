# syswatch

A minimal POSIX shell system status snapshot. Prints a single line each for host info, CPU/memory, and disk usage — color-coded by threshold.

```
Latitude 7390 odd@imadeitportable 2026-04-20 20:25:24 up 3:00
CPU 23% [baloo_file_extr 32%] MEM 23%
DISK / 67% /home 34% /var 74% /efi 9% /home/odd/src 1%
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
