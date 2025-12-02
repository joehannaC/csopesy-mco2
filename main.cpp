#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <memory>
#include "Process.h"
#include "Instruction.h"
#include "process_list.h"
#include "console.h"
#include "Scheduler.h"
#include <stdexcept>
#include <iomanip>

int numCPUs = 1;
std::string schedulerType = "FCFS";
int quantumCycles = 100;
int batchProcessFreq = 1;
int minInstructions = 1;
int maxInstructions = 10;
int delaysPerExec = 0;
int maxOverallMem = 65536; // default total system memory
int memPerFrame = 256;      // default per-frame memory
int minMemPerProc = 64;     // default min memory per process
int maxMemPerProc = 4096;   // default max memory per process

Scheduler sched;

void setConfig() {
    std::ifstream configFile("config.txt");
    if (configFile.is_open()) {
        std::string line;
        while (std::getline(configFile, line)) {
            size_t delim = line.find(' ');
            if (delim == std::string::npos) continue;
            std::string key = line.substr(0, delim);
            std::string value = line.substr(delim + 1);

            if (key == "num-cpu") numCPUs = std::stoi(value);
            else if (key == "scheduler") schedulerType = value.substr(1, value.size()-2); // remove quotes
            else if (key == "quantum-cycles") quantumCycles = std::stoi(value);
            else if (key == "batch-process-freq") batchProcessFreq = std::stoi(value);
            else if (key == "min-ins") minInstructions = std::stoi(value);
            else if (key == "max-ins") maxInstructions = std::stoi(value);
            else if (key == "delay-per-exec") delaysPerExec = std::stoi(value);
            else if (key == "max-overall-mem") maxOverallMem = std::stoi(value);
            else if (key == "mem-per-frame") memPerFrame = std::stoi(value);
            else if (key == "min-mem-per-proc") minMemPerProc = std::stoi(value);
            else if (key == "max-mem-per-proc") maxMemPerProc = std::stoi(value);
        }
        configFile.close();
        std::cout << "Configuration loaded.\n";
    }

    sched.numCPUs = numCPUs;
    sched.schedulerType = schedulerType;
    sched.quantumCycles = quantumCycles;
    sched.batchProcessFreq = batchProcessFreq;
    sched.minInstructions = minInstructions;
    sched.maxInstructions = maxInstructions;
    sched.delaysPerExec = delaysPerExec;
}


void processSMI(const ProcessList& plist, int totalMemoryKiB) {
    int usedMemory = 0;
    int runningProcesses = 0;

    const auto& allProcs = plist.getAllProcesses();
    for (const auto& pPtr : allProcs) {
        const Process& p = *pPtr;
        usedMemory += p.getMemoryUsed();  // memory in KiB
        if (p.getState() == ProcessState::RUNNING)
            runningProcesses++;
    }

    double cpuUtil = 0.0;
    if (!allProcs.empty())
        cpuUtil = (runningProcesses * 100.0) / allProcs.size();

    std::cout << "----------------------------------------------\n";
    std::cout << "PROCESS-SMI V01.00 DRIVER VERSION: \n";
    std::cout << "CPU Utilization: " << cpuUtil << " %\n";
    std::cout << "Memory Usage: " << usedMemory / 1024.0 << " MiB / "
              << totalMemoryKiB / 1024.0 << " MiB\n";
    std::cout << "Memory Utilization: " << (usedMemory * 100.0 / totalMemoryKiB) << " %\n\n";

    std::cout << "Running processes and memory usage:\n";
    for (const auto& pPtr : allProcs) {
        const Process& p = *pPtr;
        double memMiB = static_cast<double>(p.getMemoryUsed()) / 1024.0;
        if (memMiB < 0.01) memMiB = 0.01; // optional minimum display
        std::cout << p.getProcessName() << " " << memMiB << " MiB"
                  << " | State: " << (p.getState() == ProcessState::RUNNING ? "RUNNING" :
                                      p.getState() == ProcessState::READY ? "READY" : "FINISHED")
                  << "\n";
    }

    std::cout << "----------------------------------------------\n";
}




