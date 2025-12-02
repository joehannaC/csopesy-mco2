#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cctype>

class Process;

class Instruction {
public:
    enum class Type { DECLARE, ADD, SUB, READ, WRITE, PRINT, SLEEP, FOR, UNKNOWN };
    Type type;
    std::string parameters;

    Instruction() : type(Type::UNKNOWN) {}
    Instruction(Type t, const std::string& params) : type(t), parameters(params) {}
    Instruction(const std::string& instrStr) { *this = fromString(instrStr); }

    void execute(Process* process);

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, last - first + 1);
    }

    static Instruction fromString(const std::string& instrStr) {
        std::istringstream iss(instrStr);
        std::string cmd;
        iss >> cmd;
        std::string params;
        std::getline(iss, params);
        params = trim(params);

        if (cmd == "DECLARE") return Instruction(Type::DECLARE, params);
        if (cmd == "ADD") return Instruction(Type::ADD, params);
        if (cmd == "SUB") return Instruction(Type::SUB, params);
        if (cmd == "READ") return Instruction(Type::READ, params);
        if (cmd == "WRITE") return Instruction(Type::WRITE, params);
        if (cmd == "PRINT") return Instruction(Type::PRINT, params);
        if (cmd == "SLEEP") return Instruction(Type::SLEEP, params);
        if (cmd == "FOR") return Instruction(Type::FOR, params);

        throw std::runtime_error("Unknown instruction: " + cmd);
    }
};
