/*
 * tms9918.c
 *
 * tms9918 VDP emulation.
 *
 * Added per pixel sprite collision detection fixes Aztec Challenge.
 */

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "tms9918.h"
#include "system.h"
#include "z80.h"
#include "colecovision.h"

#define TMS_RAMSIZE 0x4000

#define TF_ADDRWRITE 1

/* palette definition */

/* Colecovision pallette normally:
    0 - Transparent (bg color / normally black).
    1 - Black.
    2 - Medium green.
    3 - Light green.
    4 - Dark blue.
    5 - Light blue.
    6 - Dark red (brown)
    7 - Cyan
    8 - Medium Red
    9 - Light red (pink/orange)
    10 - Dark yellow
    11 - Light yellow    --> adjusted to not be too 'grey'
    12 - Dark green
    13 - Magenta
    14 - Grey (light)
    15 - White
*/

/* Old values */
/*
const int tms9918_palbase_red[16] = {
    0x00, 0x00, 0x22, 0x66, 0x22, 0x44, 0xbb, 0x44,
    0xff, 0xff, 0xdd, 0xdd, 0x22, 0xdd, 0xbb, 0xff,
};

const int tms9918_palbase_green[16] = {
    0x00, 0x00, 0xdd, 0xff, 0x22, 0x66, 0x22, 0xdd,
    0x22, 0x66, 0xdd, 0xdd, 0x99, 0x44, 0xbb, 0xff,
};

const int tms9918_palbase_blue[16] = {
    0x00, 0x00, 0x22, 0x66, 0xff, 0xff, 0x11, 0xff,
    0x22, 0x66, 0x22, 0x66, 0x22, 0xbb, 0xbb, 0xff,
}; */


/* New values from WikiPedia */
const int tms9918_palbase_red[16] = {
    0x00, 0x00, 0x0A, 0x34, 0x2B, 0x51, 0xBD, 0x1E,
    0xFB, 0xFF, 0xBD, 0xD7, 0x0A, 0xAF, 0xB2, 0xff
};

const int tms9918_palbase_green[16] = {
    0x00, 0x00, 0xAD, 0xC8, 0x2D, 0x4B, 0x29, 0xE2,
    0x2C, 0x5F, 0xA2, 0xB4, 0x8C, 0x32, 0xB2, 0xff
};

const int tms9918_palbase_blue[16] = {
    0x00, 0x00, 0x1E, 0x4C, 0xE3, 0xFB, 0x25, 0xEF,
    0x2B, 0x4C, 0x2B, 0x54, 0x18, 0x9A, 0xB2, 0xff
};

unsigned char tms9918_readport0(tms9918 *vdp) {
    unsigned char retval;
    
    retval = vdp->readahead;
    vdp->readahead = vdp->memory[vdp->address++];
    vdp->address &= 0x3fff;
    vdp->flags &= ~TF_ADDRWRITE;
    
    return retval;
}

unsigned char tms9918_readport1(tms9918 *vdp) {
    unsigned char retval;

    retval = vdp->status;
    
    vdp->status &= 0x1f;			// Keep fifth sprite active.
    vdp->flags &= ~TF_ADDRWRITE;

	z80_set_nmi_line(CLEAR_LINE);

    return retval;
}

void tms9918_writeport0(tms9918 *vdp, unsigned char data) {
    vdp->readahead = data;
    vdp->memory[vdp->address++] = data;
    vdp->address &= 0x3fff;
    vdp->flags &= ~TF_ADDRWRITE;
}

void tms9918_writeport1(tms9918 *vdp, unsigned char data) {
    if (vdp->flags & TF_ADDRWRITE) {
		if (data & 0x80) {
		    vdp->regs[data & 7] = vdp->addrsave;
		    
		    // Are we setting the IRQ enable and do we have a vblank pending?
		    if ( (data & 7) == 1 ) {
		    	
		    	if (vdp->addrsave & 0x20) {
		    		
		    		if (vdp->status & 0x80) {
		    			
		    			if (vdp->scanline != 192) {
		    				// Trigger an interrupt.
		    				z80_set_nmi_line(ASSERT_LINE);
						}
						
					}
					
				}
				
			}
		    
		} else {
		    vdp->address = (vdp->addrsave | (data << 8)) & 0x3fff;
		    if (!(data & 0x40)) {
				vdp->readahead = vdp->memory[vdp->address++];
				vdp->address &= 0x3fff;
		    }
		}
		vdp->flags &= ~TF_ADDRWRITE;
    } else {
		vdp->addrsave = data;
		vdp->flags |= TF_ADDRWRITE;
    }
}

