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
using namespace std;

// IF階段(包含IF/ID)
struct IFStage
{
    int pc;      // pc暫存器
    bool finish; // 階段是否完成

    IFStage() : pc(0) {}

    // 抓取指令
    void fetch(fstream &outfile, const vector<vector<string>> &instructions, vector<vector<string>> &executings, int stall, bool &idFinish)
    {
        finish = false; // IF階段開始

        outfile << "\t" << instructions[pc][0] << ": IF\n";

        // 如果要stall
        if (stall > 0)
        {
            finish = false;
            idFinish = false;
            return;
        }

        executings[0] = instructions[pc]; // 儲存正在執行的指令
        executings[1] = executings[0];    // 抓取的指令傳給ID階段
        executings[0][0] = "";            // 清除指令避免影響後續判斷

        // debug
        // cout << "if: "
        //      << "pc: " << pc << endl;
        // for (int i = 0; i < 4; i++)
        // {
        //     cout << executings[0][i] << " ";
        // }
        // cout << endl;

        // pc+4
        pc++;

        finish = true;    // IF階段完成
        idFinish = false; // ID階段開始
    }
};

// ID階段(包含ID/EX)
struct IDStage
{
    int rs;            // 來源暫存器1
    int rt;            // 來源/目標暫存器2
    int rd;            // 目標暫存器
    int writeRegister; // 目標暫存器
    int offset;        // 位移(給lw、sw、beq使用)
    string operation;  // 要做的運算
    bool finish;       // 階段是否完成
    Control control;   //控制信號

    // finish初始值為true是避免第一次就跑進來做
    IDStage() : rd(0), writeRegister(0), offset(0), operation(""), finish(true) {}

    // 解碼指令
    void decode(fstream &outfile, vector<vector<string>> &executings, RegisterFile &registerFile, int &stall,
                Control &exeControl, int exeRd, Control &memControl, int memRd, bool &taken, bool &exeFinish)
    {
        // debug
        // cout << "id: " << endl;
        // for (int i = 0; i < 4; i++)
        // {
        //     cout << executings[1][i] << " ";
        // }
        // cout << endl;

        bool wait = false; // 是否需要因taken而做一次stall

        // 如果要stall
        if (stall > 0)
        {
            outfile << "\t" << executings[1][0] << ": ID\n"; // 重複做ID
            finish = false;
            return;
        }

        // 取出變數並調整控制信號
        if (executings[1][0] == "add")
        {
            operation = "add";
            rd = getRegister(executings[1][1]);
            rs = getRegister(executings[1][2]);
            rt = getRegister(executings[1][3]);
            control.controlForRFormat();
        }
        else if (executings[1][0] == "sub")
        {
            operation = "sub";
            rd = getRegister(executings[1][1]);
            rs = getRegister(executings[1][2]);
            rt = getRegister(executings[1][3]);
            control.controlForRFormat();
        }
        else if (executings[1][0] == "lw")
        {
            operation = "lw";
            rt = getRegister(executings[1][1]);
            try
            {
                offset = stol(executings[1][2]) / 4; // 除以4轉位置
            }
            catch (...)
            {
                cout << "lw gone wrong\n";
            }

            rs = -1; // 因為題目設計的關係，所以不用讀取
            rd = rt; // 為了hazard判斷，讓rd=rt
            control.controlForLw();
        }
        else if (executings[1][0] == "sw")
        {
            operation = "sw";
            rt = getRegister(executings[1][1]);
            try
            {
                offset = stol(executings[1][2]) / 4; // 除以4轉位置
            }
            catch (...)
            {
                cout << "sw gone wrong\n";
            }

            rs = -1; // 因為題目設計的關係，所以不用讀取
            rd = rt; // 為了hazard判斷，讓rd=rt
            control.controlForSw();
        }
        else if (executings[1][0] == "beq")
        {
            operation = "beq";
            rt = getRegister(executings[1][1]);
            rs = getRegister(executings[1][2]);
            try
            {
                offset = stol(executings[1][3]);
            }
            catch (...)
            {
                cout << "beq gone wrong\n";
            }
            rd = rt;
            control.controlForBeq();

            // beq移到在ID得知
            if (registerFile.registers[rs] == registerFile.registers[rt])
            {
                wait = true;
            }
        }

        // 偵測hazard
        detectHazard(outfile, exeControl, exeRd, memControl, memRd, stall, wait);

        // 如果是taken
        if (taken)
        {
            // 原本抓的指令清掉
            executings[1][0] = "";
            taken = false;
        }

        // 如果要做該指令
        if (executings[1][0] != "")
        {
            outfile << "\t" << executings[1][0] << ": ID\n";
        }

        // 如果不是stall數小於2(不需重做ID)
        if (stall < 2)
        {
            executings[2] = executings[1]; // 將指令傳到EXE階段
            executings[1][0] = "";         // 清除指令避免影響後續判斷
        }

        finish = true;     // ID階段完成
        exeFinish = false; // EXE階段開始
    }

