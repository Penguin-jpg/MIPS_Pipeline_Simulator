#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <bitset>
using namespace std;

struct Memory
{
    vector<bitset<32>> memory;

    Memory()
    {
        memory.resize(32); // 32個word

        // 每個word初始值為1
        for (int i = 0; i < 32; i++)
        {
            memory[i] = bitset<32>(1);
        }
    }
};

#endif