struct sprite_data {
    uint8 y_pos;
    uint8 x_pos;
    uint8 pattern;
    uint8 color;
};

struct sprite_cache {
    int x_pos;
    uint8 *pattern;
    uint8 color;
};

int tms9918_cache_sprites(tms9918 *vdp, struct sprite_cache *cache, int sprite_size) {
    struct sprite_data *sprites;
    unsigned char *pattern_table;
    int num_sprites;
    int i;
    
    num_sprites = 0;

    sprites = (struct sprite_data *)&vdp->memory[(vdp->regs[5] & 0x7f) << 7];
    pattern_table = &vdp->memory[(vdp->regs[6] & 0x07) << 11];
    
    for (i = 0; i < 32; i++) {
    	
		if (sprites[i].y_pos == 208) {
		    break;
		}

		if (sprites[i].y_pos >= vdp->scanline) {
		    continue;
		}

		if ((sprites[i].y_pos + sprite_size) < vdp->scanline) {
		    continue;
		}

		if (num_sprites == 4) {
		    vdp->status |= 0x40; /* fifth sprite flag */
		    break;
		}
		
		cache[num_sprites].color = sprites[i].color & 0x0f;
		cache[num_sprites].x_pos = sprites[i].x_pos;

		if (sprites[i].color & 0x80) {
		    cache[num_sprites].x_pos -= 32;
		}

		cache[num_sprites].pattern = &pattern_table[sprites[i].pattern << 3];
		cache[num_sprites].pattern += vdp->scanline - (sprites[i].y_pos + 1);
		
		num_sprites++;
    }

    /* fifth sprite id */
    vdp->status &= 0xe0;
    vdp->status |= i;
    
    return num_sprites;
}

/*
 * Check individual sprite data lines for bit overlaps.
 */
int tms9918_check_sprite_overlap(struct sprite_cache sprite1,struct sprite_cache sprite2,int sprite_size) {
	int distance;
	
	unsigned short sprite1_pattern;
	unsigned short sprite2_pattern;

	if (sprite_size == 8) {
		sprite1_pattern = sprite1.pattern[0];
		sprite2_pattern = sprite2.pattern[0];
	} else {
		sprite1_pattern = sprite1.pattern[0];
		sprite1_pattern <<= 8;
		sprite1_pattern |= sprite1.pattern[16];
		
		sprite2_pattern = sprite2.pattern[0];
		sprite2_pattern <<= 8;
		sprite2_pattern |= sprite2.pattern[16];
	}
	
	if (sprite1.x_pos >= sprite2.x_pos) {
		distance = sprite1.x_pos - sprite2.x_pos;
		sprite1_pattern >>= distance;
	} else {
		distance = sprite2.x_pos - sprite1.x_pos;
		sprite2_pattern >>= distance;
	}
	
	// Any pixels overlap?
	sprite1_pattern &= sprite2_pattern;
	
	// Any bits on?
	if (sprite1_pattern > 0) {
		return(1);
	} else {
		return(0);
	}
}

int tms9918_check_sprite_collision(tms9918 *vdp, struct sprite_cache *cache, int sprite_size, int num_sprites) {
	
    switch (num_sprites) {
    	case 4:
			/* NOTE: & 0x1ff here is to ensure that the result is positive */
			if (((cache[3].x_pos - cache[2].x_pos) & 0x1ff) < sprite_size) {
			    return(tms9918_check_sprite_overlap(cache[3],cache[2],sprite_size));
			}
			
			if (((cache[3].x_pos - cache[1].x_pos) & 0x1ff) < sprite_size) {
			    return(tms9918_check_sprite_overlap(cache[3],cache[1],sprite_size));
			}
			
			if (((cache[3].x_pos - cache[0].x_pos) & 0x1ff) < sprite_size) {
			    return(tms9918_check_sprite_overlap(cache[3],cache[0],sprite_size));
			}

	    case 3:
			if (((cache[2].x_pos - cache[1].x_pos) & 0x1ff) < sprite_size) {
			    return(tms9918_check_sprite_overlap(cache[2],cache[1],sprite_size));
			}
			
			if (((cache[2].x_pos - cache[0].x_pos) & 0x1ff) < sprite_size) {
			    return(tms9918_check_sprite_overlap(cache[2],cache[0],sprite_size));
			}

	    case 2:
			if (((cache[1].x_pos - cache[0].x_pos) & 0x1ff) < sprite_size) {
			    return(tms9918_check_sprite_overlap(cache[1],cache[0],sprite_size));
			}

	    case 1:
	    default:
		/* not enough sprites to collide */
		return 0;
    }
    
}

