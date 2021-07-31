/*
    Copyright (C) 1998, 1999, 2000  Charles Mac Donald

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "z80.h"
#include "system.h"
#include "sn76496.h"
#include "colecovision.h"
#include "tms9918.h"

t_bitmap bitmap;
t_cart cart;                
t_snd snd;
t_input input;

void emu_system_init(int rate) {
    /* Initialize the Colecovision emulation */
    colecovision_init();

    /* Enable sound emulation if the sample rate was specified */
    audio_init(rate);

    /* Clear emulated button state */
    memset(&input, 0, sizeof(t_input));
}

void audio_init(int rate) {
    /* Clear sound context */
    // memset(&snd, 0, sizeof(t_snd));
	snd.enabled = 0;
	
    /* Reset logging data */
    snd.log = 0;
    snd.callback = NULL;

    /* Oops.. sound is disabled */
    if(!rate) return;

    /* Calculate buffer size in samples */
    snd.bufsize = rate == 15720 ? 262 : 312;   // EWWWW

    /* Sound output */
    if (!snd.buffer[0]) {
    	snd.buffer[0] = (signed short int *)malloc(snd.bufsize * 2);
    }
    
    if (!snd.buffer[1]) {
    	snd.buffer[1] = (signed short int *)malloc(snd.bufsize * 2);
    }
    
    if(!snd.buffer[0] || !snd.buffer[1]) return;
    
    memset(snd.buffer[0], 0, snd.bufsize * 2);
    memset(snd.buffer[1], 0, snd.bufsize * 2);

    /* Set up SN76489 emulation */
    SN76496_init(0, MASTER_CLOCK, 255, rate);

    /* Inform other functions that we can use sound */
    snd.enabled = 1;
}

void system_shutdown(void) {
    
	if(snd.enabled) {
    	
    }
    
}


void system_reset(void) {
    cpu_reset();
    
    colecovision_reset();
    
    if(snd.enabled) {
    	
    }
    
}

