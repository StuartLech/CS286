#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>
#include <vector>
#include <sstream>
#include "utils.h"
#include "instruction.h"
using namespace std;

#define START_ADDRESS 96        // address of the first instruction

// load program from the given file, saves it in the vector, returns true if success
// and false if there's an error
bool load_program(char *ifilename, vector<int> &program)
{
    int iFileFD = open(ifilename, O_RDONLY);
    if (iFileFD < 0) 
    {
        cout << "Unable to open file " << ifilename << endl;
        return false;
    }
    // get file size
    off_t file_size = lseek(iFileFD, 0, SEEK_END);
    lseek(iFileFD, 0, SEEK_SET);

    // add values before start
    for (int i = 0; i < START_ADDRESS; i+= 4)
        program.push_back(0);

    for (unsigned i = 0; i < file_size; i+= 4)
    {
        char buffer[4];
        int count = read(iFileFD, buffer, 4);
        if (count == 4)
        {
            int inst;
            char *iPtr = (char *) &inst;
            iPtr[0] = buffer[3];
            iPtr[1] = buffer[2];
            iPtr[2] = buffer[1];
            iPtr[3] = buffer[0];
            program.push_back(inst);
        }
    }
    close(iFileFD);
    return true;
}

// save disassembled output to a file, saves the position of the start of data
// in the datapos, returns true if successful, false otherwise
bool save_disassembled_prog(char *ofilename, vector<int> program, int *datapos)
{
    bool after_break = false;
    string filename = string(ofilename) + "_dis.txt";

    // create disassembled output file
    int oFileFD = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (oFileFD < 0) 
    {
        cout << "Unable to open output file " << filename << endl;
        return false;
    }

    // disassemble program from starting address and up to break instruction
    for (unsigned address = START_ADDRESS; address < program.size() * 4; address+= 4)
    {
        unsigned idx = address >> 2;  // index in program
        if (!after_break)
        {
            stringstream ss;
            // create new instruction from read data
            Instruction inst = Instruction(program[idx]);
            ss << inst.to_binary() << "\t" <<  address << "\t" << inst.disassemble() << endl;
            string line = ss.str();
            write(oFileFD, line.c_str(), line.length());
            if (inst.get_name() == "BREAK")
            {
                after_break = true;
                *datapos = address + 4; // position of start of data is next
            }
        }
        else
        {
            stringstream ss;
            ss << get_val_bits(program[idx], 0, 32) << "\t" << address << "\t" << to_string(program[idx]) << endl;
            string line = ss.str();
            write(oFileFD, line.c_str(), line.length());
        }
    }
    close(oFileFD);
    return true;
}

// simulate the program instructions saved in the program argument
// saves the simulation output to the given file
// returns true if successful, false if any error is found
bool simulate(char *ofilename, vector<int> program, int datapos)
{
    int pc = START_ADDRESS;
    bool is_break;
    int cycle = 1;
    vector<int> registers;

    // create simulation output file
    string filename = string(ofilename) + "_sim.txt";

    int oFileFD = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (oFileFD < 0) 
    {
        cout << "Unable to open output file " << filename << endl;
        return false;
    }


    // initialize registers to zero
    for (int i = 0; i < 32; i++)
        registers.push_back(0);

    // repeat until a break is found
    is_break = false;
    while (!is_break)
    {
        // convert pc to an index for program vector
        int nInst = pc >> 2;
        if (nInst < 0 ||  nInst >= datapos)
        {
            cout << "Invalid instruction address" << endl;
            return false;
        }
        // fetch instruction
        Instruction inst = Instruction(program[nInst]);
        pc += 4;

        if (inst.is_valid())
        {
            stringstream ss;
            ss << "====================" << endl;
            ss << "cycle:" << cycle << "\t" << (pc - 4) << "\t" << inst.disassemble() << endl;

            // simulate instruction
            if (!inst.execute(program, registers, &pc))
            {
                close(oFileFD);
                return false;
            }

            // print updated registers
            ss << "\nregisters:" << endl;
            for (int i = 0; i < 4; i++)
            {
                ss << "r";
                if (i < 2)
                    ss << "0";
                ss << to_string(i * 8) << ":";
                for (int j = 0; j < 8; j++)
                    ss << "\t" << registers[i*8 + j];
                ss << endl;
            }
            // print updated data
            ss << "\ndata:" << endl;
            int dpos = datapos;
            unsigned idx = dpos >> 2;
            while (idx < program.size())
            {
                ss << to_string(dpos) << ":";
                for (int j = 0; j < 8 && idx < program.size(); j++)
                {
                    ss << "\t" << program[idx];
                    dpos += 4;
                    idx++;
                }
                ss << endl;
            }
            ss << endl;

            // save output to file
            string output = ss.str();
            write(oFileFD, output.c_str(), output.length());

            cycle++;
            is_break = inst.get_name() == "BREAK";
        }
    }
    close(oFileFD);
    return true;
}


int main(int argc, char** argv)
{
    char *ifilename = NULL, *ofilename = NULL;
    int datapos; 
    vector<int> program;

    if (argc != 5)
    {
        cout << "Usage: " << endl;
        cout << "    " << argv[0] << "  -i INPUTFILENAME -o OUTPUTFILENAME" << endl;
        return 0;
    }
    if (string(argv[1]) == "-i" && string(argv[3]) == "-o")
    {
        ifilename = argv[2];
        ofilename = argv[4];
    }
    else if (string(argv[1]) == "-o" && string(argv[3]) == "-i")
    {
        ofilename = argv[2];
        ifilename = argv[4];
    }
    else 
    {
        cout << "Invalid arguments." << endl;
        cout << "Usage: " << endl;
        cout << "    " << argv[0] << "  -i INPUTFILENAME -o OUTPUTFILENAME" << endl;
        return 1;
    }

    if (!load_program(ifilename, program))
        return 1;

    if (!save_disassembled_prog(ofilename, program, &datapos))
        return 1;

    if (!simulate(ofilename, program, datapos))
        return 1;

    return 0;
}