void tms9918_render_sprites(tms9918 *vdp, unsigned char *cur_vbp)
{
    struct sprite_cache cache[4];
    int num_sprites;
    int i;
    int sprite_size;

    sprite_size = (vdp->regs[1] & 0x02)? 16: 8;
    
    num_sprites = tms9918_cache_sprites(vdp, cache, sprite_size);

	// Clear collision flag.
	// vdp->status &= ~0x20;

    if (tms9918_check_sprite_collision(vdp, cache, sprite_size, num_sprites)) {
		vdp->status |= 0x20; /* sprite collision flag */
    }
    
    for (i = num_sprites - 1; i >= 0; i--) {
		uint8 color;
		uint8 *tmp_vbp;
		uint8 *pattern;
		uint16 data;
	
		color = vdp->palette[cache[i].color];
		tmp_vbp = cur_vbp + cache[i].x_pos;
		pattern = cache[i].pattern;
	
		if (!cache[i].color) {
		    /* no point in drawing an invisible sprite */
		    continue;
		}
	
		if ((cache[i].x_pos + sprite_size) < 0) {
		    continue;
		}
	
		data = (pattern[0] << 8) | pattern[16];
	
		if (cache[i].x_pos < 0) {
		    /* clip to left edge of screen */
		    data &= 0xffff >> -cache[i].x_pos;
		} else if ((cache[i].x_pos + sprite_size) > 255) {
		    /* clip to right edge of screen */
		    data &= ((sprite_size == 8)? 0xff00: 0xffff)
			<< ((cache[i].x_pos + sprite_size + 1) & 0xff);
		}
		
		if (data & 0x8000) {
		    *tmp_vbp = color;
		}
		
		tmp_vbp++;
		if (data & 0x4000) {
		    *tmp_vbp = color;
		}
		
		tmp_vbp++;
		if (data & 0x2000) {
		    *tmp_vbp = color;
		}
		
		tmp_vbp++;
		if (data & 0x1000) {
		    *tmp_vbp = color;
		}
		
		tmp_vbp++;
		if (data & 0x0800) {
		    *tmp_vbp = color;
		}
		
		tmp_vbp++;
		if (data & 0x0400) {
		    *tmp_vbp = color;
		}
		
		tmp_vbp++;
		if (data & 0x0200) {
		    *tmp_vbp = color;
		}
		
		tmp_vbp++;
		if (data & 0x0100) {
		    *tmp_vbp = color;
		}
		
		if (sprite_size == 16) {
		    tmp_vbp++;
		    if (data & 0x80) {
				*tmp_vbp = color;
		    }
		    
		    tmp_vbp++;
		    if (data & 0x40) {
				*tmp_vbp = color;
		    }
		    
		    tmp_vbp++;
		    if (data & 0x20) {
				*tmp_vbp = color;
		    }
		    
		    tmp_vbp++;
		    if (data & 0x10) {
				*tmp_vbp = color;
		    }
		    
		    tmp_vbp++;
		    if (data & 0x08) {
				*tmp_vbp = color;
		    }
		    
		    tmp_vbp++;
		    if (data & 0x04) {
				*tmp_vbp = color;
		    }
		    
		    tmp_vbp++;
		    if (data & 0x02) {
				*tmp_vbp = color;
		    }
		    
		    tmp_vbp++;
		    if (data & 0x01) {
				*tmp_vbp = color;
		    }
		}
    }
}

void tms9918_render_line_mode_0(tms9918 *vdp) {
    unsigned char *cur_vbp;
    unsigned char *nametable;
    unsigned char *patterntable;
    unsigned char *colortable;
    unsigned char color0;
    unsigned char color1;
    unsigned char pattern;
    int i;
    
    cur_vbp = vdp->videoout + (vdp->scanline * 256);
    nametable = vdp->memory + ((vdp->regs[2] & 0x0f) << 10);

    patterntable = vdp->memory + ((vdp->regs[4] & 0x07) << 11);
    patterntable += vdp->scanline & 7;
    nametable += (vdp->scanline & ~7) << 2;
    colortable = vdp->memory + (vdp->regs[3] << 6);
    
    for (i = 0; i < 32; i++) {
		pattern = patterntable[nametable[i] << 3];
		color0 = vdp->palette[colortable[nametable[i] >> 3] & 15];
		color1 = vdp->palette[colortable[nametable[i] >> 3] >> 4];
		if (pattern & 0x80) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x40) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x20) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x10) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x08) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x04) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x02) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x01) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
    }
}

