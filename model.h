/*!
 * Declaration of the functions related to model creation and initalization
 * 
 * Copyright (c) 2010 by sidharth
 */
#ifndef MODEL_H
#define MODEL_H




#define CARROM_MEN_RADIUS 18
#define DEGREE 180
#define PI 3.141
#define CARROM_STRIKER_RADIUS 22
#define CARROM_MEN_MASS 12
#define CARROM_STRIKER_MASS 18





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
void initCarromBoardModel();

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
void initQuadricModels();
#endif
