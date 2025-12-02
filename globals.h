#pragma once
#include <atomic>

extern std::atomic<bool> cpuRunning;
extern std::atomic<bool> generatorRunning;
extern std::atomic<long> idleTicks;
extern std::atomic<long> activeTicks;