void tms9918_render_line_mode_1(tms9918 *vdp) {
    unsigned char *cur_vbp;
    unsigned char *nametable;
    unsigned char *patterntable;
    unsigned char pattern;
    unsigned char color0;
    unsigned char color1;
    int i;
    
    // cur_vbp = video_get_vbp(vdp->scanline);
    cur_vbp = vdp->videoout + (vdp->scanline * 256);
    nametable = vdp->memory + ((vdp->regs[2] & 0x0f) << 10);

    patterntable = vdp->memory + ((vdp->regs[4] & 0x07) << 11);
    patterntable += vdp->scanline & 7;
    nametable += (vdp->scanline & ~7) * 5;
    
    color0 = vdp->palette[vdp->regs[7] & 15];
    color1 = vdp->palette[vdp->regs[7] >> 4];

    for (i = 0; i < 40; i++) {
		pattern = patterntable[nametable[i] << 3];
		if (pattern & 0x80) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x40) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x20) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x10) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x08) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x04) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
    }

    /*
     * FIXME: This looks a little hacky. If it must remain perhaps
     * it should be 8 pels on either side of the main display?
     */
    for (i=0; i<16; i++)
	*cur_vbp++ = color0;
}

/*
 * There was a bug affecting Uridium, Bagman and Tankwars.
 *
 * It was not selecting the correct color table
 * for the second and third screen block.
 */
void tms9918_render_line_mode_2(tms9918 *vdp) {
    unsigned char *cur_vbp;
    unsigned char *nametable;
    unsigned char *patterntable;
    unsigned char *colortable;
    unsigned char pattern;
    unsigned char color0;
    unsigned char color1;
    int i;
    
    // cur_vbp = video_get_vbp(vdp->scanline);
    cur_vbp = vdp->videoout + (vdp->scanline * 256);
    
	nametable = vdp->memory + ( (vdp->regs[2] & 0x0f ) << 10 );

    colortable = vdp->memory + ( (vdp->regs[3] & 0x80) ? 0x2000: 0);
    patterntable = vdp->memory + ( (vdp->regs[4] & 0x04) ? 0x2000: 0);

    patterntable += vdp->scanline & 7;
    colortable += vdp->scanline & 7;
    
   if (vdp->scanline >= 0x80) {
		
		if (vdp->regs[4] & 0x02) {
		    patterntable += (0x200 << 3);
		    // colortable += (0x200 << 3);
		}
		
		if (vdp->regs[3] & 0x40) {
			colortable += (0x200 << 3);
		}
		
	} else if (vdp->scanline >= 0x40) {
		
		if (vdp->regs[4] & 0x01) {
		    patterntable += (0x100 << 3);
		    // colortable += (0x100 << 3);
		}
		
		if (vdp->regs[3] & 0x20) {
			colortable += (0x100 << 3);
		}
			
	}
	    
    nametable += (vdp->scanline & ~7) << 2;
	    
	for (i = 0; i < 32; i++) {
		pattern = patterntable[nametable[i] << 3];
		color0 = vdp->palette[colortable[nametable[i] << 3] & 0x0f];
		color1 = vdp->palette[colortable[nametable[i] << 3] >> 4];
		if (pattern & 0x80) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x40) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x20) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x10) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x08) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x04) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x02) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
		if (pattern & 0x01) {
		    *cur_vbp++ = color1;
		} else {
		    *cur_vbp++ = color0;
		}
    }
    
}

