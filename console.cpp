#include "console.h"
#include "MemoryManager.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <chrono>

extern int numCPUs;
extern MemoryManager memmgr;

console::console(ProcessList &plist, Process *p)
    : processList(plist), proc(p), totalLines(100) {}

void console::handleScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    if (!proc) return;

    std::cout << "=== Screen: " << proc->getProcessName() << " ===\n";

    std::string input;
    while (true) {
        std::cout << "\nroot:/>";
        std::getline(std::cin, input);

        if (input == "exit") {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
            break;
        }
        else if (input == "process-smi") {
            auto latestProc = processList.findProcess(proc->getProcessName());

            std::cout << "\nProcess name: " << latestProc->getProcessName()
                      << "\nPID: " << latestProc->getPid()
                      << "\nLogs:\n";

            for (const auto &entry : latestProc->getLogs())
                std::cout << entry << "\n";

            std::cout << "Current instruction line: " << latestProc->getCurrentLine()
                      << "\nLines of code: " << latestProc->getLineCount() << "\n";

            if (latestProc->getState() == ProcessState::FINISHED)
                std::cout << "\nFinished!\n";
        }
        else {
            std::cout << "Unknown command. Supported commands: 'exit', 'process-smi'\n";
        }
    }
}

void console::handleScreenLS() {
    std::cout << "--------------------------------\n";

    const auto& allProcs = processList.getAllProcesses();

    // Calculate CPU utilization
    int runningProcesses = 0;
    for (const auto& p : allProcs) {
        if (p->getState() == ProcessState::RUNNING)
            runningProcesses++;
    }

    double cpuUtil = allProcs.empty() ? 0.0 : (runningProcesses * 100.0) / allProcs.size();
    std::cout << "CPU Utilization: " << std::fixed << std::setprecision(2)
              << cpuUtil << " %\n";

    // Running processes
    std::cout << "\nRunning processes:\n";
    for (const auto &p : allProcs) {
        if (p->getState() == ProcessState::RUNNING) {
            // Lock or atomically read the core if needed
            int core = p->getCurrentCore();
            std::cout << p->getProcessName()
                      << " | State: RUNNING"
                      << " | Core: " << (core == -1 ? "Unassigned" : std::to_string(core))
                      << " | Line: " << p->getCurrentLine()
                      << "/" << p->getLineCount()
                      << "\n";
        }
    }

    // Finished processes
    std::cout << "\nFinished processes:\n";
    for (const auto &p : allProcs) {
        if (p->getState() == ProcessState::FINISHED) {
            std::cout << p->getProcessName()
                      << " | State: FINISHED"
                      << " | Total lines: " << p->getLineCount()
                      << "/" << p->getLineCount()
                      << "\n";
        }
    }

    std::cout << "--------------------------------\n";
}





void console::reportUtil() {
    std::ofstream logFile("csopesy-log.txt", std::ios::app);
    if (!logFile.is_open()) { std::cerr << "Failed to open log file.\n"; return; }

    int running = 0;
    int finished = 0;

    for (const auto &p : processList.getAllProcesses()) {
        if (p->getState() == ProcessState::RUNNING) running++;
        else finished++;
    }

    int totalMem = 64;
    int usedMem = memmgr.getUsedFrames();
    std::cout << "Memory Usage: " << usedMem << " / " << totalMem << " frames\n";

    logFile << "=== Utilization Report ===\n";
    logFile << "Total CPU cores: " << numCPUs << "\n";
    logFile << "Processes summary:\n";

    int coreIndex = 0;
    for (const auto &p : processList.getAllProcesses()) {
        int assignedCore = coreIndex % numCPUs;
        logFile << "Process: " << p->getProcessName()
                << ", PID: " << p->getPid()
                << ", State: " << (p->getState() == ProcessState::RUNNING ? "RUNNING" : "FINISHED")
                << ", Current line: " << p->getCurrentLine() << "/" << p->getLineCount()
                << ", Core: " << assignedCore
                << "\n";
        coreIndex++;
    }

    logFile << "=== End of Report ===\n\n";
    logFile.close();
    std::cout << "Report saved to csopesy-log.txt\n";
}
