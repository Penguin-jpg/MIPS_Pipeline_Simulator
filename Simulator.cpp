#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "Stages.h"
#include "RegisterFile.h"
#include "Memory.h"
using namespace std;

unordered_map<string, int> REGISTER_TABLE; // 暫存器號碼對照表
int numOfInstructions = 0;                 // 紀錄指令數量

void readInstructions(const string &path, Memory &memory)
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

        memory.instructions[numOfInstructions] = result;
        numOfInstructions++;
    }

    infile.close(); // 關檔
}

// 建立對照表
void buildTable()
{
    for (int i = 0; i < 32; i++)
    {
        string key = "$" + to_string(i);
        REGISTER_TABLE[key] = i;
    }
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
        cout << endl;
    }
}

int main()
{
    buildTable();
    // 記憶體
    Memory memory;
    // 讀取指令
    readInstructions("memory.txt", memory);
    // beq是否要taken
    bool taken = false;
    // register file
    RegisterFile registerFile;
    // cycle數
    int cycle = 1;
    // stall計數器
    int stall = 0;
    // 寫檔
    fstream outfile("result.txt", ios::out);
    // 五個階段
    IFStage ifStage;
    IDStage idStage;
    EXEStage exeStage;
    MEMStage memStage;
    WBStage wbStage;
    // 暫存每個階段目前正在做的指令
    vector<vector<string>> executings(5, vector<string>(4));

    // 執行
    while (true)
    {
        outfile << "Cycle " << cycle << endl;

        // 從WB階段開始是為了讓先進入的指令先往下做，不然會變single cycle
        // 執行WB
        if (!wbStage.finish)
        {
            wbStage.writeBack(outfile, registerFile, executings, memStage);
        }

        // 執行MEM
        if (!memStage.finish)
        {
            memStage.accessMemory(outfile, executings, registerFile, memory.data, stall, exeStage, wbStage.finish);
        }

        // 執行EXE
        if (!exeStage.finish)
        {
            exeStage.execute(outfile, executings, registerFile, stall, taken, ifStage, idStage, memStage.finish);
        }

        // 執行ID
        if (!idStage.finish || stall > 0)
        {
            idStage.decode(outfile, executings, registerFile, stall, REGISTER_TABLE, exeStage.control, exeStage.rd, memStage.control, memStage.rd, taken, exeStage.finish);
        }

        // 抓完所有指令了
        if (ifStage.pc == numOfInstructions)
        {
            ifStage.finish = true;
        }
        else // 執行IF(測試完成)
        {
            // 抓取指令
            ifStage.fetch(outfile, memory.instructions, executings, stall, idStage.finish);
        }

        // 當全部階段做完時，stall完成一次
        if (stall > 0)
        {
            stall--;
        }

        // 全部執行結束
        if (ifStage.finish && idStage.finish && exeStage.finish && memStage.finish && wbStage.finish)
        {
            break;
        }
        cycle++;
    }

    // 將剩餘資訊補上
    outfile << endl;
    outfile << "需要花" << cycle << "個cycles" << endl;

    // 暫存器號碼
    for (int i = 0; i < 32; i++)
    {
        outfile << "$" << i << "\t";
    }
    outfile << endl;

    // 暫存器的值
    for (int i = 0; i < 32; i++)
    {
        outfile << registerFile.registers[i] << "\t";
    }
    outfile << endl;

    // 記憶體號碼
    for (int i = 0; i < 32; i++)
    {
        outfile << "W" << i << "\t";
    }
    outfile << endl;

    // 記憶體的值
    for (int i = 0; i < 32; i++)
    {
        outfile << memory.data[i] << "\t";
    }
    outfile << endl;

    // 關檔
    outfile.close();

    return 0;
}