# syswatch

A minimal system status snapshot. Prints a single line each for host info, CPU/memory, and disk usage — color-coded by threshold.

```
ObsidianOS odd@imadeit 2026-04-21 18:07:55 up 1:54
CPU 5% [baloo_file_extr 88%] MEM 17%
DISK / 47% /var 40% /efi 9% /home 36% /home/odd/src 6%
```

Colors go green → yellow → red as usage approaches and exceeds the warn threshold.

## Usage

```sh
make
./syswatch
```

Always links statically. Uses `musl-gcc` if available, otherwise `gcc` (glibc will emit a harmless linker warning about `getpwuid`).

## Thresholds

Edit the `#define` constants at the top of `syswatch.c`:

| Constant    | Default |
|-------------|---------|
| `CPU_WARN`  | 70%     |
| `MEM_WARN`  | 80%     |
| `DISK_WARN` | 85%     |

## Dependencies

None. All data is read directly from `/proc` and `/sys`.

## License

BSD Zero Clause — see `LICENSE`.