void vmStat(const ProcessList& plist, int totalMemoryBytes, const std::string& filename = "csopesy-vmstat.txt") {
    int usedMemory = 0;
    int pagedIn = 0;
    int pagedOut = 0;

    for (const auto& p : plist.getAllProcesses()) {
        usedMemory += p->getMemoryUsed();
        pagedIn += p->getPagedIn();
        pagedOut += p->getPagedOut();
    }

    int freeMemory = totalMemoryBytes - usedMemory;
    long totalCpuTicks = activeTicks.load() + idleTicks.load();
    double cpuUtil = (totalCpuTicks > 0) ? 100.0 * activeTicks.load() / totalCpuTicks : 0.0;

    std::ofstream ofs(filename);
    if (!ofs.is_open()) throw std::runtime_error("Unable to open file: " + filename);

    ofs << "--------------------------------\n";
    ofs << "Total memory: " << totalMemoryBytes << " bytes\n";
    ofs << "Used memory: " << usedMemory << " bytes (" << std::fixed << std::setprecision(4) << usedMemory / 1024.0 / 1024.0 << " MiB)\n";
    ofs << "Free memory: " << freeMemory << " bytes (" << freeMemory / 1024.0 / 1024.0 << " MiB)\n";
    ofs << "Idle CPU ticks: " << idleTicks.load() << "\n";
    ofs << "Active CPU ticks: " << activeTicks.load() << "\n";
    ofs << "Total CPU ticks: " << totalCpuTicks << "\n";
    ofs << "CPU Utilization: " << cpuUtil << " %\n";
    ofs << "Num paged in: " << pagedIn << "\n";
    ofs << "Num paged out: " << pagedOut << "\n";
    ofs << "--------------------------------\n";

    ofs.close();
}


inline std::string trim(const std::string& s) {
                size_t start = s.find_first_not_of(" \t");
                size_t end = s.find_last_not_of(" \t");
                if (start == std::string::npos) return "";
                return s.substr(start, end - start + 1);
}


void printHeader() {
    std::cout << " ------- ------- ------- ------- ------- ------- -------\n";
    std::cout << "THE CSOPESY EMULATOR \n";
    std::cout << " Developers:\n";
    std::cout << "  - Cansino, Joehanna L.\n";
    std::cout << "  - De Guzman, Amor T.\n";
    std::cout << "  - Maloles, Luis Gabriel G.\n";
    std::cout << "  - San Buenaventure, Carlo L.\n";
    std::cout << " ------- ------- ------- ------- ------- ------- -------\n";
}

bool isPowerOfTwo(int n) {
    return (n & (n - 1)) == 0;
}

bool validMemorySize(int memSize) {
    return memSize >= 64 && memSize <= 216 && isPowerOfTwo(memSize);
}

