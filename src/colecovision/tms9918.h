/*
 * tms9918.h
 *
 * tms9918 vdp emulation.
 */

#ifndef TMS9918_H
#define TMS9918_H

/* $Id: tms9918.h,v 1.2 1999/11/27 19:16:56 nyef Exp $ */

/* VDP structure definition */

typedef struct {
    unsigned char flags;
    unsigned char readahead;
    unsigned char addrsave;
    unsigned char status;
    unsigned char *memory;
    unsigned char regs[8];
    unsigned short address;
    unsigned short scanline;

    // We have a framebuffer of 256 x 192
    unsigned char *videoout;
    
    uint8 palette[16];
} tms9918;

unsigned char tms9918_readport0(tms9918 *vdp);
unsigned char tms9918_readport1(tms9918 *vdp);
void tms9918_writeport0(tms9918 *vdp, unsigned char data);
void tms9918_writeport1(tms9918 *vdp, unsigned char data);
int tms9918_periodic(tms9918 *vdp,int scanline);

tms9918 *tms9918_create(void);

#endif /* TMS9918_H */

/*
 * $Log: tms9918.h,v $
 * Revision 1.2  1999/11/27 19:16:56  nyef
 * published the actual routine names for what was hiding behind procpointers
 * moved the vdp data structure out to tms9918.c
 *
 * Revision 1.1  1999/06/08 01:49:21  nyef
 * Initial revision
 *
 */
