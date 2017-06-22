// Copyright (c) 2017 Brian Barto
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GPL License. See LICENSE for more details.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "main.h"
#include "modules/mirrorfield.h"

/*
 * MODULE DESCRIPTION
 * 
 * The mirrorfield module manages the loading, validating, and traversing
 * of the mirror field. The cryptographic algorithm is implemented here.
 * If the debug flag is set then this module also draws the mirror field
 * and animates the encryption process.
 */

#define MIRROR_NONE      -1
#define MIRROR_FORWARD   -2
#define MIRROR_STRAIGHT  -3
#define MIRROR_BACKWARD  -4
#define DIR_UP            1
#define DIR_DOWN          2
#define DIR_LEFT          3
#define DIR_RIGHT         4

// Static Variables
//static int grid[GRID_SIZE * GRID_SIZE];
//static unsigned char perimeterChars[GRID_SIZE * 4];

struct gridnode {
	int value;
	struct gridnode *up;
	struct gridnode *down;
	struct gridnode *left;
	struct gridnode *right;
};
static struct gridnode gridnodes[MIRROR_FIELD_COUNT][GRID_SIZE * GRID_SIZE];
static struct gridnode perimeter[MIRROR_FIELD_COUNT][GRID_SIZE * 4];

// Static Function Prototypes
static struct gridnode *mirrorfield_crypt_char_advance(struct gridnode *, int, int, int);
static void mirrorfield_roll_chars(int, int, int);
static void mirrorfield_draw(struct gridnode *, int);

/*
 * The mirrorfield_init() function initializes any static variables.
 */
void mirrorfield_init(void) {
	int i, j;
	
	for (j = 0; j < MIRROR_FIELD_COUNT; ++j) {

		// Init gridnode values
		for (i = 0; i < GRID_SIZE * GRID_SIZE; ++i) {
			gridnodes[j][i].value = 0;
			gridnodes[j][i].up = NULL;
			gridnodes[j][i].down = NULL;
			gridnodes[j][i].left = NULL;
			gridnodes[j][i].right = NULL;
		}
		
		// Init perimeter values
		for (i = 0; i < GRID_SIZE * 4; ++i) {
			perimeter[j][i].value = 0;
			perimeter[j][i].up = NULL;
			perimeter[j][i].down = NULL;
			perimeter[j][i].left = NULL;
			perimeter[j][i].right = NULL;
		}
	}
}

/*
 * The mirrorfield_set() function accepts one character at a time and
 * loads them sequentially into the static variables that contain the
 * mirror field and perimeter characters.
 * 
 * Zero is returned if it gets a character it doesn't expect, although
 * this is just a cursory error checking process. 
 */
int mirrorfield_set(unsigned char ch) {
	static int i = 0;
	static int j = 0;
	
	// Set mirror values
	if (i < GRID_SIZE * GRID_SIZE * MIRROR_FIELD_COUNT) {

		// Set mirror field index
		j = i / (GRID_SIZE * GRID_SIZE);
	
		// Set mirror value
		if (ch == '/') {
			gridnodes[j][i % (GRID_SIZE * GRID_SIZE)].value = MIRROR_FORWARD;
		} else if (ch == '\\') {
			gridnodes[j][i % (GRID_SIZE * GRID_SIZE)].value = MIRROR_BACKWARD;
		} else if (ch == '-') {
			gridnodes[j][i % (GRID_SIZE * GRID_SIZE)].value = MIRROR_STRAIGHT;
		} else if (ch == ' ') {
			gridnodes[j][i % (GRID_SIZE * GRID_SIZE)].value = MIRROR_NONE;
		} else {
			return 0;
		}

	}
	
	// Set perimiter values
	else if (i < (GRID_SIZE * GRID_SIZE * MIRROR_FIELD_COUNT) + (GRID_SIZE * 4 * MIRROR_FIELD_COUNT)) {
		
		// Set mirror field index
		j = (i - (GRID_SIZE * GRID_SIZE * MIRROR_FIELD_COUNT)) / (GRID_SIZE * 4);
		
		// Setting perimeter value by index
		perimeter[j][(i - (GRID_SIZE * GRID_SIZE * MIRROR_FIELD_COUNT)) % (GRID_SIZE * 4)].value = (int)ch;
	} 
	
	// Ignore extra characters
	else {
		return 0;
	}
	
	// Increment our static counter
	++i;
	
	return 1;
}

