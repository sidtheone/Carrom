/*!
 * Interface for the mouse and keyboard handling
 * 
 * Copyright (c) 2010 by sidharth
 */
#ifndef INTERFACE_H
#define INTERFACE_H

#define SET 1
#define NOT_SET 0

#include <al/al.h>
#include <al/alc.h>
#include <al/alut.h>
//#include "sound.h"
/*!
* \brief
* Method to process mouse action to move from one screen to another 
* 
* \param button
* Tells which button is pressed
* 
* \param stateMouse
* Tells the state of the mouse button .
* 
* \param x
*  coordinate x of screen 
* 
* \param y
* coordinate y of screen 
* 
* 
* Method to process mouse action to move from one screen to another 
* 
*/
void processMouse(int button, int stateMouse, int x, int y) ;





/*!
* \brief
* Function to process the mouse movement and convert it to the angle of the striker
* 
* \param x
* location x of mouse
* 
* \param y
* location y of mouse
* 
* 
* Function to process the mouse movement and convert it to the angle of the striker
* 
* \remarks
* none
* 
*/
void processMouseActiveMotion(int x, int y) ;


/*!
* \brief
* Method used to process keyboard functionalities 
* 
* \param key
* which key is pressed
* 
* \param x
* Description of parameter x.
* 
* \param y
* Description of parameter y.
* 
* 
* Method used to process keyboard functionalities
* 
* \remarks
* none
*/
void keyboard (unsigned char key, int x, int y);



#endif