    // 偵測hazard
    void detectHazard(fstream &outfile, Control &exeControl, int exeRd, Control &memControl, int memRd, int &stall, bool wait)
    {
        // EX hazard或load-use hazard
        // load-use hazard也能在這邊檢查的原因是在沒有forwarding的情況下需要做2次stall
        if ((exeControl.regWrite || exeControl.memRead) && exeRd != 0 && (exeRd == rs || exeRd == rt))
        {
            // outfile << "load-use hazard here\n";
            stall = 2;       // 做兩次stall
            control.flush(); // 控制信號做flush
        }
        else if (memControl.regWrite && memRd != 0 && (memRd == rs || memRd == rt)) // MEM hazard(只有在確定不是EX hazard的時候才做)
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
    int rs;           // 來源暫存器1
    int rt;           // 來源/目標暫存器2
    int rd;           // 目標暫存器
    string operation; // 要執行的運算
    int ALUResult;    // ALU的運算結果
    int writeData;    // 要寫入記憶體的資料
    int offset;       // 位移
    bool zero;        // 計算結果是否為0
    bool finish;      // 階段是否完成
    Control control;  // 控制信號

    EXEStage() : rs(0), rt(0), rd(0), operation(""), ALUResult(0), writeData(0), offset(0), zero(false), finish(false) {}

    // 執行指令
    void execute(fstream &outfile, vector<vector<string>> &executings, RegisterFile &registerFile, int &stall, bool &taken, IFStage &ifStage, IDStage &idStage, bool &memFinish)
    {
        // 如果指令不為空
        if (executings[2][0] != "")
        {
            // debug
            // cout << "exe: " << endl;
            // for (int i = 0; i < 4; i++)
            // {
            //     cout << executings[2][i] << " ";
            // }
            // cout << endl;

            // 將資訊傳給EXE
            control = idStage.control;
            rs = idStage.rs;
            rt = idStage.rt;
            rd = idStage.rd;
            offset = idStage.offset;
            operation = idStage.operation;

            outfile << "\t" << executings[2][0] << ": EX " << control.regDst << control.ALUSrc << " " << control.branch
                    << control.memRead << control.memWrite << " " << control.regWrite << control.memToReg << "\n";

            executings[3] = executings[2]; // 將指令傳給MEM階段
            executings[2][0] = "";         // 清除指令避免影響後續判斷
        }

        // 執行特定運算
        if (operation == "add")
        {
            ALUResult = registerFile.registers[rs] + registerFile.registers[rt];
        }
        else if (operation == "sub")
        {
            ALUResult = registerFile.registers[rs] - registerFile.registers[rt];
        }
        else if (operation == "sw")
        {
            writeData = registerFile.registers[rt];
        }
        else if (operation == "beq")
        {
            // 當taken時，啟動zero控制信號
            if (registerFile.registers[rs] == registerFile.registers[rt])
            {
                zero = true;
            }
            if (control.branch && zero)
            {
                // outfile << "taken!!" << endl;
                ifStage.pc += offset;
                taken = true;
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
        rs = 0;
        rt = 0;
        rd = 0;
        operation = "";
        ALUResult = 0;
        writeData = 0;
        offset = 0;
        zero = false;
        control.flush();
    }
};

// MEM階段(包含MEM/WB)
struct MEMStage
{
    int rd;          // 目標暫存器
    int ALUResult;   // 由EXE階段來的ALU結果
    int writeData;   // 要寫入記憶體的資料
    int readData;    // 讀出的資料
    int offset;      // 位移
    bool finish;     // 階段是否完成
    Control control; // 控制信號

    MEMStage() : rd(0), ALUResult(0), writeData(0), offset(0), finish(false) {}

    // 存取記憶體
    void accessMemory(fstream &outfile, vector<vector<string>> &executings, RegisterFile &registerFile, vector<int> &data, int &stall, EXEStage &exeStage, bool &wbFinish)
    {
        // debug
        // cout << "mem: " << endl;
        // for (int i = 0; i < 4; i++)
        // {
        //     cout << executings[3][i] << " ";
        // }
        // cout << endl;

        // 如果指令不為空
        if (executings[3][0] != "")
        {
            // 將資訊傳給MEM
            control = exeStage.control;
            rd = exeStage.rd;
            ALUResult = exeStage.ALUResult;
            writeData = exeStage.writeData;
            offset = exeStage.offset;

            // 初始化EXE，避免影響後續判斷
            exeStage.init();

            outfile << "\t" << executings[3][0] << ": MEM " << control.branch << control.memRead << control.memWrite << " " << control.regWrite << control.memToReg << "\n";

            executings[4] = executings[3]; // 將指令傳給WB階段
            executings[3][0] = "";         // 清除指令避免影響後續判斷
        }

        // 如果要讀取記憶體
        if (control.memRead)
        {
            readData = data[offset];
        }
        else if (control.memWrite) // 如果要寫入記憶體
        {
            data[offset] = writeData;
        }

        finish = true;    // MEM階段完成
        wbFinish = false; // WB階段開始
    }

    // 初始化
    void init()
    {
        rd = 0;
        ALUResult = 0;
        writeData = 0;
        readData = 0;
        offset = 0;
        control.flush();
    }
};

// WB階段
struct WBStage
{
    int rd;          // 目標暫存器
    int ALUResult;   // 由EXE階段來的ALU結果
    int readData;    // 由MEM階段讀出的資料
    bool finish;     // 階段是否完成
    Control control; // 控制信號

    WBStage() : readData(0), rd(0), finish(false) {}

    // 寫回暫存器
    void writeBack(fstream &outfile, RegisterFile &registerFile, vector<vector<string>> &executings, MEMStage &memStage)
    {
        // debug
        // cout << "wb: " << endl;
        // for (int i = 0; i < 4; i++)
        // {
        //     cout << executings[4][i] << " ";
        // }
        // cout << endl;

        // 如果指令不為空
        if (executings[4][0] != "")
        {
            // 將資訊傳給WB
            control = memStage.control;
            ALUResult = memStage.ALUResult;
            readData = memStage.readData;
            rd = memStage.rd;

            // 初始化MEM，避免影響後續判斷
            memStage.init();

            outfile << "\t" << executings[4][0] << ": WB " << control.regWrite << control.memToReg << "\n";
            executings[4][0] = ""; // 清除指令避免影響後續判斷
        }

        // 如果需要寫入暫存器
        if (control.regWrite)
        {
            // 寫入ALU結果
            if (control.memToReg == '0')
            {
                registerFile.registers[rd] = ALUResult;
            }
            else if (control.memToReg == '1') // 寫入記憶體的資料
            {
                registerFile.registers[rd] = readData;
            }
        }

        control.flush(); // 清空控制信號
        finish = true;   // WB階段完成
    }
};

#endif