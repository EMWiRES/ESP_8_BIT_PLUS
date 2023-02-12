/*
 * New emulator main file.
 *
 * Based on ESP_8_BIT https://github.com/rossumur/ESP_8_BIT.
 *
 * (c) 2023 EMWiRES / Allard van der Bas.
 *
 * Stripped back as much as possible to use as a template for further development.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "colecovision.h"
#include "z80.h"
#include "sn76496.h"
#include "tms9918.h"
#include "system.h"
#include "fakeAY.h"

/* Colecovision context */
t_colecovision colecovision;

tms9918 *cv_vdp=NULL;

int is_joystick;
int SGM_RAM = 0;
int BIOS_RAM = 0;

unsigned char ay_reg_idx = 0;
unsigned char ay_reg[16];

int bankMask;
int currentBank;
int doBanking=0;

unsigned char *colecovision_get_video(void) {
	return(cv_vdp->videoout);
}

/* Run the virtual console emulation for one frame */
void colecovision_frame(int skip_render) {
	int cnt;
	
	// 262 scanlines 192 active.
    for(cnt = 0; cnt < 262; cnt++) {
        
		tms9918_periodic(cv_vdp,cnt);
        
		/* Run the Z80 for a line */
        z80_execute(227);
    }

    /* Update the emulated sound stream */
    SN76496Update(0, snd.buffer, snd.bufsize, 0xFF);
}

void colecovision_init(void) {
    cpu_reset();
    colecovision_reset();
}

void colecovision_reset(void) {
	// Reset the RAM.
    memset(colecovision.ram, 0, 0x400);
    
    memset(colecovision.SGMram,0,0x8000);
    
    SGM_RAM = 0;
    BIOS_RAM = 0x0F;
    currentBank = 0;
    
    memset(ay_reg,0,16);
    ay_reg_idx = 0x00;
    
    if (!cv_vdp) {
    	cv_vdp = tms9918_create();
    }
    
    // Add the video output to it.
    cv_vdp->videoout = bitmap.data;
    
    // Clear the VDP RAM.
    memset(cv_vdp->memory,0,16384);
    
    // Explicitly clear everything.
    cv_vdp->status = 0;
    cv_vdp->scanline = 0;
    cv_vdp->address = 0;
    cv_vdp->flags = 0;
    cv_vdp->readahead = 0;
    cv_vdp->addrsave = 0;
    memset(cv_vdp->regs,0,8);
    
    // SN76496_init(0, MASTER_CLOCK, 255, 15720);
}

int colecovision_irq_callback(int param) {
    return (0xFF);
}

/* Reset Z80 emulator */
void cpu_reset(void) {
    z80_reset(0);
    z80_set_irq_callback(colecovision_irq_callback);
}

/* Write to memory */
void cpu_writemem16(int address, int data) {
	/* printf("Write mem: 0x%04X 0x%02X\n",address,data); */
	
	if (address < 0x2000) {
		if (BIOS_RAM == 0x0F) {
			return;
		} else {
			colecovision.SGMram[address] = data;
			return;
		}
	} else if (address < 0x8000) {
		if (SGM_RAM) {
			colecovision.SGMram[address] = data;
		} else {
			if ( (address & 0xE000) == 0x6000 ) {
				colecovision.ram[address&0x3FF] = data;
			}
		}
	} else {
		// printf("Illegal access: 0x%04X,0x%02X\n",address,data);
	}
	
}

/* Write to an I/O port */
void cpu_writeport(int address, int data) {

    if ( (address & 0xFF) == 0x50) {
    	
		FakeAY_WriteIndex(data);
    	
	} else  if ( (address & 0xFF) == 0x51) {
    	
		FakeAY_WriteData(data);
    	
	} else if ( (address & 0xFF) == 0x7F) {
    	
    	// printf("BIOS RAM access 0x%04X,0x%02X\n",address,data);
		BIOS_RAM = data;
		
	} else if ( (address & 0xFF) == 0x53) {
		// printf("SGM RAM access 0x%04X,0x%02X\n",address,data);
		SGM_RAM = data;
		
	} else if ((address & 0xe0) == 0x80) {
		
		/* set controls to keypad mode */
		/* printf("Set controls to keypad mode.\n"); */
		is_joystick = 0;
		
    } else if ((address & 0xe0) == 0xa0) {
		
		if (address & 1) {
	    	/* printf("tms9918_writeport1(0x%02X)\n",data); */
	    	
	    	tms9918_writeport1(cv_vdp,data);
	    	
		} else {
	    	/* printf("tms9918_writeport0(0x%02X)\n",data); */
	    	
	    	tms9918_writeport0(cv_vdp,data);
	    	
		}
		
    } else if ((address & 0xe0) == 0xc0) {
		
		/* set controls to joystick mode */
		/* printf("Set controls to joystick mode.\n"); */
		is_joystick = 1;
		
    } else if ((address & 0xe0) == 0xe0) {
		
		SN76496Write(0,data);

    } else {
		
		// printf("cv_io_write: 0x%02x = 0x%02x.\n", address & 0xff, data);
		
    }
    
}