void tms9918_render_line_mode_3(tms9918 *vdp) {
    unsigned char *cur_vbp;
    unsigned char *nametable;
    unsigned char *patterntable;
    unsigned char color0;
    unsigned char color1;
    unsigned char pattern;
    int i;
    
    // cur_vbp = video_get_vbp(vdp->scanline);
    cur_vbp = vdp->videoout + (vdp->scanline * 256);
    
	nametable = vdp->memory + ((vdp->regs[2] & 0x0f) << 10);
    patterntable = vdp->memory + ((vdp->regs[4] & 0x07) << 11);
    patterntable += (vdp->scanline & 28) >> 2;
    
    nametable += (vdp->scanline & ~7) << 2;
    
    for (i = 0; i < 32; i++) {
		pattern = patterntable[nametable[i] << 3];
		
		color0 = vdp->palette[pattern & 15];
		color1 = vdp->palette[pattern >> 4];
		
		/* FIXME: long (32bit) writes should be faster */
		*cur_vbp++ = color1;
		*cur_vbp++ = color1;
		*cur_vbp++ = color1;
		*cur_vbp++ = color1;
		
		*cur_vbp++ = color0;
		*cur_vbp++ = color0;
		*cur_vbp++ = color0;
		*cur_vbp++ = color0;
    }
    
}

void tms9918_render_line_mode_unknown(tms9918 *vdp) {
	/* printf("tms9918: unsupported display mode %c%c%c.\n",
	       (vdp->regs[1] & 0x10)? '1': '-',
	       (vdp->regs[0] & 0x02)? '2': '-',
	       (vdp->regs[1] & 0x08)? '3': '-'); */
}

typedef void (*tms9918_linerenderer)(tms9918 *vdp);

tms9918_linerenderer tms9918_linerenderers[8] = {
    tms9918_render_line_mode_0,       /* 0 */
    tms9918_render_line_mode_1,       /* 1 */
    tms9918_render_line_mode_2,       /* 2 */
    tms9918_render_line_mode_unknown, /* 1 + 2 */
    tms9918_render_line_mode_3,       /* 3 */
    tms9918_render_line_mode_unknown, /* 1 + 3 */
    tms9918_render_line_mode_unknown, /* 2 + 3 */
    tms9918_render_line_mode_unknown, /* 1 + 2 + 3 */
};

void tms9918_render_line(tms9918 *vdp) {
    unsigned char *cur_vbp;
    int mode;

	cur_vbp = vdp->videoout + (vdp->scanline * 256);
	
    /* set up background color */
    if (vdp->regs[7] & 0x0f) {
		vdp->palette[0] = vdp->palette[vdp->regs[7] & 0x0f];
    } else {
		vdp->palette[0] = vdp->palette[1];
    }
    
    if (!(vdp->regs[1] & 0x40)) {
		memset(cur_vbp, vdp->palette[1], 256);
		return;
    }

    mode = 0;
    
    if (vdp->regs[1] & 0x10) {
		mode |= 1;
    }
    
    if (vdp->regs[0] & 0x02) {
		mode |= 2;
    }
    
    if (vdp->regs[1] & 0x08) {
		mode |= 4;
    }
    
    tms9918_linerenderers[mode](vdp);

    if (!(mode & 1)) {
		tms9918_render_sprites(vdp, cur_vbp);
    }

}

int tms9918_periodic(tms9918 *vdp,int scanline) {
    vdp->scanline = scanline;
    
	if (vdp->scanline < 192) {
		
		tms9918_render_line(vdp);
				
    } else if (scanline == 192) {
					
		vdp->status |= 0x80; /* signal vblank */

		// Interrupt enabled?
		if (vdp->regs[1] & 0x20) {
			// Vblank interrupt.
			z80_set_nmi_line(ASSERT_LINE);
		}
	
	}
    
	vdp->scanline++;
    
    return (vdp->status & 0x80) && (vdp->regs[1] & 0x20);
}

/*
 * Need to figure out the colors!
 */
void tms9918_init_palette(tms9918 *vdp) {
    int i;

	// Very rough conversion to 332, need to hand craft a better one.
	// We have 256 colors to choose from inherited from the SMS.
    for (i = 0; i < 16; i++) {
		vdp->palette[i] = ((tms9918_palbase_red[i] >> 5) << 5) | (tms9918_palbase_green[i] >> 5) << 2 | tms9918_palbase_blue[i] >> 6;
    }
    
}

tms9918 *tms9918_create(void) {
    tms9918 *retval;

    retval = calloc( 1,  sizeof(tms9918) );
    
	if (retval) {
		
		retval->memory = calloc(1, TMS_RAMSIZE);
		
		if (retval->memory) {
		    retval->scanline = 0;
		    tms9918_init_palette(retval);
		} else {
		    free(retval);
		    retval = NULL;
		}
		
    }
    
    if (!retval) {
		/* deb_printf("tms9918_create(): out of memory.\n"); */
    }

    return retval;
}

