#ifndef CONTROL_H
#define CONTROL_H

struct Control
{
    /**
     * 控制信號
     * regDst: 選擇寫回的暫存器或don't care
     *  '0': rt, '1': rd, 'X': don't care 
     * regWrite: 是否可寫入暫存器
     * ALUSrc: ALU的運算元來源
     * branch: 是否要branch
     * memRead: 是否可讀取記憶體
     * memWrite: 是否可寫入記憶體
     * memToReg: 選擇用記憶體或ALU的資料寫回暫存器或don't care
     *  '0': ALU, '1': 記憶體, 'X': don't care
    */

    char regDst;
    bool regWrite;
    bool ALUSrc;
    bool branch;
    bool memRead;
    bool memWrite;
    char memToReg;

    Control() : regDst('0'), regWrite(false), ALUSrc(false), branch(false), memRead(false), memWrite(false), memToReg('0') {}

    // R-Format的控制信號
    void controlForRFormat()
    {
        regDst = '1';
        regWrite = true;
        ALUSrc = false;
        branch = false;
        memRead = false;
        memWrite = false;
        memToReg = '0';
    }

    // lw的控制信號
    void controlForLw()
    {
        regDst = '0';
        regWrite = true;
        ALUSrc = true;
        branch = false;
        memRead = true;
        memWrite = false;
        memToReg = '1';
    }

    // sw的控制信號
    void controlForSw()
    {
        regDst = 'X';
        regWrite = false;
        ALUSrc = true;
        branch = false;
        memRead = false;
        memWrite = true;
        memToReg = 'X';
    }

    // beq的控制信號
    void controlForBeq()
    {
        regDst = 'X';
        regWrite = false;
        ALUSrc = false;
        branch = true;
        memRead = false;
        memWrite = false;
        memToReg = 'X';
    }

    // 控制信號歸0
    void flush()
    {
        regDst = '0';
        regWrite = false;
        ALUSrc = false;
        branch = false;
        memRead = false;
        memWrite = false;
        memToReg = '0';
    }

    // copy constructor
    Control(const Control &control) : regDst(control.regDst), regWrite(control.regWrite), ALUSrc(control.ALUSrc),
                                      branch(control.branch), memRead(control.memRead), memWrite(control.memWrite), memToReg(control.memToReg) {}
};

#endif