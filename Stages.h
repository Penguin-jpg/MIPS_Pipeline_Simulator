#ifndef STAGES_H
#define STAGES_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include "Control.h"
#include "RegisterFile.h"
#include "Utility.h"
#include "Instruction.h"
using namespace std;

int stall = 0;                     // stall數
bool taken = false;                // beq是否要taken
RegisterFile registerFile;         // register file
vector<Instruction> executings(5); // 暫存每個階段目前正在做的指令

// IF階段(包含IF/ID)
struct IFStage
{
    int pc;      // pc暫存器
    bool finish; // 階段是否完成

    IFStage() : pc(0), finish(false) {}

    // 抓取指令
    void fetch(fstream &outfile, const vector<vector<string>> &instructions, bool &idFinish)
    {
        finish = false; // IF階段開始

        outfile << "\t" << instructions[pc][0] << ": IF\n";

        // 如果要stall
        if (stall > 0)
        {
            idFinish = false;
            return;
        }

        // 錯誤訊息
        if (pc >= numOfInstructions)
        {
            cout << "pc超過上限\n"
                 << endl;
        }

        executings[0].readInfo(instructions[pc]); // 儲存正在執行的指令
        executings[1] = move(executings[0]);      // 抓取的指令傳給ID階段
        executings[0].clear();                    // 清除指令避免影響後續判斷

        // debug
        // cout << "if: "
        //      << "pc: " << pc << endl;
        // cout << executings[1].operation << endl;

        // pc+4
        pc++;

        finish = true;    // IF階段完成
        idFinish = false; // ID階段開始
    }

    // 是否抓完所有指令
    bool hasFetchedAll()
    {
        return pc == numOfInstructions;
    }
};

// ID階段(包含ID/EX)
struct IDStage
{
    int readData1;   // 讀出的資料1
    int readData2;   // 讀出的資料2
    bool finish;     // 階段是否完成
    Control control; //控制信號

    // finish初始值為true是避免第一次就跑進來做
    IDStage() : readData1(0), readData2(0), finish(true) {}

    // 解碼指令
    void decode(fstream &outfile, const Control &exeControl, bool &zero, const Control &memControl, bool &exeFinish)
    {
        // debug
        // cout << "id: " << endl;
        // for (int i = 0; i < 4; i++)
        // {
        //     cout << executings[1].operation << " ";
        // }
        // cout << endl;

        bool wait = false; // 是否需要因taken而做一次stall

        // 如果要stall
        if (stall > 0)
        {
            outfile << "\t" << executings[1].operation << ": ID\n"; // 重複做ID
            finish = false;
            return;
        }

        // 如果是taken，不輸出抓錯的指令的ID
        if (taken)
        {
            taken = false;
            finish = true;
            exeFinish = false;
            return;
        }
        else
        {
            outfile << "\t" << executings[1].operation << ": ID\n";
        }

        // 取出變數並調整控制信號
        if (executings[1].operation == "add" || executings[1].operation == "sub")
        {
            readData1 = registerFile.registers[executings[1].rs];
            readData2 = registerFile.registers[executings[1].rt];
            control.controlForRFormat();
        }
        else if (executings[1].operation == "lw")
        {
            control.controlForLw();
        }
        else if (executings[1].operation == "sw")
        {
            readData2 = registerFile.registers[executings[1].rt];
            control.controlForSw();
        }
        else if (executings[1].operation == "beq")
        {
            control.controlForBeq();

            // beq移到在ID得知
            if (registerFile.registers[executings[1].rs] == registerFile.registers[executings[1].rt])
            {
                wait = true;
                zero = true;
            }
        }

        // 偵測hazard
        detectHazard(exeControl, memControl, wait);

        // 如果stall數不等2時，代表沒有stall或等待beq結果，而beq指令可以先往下傳
        if (stall != 2)
        {
            executings[2] = move(executings[1]); // 將指令傳到EXE階段
            executings[1].clear();               // 清除指令避免影響後續判斷
        }

        finish = true;     // ID階段完成
        exeFinish = false; // EXE階段開始
    }

    // 偵測hazard
    void detectHazard(const Control &exeControl, const Control &memControl, bool wait)
    {
        // EX hazard或load-use hazard
        // load-use hazard也能在這邊檢查的原因是在沒有forwarding的情況下需要做2次stall
        if ((exeControl.regWrite || exeControl.memRead) && executings[2].rd != 0 && (executings[2].rd == executings[1].rs || executings[2].rd == executings[1].rt))
        {
            stall = 2;       // 做兩次stall
            control.flush(); // 控制信號做flush
        }
        else if (memControl.regWrite && executings[3].rd != 0 && (executings[3].rd == executings[1].rs || executings[3].rd == executings[1].rt)) // MEM hazard(只有在確定不是EX hazard的時候才做)
        {
            stall = 2;
            control.flush();
        }
        else if (wait) // control hazard (只有在條件成立時才需stall)
        {
            stall = 1; // 做stall，等待beq結果
            // 控制信號不做flush，因為beq到EXE階段時，ID階段是閒置狀態，不會有人傳資訊給beq
        }
    }
};

