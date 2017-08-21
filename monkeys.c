/*
The MIT License (MIT)

Copyright (c) 2015 David Catt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/*I apologize in advance for the messy coding style.*/

// Modifications:
//   replaced comparisons with -1 by comparisons with 255
//   set `mov' to 0 for opcode NOP
//   removed unused variable `idx'


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#define LINESZ 512
#define INITSZ 2048
#define GROWTH 1.41421356237
#define ADJACENT(ax,ay,bx,by) (abs(ax-bx)<2&&abs(ay-by)<2)
#define RANDOM (((z=36969*(z&65535)+(z>>16))+(w=18000*(w&65535)+(w>>16)))>>4)
#define ISEOL(a) (a=='\r'||a=='\n'||!a)
unsigned long z,w;
typedef unsigned char BYTE;
/* Each instruction is bits 0..4 opcode and bits 5..7 monkey number */
enum Instructions {
	NOP = 0,
	UP = 1, /* Movement will increment the monkey if there are no adjacent monkeys */
	DOWN = 2, /* Movement will decrement the monkey if it hits an edge */
	LEFT = 3,
	RIGHT = 4,
	LEARN = 5, /* Read value from console */
	YELL = 6, /* Print value to console */
	PLAY = 7, /* Randomize the monkey's value */
	SLEEP = 8, /* Sleep only if not carrying banana */
	WAKE = 9,
	GRAB = 10,
	DROP = 11,
	EAT = 12,
	MARK = 13, /* Remember program counter */
	BACK = 14, /* Set program counter */
	TEACH = 15,
	FIGHT = 16,
	BOND = 17,
	EGO = 18,
	END = 19
};
typedef struct Banana {
	BYTE position;
	BYTE state; /* 0 = eaten, 1 = on grid, 2 = carried */
} Banana;
typedef struct Monkey {
	BYTE value;
	BYTE position;
	BYTE x;
	BYTE y;
	BYTE awake;
	BYTE banana;
	size_t memory;
} Monkey;

BYTE bcount; /* global banana count */
BYTE bgrid[100]; /* banana counts */
Banana bananas[14];
Monkey monkeys[07];
BYTE*instructions;
size_t programIdx;

