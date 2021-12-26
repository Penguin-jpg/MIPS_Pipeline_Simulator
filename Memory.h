#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <string>
using namespace std;

struct Memory
{
    vector<int> data;
    vector<vector<string>> instructions; // 指令

    Memory()
    {
        data.resize(32);           // 32個word
        instructions.resize(1000); // 預設1000個指令

        // 每個word初始值為1
        for (int i = 0; i < 32; i++)
        {
            data[i] = 1;
        }
    }
};

#endif