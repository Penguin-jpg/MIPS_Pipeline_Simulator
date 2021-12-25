#ifndef CONTROL_H
#define CONTROL_H

struct Control
{
    /**
     * 控制信號
     * regDst: 選擇寫回的暫存器
     * regWrite: 是否可寫入暫存器
     * ALUSrc: ALU的運算元來源
     * branch: 是否要branch
     * memRead: 是否可讀取記憶體
     * memWrite: 是否可寫入記憶體
     * memToReg: 選擇用記憶體或ALU的資料寫回暫存器
    */

    bool regDst;
    bool regWrite;
    bool ALUSrc;
    bool branch;
    bool memRead;
    bool memWrite;
    bool memToReg;

    Control()
    {
        init();
    }

    // copy constructor
    Control(const Control &control)
    {
        regDst = control.regDst;
        regWrite = control.regWrite;
        ALUSrc = control.ALUSrc;
        branch = control.branch;
        memRead = control.memRead;
        memWrite = control.memWrite;
        memToReg = control.memToReg;
    }

    // initialize
    void init()
    {
        regDst = false;
        regWrite = false;
        ALUSrc = false;
        branch = false;
        memRead = false;
        memWrite = false;
        memToReg = false;
    }
};

#endif