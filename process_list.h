#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <thread>
#include "Process.h"

extern int processCounter; 

class ProcessList {
private:
    std::vector<std::shared_ptr<Process>> processes;

public:

    void addProcess(const Process& p);

    std::shared_ptr<Process> createProcess(const std::string& name, int memorySize, const std::string& instructionsStr);

    std::shared_ptr<Process> findProcess(const std::string& name);

    std::shared_ptr<Process> findProcessByPid(int pid);

    std::vector<std::shared_ptr<Process>>& getAllProcesses() { return processes; }
    const std::vector<std::shared_ptr<Process>>& getAllProcesses() const { return processes; }

    void displayAll() const;

    
};
