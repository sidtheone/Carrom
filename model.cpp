/*!
 * Defination of the functions related to model creation and initalization
 * 
 * Copyright (c) 2010 by sidharth
 */
#include "model.h"
#include "variables.h"
#include "object.h"
#include "vector3D.h"






/*!
 * \brief
 * Used to initalizise the first state of the carrom board
 * 
 * 
 *  This function defines various objects like carromboard , carromMen , striker and their coordinates and their velocities
 * 
 * \remarks
 * will work if board center coordinates are (0, 0) 
 * 
 */
void initCarromBoardModel()
{
	//creating and setting up carrom board
	CarromBoard *carromBoard = new CarromBoard(BOARD_LENGTH, BOARD_BREADTH, BOARD_HEIGHT, BOARD_BOUNDARY_HEIGHT, BOARD_BOUNDARY_BREADTH,BOARD_POCKET_RADIUS);
	objectVector.push_back(carromBoard);
	//cdObjectVector.push_back(carromBoard);

	//creating and setting up carrom men
	for(int i=0;i<1;i++)
	{ 
		CarromMen *carromMen = new CarromMen(Vector3D(0,0,0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_RED);
		objectVector.push_back(carromMen);
		cdObjectVector.push_back(carromMen);

		CarromMen *carromMen1 = new CarromMen(Vector3D(0,  (2 * CARROM_MEN_RADIUS  +1) , 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_BLACK);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);

		carromMen1 = new CarromMen(Vector3D( (2 * CARROM_MEN_RADIUS  +1) * cos(30 * PI /DEGREE),  (2 * CARROM_MEN_RADIUS  +1) * sin(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_WHITE);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(2* (2 * CARROM_MEN_RADIUS  - 4) * sin(30 * PI /DEGREE), 2* (2 * CARROM_MEN_RADIUS  - 4) * cos(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_BLACK);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(2* (2 * CARROM_MEN_RADIUS  +1) * cos(30 * PI /DEGREE), 2* (2 * CARROM_MEN_RADIUS  +1) * sin(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_WHITE);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(0, 2* (2 * CARROM_MEN_RADIUS  +1) , 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_WHITE);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(2* (2 * CARROM_MEN_RADIUS  - 4), 0 , 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_BLACK);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
	
	//mirror 1


		carromMen1 = new CarromMen(Vector3D(- (2 * CARROM_MEN_RADIUS  +1) * cos(30 * PI /DEGREE),  (2 * CARROM_MEN_RADIUS  +1) * sin(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_WHITE);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(-2* (2 * CARROM_MEN_RADIUS  - 4) * sin(30 * PI /DEGREE), 2* (2 * CARROM_MEN_RADIUS  - 4) * cos(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_BLACK);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(-2* (2 * CARROM_MEN_RADIUS  +1) * cos(30 * PI /DEGREE), 2* (2 * CARROM_MEN_RADIUS  +1) * sin(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_WHITE);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		

		
		carromMen1 = new CarromMen(Vector3D(-2* (2 * CARROM_MEN_RADIUS  - 4), 0 , 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2,OBJECT_CARROM_MEN_BLACK);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
	
	//mirror 2
		
		carromMen1 = new CarromMen(Vector3D(0, - (2 * CARROM_MEN_RADIUS  +1) , 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2,OBJECT_CARROM_MEN_WHITE);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);

		carromMen1 = new CarromMen(Vector3D( (2 * CARROM_MEN_RADIUS  +1) * cos(30 * PI /DEGREE), - (2 * CARROM_MEN_RADIUS  +1) * sin(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2,OBJECT_CARROM_MEN_BLACK);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(2* (2 * CARROM_MEN_RADIUS  - 4) * sin(30 * PI /DEGREE), -2* (2 * CARROM_MEN_RADIUS  - 4) * cos(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2,OBJECT_CARROM_MEN_BLACK);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(2* (2 * CARROM_MEN_RADIUS  +1) * cos(30 * PI /DEGREE), -2* (2 * CARROM_MEN_RADIUS  +1) * sin(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2,OBJECT_CARROM_MEN_WHITE);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
	

		carromMen1 = new CarromMen(Vector3D(- (2 * CARROM_MEN_RADIUS  +1) * cos(30 * PI /DEGREE), - (2 * CARROM_MEN_RADIUS  +1) * sin(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2,OBJECT_CARROM_MEN_BLACK);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(-2* (2 * CARROM_MEN_RADIUS  - 4) * sin(30 * PI /DEGREE),- 2* (2 * CARROM_MEN_RADIUS  - 4) * cos(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_BLACK);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(-2* (2 * CARROM_MEN_RADIUS  +1) * cos(30 * PI /DEGREE),- 2* (2 * CARROM_MEN_RADIUS  +1) * sin(30 * PI /DEGREE), 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2, OBJECT_CARROM_MEN_WHITE);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);
		
		carromMen1 = new CarromMen(Vector3D(0, -2 * (2 * CARROM_MEN_RADIUS  +1) , 0), Vector3D(0,0,0), Vector3D(0,0,0), 0.2, CARROM_MEN_RADIUS, CARROM_MEN_MASS, 2,OBJECT_CARROM_MEN_BLACK);
		objectVector.push_back(carromMen1);
		cdObjectVector.push_back(carromMen1);

		
	}
	//creating striker
	CarromMen *carromMen2 = new CarromMen( Vector3D(0,-290,0) ,Vector3D(0 ,0 , 0), Vector3D(0, 0, 0), 0.2, CARROM_STRIKER_RADIUS, CARROM_STRIKER_MASS, 2, OBJECT_STRIKER );
	
	objectVector.push_back(carromMen2);
	cdObjectVector.push_back(carromMen2);

}

/*!
 * \brief
 * Function to define quadric models
 *  
 * Function defines various quadric models for later usage like  disk , cylinder , cone
 * 
 * \remarks
 * Defines models as primities that can be scaled for suiting the needs
 * 
 */
void initQuadricModels()
{	
	GLUquadricObj *qobj;
	startList = glGenLists(4);
	qobj = gluNewQuadric();
	//Cylinder Quadric Model
	gluQuadricDrawStyle(qobj, GLU_FILL); /* filled polygon */
	gluQuadricNormals(qobj, GLU_SMOOTH);
	glNewList(startList, GL_COMPILE);
	gluCylinder(qobj, 1.0, 1.0, 1.0, 50, 5);
	glEndList();
	//Disc Quadric Model
	gluQuadricDrawStyle(qobj, GLU_FILL); /* filled polygon */
	gluQuadricNormals(qobj, GLU_SMOOTH);
	glNewList(startList+1, GL_COMPILE);
	gluDisk(qobj, 0.0, 1.0, 50, 4);
	glEndList();
	//Ring quadric model
	gluQuadricDrawStyle(qobj, GLU_FILL); /* filled polygon */
	gluQuadricNormals(qobj, GLU_SMOOTH);
	glNewList(startList+2, GL_COMPILE);
	gluDisk(qobj, 0.92, 1.0, 50, 4);
	glEndList();
	//Cone quadric model
	gluQuadricDrawStyle(qobj, GLU_FILL); /* filled polygon */
	gluQuadricNormals(qobj, GLU_SMOOTH);
	glNewList(startList+3, GL_COMPILE);
	gluCylinder(qobj, .5, 0.001, 1.0, 50, 5);
	glEndList();


}