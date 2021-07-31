
#ifndef _COLECOVISION_H_
#define _COLECOVISION_H_

/* Colecovision context */
typedef struct {
    uint8 ram[0x400];
    
	uint8 *bios;
    uint8 *cartridge;
    
    uint8 irq;
}t_colecovision;

/* Global data */
extern t_colecovision colecovision;

/* Function prototypes */
void colecovision_frame(int skip_render);
void colecovision_init(void);
void colecovision_reset(void);
void cpu_reset(void);
int  colecovision_irq_callback(int param);

unsigned char *colecovision_get_video(void);
#endif
