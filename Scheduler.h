#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <condition_variable>
#include "Process.h"
#include "Instruction.h"
#include "process_list.h"
#include "globals.h"
#include "MemoryManager.h"
extern MemoryManager memmgr;



class Scheduler {
public:
    int numCPUs = 1;
    std::string schedulerType = "FCFS";
    int quantumCycles = 5;
    int batchProcessFreq = 1000;  // milliseconds
    int minInstructions = 3;
    int maxInstructions = 10;
    int delaysPerExec = 0;        // milliseconds

    //std::atomic<bool> generatorRunning { false };
    //std::atomic<bool> cpuRunning { false };

    ProcessList allProcesses;

private:
    //std::atomic<bool> cpuRunning { false };
    //std::atomic<bool> generatorRunning { false };
    std::vector<std::thread> cpuCores;
    std::thread generatorThread;
    std::mutex mtx;
    std::condition_variable cv;  // for immediate generator stop
    int processCounter = 1;

public:
    Scheduler() {}
    ~Scheduler() { stop(); }

    void schedulerStart() {
        if (!cpuRunning) {
            cpuRunning = true;

            for (int i = 0; i < numCPUs; ++i) {
                cpuCores.emplace_back([this, i]() {
                    while (cpuRunning) {
                        Process* proc = nullptr;

                        if (schedulerType == "RR" || schedulerType == "rr")
                            proc = getNextProcessRR();
                        else
                            proc = getNextProcessFCFS();

                        if (proc) {
                            proc->setCurrentCore(i);      // assign core first
                            while (proc->getState() == ProcessState::RUNNING && cpuRunning) {
                                proc->executeNextInstruction(i);
                                activeTicks++;

                                std::cout << "[Core " << i << "] Process " << proc->getProcessName()
                                        << " executing instruction " << proc->getCurrentLine() + 1
                                        << "/" << proc->getLineCount()
                                        << " | State: RUNNING\n";

                                if (delaysPerExec > 0)
                                    std::this_thread::sleep_for(std::chrono::milliseconds(delaysPerExec));
                            }

                            if (proc->getState() != ProcessState::FINISHED && cpuRunning) {
                                std::lock_guard<std::mutex> lock(mtx);
                                proc->setState(ProcessState::READY);
                                std::cout << "[Core " << i << "] Process " << proc->getProcessName() 
                                        << " set back to READY.\n";
                            } else {
                                std::cout << "[Core " << i << "] Process " << proc->getProcessName() 
                                        << " finished execution.\n";
                            }
                        } else {
                            idleTicks++;
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                    }
                });
            }


            std::cout << "CPU threads started.\n";
        }

        if (!generatorRunning) {
            generatorRunning = true;
            generatorThread = std::thread([this]() {
                std::unique_lock<std::mutex> lock(mtx);
                while (generatorRunning) {
                    generateRandomProcess();
                    cv.wait_for(lock, std::chrono::milliseconds(batchProcessFreq), [this]() {
                        return !generatorRunning.load();
                    });
                }
            });
            std::cout << "Process generator started.\n";
        }
    }

    void schedulerTest(int fastFreqMs = 100) {
        batchProcessFreq = fastFreqMs;
        delaysPerExec = 300;
        schedulerStart();
    }

void schedulerStop() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        generatorRunning = false;
    }
    memmgr.disableLogging();

    cv.notify_all();
    if (generatorThread.joinable()) generatorThread.join();
}

void stop() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        generatorRunning = false;
        cpuRunning = false;
    }

    cv.notify_all();

    if (generatorThread.joinable()) generatorThread.join();

    
    for (auto& t : cpuCores) {
        if (t.joinable()) t.join();
    }
    cpuCores.clear();

    std::cout << "Scheduler fully stopped.\n";
}


private:
    Process* getNextProcessRR() {
        std::lock_guard<std::mutex> lock(mtx);
        static size_t rr_index = 0;
        auto& procs = allProcesses.getAllProcesses();
        if (procs.empty()) return nullptr;

        for (size_t i = 0; i < procs.size(); ++i) {
            size_t idx = (rr_index + i) % procs.size();
            if (procs[idx]->getState() == ProcessState::READY) {
                procs[idx]->setState(ProcessState::RUNNING);
                rr_index = (idx + 1) % procs.size();
                return procs[idx].get();
            }
        }
        return nullptr;
    }

    Process* getNextProcessFCFS() {
        std::lock_guard<std::mutex> lock(mtx);
        auto& procs = allProcesses.getAllProcesses();
        for (auto& p : procs) {
            if (p->getState() == ProcessState::READY) {
                p->setState(ProcessState::RUNNING);
                return p.get();
            }
        }
        return nullptr;
    }

    void generateRandomProcess() {
        int numInstr = minInstructions + (std::rand() % (maxInstructions - minInstructions + 1));
        if (numInstr < 3) numInstr = 3;

        std::vector<Instruction> instrs;
        std::vector<std::string> vars;

        for (int i = 0; i < numInstr / 3; ++i) {
            std::string varName = "x" + std::to_string(i);
            instrs.push_back(Instruction(Instruction::Type::DECLARE, varName + " 0"));
            vars.push_back(varName);
        }

        for (int i = 0; i < numInstr / 3; ++i) {
            if (vars.size() >= 2) {
                std::string cmd = vars[0] + " " + vars[0] + " " + vars[1];
                instrs.push_back(Instruction(Instruction::Type::ADD, cmd));
                instrs.push_back(Instruction(Instruction::Type::SUB, cmd));
            } else if (!vars.empty()) {
                std::string cmd = vars[0] + " " + vars[0] + " 1";
                instrs.push_back(Instruction(Instruction::Type::ADD, cmd));
            }
        }

        for (int i = 0; i < numInstr / 6; ++i) {
            if (!vars.empty()) {
                std::string var = vars[i % vars.size()];
                std::ostringstream addr;
                addr << "0x" << std::hex << (0x500 + i * 2);
                instrs.push_back(Instruction(Instruction::Type::WRITE, addr.str() + " " + var));
                std::string readVar = "r" + std::to_string(i);
                instrs.push_back(Instruction(Instruction::Type::READ, readVar + " " + addr.str()));
                vars.push_back(readVar);
            }
        }

        for (auto& v : vars) {
            instrs.push_back(Instruction(Instruction::Type::PRINT, v));
        }

        std::ostringstream oss;
        oss << "p" << std::setw(2) << std::setfill('0') << processCounter;
        std::string procName = oss.str();

        Process newProc(processCounter, procName, instrs);
        newProc.setState(ProcessState::READY);
        allProcesses.addProcess(newProc);

        std::cout << "Generated process: " << procName << " with " << instrs.size() << " instructions.\n";
        processCounter++;
    }
};