/* Read from an I/O port */
int cpu_readport(int address) {
	unsigned char answer;

	if ( (address & 0xFF) == 0x52) {
		
		// printf("Reading psg index: 0x%02X\n",psg_index);
		return( FakeAY_ReadData() );
		
	} else if ((address & 0xe0) == 0xa0) {
		
		if (address & 1) {
	    	return tms9918_readport1(cv_vdp);
		} else {
	    	return tms9918_readport0(cv_vdp);
		}
		
    } else if ((address & 0xe0) == 0xe0) {
		
		if (address & 0x02) {
	    	
	    	if (is_joystick) {
	    		answer = 0;
	    		
	    		if (input.pad[1] & INPUT_UP)
					answer |= 0x01;
					
				if (input.pad[1] & INPUT_DOWN)
					answer |= 0x04;
					
				if (input.pad[1] & INPUT_LEFT)
					answer |= 0x08;
					
				if (input.pad[1] & INPUT_RIGHT)
					answer |= 0x02;
					
				if (input.pad[1] & INPUT_BUTTON2)
					answer |= 0x40;
				
				answer = ~answer;
					    		
				/* joystick mode */
				return( answer );
				
			} else {
				
				answer = 0;
				
				if (input.pad[1] & INPUT_BUTTON1)
					answer = 0x02;	// 1
					
				answer = ~answer;
					
				/* keypad mode */
				return( answer );
			}
	    	
		} else {
			
	    	if (is_joystick) {
	    		answer = 0x7F;
	    		
	    		if (input.pad[0] & INPUT_UP)
					answer &= 0xFE;
					
				if (input.pad[0] & INPUT_DOWN)
					answer &= 0xFB;
					
				if (input.pad[0] & INPUT_LEFT)
					answer &= 0xF7;
					
				if (input.pad[0] & INPUT_RIGHT)
					answer &= 0xFD;

				if (input.pad[0] & INPUT_BUTTON2)
					answer &= 0xBF;
					    		
				/* joystick mode */
				return( answer );

			} else {
				
				answer = 0x70;
				if (input.pad[0]&INPUT_BUTTON1) {
					answer &= 0xbf;
				}		
					
				if (input.pad[0] & INPUT_KEY1)
					return (answer | 0x0d);	// 1
					
				if (input.pad[0] & INPUT_KEY2)
					return (answer | 0x07);
					
				if (input.pad[0] & INPUT_KEY_STAR)
					return (answer | 0x09);
					
				if (input.pad[0] & INPUT_KEY_HASH)
					return (answer | 0x06);
					
				/* keypad mode */
				return( answer | 0x0F);
			}
	    	
		}
		
    } else {
		/* printf("cv_io_read: 0x%02x.\n", address & 0xff); */
    }

    return 0;
}

int cpu_readmem16(int address) {
	/* printf("Read mem: 0x%04X\n",address); */
	if (address < 0x2000) {
		
		if (BIOS_RAM == 0x0F) {
			return(cart.bios[address]);
		} else {
			return(colecovision.SGMram[address]);
		}
		
	} else if (address < 0x6000) {
		
		if (SGM_RAM) {
			return(colecovision.SGMram[address]);
		} else {
			return(0);			
		}

	} else if (address < 0x8000) {
		
		if (SGM_RAM) {
			return( colecovision.SGMram[address]);	
		} else {
			return( colecovision.ram[address & 0x3FF] );
		}
		
	}

	if (doBanking) {
		
		if (address >= 0xFFC0) {
			
			// printf("Bank access 0x%04X this is bank %d\n",address,address & bankMask);
			
			currentBank = address & bankMask;
			
			return(cart.rom[bankMask*16384 + (address & 0x3FFF)]);
			
		} if (address < 0xC000) {
			
			// Get it from the top of the ROM.
			return(cart.rom[bankMask*16384 + (address & 0x3FFF)]);
			
		} else {
			
			return(cart.rom[currentBank*16384 + (address & 0x3FFF)]);
			
		}
		
	} else {
		
		return(cart.rom[address & 0x7FFF]);
		
	}

}

