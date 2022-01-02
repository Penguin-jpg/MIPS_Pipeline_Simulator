#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <vector>
#include <string>
#include "Utility.h"
using namespace std;

struct Instruction
{
    string operation; // 要做的運算
    int rs;
    int rt;
    int rd;
    int offset;

    Instruction() : operation(""), rs(0), rt(0), rd(0), offset(0) {}

    void readInfo(const vector<string> &instruction)
    {
        operation = instruction[0];

        if (operation == "add" || operation == "sub")
        {
            rd = getRegister(instruction[1]);
            rs = getRegister(instruction[2]);
            rt = getRegister(instruction[3]);
        }
        else if (operation == "lw" || operation == "sw")
        {
            rt = getRegister(instruction[1]);
            try
            {
                offset = stol(instruction[2]) / 4; // 除以4轉位置
            }
            catch (...)
            {
                cout << "lw or sw gone wrong" << endl;
            }
            rs = -1;
            rd = rt;
        }
        else if (operation == "beq")
        {
            rt = getRegister(instruction[1]);
            rs = getRegister(instruction[2]);
            try
            {
                offset = stol(instruction[3]);
            }
            catch (...)
            {
                cout << "beq gone wrong" << endl;
            }
            rd = rt;
        }
    }

    void clear()
    {
        operation = "";
    }

    bool isEmpty()
    {
        return operation == "";
    }
};

#endif