int loadProgram(FILE* fi) {
	char buf[LINESZ];
	BYTE cur, tmp, *tpt;
	size_t psz = 0, bsz = INITSZ;
	if(!(instructions = (BYTE*)malloc(bsz))) return 1;
	while(fgets(buf, LINESZ, fi)) {
		cur = buf[0] - '1';
		if(cur < 7) {
			if(buf[1] == ' ') {
				cur <<= 5;
				/* Find instruction */
				tmp = buf[2];
				if(tmp == 'B') {
					tmp = buf[3];
					if(tmp == 'A') {
						if(buf[4] == 'C' && buf[5] == 'K' && ISEOL(buf[6])) cur |= BACK;
					} else if(tmp == 'O') {
						if(buf[4] == 'N' && buf[5] == 'D' && ISEOL(buf[6])) cur |= BOND;
					}
				} else if(tmp == 'D') {
					tmp = buf[3];
					if(tmp == 'O') {
						if(buf[4] == 'W' && buf[5] == 'N' && ISEOL(buf[6])) cur |= DOWN;
					} else if(tmp == 'R') {
						if(buf[4] == 'O' && buf[5] == 'P' && ISEOL(buf[6])) cur |= DROP;
					}
				} else if(tmp == 'E') {
					tmp = buf[3];
					if(tmp == 'A') {
						if(buf[4] == 'T' && ISEOL(buf[5])) cur |= EAT;
					} else if(tmp == 'G') {
						if(buf[4] == 'O' && ISEOL(buf[5])) cur |= EGO;
					}
				} else if(tmp == 'F') {
					if(buf[3] == 'I' && buf[4] == 'G' && buf[5] == 'H' && buf[6] == 'T' && ISEOL(buf[7])) cur |= FIGHT;
				} else if(tmp == 'G') {
					if(buf[3] == 'R' && buf[4] == 'A' && buf[5] == 'B' && ISEOL(buf[6])) cur |= GRAB;
				} else if(tmp == 'L') {
					if(buf[3] == 'E') {
						tmp = buf[4];
						if(tmp == 'A') {
							if(buf[5] == 'R' && buf[6] == 'N' && ISEOL(buf[7])) cur |= LEARN;
						} else if(tmp == 'F') {
							if(buf[5] == 'T' && ISEOL(buf[6])) cur |= LEFT;
						}
					}
				} else if(tmp == 'M') {
					if(buf[3] == 'A' && buf[4] == 'R' && buf[5] == 'K' && ISEOL(buf[6])) cur |= MARK;
				} else if(tmp == 'P') {
					if(buf[3] == 'L' && buf[4] == 'A' && buf[5] == 'Y' && ISEOL(buf[6])) cur |= PLAY;
				} else if(tmp == 'R') {
					if(buf[3] == 'I' && buf[4] == 'G' && buf[5] == 'H' && buf[6] == 'T' && ISEOL(buf[7])) cur |= RIGHT;
				} else if(tmp == 'S') {
					if(buf[3] == 'L' && buf[4] == 'E' && buf[5] == 'E' && buf[6] == 'P' && ISEOL(buf[7])) cur |= SLEEP;
				} else if(tmp == 'T') {
					if(buf[3] == 'E' && buf[4] == 'A' && buf[5] == 'C' && buf[6] == 'H' && ISEOL(buf[7])) cur |= TEACH;
				} else if(tmp == 'U') {
					if(buf[3] == 'P' && ISEOL(buf[4])) cur |= UP;
				} else if(tmp == 'W') {
					if(buf[3] == 'A' && buf[4] == 'K' && buf[5] == 'E' && ISEOL(buf[6])) cur |= WAKE;
				} else if(tmp == 'Y') {
					if(buf[3] == 'E' && buf[4] == 'L' && buf[5] == 'L' && ISEOL(buf[6])) cur |= YELL;
				}
				/* Output instruction if neccesary */
				if(cur & 31) {
					if((psz + 2) >= bsz) {
						if(!(tpt = (BYTE*)realloc(instructions, bsz *= GROWTH))) { free(instructions); return 1; }
						instructions = tpt;
					}
					instructions[psz++] = cur;
				}
			}
		}
	}
	instructions[psz] = END;
	return 0;
}
void initState(void) {
	time_t t; char*outline = "..!1.!...........2!..........!.3.!.............!...!....!.....5.!4........6...!.......!....7......!.";
	size_t idx;Monkey*mptr;
	bcount = 0;
	for(idx = 0; idx < 100; ++idx, ++outline) {
		if(*outline == '!' || *outline == '6' || *outline == '7') {
			bananas[bcount].state = 1;
			bananas[bcount].position = idx;
			++bcount; bgrid[idx] = 1;
		} else bgrid[idx] = 0;
		if(*outline >= '1' && *outline <= '7') {
			mptr = &(monkeys[*outline - '1']);
			mptr->value = 0;
			mptr->position = idx;
			mptr->x = idx % 10;
			mptr->y = idx / 10;
			mptr->awake = 1;
			mptr->banana = 255;
			mptr->memory = 255;
		}
	}
	/* Set auxilary */
	programIdx = 0;
	/* Setup RNG */
	time(&t);
	z = w = clock() + t;
}
void runProgram(void) {
	BYTE opcode, monkey, target, tmp, mov, adj;
	Monkey*mptr,*optr; size_t idx;
	for(; (opcode = instructions[programIdx]) != END; ++programIdx) {
		monkey = opcode >> 5; opcode &= 31;
		mptr = &(monkeys[monkey]);
		if(mptr->awake) {
			if(opcode < 5) {
				target = mptr->position;
				switch(opcode) {
					case UP: if(mptr->y > 0) { target -= 10; --mptr->y; mov = 1; } else mov = 0; break;
					case DOWN: if(mptr->y < 9) { target += 10; ++mptr->y; mov = 2; } else mov = 0; break;
					case LEFT: if(mptr->x > 0) { --target; --mptr->x; mov = 3; } else mov = 0; break;
					case RIGHT: if(mptr->x < 9) { ++target; ++mptr->x; mov = 4; } else mov = 0; break;
					default: mov = 0;
				}
				if(mov) { /*If monkey can move*/
					adj = 0;
					for(idx = 0; idx < 7; ++idx) {
						if(idx != monkey) {
							optr = &(monkeys[idx]);
							if(optr->position == target) {
								if(!optr->awake) optr->awake = 1;
								else if((mptr->banana == 255) ^ (optr->banana == 255)) {
									tmp = mptr->banana;
									optr->banana = mptr->banana;
									mptr->banana = tmp;
								} else { ++mptr->value; ++optr->value; }
								switch(mov) {
									case 1: ++mptr->y; break;
									case 2: --mptr->y; break;
									case 3: ++mptr->x; break;
									case 4: --mptr->x; break;
								}
								mov = 0; break;
							} else {
								adj |= ADJACENT(mptr->x, mptr->y, optr->x, optr->y); /*Keep track of adjacency*/
							}
						}
					}
					if(mov) {
						if(!adj) ++mptr->value;
						mptr->position = target;
					}
				} else --mptr->value;
			} else {
				switch(opcode) {
					case LEARN: mptr->value = getchar(); break;
					case YELL: putchar(mptr->value); break;
					case PLAY: mptr->value = RANDOM; break;
					case SLEEP: if(mptr->banana == 255) mptr->awake = 0; break; /*Sleep if monkey is not carrying banana*/
					/*case WAKE:*/
					case GRAB:
						if(mptr->banana == 255 && bgrid[mptr->position]) {
							for(idx = 0; idx < 14; ++idx) {
								if(bananas[idx].state == 1) { /*If banana exists and is not held*/
									bananas[idx].state = 2; /*Set banana to held*/
									mptr->banana = idx;
									break;
								}
							}
							--bgrid[mptr->position];
						}
						break;
					case DROP:
						tmp = mptr->banana;
						if(tmp != 255) {
							bananas[tmp].state = 1; /*Set banana to not held*/
							bananas[tmp].position = mptr->position;
							++bgrid[mptr->position];
							mptr->banana = 255;
						}
						break;
					case EAT:
						if(mptr->banana != 255) {
							bananas[mptr->banana].state = 0; /*Set banana to nonexistant*/
							mptr->banana = 255;
							if(!--bcount) return;
						}
						break;
					case MARK: mptr->memory = programIdx; break;
					case BACK: programIdx = mptr->memory; break;
					case TEACH:
						tmp = mptr->value;
						for(idx = 0; idx < 7; ++idx) {
							if(idx != monkey) {
								if(ADJACENT(mptr->x, mptr->y, monkeys[idx].x, monkeys[idx].y)) {
									monkeys[idx].value += tmp;
								}
							}
						}
						break;
					case FIGHT:
						tmp = mptr->value;
						for(idx = 0; idx < 7; ++idx) {
							if(idx != monkey) {
								if(ADJACENT(mptr->x, mptr->y, monkeys[idx].x, monkeys[idx].y)) {
									monkeys[idx].value -= tmp;
								}
							}
						}
						break;
					case BOND:
						tmp = mptr->value;
						for(idx = 0; idx < 7; ++idx) {
							if(idx != monkey) {
								if(ADJACENT(mptr->x, mptr->y, monkeys[idx].x, monkeys[idx].y)) {
									monkeys[idx].value *= tmp;
								}
							}
						}
						break;
					case EGO:
						tmp = mptr->value;
						for(idx = 0; idx < 7; ++idx) {
							if(idx != monkey) {
								if(ADJACENT(mptr->x, mptr->y, monkeys[idx].x, monkeys[idx].y)) {
									monkeys[idx].value /= tmp;
								}
							}
						}
						break;
				}
			}
		} else if(opcode == WAKE) {
			mptr->awake = 1;
		}
	}
}
void freeState(void) {
	free(instructions);
}

int main(int argc, char** argv) {
	FILE*f;
	if(argc == 2) {
		if((f = (*argv[1] == ':' ? stdin : fopen(argv[1], "rb")))) {
			if(!loadProgram(f)) {
				fclose(f);
				initState();
				runProgram();
				freeState();
				return 0;
			} else { fclose(f); fputs("ERROR: Could not load program\n", stderr); return 1; }
		} else { fputs("ERROR: Could not open program file\n", stderr); return 1; }
	} else { fputs(argc < 2 ? "usage: Monkeys [program_file]\n" : "ERROR: too many arguments\n", stderr); return 1; }
}
