/*!
* Defines and declares objects and their methods
* 
* Copyright (c) 2010 by sidharth
*/
#ifndef OBJECT_H
#define OBJECT_H

#include "variables.h"
#include "vector3D.h"



#define OBJECT_BOARD_SURFACE	0
#define OBJECT_BOARD_BOUNDARY	1
#define OBJECT_BOARD_POCKETS	2
#define OBJECT_CARROM_MEN_RED	3
#define OBJECT_CARROM_MEN_BLACK 4
#define OBJECT_CARROM_MEN_WHITE 5
#define OBJECT_STRIKER			6
#define OBJECT_CARROM_BOARD		7




/*!
* \brief
* Object class Base class for other types of object
* 
* Contains a field typeof that tells of the type of the object
* 
* \remarks
* need to make this abstract
* 
*/
class Object
{ 
public:

	int objectType;



};


/*!
* \brief
* Board Surface Class Declration 
* 
* Used to store information about the board strucutre
* 
* \remarks
* Can add material properties
* 
*/
class BoardSurface : public Object
{
public:

	float length, breadth, height;
	/*!
	* \brief
	* Constructor
	* 
	* \param length
	* 
	* 
	* \param breadth
	* 
	* 
	* \param height
	* 
	* Write detailed description for BoardSurface here.
	* 
	*/
	BoardSurface(float length, float breadth, float height) : length(length), breadth(breadth), height(height)
	{
		objectType = OBJECT_BOARD_SURFACE;
	}
};


/*!
* \brief
*  BoardBoundary Class Declration
* 
* Contains dimensions of the board boundary
* 
* 
*/
class BoardBoundary : public Object
{
public:

	float breadth, height;
	/*!
	* \brief
	* Constructor of BoardBoundary
	* 
	* \param breadth
	* 
	* \param height
	*/
	BoardBoundary(float breadth,float height) : breadth(breadth), height(height)
	{
		objectType = OBJECT_BOARD_BOUNDARY;
	}
};


/*!
 * \brief
 *Booard Pocket Object Defination
 * 
 *Contains the decription fields of Pocket Object
 * 
 */
class BoardPocket : public Object
{
public:

	float radius;
	Vector3D position;

	/*!
	 * \brief
	 * Constructor
	 * 
	 * \param radius
	 */
	BoardPocket(float radius) : radius(radius)
	{
		objectType = OBJECT_BOARD_POCKETS;
	}
	/*!
	 * \brief
	 * Default Constructor  
	 * 
	 * 
	 */
	BoardPocket()
	{radius=0;
	objectType = OBJECT_BOARD_POCKETS;
	}
	/*!
	 * \brief
	 * Function to set radius
	 * 
	 * \param radius
	 */
	void setRadius(float Rad)
	{
		radius=Rad;
	}
};


/*!
 * \brief
 * Carrom Board Class
 * 
 * Contains objects of boardBoundary Boardpockts boardsurface and handles initialization of them
 * 
 * \remarks
 * Write remarks for CarromBoard here.
 * 
 * \see
 * Separate items with the '|' character.
 */
class CarromBoard : public Object
{
public:
	BoardBoundary boardBoundary;
	BoardPocket boardPockets[4];
	BoardSurface boardSurface;

	/*!
	 * \brief	
	 * 
	 * 
	 * \param length
	 * 
	 * 
	 * \param breadth
	 * 
	 * 
	 * \param height
	 * 
	 * 
	 * \param boundaryHeight
	 * .
	 * 
	 * \param boundaryBreadth
	 * 
	 * 
	 * \param pocketRadius
	 * .
	 * 
	 */
	CarromBoard(float length, float breadth, float height, float boundaryHeight, float boundaryBreadth, float pocketRadius) : boardBoundary(boundaryBreadth,boundaryHeight), boardSurface(length,breadth,height)
	{
		//write code for board pockets
		boardPockets[0].setRadius(pocketRadius);
		boardPockets[0].position.Set(((length/2) - pocketRadius), ((length/2) - pocketRadius),0);
		boardPockets[1].setRadius(pocketRadius);
		boardPockets[1].position.Set(-((length/2) - pocketRadius), ((length/2) - pocketRadius),0);
		boardPockets[2].setRadius(pocketRadius);
		boardPockets[2].position.Set(((length/2) - pocketRadius), -((length/2) - pocketRadius),0);
		boardPockets[3].setRadius(pocketRadius);
		boardPockets[3].position.Set(-((length/2) - pocketRadius), -((length/2) - pocketRadius),0);		
		objectType = OBJECT_CARROM_BOARD;	
	}

};


/*!
 * \brief
 * CarromMen class to store data about Carrom Mens and Striker
 * 
 * Contains Velocity Force position and visible fields;
 * 
 * \remarks
 * Write remarks for CarromMen here.
 * 
 * \see
 * Separate items with the '|' character.
 */
class CarromMen : public Object
{
public:
	Vector3D position, velocity, force;
	float uFriction, radius, mass, height;
	bool visible;
	bool colliding;
	/*!
	 * \brief
	 *
	 * 
	 * \param position
	 * 
	 * 
	 * \param velocity
	 * 
	 * 
	 * \param force
	 * 
	 * 
	 * \param uFriction
	 * 
	 * 
	 * \param radius
	 * 
	 * 
	 * \param height
	 *
	 * 
	 * \param mass
	 *
	 * 
	 * \param objType
	 * 
	 */
	CarromMen(Vector3D position, Vector3D velocity, Vector3D force, float uFriction, float radius,float height, float mass, int objType) : position(position), velocity(velocity), force(force), uFriction(uFriction), mass(mass), radius(radius), height(height)
	{
		objectType=objType;
		visible = true;
		colliding = false;
	}
};

#endif