/*
 * The mirrorfield_validate() function checks that the data contained in
 * the static variables for the mirror field and perimeter character is
 * valid.
 * 
 * Zero is returned if invalid.
 */
int mirrorfield_validate(void) {
	int i, j, k;

	// Check mirrors
	for (k = 0; k < MIRROR_FIELD_COUNT; ++k) {
		for (i = 0; i < GRID_SIZE * GRID_SIZE; ++i) {
			if (gridnodes[k][i].value > MIRROR_NONE || gridnodes[k][i].value < MIRROR_BACKWARD) {
				return 0;
			}
		}
	}
	
	// Check for duplicate perimeter chars
	for (k = 0; k < MIRROR_FIELD_COUNT; ++k) {
		for (i = 0; i < GRID_SIZE * 4; ++i) {
			for (j = i+1; j < GRID_SIZE * 4; ++j) {
				if (perimeter[k][i].value == perimeter[k][j].value) {
					return 0;
				}
			}
		}
	}
	
	return 1;
}

/*
 * The mirrorfield_link() function creates links between nodes to speed
 * up the encryption/decryption process.
 */
void mirrorfield_link(void) {
	int i, j, k;
	struct gridnode *temp;
	
	// Looping over each mirror field
	for (k = 0; k < MIRROR_FIELD_COUNT; ++k) {

		// Linking up/down
		for (i = 0; i < GRID_SIZE; ++i) {

			temp = &perimeter[k][i];
	
			for (j = i; j < GRID_SIZE * GRID_SIZE; j += GRID_SIZE) {
				temp->down = &gridnodes[k][j];
				gridnodes[k][j].up = temp;
				temp = &gridnodes[k][j];
			}
			
			temp->down = &perimeter[k][i + (GRID_SIZE * 2)];
			perimeter[k][i + (GRID_SIZE * 2)].up = temp;
		}

		// Linking right/left
		for (i = 0; i < GRID_SIZE; ++i) {
			
			temp = &perimeter[k][i + (GRID_SIZE * 3)];
			
			for (j = i * GRID_SIZE; j < (i * GRID_SIZE) + GRID_SIZE; ++j) {
					temp->right = &gridnodes[k][j];
					gridnodes[k][j].left = temp;
					temp = &gridnodes[k][j];
			}
			
			temp->right = &perimeter[k][i + GRID_SIZE];
			perimeter[k][i + GRID_SIZE].left = temp;
		}

	}
}

/*
 * The mirrorfield_crypt_char() function receives a cleartext character
 * and traverses the mirror field to find it's cyphertext equivelent,
 * which is then returned. It also calls mirrorfield_roll_chars() after
 * the cyphertext character is determined.
 */
