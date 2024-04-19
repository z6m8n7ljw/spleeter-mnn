#!/bin/bash

# 定义 adb 的路径
adb_path="/Users/jerry/Library/Android/sdk/platform-tools/adb"

# 每 0.2 秒打印 CPU 和 GPU 使用率
while true; do
    pid=$($adb_path shell ps | grep com.bilibili.audioseparation | awk '{print $2}')
    
    # 如果 PID 存在
    if [ -n "$pid" ]; then
        cpu_usage=$($adb_path shell top -n 1 | grep $pid | awk '{print $9}')
        gpu_usage=$($adb_path shell cat /sys/class/kgsl/kgsl-3d0/gpubusy | awk ' {print $1 / ($2+1) * 100}')
        echo "CPU Usage: $cpu_usage %  GPU Usage: $gpu_usage %"
    fi
    
    sleep 0.2
done

