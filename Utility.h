#ifndef UTILITY_H
#define UTILITY_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "RegisterFile.h"
#include "Memory.h"
using namespace std;

unordered_map<string, int> REGISTER_TABLE; // 暫存器號碼對照表
int numOfInstructions = 0;                 // 紀錄指令數量

// 讀取指令
void readInstructions(const string &path, vector<vector<string>> &instructions)
{
    /**
     *  讀取指令(每個指令會被拆解為小部分)
     *  add: add, rd, rs, rt
     *  sub: sub, rd, rs, rt
     *  beq: beq, rs, rt, label
     *  lw:  lw, rt, offset, rs
     *  sw:  sw, rt, offset, rs
    */

    fstream infile(path); // 讀檔
    stringstream ss;      // 協助文字處理(stringstream能自動切空格)
    string line;          // 讀取的一行文字
    string word;          // 每行文字裡的字

    if (!infile.is_open())
    {
        cout << "Failed to open file\n";
        return;
    }

    while (getline(infile, line))
    {
        vector<string> result(4);
        ss.clear();
        ss << line;
        int index = 0;

        // 讀取每個字
        while (ss >> word)
        {
            result[index] = word;
            index++;
        }

        // 去除多餘符號
        if (result[0] == "add" || result[0] == "sub" || result[0] == "beq") // add或sub或beq
        {
            for (int i = 1; i < result.size() - 1; i++)
            {
                result[i].pop_back(); // 去除逗點
            }
        }
        else if (result[0] == "lw" || result[0] == "sw") // lw或sw
        {
            result[1].pop_back(); // 去除逗點
            string offset = "";   // 位移
            string reg = "";      // 暫存器號碼
            int temp = 0;         // 記錄左括號位置

            // 取出offset
            for (int i = 0; result[2][i] != '('; i++)
            {
                offset += result[2][i];
                temp = i; // 最後會停在offset的最後一位數字
            }

            // 取出暫存器號碼
            for (int i = temp + 2; result[2][i] != ')'; i++)
            {
                reg += result[2][i];
            }

            // 更新
            result[2] = offset;
            result[3] = reg;
        }

        instructions[numOfInstructions] = result;
        numOfInstructions++;
    }
}

// 建立對照表
void buildTable()
{
    string key = "";
    for (int i = 0; i < 32; i++)
    {
        key = "$" + to_string(i);
        REGISTER_TABLE[key] = i;
    }
}

// 取得暫存器號碼
int getRegister(const string &key)
{
    return REGISTER_TABLE[key];
}

// 輸出所有指令(debug用)
void printInstructions(const vector<vector<string>> &instructions)
{
    for (int i = 0; i < numOfInstructions; i++)
    {
        for (int j = 0; j < instructions[i].size(); j++)
        {
            cout << instructions[i][j] << " ";
        }
        cout << "\n";
    }
}

// 將剩餘資訊寫出
void writeInfo(fstream &outfile, const RegisterFile &registerFile, const Memory &memory, const int cycle)
{
    outfile << endl;
    outfile << "需要花" << cycle << "個cycles"
            << "\n";

    // 暫存器號碼
    for (int i = 0; i < 32; i++)
    {
        outfile << "$" << i << "\t";
    }
    outfile << "\n";

    // 暫存器的值
    for (int i = 0; i < 32; i++)
    {
        outfile << registerFile.registers[i] << "\t";
    }
    outfile << "\n";

    // 記憶體號碼
    for (int i = 0; i < 32; i++)
    {
        outfile << "W" << i << "\t";
    }
    outfile << "\n";

    // 記憶體的值
    for (int i = 0; i < 32; i++)
    {
        outfile << memory.data[i] << "\t";
    }
    outfile << "\n";
}

#endif