int main() {
    printHeader();
    ProcessList plist;
    std::string command;

    while (command != "exit") {
        std::cout << "Enter command: ";
        std::getline(std::cin, command);

        if (command == "initialize") {
            std::cout << "Running initialization of config.txt file.\n";
            setConfig();
        }
        else if (command == "help") {
            std::cout << "Available commands:\n";
            std::cout << " initialize                     - Load configuration\n";
            std::cout << " scheduler-start                - Run scheduler\n";
            std::cout << " screen -s <name> <memory>      - Create new process with memory\n";
            std::cout << " screen -r <name>               - Re-access process screen\n";
            std::cout << " screen -c <name> <mem> \"inst\"   - Create process with instructions\n";
            std::cout << " screen -ls                     - List all processes\n";
            std::cout << " scheduler-stop                 - Stop scheduler\n";
            std::cout << " process-smi                    - summarized view of the available/used memory\n";
            std::cout << " vmstat                         - detailed view of the active/inactive processes, available/used memory, and pages.\n";
            std::cout << " report-util                    - Generate CPU utilization report\n";
            std::cout << " exit                           - Quit program\n";
        }
        else if (command.rfind("screen ", 0) == 0) {
            std::string option = command.substr(7);

            // Case 1: screen -s <existingProcess>
            if (option.rfind("-s ", 0) == 0) {
                std::string rest = option.substr(3);
                std::istringstream iss(rest);

                std::string procName;
                int memSize = -1;

                iss >> procName;

                // If there is NO memory after the name -> open existing process
                if (!(iss >> memSize)) {
                    std::shared_ptr<Process> proc = plist.findProcess(procName);
                    auto existing = plist.findProcess(procName);
                    if (!existing) {
                        std::cout << "Process '" << procName << "' not found.\n";
                        continue;
                    }else if (proc->getState() == ProcessState::FINISHED) {
                        std::cout << "Process " << procName << " not found.\n";
                        continue;
                    }

                    console c(plist, existing.get());
                    c.handleScreen();
                    continue;
                }

                // Case 2: screen -s <new_process> <mem>
                if (!validMemorySize(memSize)) {
                    std::cout << "Invalid memory allocation. Must be between 64-4096 bytes and a power of 2.\n";
                    continue;
                }

                // Read instructions text from user
                std::cout << "Enter instructions (finish with a blank line):\n";
                std::string line, instructionsStr;
                std::getline(std::cin, line);

                while (true) {
                    std::getline(std::cin, line);
                    if (line.empty()) break;
                    instructionsStr += line + "\n";
                }

                auto proc = plist.createProcess(procName, memSize, instructionsStr);

                console c(plist, proc.get());
                c.handleScreen();
            }
        

            else if (option.rfind("-r ", 0) == 0) {
                std::string procName = option.substr(3);
                try {
                    std::shared_ptr<Process> proc = plist.findProcess(procName);
                    if (proc->getState() == ProcessState::FINISHED) {
                        std::cout << "Process " << procName << " not found.\n";
                    } else if (!proc->isScreened) {
                        std::cout << "Process " << procName << " has not been accessed before. Use -s first.\n";
                    } else {
                        console c(plist, proc.get());
                        c.handleScreen();
                    }
                } catch (std::runtime_error &) {
                    std::cout << "Process " << procName << " not found.\n";
                }
            }

            else if (option.rfind("-c ", 0) == 0) {
                std::istringstream iss(option.substr(3));
                std::string procName, instructionsStr;
                int memSize;

                if (!(iss >> procName >> memSize)) {
                    std::cout << "Invalid screen -c command format.\n";
                    continue;
                }

                size_t firstQuote = option.find('"');
                size_t lastQuote = option.rfind('"');
                if (firstQuote == std::string::npos || lastQuote == firstQuote) {
                    std::cout << "Instructions string missing or invalid.\n";
                    continue;
                }

                instructionsStr = option.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                instructionsStr = Instruction::trim(instructionsStr);

                if (!validMemorySize(memSize)) {
                    std::cout << "Invalid memory allocation. Must be 64-216 bytes and power of 2.\n";
                    continue;
                }

                try {
                    auto proc = plist.createProcess(procName, memSize, instructionsStr);
                    proc->markScreened();

                    std::cout << "Process " << procName << " created with PID " << proc->getPid() << ".\n";

                    while (proc->getState() != ProcessState::FINISHED) {
                        proc->executeNextInstruction(0);
                        const auto& logs = proc->getLogs();
                        if (!logs.empty()) std::cout << logs.back() << "\n";
                    }

                    std::cout << "Process finished.\n";
                } catch (const std::exception& e) {
                    std::cout << "Error creating process: " << e.what() << "\n";
                }
            }
            else if (option == "-ls") {
                console c(plist, nullptr);
                c.handleScreenLS();
            } else {
                std::cout << "Unknown screen parameters.\n";
            }
        }
        else if (command == "scheduler-start") {
            sched.allProcesses = plist;
            //sched.start();
            sched.schedulerStart();
        }
        else if (command == "scheduler-stop") {
            sched.schedulerStop();
            std::cout << "Scheduler generator stopped.\n";
            plist = sched.allProcesses;
            //plist.displayAll();
        }
        else if (command == "scheduler-test") {
            sched.allProcesses = plist;
            sched.schedulerTest();
            plist = sched.allProcesses;
        }

        else if (command == "process-smi") {
            processSMI(plist, maxOverallMem);
        }
        else if (command == "vmstat") {
            vmStat(plist, maxOverallMem);
        }
        else if (command == "report-util") {
            console c(plist, nullptr);
            c.reportUtil();
        }
        else if (command == "exit") {
            std::cout << "Exiting program.\n";
        }
        else {
            std::cout << "Unknown command: " << command << "\n";
        }
    }

    return 0;
}
