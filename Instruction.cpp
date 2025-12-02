#include "Instruction.h"
#include "MemoryManager.h"
#include "Process.h"  
#include <sstream>
#include <thread>
#include <iostream>
#include <algorithm>
#include <cctype>

extern MemoryManager memmgr;

// Helper trim function
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

void Instruction::execute(Process* process) {
    extern std::atomic<long> activeTicks;
    activeTicks++; 
    std::string params = trim(parameters);

    try {
        switch (type) {

        case Type::PRINT: {
            std::string output = params;

            
            for (auto& [var, val] : process->symbolTable) {
                size_t pos = 0;
                while ((pos = output.find(var, pos)) != std::string::npos) {
                    bool leftOK = (pos == 0 || !isalnum(output[pos - 1]));
                    bool rightOK = (pos + var.size() >= output.size() || !isalnum(output[pos + var.size()]));
                    if (leftOK && rightOK) {
                        output.replace(pos, var.length(), std::to_string(val));
                        pos += std::to_string(val).length();
                    } else {
                        pos += var.length();
                    }
                }
            }

            process->logs.push_back("PRINT: " + output);
            break;
        }

        case Type::DECLARE: {
            if (process->symbolTable.size() >= 32) {
                process->logs.push_back("Symbol table full. DECLARE ignored.");
                break;
            }

            std::istringstream iss(params);
            std::string name;
            int value = 0;
            iss >> name >> value;
            process->symbolTable[name] = std::clamp(value, 0, 65535);
            break;
        }

        case Type::ADD:
        case Type::SUB: {
            std::istringstream iss(params);
            std::string dst, src1, src2;
            iss >> dst >> src1 >> src2;

            if (!process->symbolTable.count(src1) || !process->symbolTable.count(src2))
                throw std::runtime_error("Undefined variable in ADD/SUB");

            int val1 = process->symbolTable[src1];
            int val2 = process->symbolTable[src2];
            int result = (type == Type::ADD) ? val1 + val2 : val1 - val2;
            result = std::clamp(result, 0, 65535);

            process->symbolTable[dst] = result;
            break;
        }

        case Type::READ: {
            std::istringstream iss(params);
            std::string var, addrStr;
            iss >> var >> addrStr;

            uint16_t addr = std::stoul(trim(addrStr), nullptr, 16);
            if (!memmgr.validAddress(addr))
                throw std::runtime_error("Invalid memory READ address");

            uint16_t val = memmgr.read(process, addr);
            process->symbolTable[var] = val;

            // Track paging
            process->incrementPagedIn();
            break;
        }

        case Type::WRITE: {
            std::istringstream iss(params);
            std::string addrStr, valueStr;
            iss >> addrStr >> valueStr;

            uint16_t addr = std::stoul(trim(addrStr), nullptr, 16);
            if (!memmgr.validAddress(addr))
                throw std::runtime_error("Invalid memory WRITE address");

            uint16_t value = 0;
            valueStr = trim(valueStr);
            if (process->symbolTable.count(valueStr))
                value = process->symbolTable[valueStr];
            else
                value = std::clamp(std::stoi(valueStr), 0, 65535);

            memmgr.write(process,addr, value);

            // Track paging
            process->incrementPagedOut();
            break;
        }


        case Type::SLEEP:
            std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(params)));
            break;

        case Type::FOR:
            
            break;

        default:
            break;
        }
    } catch (std::exception& e) {
        process->logs.push_back(std::string("Error: ") + e.what() + " at: " + parameters);
        process->state = ProcessState::FINISHED;
    } catch (...) {
        process->logs.push_back("Unknown error at: " + parameters);
        process->state = ProcessState::FINISHED;
    }
}

