#include "battle_city.h"
#include "map.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xio.h"
#include <math.h>

/*		SCREEN PARAMETERS		 - in this case, "screen" stands for one full-screen picture	 */
#define SCREEN_BASE_ADDRESS			6900
#define SCR_HEIGHT					30
#define SCR_WIDTH					40

/*		FRAME HEADER		*/
#define HEADER_BASE_ADDRESS			7192

/*      FRAME       */
#define FRAME_BASE_ADDRESS			7392 // FRAME_OFFSET in battle_city.vhd
#define SIDE_PADDING				12
#define VERTICAL_PADDING			7

/*		find out what the below lines stand for		 */
#define MAP_X							0
#define MAP_X2							640
#define MAP_Y							4
#define MAP_W							64
#define MAP_H							56
/*		find out what the above lines stand for		 */

#define REGS_BASE_ADDRESS               ( SCREEN_BASE_ADDRESS + SCR_WIDTH * SCR_HEIGHT )

#define BTN_DOWN( b )                   ( !( b & 0x01 ) )
#define BTN_UP( b )                     ( !( b & 0x10 ) )
#define BTN_LEFT( b )                   ( !( b & 0x02 ) )
#define BTN_RIGHT( b )                  ( !( b & 0x08 ) )
#define BTN_SHOOT( b )                  ( !( b & 0x04 ) )

/*			these are the beginnings and endings of registers that store moving sprites		 */
#define TANK1_REG_L                     8
#define TANK1_REG_H                     9
#define TANK_AI_REG_L                   4
#define TANK_AI_REG_H                   5
#define TANK_AI_REG_L2                  6
#define TANK_AI_REG_H2                  7
#define TANK_AI_REG_L3                  2
#define TANK_AI_REG_H3                  3
#define TANK_AI_REG_L4                  10
#define TANK_AI_REG_H4                  11
#define TANK_AI_REG_L5                  12
#define TANK_AI_REG_H5                  13
#define TANK_AI_REG_L6                  14
#define TANK_AI_REG_H6                  15
#define TANK_AI_REG_L7                  16
#define TANK_AI_REG_H7                  17
#define BASE_REG_L						0
#define BASE_REG_H	                    1

int lives = 0;
int score = 0;
int mapPart = 1;
int udario_glavom_skok = 0;
int map_move = 0;
int brojac = 0;
int udario_u_blok = 0;

/*			16x16 IMAGES		 */
unsigned short SPRITES[53] = {0x00FF, 0x013F, 0x017F, 0x01BF, 0x01FF, 0x023F, 0x027F, 0x02BF, 0x02FF, 0x033F, 0x037F, 0x03BF, 0x03FF, 0x043F, 0x047F, 0x04BF, 0x04FF, 0x053F, 0x057F, 0x05BF, 0x05FF, 0x063F, 0x067F, 0x06BF, 0x06FF, 0x073F, 0x077F, 0x07BF, 0x07FF, 0x083F, 0x087F, 0x08BF, 0x08FF, 0x093F, 0x097F, 0x09BF, 0x09FF, 0x0A3F, 0x0A7F, 0x0ABF, 0x0AFF, 0x0B3F, 0x0B7F, 0x0BBF, 0x0BFF, 0x0C3F, 0x0C7F, 0x0CBF, 0x0CFF, 0x0D3F, 0x0D7F, 0x0DBF, 0x0DFF, 0x0E3F };

/*		 ACTIVE FRAME		*/
unsigned short* frame;

typedef enum {
	b_false, b_true
} bool_t;

typedef enum {
	DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_STILL
} direction_t;

typedef struct {
	unsigned int x;
	unsigned int y;
	direction_t dir;
	unsigned int type;

	bool_t destroyed;

	unsigned int reg_l;
	unsigned int reg_h;
} characters;

