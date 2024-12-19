#include "utils.h"
using namespace std;

// gets the given bits of the value and returns them as a string of ones and zeros
string get_val_bits(int val, int off, int nbits)
{
    unsigned int mask;
    string bits = "";
    mask = 1 << (nbits - 1);
    val >>= off;
    for (int i = 0; i < nbits; i++)
    {
        bits += ((val & mask) != 0)? "1" : "0";
        mask >>= 1;
    }
    return bits;
}

// expand the sign of the value of nbits to 32 bits
int expand_sign(int val, int nbits)
{
    int rem = 32 - nbits;
    val <<= rem;    // move bits up
    val >>= rem;    // move bits down, leaving the sign to 32 bits
    return val;
}
