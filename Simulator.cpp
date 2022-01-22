#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "Stages.h"
#include "Memory.h"
using namespace std;

int main()
{
    buildTable();
    // 記憶體
    Memory memory;
    // 讀取指令
    readInstructions("memory.txt", memory.instructions);

    // cycle數
    int cycle = 1;
    // 寫檔
    fstream outfile("result.txt", ios::out);
    // 五個階段
    IFStage ifStage;
    IDStage idStage;
    EXEStage exeStage;
    MEMStage memStage;
    WBStage wbStage;

    // 執行
    while (true)
    {
        outfile << "Cycle " << cycle << endl;

        // 從WB階段開始是為了讓先進入的指令先往下做，不然會變single cycle
        // 執行WB
        if (!wbStage.finish)
        {
            wbStage.writeBack(outfile, memStage);
        }

        // 執行MEM
        if (!memStage.finish)
        {
            memStage.accessMemory(outfile, memory.data, exeStage, wbStage.finish);
        }

        // 執行EXE
        if (!exeStage.finish)
        {
            exeStage.execute(outfile, ifStage, idStage, memStage.finish);
        }

        // 執行ID
        if (!idStage.finish || stall > 0)
        {
            idStage.decode(outfile, exeStage.control, memStage.control, exeStage.finish);
        }

        // 檢查是否抓完所有指令了
        if (!ifStage.hasFetchedAll())
        {
            // 抓取指令
            ifStage.fetch(outfile, memory.instructions, idStage.finish);
        }
        else
        {
            ifStage.finish = true;
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

        // 經過一個cycle
        cycle++;
    }

    // 寫出剩餘資訊
    writeInfo(outfile, registerFile, memory, cycle);

    return 0;
}