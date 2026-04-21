#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <pwd.h>

#define CPU_WARN  70
#define MEM_WARN  80
#define DISK_WARN 85

#define RED "\033[0;31m"
#define YEL "\033[1;33m"
#define GRN "\033[0;32m"
#define BLU "\033[0;34m"
#define BLD "\033[1m"
#define DIM "\033[2m"
#define RST "\033[0m"

static const char *col(int v, int warn)
{
    if (v >= warn)             return RED;
    if (v >= warn * 85 / 100)  return YEL;
    return GRN;
}

typedef struct { unsigned long long idle, total; } CpuSnap;

static CpuSnap snap_cpu(void)
{
    CpuSnap s = {0, 0};
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return s;
    unsigned long long u, n, sy, id, iw, irq, sirq, st;
    fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
           &u, &n, &sy, &id, &iw, &irq, &sirq, &st);
    fclose(f);
    s.idle  = id + iw;
    s.total = u + n + sy + id + iw + irq + sirq + st;
    return s;
}

static int mem_pct(void)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    char line[128];
    unsigned long long total = 0, avail = 0, val;
    while (fgets(line, sizeof line, f)) {
        if      (sscanf(line, "MemTotal: %llu",     &val) == 1) total = val;
        else if (sscanf(line, "MemAvailable: %llu", &val) == 1) avail = val;
        if (total && avail) break;
    }
    fclose(f);
    return total ? (int)((total - avail) * 100 / total) : 0;
}

#define MAX_PROCS 4096
typedef struct { int pid; unsigned long long cpu; } ProcSnap;

static int snap_procs(ProcSnap *out)
{
    DIR *d = opendir("/proc");
    if (!d) return 0;
    int n = 0;
    struct dirent *e;
    while (n < MAX_PROCS && (e = readdir(d))) {
        /* skip non-numeric entries */
        if (e->d_name[0] < '1' || e->d_name[0] > '9') continue;
        char *p = e->d_name + 1;
        while (*p >= '0' && *p <= '9') p++;
        if (*p) continue;

        char path[PATH_MAX];
        snprintf(path, sizeof path, "/proc/%s/stat", e->d_name);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        char buf[512];
        int ok = (fgets(buf, sizeof buf, f) != NULL);
        fclose(f);
        if (!ok) continue;

        /* utime and stime follow the closing ')' of comm */
        char *after = strrchr(buf, ')');
        if (!after) continue;
        unsigned long utime, stime;
        /* fields after ')': state ppid pgrp session tty tpgid flags
           minflt cminflt majflt cmajflt utime stime */
        if (sscanf(after + 1,
                   " %*c %*d %*d %*d %*d %*d %*d"
                   " %*d %*d %*d %*d %lu %lu",
                   &utime, &stime) != 2) continue;

        out[n].pid = atoi(e->d_name);
        out[n].cpu = utime + stime;
        n++;
    }
    closedir(d);
    return n;
}

static void top_proc(char *out, size_t n,
                     const ProcSnap *s0, int n0,
                     const ProcSnap *s1, int n1,
                     unsigned long long sys_dt)
{
    int best_pid = -1;
    unsigned long long best_delta = 0;

    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n0; j++) {
            if (s1[i].pid != s0[j].pid) continue;
            unsigned long long d = s1[i].cpu - s0[j].cpu;
            if (d > best_delta) { best_delta = d; best_pid = s1[i].pid; }
            break;
        }
    }

    if (best_pid < 0 || best_delta == 0 || !sys_dt) { snprintf(out, n, "?"); return; }

    char path[64], comm[64] = "?";
    snprintf(path, sizeof path, "/proc/%d/comm", best_pid);
    FILE *f = fopen(path, "r");
    if (f) { fgets(comm, sizeof comm, f); comm[strcspn(comm, "\n")] = '\0'; fclose(f); }

    /* sys_dt is the sum across all CPUs; scale to match ps's per-CPU % */
    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nproc < 1) nproc = 1;
    unsigned long long pct = best_delta * (unsigned long long)nproc * 100 / sys_dt;
    snprintf(out, n, "%s %llu%%", comm, pct);
}

static void fmt_uptime(char *out, size_t n)
{
    FILE *f = fopen("/proc/uptime", "r");
    if (!f) { snprintf(out, n, "?"); return; }
    double secs; fscanf(f, "%lf", &secs); fclose(f);
    long s = (long)secs;
    long d = s / 86400, h = (s % 86400) / 3600, m = (s % 3600) / 60;
    if (d) snprintf(out, n, "%ld day%s, %ld:%02ld", d, d == 1 ? "" : "s", h, m);
    else   snprintf(out, n, "%ld:%02ld", h, m);
}

