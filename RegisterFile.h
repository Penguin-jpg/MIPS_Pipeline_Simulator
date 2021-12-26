#ifndef REGISTERFILE_H
#define REGISTERFILE_H

#include <vector>
using namespace std;

// register file
struct RegisterFile
{
    vector<int> registers;

    RegisterFile()
    {
        registers.resize(32); // 32個暫存器
        registers[0] = 0;     // 0號暫存器值為0

        // 其他暫存器初始值為1
        for (int i = 1; i < 32; i++)
        {
            registers[i] = 1;
        }
    }
};

#endif