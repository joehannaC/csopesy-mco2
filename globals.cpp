#include "MemoryManager.h"
#include "process_list.h"

MemoryManager memmgr;

int processCounter = 1;

std::atomic<bool> cpuRunning{false};
std::atomic<bool> generatorRunning{false};
std::atomic<long> idleTicks{0};
std::atomic<long> activeTicks{0};
