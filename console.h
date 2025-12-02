#pragma once
#include <iostream>
#include <string>
#include "process_list.h"

class console {
private:
    ProcessList &processList;
    Process *proc;
    int totalLines;

public:
    console(ProcessList &plist, Process *p = nullptr);
    void handleScreen();
    void handleScreenLS();
    void reportUtil();
};
