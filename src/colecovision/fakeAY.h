#ifndef _AY38910_H
#define _AY38910_H

void FakeAY_Loop(void);
void FakeAY_WriteData(unsigned char Value);

extern unsigned char ay_reg_idx;
extern unsigned char ay_reg[];
    
extern unsigned short envelope_period;
extern unsigned short envelope_counter;
extern unsigned short noise_period;
extern unsigned char a_idx;
extern unsigned char b_idx;
extern unsigned char c_idx;

// -----------------------------------
// Write the AY register index...
// -----------------------------------
void FakeAY_WriteIndex(unsigned char Value);

// -----------------------------------
// Read an AY data value...
// -----------------------------------
unsigned char FakeAY_ReadData(void);


#endif
