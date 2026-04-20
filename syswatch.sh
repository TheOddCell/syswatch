#!/usr/bin/env bash
set -euo pipefail

# Thresholds (percent)
CPU_WARN=70
MEM_WARN=80
DISK_WARN=85

RED='\033[0;31m'
YEL='\033[1;33m'
GRN='\033[0;32m'
BLD='\033[1m'
RST='\033[0m'

colorize() {
    local val=$1 warn=$2
    if   (( val >= warn ));       then echo -e "${RED}${val}%${RST}"
    elif (( val >= warn * 85/100 )); then echo -e "${YEL}${val}%${RST}"
    else echo -e "${GRN}${val}%${RST}"
    fi
}

cpu_usage() {
    # Average idle across all CPUs, then invert
    local idle
    idle=$(vmstat 1 2 | awk 'NR==4{print $15}')
    echo $(( 100 - idle ))
}

mem_usage() {
    free | awk '/^Mem:/{printf "%d", $3/$2*100}'
}

disk_usage() {
    local path=${1:-/}
    df -P "$path" | awk 'NR==2{sub(/%/,"",$5); print $5}'
}

section() {
    echo -e "\n${BLD}==> $1${RST}"
}

show_cpu() {
    section "CPU"
    local pct; pct=$(cpu_usage)
    echo -e "  Usage: $(colorize "$pct" "$CPU_WARN")"
}

show_mem() {
    section "Memory"
    local pct; pct=$(mem_usage)
    local total used avail
    read -r total used avail < <(free -h | awk '/^Mem:/{print $2, $3, $7}')
    echo -e "  Usage: $(colorize "$pct" "$MEM_WARN")  (${used} / ${total}, ${avail} free)"
}

show_disk() {
    section "Disk"
    while IFS= read -r line; do
        local pct mount
        pct=$(echo "$line" | awk '{sub(/%/,"",$5); print $5}')
        mount=$(echo "$line" | awk '{print $6}')
        echo -e "  ${mount}: $(colorize "$pct" "$DISK_WARN")"
    done < <(df -Ph | awk 'NR>1 && $1!~/tmpfs|devtmpfs|udev/')
}

show_top_procs() {
    section "Top Processes (by CPU)"
    ps -eo pid,comm,%cpu,%mem --sort=-%cpu | head -6 | \
        awk 'NR==1{printf "  %-8s %-20s %6s %6s\n",$1,$2,$3,$4; next}
             {printf "  %-8s %-20s %6s %6s\n",$1,$2,$3,$4}'
}

main() {
    echo -e "${BLD}syswatch — $(hostname) — $(date '+%Y-%m-%d %H:%M:%S')${RST}"
    show_cpu
    show_mem
    show_disk
    show_top_procs
    echo
}

main "$@"
