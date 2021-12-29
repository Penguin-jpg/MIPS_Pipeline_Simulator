#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "Utility.h"
#include "Stages.h"
#include "RegisterFile.h"
#include "Memory.h"
using namespace std;

int main()
{
    buildTable();
    // 記憶體
    Memory memory;
    // 讀取指令
    readInstructions("memory.txt", memory.instructions);
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
            idStage.decode(outfile, executings, registerFile, stall, exeStage.control, exeStage.rd, memStage.control, memStage.rd, taken, exeStage.finish);
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