unsigned char mirrorfield_crypt_char(unsigned char ch, int debug) {
	int i, d;
	unsigned char sv, ev, rv;
	struct gridnode *startnode = NULL;
	struct gridnode *endnode = NULL;
	static int m = 0;
	
	// Get starting node
	for (i = 0; i < GRID_SIZE * 4; ++i) {
		if (perimeter[m][i].value == (int)ch) {
			startnode = &perimeter[m][i];
			break;
		}
	}
	
	// Set initial direction
	if (startnode->down != NULL) {
		d = DIR_DOWN;
	} else if (startnode->up != NULL) {
		d = DIR_UP;
	} else if (startnode->left != NULL) {
		d = DIR_LEFT;
	} else if (startnode->right != NULL) {
		d = DIR_RIGHT;
	}
	
	// Traverse the mirror field and find the cyphertext node
	endnode = mirrorfield_crypt_char_advance(startnode, d, m, debug);
	
	// Store start/end values before we roll them
	sv = startnode->value;
	ev = endnode->value;
	rv = ev;
	
	// Roll start and end values
	mirrorfield_roll_chars(sv, ev, m);
	
	// This is a way of returning the cleartext char as the cyphertext
	// char and still preserve decryption.
	if (perimeter[m][(ev+sv)%(GRID_SIZE*4)].value == (ev+sv)%(GRID_SIZE*4)) {
		rv = sv;
	}
	
	// Cycle mirror field index
	m = (m + 1) % MIRROR_FIELD_COUNT;
	
	return rv;
}

/*
 * The mirrorfield_crypt_char_advance() is a recursive function that traverses
 * the mirror field and returns a pointer to the node containing the cypthertext
 * character. This function also handles mirror rotation.
 */
static struct gridnode *mirrorfield_crypt_char_advance(struct gridnode *p, int d, int m, int debug) {
	struct gridnode *t;
	
	// For the debug flag
	struct timespec ts;
	ts.tv_sec = debug / 1000;
	ts.tv_nsec = (debug % 1000) * 1000000;
	if (debug) {
		mirrorfield_draw(p, m);
		fflush(stdout);
		nanosleep(&ts, NULL);
	}

	// Advance character
	switch (d) {
		case DIR_DOWN:
			p = p->down;
			break;
		case DIR_LEFT:
			p = p->left;
			break;
		case DIR_RIGHT:
			p = p->right;
			break;
		case DIR_UP:
			p = p->up;
			break;
	}
	
	// If we don't have the cypthertext, determine new direction and perform
	// a recursive call.
	if (p->value < 0) {

		switch (p->value) {
			case MIRROR_FORWARD:
				switch (d) {
					case DIR_DOWN:
						d = DIR_LEFT;
						break;
					case DIR_LEFT:
						d = DIR_DOWN;
						break;
					case DIR_RIGHT:
						d = DIR_UP;
						break;
					case DIR_UP:
						d = DIR_RIGHT;
						break;
				}
				break;
			case MIRROR_BACKWARD:
				switch (d) {
					case DIR_DOWN:
						d = DIR_RIGHT;
						break;
					case DIR_LEFT:
						d = DIR_UP;
						break;
					case DIR_RIGHT:
						d = DIR_DOWN;
						break;
					case DIR_UP:
						d = DIR_LEFT;
						break;
				}
				break;
				
		}
		
		// Perform recursive call. t will be our cyphertext node.
		t = mirrorfield_crypt_char_advance(p, d, m, debug);
		
		// Rotate mirror after we get cyphertext
		switch (p->value) {
			case MIRROR_FORWARD:
				p->value = MIRROR_STRAIGHT;
				break;
			case MIRROR_BACKWARD:
				p->value = MIRROR_FORWARD;
				break;
			case MIRROR_STRAIGHT:
				p->value = MIRROR_BACKWARD;
				break;
		}
		
		// After rotating mirror at p, assign it the cyphertext node.
		p = t;
	}
	
	// Return cyphertext node
	return p;
}

/*
 * The mirrorfield_roll_chars() function received the starting and ending
 * nodes of the cleartext and cyphertext character respectively, and
 * implements a character rolling process to reposition the nodes and
 * increase randomness in the output. No value is returned.
 */
