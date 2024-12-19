#include <iostream>
#include <string>
#include <unistd.h>
#include "utils.h"
#include "instruction.h"
using namespace std;

// Instruction constructor, fills in all the fields depending on the instruction
Instruction::Instruction(int inst) {
    const char *instructionNames[] {
        "J", "JR", "BEQ", "BLTZ", "ADD", "ADDI", "SUB", "SW", 
        "LW", "SLL", "SRL", "MUL", "AND", "OR", "MOVZ", "NOP",
        "BREAK", "INVALID"
    };
    // fill in all the instruction fields, they will be used depending on the 
    // instruction type
    valid = (inst >> 31) & 1;
    opcode = (inst >> 26) & 0x1F;
    rs = (inst >> 21) & 0x1F;
    rt = (inst >> 16) & 0x1F;
    rd = (inst >> 11) & 0x1F;
    imm = inst & 0xFFFF;
    shamt = (inst >> 6) & 0x1F;
    func = (inst & 0x3F);
    address = (inst & 0x3FFFFFF);

    ID = INVALID;           // assume instruction is not valid by default
    switch (opcode)
    {
    case 0x0: 
        switch (func)
        {
        case 0:  type = _R; ID = SLL; break;
        case 2:  type = _R; ID = SRL; break;
        case 8:  type = _I; ID = JR; break;
        case 10: type = _R; ID = MOVZ; break;
        case 13: type = _J; ID = BREAK; break;
        case 32: type = _R; ID = ADD; break;
        case 34: type = _R; ID = SUB; break;
        case 36: type = _R; ID = AND; break;
        case 37: type = _R; ID = OR; break;
        }
        break;
    case 0x1:
        if (rt == 0) 
        {
            type = _I; 
            ID = BLTZ;
        }
        break;
    case 0x2: type = _J; ID = J; break;
    case 0x4: type = _I; ID = BEQ; break;
    case 0x8: type = _I; ID = ADDI; break;
    case 0xb: type = _I; ID = SW; break;
    case 0x3: type = _I; ID = LW; break;
    case 0x1c: 
        if (func == 2) 
        {
            type = _R; 
            ID = MUL;
        }
        break;
    }
    if ((inst & 0x7FFFFFFF) == 0) // all zeros is nop, exclude valid bit
        ID = NOP;
    if (valid == 0)
        ID = INVALID;
    name = instructionNames[ID];
}


// return instruction bits as a string in the required format
string Instruction::to_binary()
{
    string bits = "";
    bits += get_val_bits(valid, 0, 1);
    bits += " ";
    bits += get_val_bits(opcode, 0, 5);
    bits += " ";
    bits += get_val_bits(rs, 0, 5);
    bits += " ";
    bits += get_val_bits(rt, 0, 5);
    bits += " ";
    bits += get_val_bits(rd, 0, 5);
    bits += " ";
    bits += get_val_bits(shamt, 0, 5);
    bits += " ";
    bits += get_val_bits(func, 0, 6);
    return bits;
}

// Disassemble instruction and return it as a string
string Instruction::disassemble() {
    string dasm;
    if (ID == INVALID)
        return "Invalid Instruction";
    if (ID == NOP || ID == BREAK)
        return name;
    dasm = name + "\t";
    if (type == _R)
    {
        if (ID == SLL || ID == SRL)
            dasm += "R" + to_string(rd) + ", R" + to_string(rt) + ", #" + to_string(shamt);
        else
            dasm += "R" + to_string(rd) + ", R" + to_string(rs) + ", R" + to_string(rt);
    }
    else if (type == _J)
    {
        // target address for jump must be multiplied by 4
        dasm += "#" + to_string(address << 2);
    }
    else // I
    {
        int immediate = expand_sign(imm, 16); // expand 16 to 32 bits with sign
        int offset = immediate << 2; // offsets for branch must be multiplied by 4
        switch (ID)
        {
        case JR: 
            dasm += "R" + to_string(rs); 
            break;
        case BLTZ:
            dasm += "R" + to_string(rs) + ", #" + to_string(offset);
            break;
        case BEQ:
            dasm += "R" + to_string(rs) + ", R" + to_string(rt) + ", #" + to_string(offset);
            break;
        case SW: case LW:
            dasm += "R" + to_string(rt) + ", " + to_string(immediate) + "(" + "R" + to_string(rs) + ")";
            break;
        default: // ADDI
            dasm += "R" + to_string(rt) + ", R" + to_string(rs) + ", #" + to_string(immediate);
        }
    }
    return dasm;
}

// executes the instruction using the contents of memory, registers and pc,
// uses the base address for the start of memory and assumes memory is addressable
// by words.
// Returns true if success or false if any error is found
bool Instruction::execute(vector<int> &memory, vector<int> &registers, int *pc)
{
    int immediate = expand_sign(imm, 16); // expand 16 to 32 bits with sign
    int offset = immediate << 2;
    unsigned mem_addr;
    unsigned long val;

    switch (ID)
    {
        case J:
            *pc = address << 2; // jump to address
            break;
        case JR:
            *pc = registers[rs]; // jump to register contents
            break;
        case BEQ:
            if (registers[rs] == registers[rt]) // jump if equal
                *pc += offset;
            break;
        case BLTZ:
            if (registers[rs] < 0) // jump if < 0
                *pc += offset;
            break;
        case ADD:
            if (rd != 0)    // avoid changing register 0
                registers[rd] = registers[rs] + registers[rt];
            break;
        case ADDI:
            if (rt != 0)    // avoid changing register 0
                registers[rt] = registers[rs] + immediate;
            break;
        case SUB:
            if (rd != 0)    // avoid changing register 0
                registers[rd] = registers[rs] - registers[rt];
            break;
        case SW:
            mem_addr = immediate + registers[rs];
            if (mem_addr & 0x3)
            {
                cout << "Unaligned memory address for SW" << endl;
                return false;
            }
            // adjust to word memory
            mem_addr >>= 2;
            if (mem_addr >= memory.size())
            {
                cout << "Invalid memory address for SW" << endl;
                return false;
            }
            memory[mem_addr] = registers[rt];
            break;
        case LW:
            mem_addr = immediate + registers[rs];
            if (mem_addr & 0x3)
            {
                cout << "Unaligned memory address for LW" << endl;
                return false;
            }
            // adjust to start address and word memory
            mem_addr >>= 2;
            if (mem_addr >= memory.size())
            {
                cout << "Invalid memory address for LW" << endl;
                return false;
            }
            if (rt != 0)    // avoid changing register 0
                registers[rt] = memory[mem_addr];
            break;
        case SLL:
            if (rd != 0)    // avoid changing register 0
                registers[rd] = registers[rt] << shamt;
            break;
        case SRL:
            val = registers[rt];
            if (rd != 0)    // avoid changing register 0
                registers[rd] = val >> shamt;
            break;
        case MUL:
            val = registers[rs] * registers[rt];
            if (rd != 0)    // avoid changing register 0
                registers[rd] = val;
            break;
        case AND:
            if (rd != 0)    // avoid changing register 0
                registers[rd] = registers[rs] & registers[rt];
            break;
        case OR:
            if (rd != 0)    // avoid changing register 0
                registers[rd] = registers[rs] | registers[rt];
            break;
        case MOVZ:
            if (rd != 0 && registers[rt] == 0)
                registers[rd] = registers[rs];
            break;
        case BREAK:
            return true;
        case NOP:      // do nothing
        case INVALID:
            break;
    }
    return true;
}
