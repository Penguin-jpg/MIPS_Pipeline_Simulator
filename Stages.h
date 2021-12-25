#ifndef STAGES_H
#define STAGES_H

#include <iostream>
#include <vector>
#include "Control.h"
#include "RegisterFile.h"
using namespace std;

// IF階段
struct IFStage
{
    /**
     * 控制信號
     * pcSrc: 選擇branch後的pc還是pc+4
    */
    bool pcSrc;

    int pc; // pc暫存器

    IFStage() : pcSrc(false), pc(0) {}

    // 抓取指令
    void fetch(fstream &outfile, const vector<vector<string>> &instructions, vector<vector<string>> &executings)
    {
        /**
         * outfile: 寫出的資料流
         * instructions: 指令集
         * executings: 正在執行的指令
        */

        outfile << "\t" << instructions[pc][0] << ": IF" << endl;

        // 儲存正在執行的指令
        for (int i = 0; i < 4; i++)
        {
            executings[0][i] = instructions[pc][i];
        }

        for (int i = 0; i < 4; i++)
        {
            cout << executings[0][i] << " ";
        }
        cout << endl;

        // pc+4
        pc++;
    }
};

// ID階段
struct IDStage
{
    int pc;            // ID階段的pc
    int readData1;     // 暫存器資料1
    int readData2;     // 暫存器資料2
    int writeRegister; // 目標暫存器
    int constant;      // 常數
    Control control;   // ID階段的控制信號

    IDStage() : pc(0), readData1(0), readData2(0), writeRegister(0), constant(0) {}
};

// EXE階段
struct EXEStage
{
    /**
     * 控制信號
     * regDst: 選擇寫回的暫存器或don't care
     * regWrite: 是否可寫入暫存器
     * ALUSrc: ALU的運算元來源
     * branch: 是否要branch
     * memRead: 是否可讀取記憶體
     * memWrite: 是否可寫入記憶體
     * memToReg: 選擇用記憶體或ALU的資料寫回暫存器
    */
    int regDst;
    bool regWrite;
    bool ALUSrc;
    bool branch;
    bool memRead;
    bool memWrite;
    bool memToReg;

    int rs;        // 來源暫存器1
    int rt;        // 來源/目標暫存器2
    int rd;        // 目標暫存器
    int ALUResult; // ALU的運算結果
    int writeData; // 要寫入記憶體的資料
    int address;   // 要使用的記憶體位址
    int addResult; // pc加上常數的結果(branch用的pc)
    bool zero;     // 結果是否為0

    EXEStage() : regDst(false), regWrite(false), ALUSrc(false), branch(false), memRead(false), memWrite(false), memToReg(false), ALUResult(0), writeData(0), address(0), addResult(0), zero(false) {}
};

// MEM階段
struct MEMStage
{
    /**
     * 控制信號
     * branch: 是否要branch
     * memRead: 是否可讀取記憶體
     * memWrite: 是否可寫入記憶體
     * memToReg: 選擇用記憶體或ALU的資料寫回暫存器或don't care
    */
    bool branch;
    bool memRead;
    bool memWrite;
    int memToReg;

    int ALUResult;   // 由EXE階段來的ALU結果
    int writeData;   // 要寫入記憶體的資料
    int readData;    // 讀出的資料
    int destination; // 目的地

    MEMStage() : branch(false), memRead(false), memWrite(false), memToReg(false), ALUResult(0), writeData(0), destination(0) {}
};

// WB階段
struct WBStage
{
    /**
     * 控制信號
     * regWrite: 是否可寫入暫存器
     * memToReg: 選擇用記憶體或ALU的資料寫回暫存器或don't care
    */
    bool regWrite;
    int memToReg;

    int ALUResult;   // 由EXE階段來的ALU結果
    int readData;    // 由MEM階段讀出的資料
    int destination; // 目的地

    WBStage() : regWrite(false), memToReg(false), ALUResult(0), readData(0), destination(0) {}

    // 寫回暫存器
    void writeBack(fstream &outfile, RegisterFile &registerFile, vector<vector<string>> &executings)
    {
        /**
         * outfile: 寫出的資料流
         * executings: 正在執行的指令
        */

        // 如果尚未輸出
        if (executings[4][0] != "done")
        {
            outfile << "\t" << executings[4][0] << ": WB"
                    << " " << regWrite;

            // don't care
            if (memToReg == 2)
            {
                outfile << "X" << endl;
            }
            else
            {
                outfile << memToReg << endl;
            }

            executings[4][0] = "done";
        }

        // 如果需要寫入暫存器
        if (regWrite)
        {
            // 寫入ALU結果
            if (memToReg == 0)
            {
                registerFile.registers[destination] = bitset<32>(ALUResult);
            }
            else // 寫入記憶體的資料
            {
                registerFile.registers[destination] = bitset<32>(readData);
            }
        }

        // 初始化控制信號，避免再次進入上面的判斷
        regWrite = false;
        memToReg = 0;
    }
};

#endif