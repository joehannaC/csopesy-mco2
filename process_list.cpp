#include "process_list.h"
#include <stdexcept>
#include <thread>

void ProcessList::addProcess(const Process& p) {
    for (const auto& existing : processes) {
        if (existing->getProcessName() == p.getProcessName()) {
            std::cerr << "Warning: Process with name '" << p.getProcessName() << "' already exists.\n";
            return;
        }
    }

    auto newProc = std::make_shared<Process>(p);
    processes.push_back(newProc);
    std::thread([newProc]() { newProc->run(); }).detach();
}

std::shared_ptr<Process> ProcessList::createProcess(const std::string& name, int memorySize, const std::string& instructionsStr) {
    // Check for duplicate
    for (const auto& existing : processes) {
        if (existing->getProcessName() == name) {
            throw std::runtime_error("Process with name '" + name + "' already exists.");
        }
    }

    int id = processCounter++;
    auto newProc = std::make_shared<Process>(id, name, memorySize, instructionsStr);
    processes.push_back(newProc);

    std::thread([newProc]() { newProc->run(); }).detach();

    std::cout << "Added process " << name << " with ID " << id << " and memory " << memorySize << "\n";

    return newProc;
}

std::shared_ptr<Process> ProcessList::findProcess(const std::string& name) {
    for (auto& p : processes) {
        if (p->getProcessName() == name)
            return p;
    }
    throw std::runtime_error("Process not found: " + name);
}

std::shared_ptr<Process> ProcessList::findProcessByPid(int pid) {
    for (auto& p : processes) {
        if (p->getPid() == pid)
            return p;
    }
    throw std::runtime_error("Process not found with PID: " + std::to_string(pid));
}

void ProcessList::displayAll() const {
    if (processes.empty()) {
        std::cout << "No processes available.\n";
        return;
    }

    //std::cout << "\n=== Process List ===\n";
    for (const auto &p : processes) {
        std::string stateStr;
        switch (p->getState()) {
            case ProcessState::READY:    stateStr = "Ready"; break;
            case ProcessState::RUNNING:  stateStr = "Running"; break;
            case ProcessState::FINISHED: stateStr = "Finished"; break;
            default:                      stateStr = "Unknown"; break;
        }

        double memMiB = static_cast<double>(p->getMemoryUsed()) / 1024.0; // KiB -> MiB
        if (memMiB < 0.01) memMiB = 0.01;

        /*std::cout << "Process Name: " << p->getProcessName()
                  << " | PID: " << p->getPid()
                  << " | State: " << stateStr
                  << " | Instruction: " << p->getCurrentLine() << "/" << p->getLineCount()
                  << " | Memory: " << memMiB << " MiB"
                  << std::endl;*/
    }
    //std::cout << "====================\n";
}
