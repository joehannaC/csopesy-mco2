#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <thread>
#include <stdexcept>
#include "Instruction.h"
#include "globals.h"
#include <atomic>


enum class ProcessState {
    READY,
    RUNNING,
    FINISHED
};

class Process {
private:
    int memorySize;
    int memoryUsed = 0;
    int peakMemoryUsed = 0;
    int id;
    std::string name;
    std::vector<Instruction> instructions;
    int currentInstructionIndex = 0;
    int totalLinesOfCode = 0;
    int pagedInCount = 0;
    int pagedOutCount = 0;
    int currentCore = -1; 
    

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::ostringstream timestamp;
        timestamp << "[" << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") << "]";
        return timestamp.str();
    }

    void calculateTotalLines() {
        totalLinesOfCode = 0;
        for (const auto& instr : instructions) {
            if (instr.type == Instruction::Type::FOR) {
                try {
                    int loopCount = std::stoi(instr.parameters);
                    totalLinesOfCode += loopCount;
                } catch (...) { totalLinesOfCode++; }
            } else totalLinesOfCode++;
        }
    }

    void updatePeakMemory() {
        if (memoryUsed > peakMemoryUsed) peakMemoryUsed = memoryUsed;
    }

public:
    void setCurrentCore(int core) { currentCore = core; }
    int getCurrentCore() const { return currentCore; }

    std::vector<std::string> logs;
    ProcessState state;
    std::unordered_map<std::string, int> symbolTable;
    bool isScreened = false;

    void markScreened() { isScreened = true; }
    const std::vector<std::string>& getLogs() const { return logs; }

    void incrementPagedIn() { pagedInCount++; }
    void incrementPagedOut() { pagedOutCount++; }
    int getPagedIn() const { return pagedInCount; }
    int getPagedOut() const { return pagedOutCount; }

    int getMemoryUsed() const { return memoryUsed; }
    double getMemoryUsedMiB() const { return static_cast<double>(memoryUsed) / 1024.0; }
    double getPeakMemoryUsedMiB() const { return static_cast<double>(peakMemoryUsed) / 1024.0; }

    void allocateMemory(int kb) {
        memoryUsed += kb;
        if (memoryUsed > memorySize) memoryUsed = memorySize;
        updatePeakMemory();
    }

    void freeMemory(int kb) {
        memoryUsed -= kb;
        if (memoryUsed < 0) memoryUsed = 0;
    }

    
    Process(int pid, const std::string& procName, const std::vector<Instruction>& instrs, int memSize = 64)
        : id(pid), name(procName), instructions(instrs), memorySize(memSize), state(ProcessState::READY)
    {
        if (memSize < 64 || memSize > 216 || (memSize & (memSize - 1)) != 0) {
            throw std::runtime_error("Invalid memory allocation for process '" + name + "'. Must be power of 2 and 64-216 KiB.");
        }
        calculateTotalLines();
        logs.push_back(getCurrentTimestamp() + " Process created with memory " + std::to_string(memorySize) + " KiB.");
    }

    Process(int pid, const std::string& procName, int memSize, const std::string& instructionsStr)
        : id(pid), name(procName), memorySize(memSize), state(ProcessState::READY)
    {
        if (memSize < 64 || memSize > 216 || (memSize & (memSize - 1)) != 0) {
            throw std::runtime_error("Invalid memory allocation for process '" + name + "'. Must be power of 2 and 64-216 KiB.");
        }

        std::stringstream ss(instructionsStr);
        std::string instr;
        while (std::getline(ss, instr, ';')) {
            instr = Instruction::trim(instr);
            if (instr.empty()) continue;
            instructions.push_back(Instruction::fromString(instr));
        }

        if (instructions.empty() || instructions.size() > 50) {
            throw std::runtime_error("Instruction count must be between 1 and 50 for process '" + name + "'.");
        }

        calculateTotalLines();
        logs.push_back(getCurrentTimestamp() + " Process created with memory " + std::to_string(memorySize) + " KiB.");
    }

    
    int getPid() const { return id; }
    std::string getProcessName() const { return name; }
    ProcessState getState() const { return state; }
    void setState(ProcessState newState) { state = newState; }
    int getCurrentLine() const { return currentInstructionIndex; }
    int getLineCount() const { return instructions.size(); }

    
    void executeNextInstruction(int coreId) {
        extern std::atomic<bool> cpuRunning;

        if (!cpuRunning) {
            state = ProcessState::FINISHED;
            logs.push_back(getCurrentTimestamp() + " Execution stopped due to scheduler stop.");
            return;
        }

        if (currentInstructionIndex >= instructions.size()) {
            state = ProcessState::FINISHED;
            return;
        }

        Instruction& instr = instructions[currentInstructionIndex];

        std::string logEntry = getCurrentTimestamp() +
            " Core [" + std::to_string(coreId) + "] \"" + instr.parameters + "\" from " + name;
        logs.push_back(logEntry);

        if (instr.type == Instruction::Type::WRITE || instr.type == Instruction::Type::DECLARE) {
            allocateMemory(1);
        }

        instr.execute(this);
        currentInstructionIndex++;

        if (currentInstructionIndex >= instructions.size())
            state = ProcessState::FINISHED;
    }

    void run(int coreId = 0, int delayMs = 10) {
        state = ProcessState::RUNNING;
        while (currentInstructionIndex < instructions.size() && cpuRunning) {
            executeNextInstruction(coreId);
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
        if (!cpuRunning) {
            logs.push_back(getCurrentTimestamp() + " Execution halted due to scheduler stop.");
        }
        state = ProcessState::FINISHED;
    }
};