static void mirrorfield_roll_chars(int s, int e, int m) {
	int i, t, x1, x2;
	static int g1 = 0;
	static int g2 = GRID_SIZE * 2;
	static int c = 0;

	// Get rotate order
	if (perimeter[m][s].value > perimeter[m][e].value) {
		x1 = s;
		x2 = e;
	} else {
		x1 = e;
		x2 = s;
	}

	// Get perimeter index for value x1
	for (i = 0; perimeter[m][i].value != x1; ++i);
		;
	// Rotate x1 to new position.
	t = perimeter[m][i].value;
	perimeter[m][i].value = perimeter[m][g1].value;
	perimeter[m][g1].value = t;
	
	// Get perimeter index for value x2
	for (i = 0; perimeter[m][i].value != x2; ++i);
		;
	// Rotate x1 to new position.
	t = perimeter[m][i].value;
	perimeter[m][i].value = perimeter[m][g2].value;
	perimeter[m][g2].value = t;
	
	// The g holds the roll position
	if (++c == MIRROR_FIELD_COUNT) {
		g1 = (g1 + 1) % (GRID_SIZE * 4);
		g2 = (g2 + 1) % (GRID_SIZE * 4);
		c = 0;
	}
	
	return;
}

/*
 * The mirrorfield_draw() function draws the current state of the mirror
 * field and perimeter characters. It receives x/y coordinates and highlights
 * that position on the field.
 */
static void mirrorfield_draw(struct gridnode *p, int m) {
	int r, c;
	static int resetCursor = 0;
	
	// Save cursor position if we need to reset it
	// Otherwise, clear screen.
	if (resetCursor)
		printf("\033[s");
	else
		printf("\033[2J");

	// Set cursor position to 0,0
	printf("\033[H");
	
	for (r = -1; r <= GRID_SIZE; ++r) {

		for (c = -1; c <= GRID_SIZE; ++c) {

			// Highlight cell if r/c match
			if (r >= 0 && r < GRID_SIZE && c >= 0 && c < GRID_SIZE && &gridnodes[m][(r * GRID_SIZE) + c] == p) {
				printf("\x1B[30m"); // foreground black
				printf("\x1B[47m"); // background white
			}
			
			// Print apropriate mirror field character
			if (r == -1 && c == -1) {                        // Upper left corner
				printf("%2c", ' ');
			} else if (r == -1 && c == GRID_SIZE) {          // Upper right corner
				printf("%2c", ' ');
			} else if (r == GRID_SIZE && c == -1) {          // Lower left corner
				printf("%2c", ' ');
			} else if (r == GRID_SIZE && c == GRID_SIZE) {   // Lower right corner
				printf("%2c", ' ');
			} else if (r == -1) {                            // Top chars
				printf("%2x", gridnodes[m][c].up->value);
			} else if (c == GRID_SIZE) {                     // Right chars
				printf("%2x", gridnodes[m][(r * GRID_SIZE) + (GRID_SIZE-1)].right->value);
			} else if (r == GRID_SIZE) {                     // Bottom chars
				printf("%2x", gridnodes[m][c + (GRID_SIZE * 3)].down->value);
			} else if (c == -1) {                            // Left chars
				printf("%2x", gridnodes[m][r * GRID_SIZE].left->value);
			} else if (gridnodes[m][(r * GRID_SIZE) + c].value == MIRROR_FORWARD) {
				printf("%2c", '/');
			} else if (gridnodes[m][(r * GRID_SIZE) + c].value == MIRROR_BACKWARD) {
				printf("%2c", '\\');
			} else if (gridnodes[m][(r * GRID_SIZE) + c].value == MIRROR_STRAIGHT) {
				printf("%2c", '-');
			} else {
				printf("%2c", ' ');
			}

			// Un-Highlight cell if r/c match
			if (r >= 0 && r < GRID_SIZE && c >= 0 && c < GRID_SIZE && &gridnodes[m][(r * GRID_SIZE) + c] == p)
				printf("\x1B[0m");

		}
		printf("\n");
	}
	printf("\n");
	
	// Restore cursor position if we need to
	if (resetCursor)
		printf("\033[u");
	else
		resetCursor = 1;
}
