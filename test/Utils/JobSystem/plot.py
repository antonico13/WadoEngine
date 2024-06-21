import json

import matplotlib.pyplot as plt
import numpy as np


threadCounts = [20, 40, 80, 160, 320, 640, 800]

wado = open('fiber.json')
wadoData = json.load(wado)

wadoWallClock = []
wadoCpuTime = []
wadoStdDevClock = []
wadoStdDevCpu = []

for i in wadoData['benchmarks']:    
    if not i.get('aggregate_name') is None:
        if i['aggregate_name'] == "mean":
            wadoWallClock.append(i['real_time'])
            wadoCpuTime.append(i['cpu_time'])
        if i['aggregate_name'] == "stddev":
            wadoStdDevClock.append(i['real_time'])
            wadoStdDevCpu.append(i['cpu_time'])

std = open('thread.json')
stdData = json.load(std)

stdWallClock = []
stdCpuTime = []
stdStdDevClock = []
stdStdDevCpu = []

for i in stdData['benchmarks']:    
    if not i.get('aggregate_name') is None:
        if i['aggregate_name'] == "mean":
            stdWallClock.append(i['real_time'])
            stdCpuTime.append(i['cpu_time'])
        if i['aggregate_name'] == "stddev":
            stdStdDevClock.append(i['real_time'])
            stdStdDevCpu.append(i['cpu_time'])

plt.errorbar(threadCounts, stdWallClock, stdStdDevClock, fmt = "x", ecolor="black", elinewidth=0.5, capsize=3, capthick=0.5, label = "C++ std::thread")
plt.errorbar(threadCounts, wadoWallClock, wadoStdDevClock, fmt = "+", ecolor="black", elinewidth=0.5, capsize=3, capthick=0.5, label = "Wado Fibers")
plt.grid(True, which='both', linestyle='--', linewidth=0.5)
plt.xlabel("Initial task count")
plt.ylabel("Mean execution time (ns) over 10 runs")

plt.legend() 
plt.show()