static void get_product(char *out, size_t n)
{
    /* try DMI product name */
    FILE *f = fopen("/sys/class/dmi/id/product_name", "r");
    if (f) {
        fgets(out, (int)n, f);
        out[strcspn(out, "\n")] = '\0';
        fclose(f);
        if (*out && strcmp(out, "System Product Name") != 0)
            return;
    }

    /* fall back to NAME from /etc/os-release, stripping quotes */
    f = fopen("/etc/os-release", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof line, f)) {
            if (strncmp(line, "NAME=", 5) != 0) continue;
            char *val = line + 5;
            val[strcspn(val, "\n")] = '\0';
            if (*val == '"') {
                val++;
                val[strcspn(val, "\"")] = '\0';
            }
            if (*val) { snprintf(out, n, "%s", val); fclose(f); return; }
        }
        fclose(f);
    }

    /* last resort: sysname + release via uname(2) */
    struct utsname u;
    if (uname(&u) == 0)
        snprintf(out, n, "%s %s", u.sysname, u.release);
    else
        snprintf(out, n, "unknown");
}

#define MAX_DISKS 64

static void print_disks(void)
{
    FILE *f = fopen("/proc/mounts", "r");
    if (!f) { puts("DISK"); return; }
    char line[512], seen[MAX_DISKS][128];
    int  ns = 0;
    printf("DISK");
    while (fgets(line, sizeof line, f)) {
        char dev[128], mnt[256];
        if (sscanf(line, "%127s %255s", dev, mnt) < 2) continue;
        if (strncmp(dev, "/dev/", 5) != 0)  continue;
        if (strncmp(mnt, "/run",  4) == 0)  continue;
        int dup = 0;
        for (int i = 0; i < ns; i++)
            if (!strcmp(seen[i], dev)) { dup = 1; break; }
        if (dup || ns >= MAX_DISKS) continue;
        memcpy(seen[ns++], dev, sizeof seen[0]);
        struct statvfs st;
        if (statvfs(mnt, &st) || !st.f_blocks) continue;
        int pct = (int)((st.f_blocks - st.f_bfree) * 100 / st.f_blocks);
        printf(" %s %s%d%%%s", mnt, col(pct, DISK_WARN), pct, RST);
    }
    fclose(f);
    printf("\n");
}

int main(void)
{
    /* start CPU + per-process sample window */
    CpuSnap s0 = snap_cpu();
    static ProcSnap ps0[MAX_PROCS], ps1[MAX_PROCS];
    int np0 = snap_procs(ps0);
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* gather everything else while the window runs */
    int  mem = mem_pct();
    char upt[64];   fmt_uptime(upt, sizeof upt);
    char prod[256]; get_product(prod, sizeof prod);

    char host[64];
    gethostname(host, sizeof host);

    const char *user = getenv("USER");
    char ubuf[64];
    if (!user) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            strncpy(ubuf, pw->pw_name, sizeof ubuf - 1);
            ubuf[sizeof ubuf - 1] = '\0';
            user = ubuf;
        } else {
            user = "?";
        }
    }

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char ts[32];
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", tm);

    /* sleep the remainder of the 200ms sample window */
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long elapsed = (t1.tv_sec  - t0.tv_sec)  * 1000
                 + (t1.tv_nsec - t0.tv_nsec) / 1000000;
    if (elapsed < 200) {
        struct timespec rem = { 0, (200 - elapsed) * 1000000L };
        nanosleep(&rem, NULL);
    }

    /* close the CPU + per-process sample window */
    CpuSnap s1 = snap_cpu();
    int np1 = snap_procs(ps1);
    unsigned long long dt = s1.total - s0.total;
    unsigned long long di = s1.idle  - s0.idle;
    int cpu = dt ? (int)((dt - di) * 100 / dt) : 0;

    char proc[128];
    top_proc(proc, sizeof proc, ps0, np0, ps1, np1, dt);

    printf(BLD BLU "%s" RST " " BLD GRN "%s@%s" RST " " DIM "%s up %s" RST "\n",
           prod, user, host, ts, upt);
    printf("CPU %s%d%%" RST " " DIM "[%s]" RST " MEM %s%d%%" RST "\n",
           col(cpu, CPU_WARN), cpu, proc,
           col(mem, MEM_WARN), mem);
    print_disks();
    return 0;
}
