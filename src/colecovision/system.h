
#ifndef _SYSTEM_H_
#define _SYSTEM_H_


/* Macro to get offset to actual display within bitmap */
#define BMP_X_OFFSET        0
#define BMP_Y_OFFSET        0

#define BMP_WIDTH           256
#define BMP_HEIGHT          192

/* Mask for removing unused pixel data */
#define PIXEL_MASK          (0x1F)

/* These can be used for 'input.pad[]' */
#define INPUT_UP          (0x00000001)
#define INPUT_DOWN        (0x00000002)
#define INPUT_LEFT        (0x00000004)
#define INPUT_RIGHT       (0x00000008)
#define INPUT_BUTTON2     (0x00000020)
#define INPUT_BUTTON1     (0x00000040)
#define INPUT_KEY1		  (0x00000080)
#define INPUT_KEY2        (0x00000100)
#define INPUT_KEY_HASH    (0x00000200)
#define INPUT_KEY_STAR    (0x00000400)

/* These can be used for 'input.system' */
#define INPUT_START       (0x00000001)    /* Game Gear only */    
#define INPUT_PAUSE       (0x00000002)    /* Master System only */
#define INPUT_SOFT_RESET  (0x00000004)    /* Master System only */
#define INPUT_HARD_RESET  (0x00000008)    /* Works for either console type */

/* User input structure */
typedef struct {
    int pad[2];
    int system;
}t_input;

/* Sound emulation structure */
typedef struct {
    int enabled;
    int bufsize;
    signed short *buffer[2];
    int log;
    void (*callback)(int data);
}t_snd;

/* Game image structure */
typedef struct {
    uint8 rom[32*1024];
    // uint8 *rom;
    uint8 bios[8192];
    // uint8 *bios;
} t_cart;

/* Bitmap structure */
typedef struct {
    unsigned char *data;
    
	int width;
    int height;
    
    int pitch;
    int depth;
} t_bitmap;

/* Global variables */
extern t_bitmap bitmap;     /* Display bitmap */
extern t_snd snd;           /* Sound streams */
extern t_cart cart;         /* Game cartridge data */
extern t_input input;       /* Controller input */

/* Function prototypes */
void emu_system_init(int rate);
void system_shutdown(void);
void system_reset(void);
void audio_init(int rate);

#endif /* _SYSTEM_H_ */
