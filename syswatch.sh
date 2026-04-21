#!/bin/sh
set -eu

CPU_WARN=70
MEM_WARN=80
DISK_WARN=85

RED='\033[0;31m'
YEL='\033[1;33m'
GRN='\033[0;32m'
BLU='\033[0;34m'
BLD='\033[1m'
DIM='\033[2m'
RST='\033[0m'

cpu=$(vmstat 1 2 | awk 'NR==4{print 100 - $15}')
top_proc=$(ps -eo comm,%cpu --sort=-%cpu | awk 'NR==2{printf "%s %.0f%%", $1, $2}')
mem_pct=$(free | awk '/^Mem:/{printf "%d", $3/$2*100}')
set -- $(free -h | awk '/^Mem:/{print $2, $3, $7}')
total=$1 used=$2

cpu_threshold=$(( CPU_WARN * 85 / 100 ))
if   [ "$cpu" -ge "$CPU_WARN" ];      then cpu_col="${RED}${cpu}%${RST}"
elif [ "$cpu" -ge "$cpu_threshold" ]; then cpu_col="${YEL}${cpu}%${RST}"
else                                       cpu_col="${GRN}${cpu}%${RST}"
fi

mem_threshold=$(( MEM_WARN * 85 / 100 ))
if   [ "$mem_pct" -ge "$MEM_WARN" ];      then mem_col="${RED}${mem_pct}%${RST}"
elif [ "$mem_pct" -ge "$mem_threshold" ]; then mem_col="${YEL}${mem_pct}%${RST}"
else                                           mem_col="${GRN}${mem_pct}%${RST}"
fi

disk_threshold=$(( DISK_WARN * 85 / 100 ))
disks=""
while read -r pct mount; do
    if   [ "$pct" -ge "$DISK_WARN" ];      then disks="${disks} ${mount} ${RED}${pct}%${RST}"
    elif [ "$pct" -ge "$disk_threshold" ]; then disks="${disks} ${mount} ${YEL}${pct}%${RST}"
    else                                        disks="${disks} ${mount} ${GRN}${pct}%${RST}"
    fi
done << EOF
$(df -Ph | awk 'NR>1 && $1~/^\/dev/ && $6!~/^\/run/{sub(/%/,"",$5); print $5, $6}')
EOF

printf '%b\n' "${BLD}${BLU}$(cat /sys/class/dmi/id/product_name 2>/dev/null || hostname)${RST} ${BLD}${GRN}$(id -un)@$(hostname)${RST} ${DIM}$(date '+%Y-%m-%d %H:%M:%S') up $(uptime | awk -F'up |,' 'NR==1{gsub(/^ +| +$/,"",$2); print $2}')${RST}"
printf '%b\n' "CPU ${cpu_col} ${DIM}[${top_proc}]${RST} MEM ${mem_col}"
printf '%b\n' "DISK${disks}"
