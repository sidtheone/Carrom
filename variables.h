/*!
 * Contains varibales used in differnt files
 * 
 * Copyright (c) 2010 by sidharth
 */
#ifndef VARIABLES_H
#define VARIABLES_H
#include <vector>
#include "object.h"
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <al/al.h>
#include <al/alc.h>
#include <al/alut.h>
//board characterstics
#define BOARD_BREADTH			740
#define BOARD_LENGTH			740
#define BOARD_HEIGHT			4
#define BOARD_BOUNDARY_HEIGHT	70
#define BOARD_BOUNDARY_BREADTH	80
#define BOARD_POCKET_RADIUS		28
//game states
#define STATE_BOARD_PLACE_STRIKER 0
#define STATE_BOARD_SIMULATION 1
#define STATE_BOARD_STRIKER_DIRECTION 2
#define STATE_BOARD_STRIKER_POWER 3
//sounds characterstics
#define SOUND_POCKET 3
#define SOUND_CARROMMEN_COLLISION 0
#define SOUND_CARROMMEN_WALL 2
#define SOUND_STRIKER_WALL 1
#define SOUND_POWER_BAR 4
using namespace std;
extern int state , w, h;
extern float strikerSpeed, angle;
//object lists
extern vector<Object *> objectVector ;
extern vector<Object *> cdObjectVector;
//Quadrics list
extern GLuint startList;
//Audio Sources List
extern ALuint AudioSource[];
#endif