characters mario = { 
		240,	                        // x
		240, 		                    // y
		DIR_RIGHT,              		// dir
		0x02BF,							// type - sprite address in ram.vhdl
		b_false,                		// destroyed
		TANK1_REG_L,            		// reg_l
		TANK1_REG_H             		// reg_h
		};

/*characters enemie1 = { 331,						// x
		431,						// y
		DIR_LEFT,              		// dir
		IMG_16x16_enemi1,  		// type

		b_false,                		// destroyed

		TANK_AI_REG_L,            		// reg_l
		TANK_AI_REG_H             		// reg_h
		};

characters enemie2 = { 450,						// x
		431,						// y
		DIR_LEFT,              		// dir
		IMG_16x16_enemi1,  		// type

		b_false,                		// destroyed

		TANK_AI_REG_L2,            		// reg_l
		TANK_AI_REG_H2             		// reg_h
		};

characters enemie3 = { 330,						// x
		272,						// y
		DIR_LEFT,              		// dir
		IMG_16x16_enemi1,  		// type

		b_false,                		// destroyed

		TANK_AI_REG_L3,            		// reg_l
		TANK_AI_REG_H3             		// reg_h
		};

characters enemie4 = { 635,						// x
		431,						// y
		DIR_LEFT,              		// dir
		IMG_16x16_enemi1,  		// type

		b_false,                		// destroyed

		TANK_AI_REG_L4,            		// reg_l
		TANK_AI_REG_H4             		// reg_h
		};
*/
int overw_x = 0;
int overw_y = 0;
void load_frame(direction_t dir) {
    switch(dir) {
        case DIR_LEFT:
            overw_x = --overw_x<0? 0 : overw_x;
            break;
        case DIR_RIGHT:
            overw_x = ++overw_x>15? 15 : overw_x;
            break;
        case DIR_UP:
            overw_y = --overw_y<0? 0 : overw_y;
            break;
        case DIR_DOWN:
            overw_x = ++overw_x>11? 11 : overw_y;
            break;
    }
    frame = overwold[overw_y*16 + overw_x];

    /*      loading next frame into memory      */
    int x,y;
    long int addr;
    for (y = 0; y < FRAME_HEIGHT; y++) {
		for (x = 0; x < FRAME_WIDTH; x++) {
			addr = XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * (FRAME_BASE_ADDRESS + y * (SIDE_PADDING + FRAME_WIDTH + SIDE_PADDING) + x);
			Xil_Out32(addr, frame[y*FRAME_WIDTH + x]);
		}
	}
    /*      TODO: add logic for updating the overworld position in header   */
    /*  idea: 1x2 gray sprites, position is 2x2 pixels     */
}

unsigned int rand_lfsr113(void) {
	static unsigned int z1 = 12345, z2 = 12345;
	unsigned int b;

	b = ((z1 << 6) ^ z1) >> 13;
	z1 = ((z1 & 4294967294U) << 18) ^ b;
	b = ((z2 << 2) ^ z2) >> 27;
	z2 = ((z2 & 4294967288U) << 2) ^ b;

	return (z1 ^ z2);
}

static void chhar_spawn(characters * chhar) {
	Xil_Out32(
			XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( REGS_BASE_ADDRESS + chhar->reg_l ),
			(unsigned int )0x8F000000 | (unsigned int )chhar->type);
	Xil_Out32(
			XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( REGS_BASE_ADDRESS + chhar->reg_h ),
			(chhar->y << 16) | chhar->x);
}

