#pragma once
#include <unordered_map>
#include <vector>
#include <deque>
#include <string>
#include <mutex>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include "Process.h"

struct MemoryPage {
    std::string processName;
    uint16_t address;
    uint16_t value;
};

class MemoryManager {
private:
    bool loggingEnabled = true;
    size_t maxFrames;
    std::deque<MemoryPage> frames; // FIFO
    std::unordered_map<std::string, std::unordered_map<uint16_t, uint16_t>> backingStore;
    mutable std::mutex mtx;

public:
    MemoryManager(size_t frames_ = 64) : maxFrames(frames_) {}

    bool canAllocate(const std::string &procName, int kb) {
        std::lock_guard<std::mutex> lock(mtx);
        return (frames.size() + kb) <= maxFrames;
    }

    void disableLogging() {
        std::lock_guard<std::mutex> lock(mtx);
        loggingEnabled = false;
    }

    void enableLogging() {
        std::lock_guard<std::mutex> lock(mtx);
        loggingEnabled = true;
    }

    void write(Process* proc, uint16_t addr, uint16_t value) {
        std::lock_guard<std::mutex> lock(mtx);
        MemoryPage* page = findPage(proc->getProcessName(), addr);
        if (!page) {
            pageFault(proc, addr, true);
            page = findPage(proc->getProcessName(), addr);
            if (!page) throw std::runtime_error("Memory write failed after page fault");
        }
        page->value = value;
    }

    uint16_t read(Process* proc, uint16_t addr) {
        std::lock_guard<std::mutex> lock(mtx);
        MemoryPage* page = findPage(proc->getProcessName(), addr);
        if (!page) {
            pageFault(proc, addr, false);
            page = findPage(proc->getProcessName(), addr);
            if (!page) throw std::runtime_error("Memory read failed after page fault");
        }
        return page->value;
    }

    size_t getUsedFrames() const {
        std::lock_guard<std::mutex> lock(mtx);
        return frames.size();
    }

    bool validAddress(uint16_t addr) const {
        return addr <= 0xFFFF;
    }

    void dumpBackingStore(const std::string &filename = "backing_store.txt") {
        std::lock_guard<std::mutex> lock(mtx);
        std::ofstream ofs(filename);
        for (auto &procPair : backingStore) {
            ofs << "Process: " << procPair.first << "\n";
            for (auto &addrVal : procPair.second)
                ofs << "  0x" << std::hex << addrVal.first << ": " << std::dec << addrVal.second << "\n";
        }
        ofs.close();
    }

private:
    MemoryPage* findPage(const std::string &procName, uint16_t addr) {
        for (auto &f : frames) {
            if (f.processName == procName && f.address == addr)
                return &f;
        }
        return nullptr;
    }

    void pageFault(Process* proc, uint16_t addr, bool isWrite) {
    if (loggingEnabled) {
        std::cout << "[MEM] Page fault: " << proc->getProcessName()
                  << " accessing 0x" << std::hex << addr
                  << (isWrite ? " for write" : " for read") << std::dec << "\n";
    }

    
    if (isWrite) proc->incrementPagedOut();
    else proc->incrementPagedIn();

    uint16_t value = 0;
    if (backingStore[proc->getProcessName()].count(addr))
        value = backingStore[proc->getProcessName()][addr];

    if (frames.size() >= maxFrames) {
        MemoryPage victim = frames.front();
        frames.pop_front();
        backingStore[victim.processName][victim.address] = victim.value;

        if (loggingEnabled) {
            std::cout << "[MEM] Evicting page: " << victim.processName
                      << " addr 0x" << std::hex << victim.address << std::dec << "\n";
        }
    }

    frames.push_back({proc->getProcessName(), addr, value});
}

};