// EXE階段(包含EXE/MEM)
struct EXEStage
{
    int ALUResult;   // ALU的運算結果
    int writeData;   // 要寫入記憶體的資料
    bool zero;       // 計算結果是否為0
    bool finish;     // 階段是否完成
    Control control; // 控制信號

    EXEStage() : ALUResult(0), writeData(0), zero(false), finish(false) {}

    // 執行指令
    void execute(fstream &outfile, IFStage &ifStage, IDStage &idStage, bool &memFinish)
    {
        // 如果指令不為空
        if (!executings[2].isEmpty())
        {
            // debug
            // cout << "exe: " << endl;
            // for (int i = 0; i < 4; i++)
            // {
            //     cout << executings[2].operation << " ";
            // }
            // cout << endl;

            // 將資訊傳給EXE
            control = idStage.control;

            outfile << "\t" << executings[2].operation << ": EX " << control.regDst << control.ALUSrc << " " << control.branch
                    << control.memRead << control.memWrite << " " << control.regWrite << control.memToReg << "\n";

            executings[3] = move(executings[2]); // 將指令傳給MEM階段
            executings[2].clear();               // 清除指令避免影響後續判斷
        }

        // 執行特定運算
        if (executings[3].operation == "add")
        {
            ALUResult = idStage.readData1 + idStage.readData2;
        }
        else if (executings[3].operation == "sub")
        {
            ALUResult = idStage.readData1 - idStage.readData2;
        }
        else if (executings[3].operation == "sw")
        {
            writeData = idStage.readData2;
        }
        else if (executings[3].operation == "beq")
        {
            if (control.branch && zero)
            {
                ifStage.pc += executings[3].offset;
                taken = true;
                zero = false;
            }
            else
            {
                taken = false;
            }
        }

        finish = true;     // EXE階段完成
        memFinish = false; // MEM階段開始
    }

    // 初始化
    void init()
    {
        ALUResult = 0;
        writeData = 0;
        zero = false;
        control.flush();
    }
};

// MEM階段(包含MEM/WB)
struct MEMStage
{
    int ALUResult;   // 由EXE階段來的ALU結果
    int writeData;   // 要寫入記憶體的資料
    int readData;    // 讀出的資料
    bool finish;     // 階段是否完成
    Control control; // 控制信號

    MEMStage() : ALUResult(0), writeData(0), readData(0), finish(false) {}

    // 存取記憶體
    void accessMemory(fstream &outfile, vector<int> &data, EXEStage &exeStage, bool &wbFinish)
    {
        // debug
        // cout << "mem: " << endl;
        // for (int i = 0; i < 4; i++)
        // {
        //     cout << executings[3].operation << " ";
        // }
        // cout << endl;

        // 如果指令不為空
        if (!executings[3].isEmpty())
        {
            // 將資訊傳給MEM
            control = exeStage.control;
            ALUResult = exeStage.ALUResult;
            writeData = exeStage.writeData;

            // 初始化EXE，避免影響後續判斷
            exeStage.init();

            outfile << "\t" << executings[3].operation << ": MEM " << control.branch << control.memRead << control.memWrite << " " << control.regWrite << control.memToReg << "\n";

            executings[4] = move(executings[3]); // 將指令傳給WB階段
            executings[3].clear();               // 清除指令避免影響後續判斷
        }

        // 如果要讀取記憶體
        if (control.memRead)
        {
            readData = data[executings[4].offset];
        }
        else if (control.memWrite) // 如果要寫入記憶體
        {
            data[executings[4].offset] = writeData;
        }

        finish = true;    // MEM階段完成
        wbFinish = false; // WB階段開始
    }

    // 初始化
    void init()
    {
        ALUResult = 0;
        writeData = 0;
        readData = 0;
        control.flush();
    }
};

// WB階段
struct WBStage
{
    int ALUResult;   // 由EXE階段來的ALU結果
    int writeData;   // 要寫回的資料
    bool finish;     // 階段是否完成
    Control control; // 控制信號

    WBStage() : ALUResult(0), writeData(0), finish(false) {}

    // 寫回暫存器
    void writeBack(fstream &outfile, MEMStage &memStage)
    {
        // debug
        // cout << "wb: " << endl;
        // for (int i = 0; i < 4; i++)
        // {
        //     cout << executings[4].operation << " ";
        // }
        // cout << endl;

        // 如果指令不為空
        if (!executings[4].isEmpty())
        {
            // 將資訊傳給WB
            control = memStage.control;
            ALUResult = memStage.ALUResult;
            writeData = memStage.readData;

            // 初始化MEM，避免影響後續判斷
            memStage.init();

            outfile << "\t" << executings[4].operation << ": WB " << control.regWrite << control.memToReg << "\n";
            executings[4].clear(); // 清除指令避免影響後續判斷
        }

        // 如果需要寫入暫存器
        if (control.regWrite)
        {
            // 寫入ALU結果
            if (control.memToReg == '0')
            {
                registerFile.registers[executings[4].rd] = ALUResult;
            }
            else if (control.memToReg == '1') // 寫入記憶體的資料
            {
                registerFile.registers[executings[4].rd] = writeData;
            }
        }

        control.flush(); // 清空控制信號
        finish = true;   // WB階段完成
    }
};

#endif