static void map_update(characters * mario) {
	int x, y, i;
	long int addr;

	if (mario->x >= 330) {
		if (udario_u_blok <= 0) {
			map_move++;
		}
		if (mario->x == 2560) {
			map_move = 2520;
		}
	}

	for (y = 0; y < FRAME_HEIGHT; y++) {
		for (x = 0; x < FRAME_WIDTH; x++) {
			addr = XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * (FRAME_BASE_ADDRESS + y * (SIDE_PADDING + FRAME_WIDTH + SIDE_PADDING) + x);
			Xil_Out32(addr, frame[y*FRAME_WIDTH + x]);
		}
	}
}
// currently, this function is cleaning the registers used for movind characters sprites; two registers are used for each sprite
static void map_reset() {
	/* Colors in palette in ram.vhdl */
	/*0 =>	x"00C84C0C", --orange/red/brown
	1 =>	x"00FCD8A8",
	2 =>	x"00000000",
	3 =>	x"002038EC",
	4 =>	x"0000A800", --green
	5 =>	x"00FCFCFC",
	6 =>	x"00747474",
	7 =>	x"00C0C0C0", */
	
	/* setting the colors from rgb to bgr format - only 3 because the others are simetrical */
	Xil_Out32(XPAR_BATTLE_CITY_PERIPH_0_BASEADDR, (unsigned int )0x000C4CC8);
	Xil_Out32(XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * (1), (unsigned int )0x00ABD8FC);
	Xil_Out32(XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * (3), (unsigned int )0x00EC3820);

	unsigned int i;
	long int addr;

	for(i = 0; i<SCR_WIDTH*SCR_HEIGHT; i++) {
		addr = XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * (SCREEN_BASE_ADDRESS + i);
		Xil_Out32(addr, SPRITES[10]); // SPRITES[10] is a black square
	}

	for (i = 0; i <= 20; i += 2) {
		Xil_Out32(
				XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( REGS_BASE_ADDRESS + i ),
				(unsigned int )0x0F000000);
	}
}

static bool_t mario_move(characters * mario, direction_t dir, int start_jump) {
	unsigned int x;
	unsigned int y;
	int i, j;

	float Xx;
	float Yy;
	int roundX = 0;
	int roundY = 0;

	int obstackle = 0;

    if (mario->x > ((MAP_X + FRAME_WIDTH) * 16 - 16)
			|| mario->y > (MAP_Y + FRAME_HEIGHT) * 16 - 16) {
		return b_false;
	}

	x = mario->x;
	y = mario->y;

	if (dir == DIR_LEFT) {
		//if (x > MAP_X * 16) {
			x--;
		//}
	} else if (dir == DIR_RIGHT) {
		x++;
	} else if (dir == DIR_UP) {
		y--; //added
		/*if (y > MAP_Y * 16) {
			for (i = 0; i < 30; i++) {
				y--;
				brojac++;

				mario->y = y;

				Xil_Out32(
						XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( REGS_BASE_ADDRESS + mario->reg_h ),
						(mario->y << 16) | mario->x);
				for (j = 0; j < 100000; j++) {
				}
			}
		}
		while (brojac != 0) {
			y++;
			brojac--;
		}*/
	} else if (dir == DIR_DOWN) {
		y++; //added
	}
	/*if (start_jump == 1) {
		if (y > MAP_Y * 16) {
			for (i = 0; i < 30; i++) {
				y--;
				brojac++;
				if (dir == DIR_LEFT)
					x--;
				else
					x++;
				mario->x = x;
				mario->y = y;

				Xil_Out32(
						XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( REGS_BASE_ADDRESS + mario->reg_h ),
						(mario->y << 16) | mario->x);
				for (j = 0; j < 100000; j++) {
				}
			}
		}
	}*/
	while (brojac != 0) {
		y++;
		brojac--;
	}
/*
	Xx = x;
	Yy = y;

	if (dir == DIR_LEFT) {
		obstackle = obstackles_detection(x, y, mapPart, map, 2);
	} else if (dir == DIR_RIGHT) {
		obstackle = obstackles_detection(x, y, mapPart, map, 1);
	}

	roundX = floor(Xx / 16);
	roundY = floor(Yy / 16);

	switch (obstackle) {
	case 0:{
		udario_u_blok = 0;
	}
	break;
	case 2: {
		//blok
		if (dir == DIR_LEFT) {
			if (x > MAP_X * 16) {
				x++;
			}
		} else if (dir == DIR_RIGHT) {
			x--;
		}
		udario_u_blok = 1;
	}
		break;
	case 3: {
		//cigla

		if (dir == DIR_LEFT) {
			if (x > MAP_X * 16) {
				x++;
			}
		} else if (dir == DIR_RIGHT) {
			x--;
		}
		udario_u_blok = 1;
	}
		break;
	case 5: {
		//coin
		score++;
		map1[roundY + 1][roundX + 1] = 0;
		map_update(&mario);
	}
		break;
	default:
		udario_u_blok = 0;
	}
*/
	mario->x = x;
	mario->y = y;

	Xil_Out32(
			XPAR_BATTLE_CITY_PERIPH_0_BASEADDR + 4 * ( REGS_BASE_ADDRESS + mario->reg_h ),
			(mario->y << 16) | mario->x);

	return b_false;
}

