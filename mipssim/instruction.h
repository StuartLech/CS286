#pragma once
#include <string>
#include <vector>

// Definition of an instruction class to handle MIPS instructions
class Instruction
{
    // numerical enumeration of all supported instructions
    enum InstructionID {
        J, JR, BEQ, BLTZ, ADD, ADDI, SUB, SW, 
        LW, SLL, SRL, MUL, AND, OR, MOVZ, NOP, 
        BREAK, INVALID
    };
    // numerical enumeration of all instruction types
    enum InstructionType {_R, _I, _J};
    InstructionID ID;   // identifier of this instruction corresponding to the InstructionID enum
    int valid;      // bit 31 of instruction
    int opcode;     // bits 26-30 of instruction
    int rs;         // bits 25-21 of instruction
    int rt;         // bits 20-16 of instruction
    int rd;         // bits 15-11 of instruction (R)
    int imm;        // bits 15-0 of instruction (I)
    int shamt;      // bits 10-6 of instruction (R, for shifts)
    int func;       // bits 5-0 of instruction (for opcode 0)
    int address;    // bits 25-0 of instruction (J)
    InstructionType type;   // instruction type (I, R, J)
    std::string name;   // name of instruction as a string
    
public:
    Instruction(int inst);
    std::string to_binary();
    std::string disassemble();
    std::string get_name() {
        return name;
    }
    bool is_valid()
    {
        return (valid == 1);
    }
    bool execute(std::vector<int> &memory, std::vector<int> &registers, int *pc);
};