int obstackles_detection(int x, int y, int deoMape, /*unsigned char * map,*/
		int dir) {
/*	unsigned char mario_position_right;
	unsigned char mario_position_left;
	unsigned char mario_position_up;

	float Xx = x;
	float Yy = y;

	int roundX = 0;
	int roundY = 0;

	roundX = floor(Xx / 16);
	roundY = floor(Yy / 16);

	mario_position_right = map1[roundY + 1][roundX + 1];
	mario_position_left = map1[roundY + 1][roundX];
	mario_position_up = map1[roundY + 1][roundX];

	if (dir == 1) {
		switch (mario_position_right) {
		case 0:
			return 0;
			break;
		case 1:
			return 1;
			break;
		case 2:
			return 2;
			break;
		case 3:
			return 3;
			break;
		case 4:
			return 4;
			break;
		case 5:
			return 5;
			break;

		}
	} else if (dir == 2) {
		switch (mario_position_left) {
		case 0:
			return 0;
			break;
		case 1:
			return 1;
			break;
		case 2:
			return 2;
			break;
		case 3:
			return 3;
			break;
		case 4:
			return 4;
			break;
		case 5:
			return 5;
			break;

		}
	} else if (dir == 3) {
		switch (mario_position_up) {
		case 0:
			return 0;
			break;
		case 1:
			return 1;
			break;
		case 2:
			return 2;
			break;
		case 3:
			return 3;
			break;
		case 4:
			return 4;
			break;
		case 5:
			return 5;
			break;

		}
	}
*/
}

void battle_city() {

	unsigned int buttons, tmpBtn = 0, tmpUp = 0;
	int i, change = 0, jumpFlag = 0;
	int block;
	frame = overworld[0];
	map_reset(/*map1*/);
	map_update(&mario);

	//chhar_spawn(&enemie1);
	//chhar_spawn(&enemie2);
	//chhar_spawn(&enemie3);
	//chhar_spawn(&enemie4);
	chhar_spawn(&mario);

	while (1) {

		buttons = XIo_In32( XPAR_IO_PERIPH_BASEADDR );

		direction_t d = DIR_STILL;
		if (BTN_LEFT(buttons)) {
			d = DIR_LEFT;
		} else if (BTN_RIGHT(buttons)) {
			d = DIR_RIGHT;
		} else if (BTN_UP(buttons)) {
			d = DIR_UP;
		} else if (BTN_DOWN(buttons)) {
			d = DIR_DOWN;
		}

        load_frame(d);        

		int start_jump = 0;
		if (BTN_UP (buttons)/* && (BTN_LEFT(buttons) || BTN_RIGHT(buttons))*/) {
			//start_jump = 1;
			d = DIR_UP;
		}
		if (BTN_DOWN (buttons) /*&& !BTN_LEFT(buttons) && !BTN_RIGHT(buttons)*/) {
			d = DIR_DOWN;
		}
		//mario_move(/*map1,*/ &mario, d, start_jump);

		//map_update(&mario);

		for (i = 0; i < 1000000; i++) {
		}

	}
}
