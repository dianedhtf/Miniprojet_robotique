/*
**File : 	control_robot.h
**Author : 	Diane d'Haultfoeuille & Viviane Blanc
**Date : 	16 mai 2022
*/

#ifndef CONTROL_ROBOT_H_
#define CONTROL_ROBOT_H_

//DEFINE DIFFERENT STATES OF THE ROBOT
#define SEARCH_LINE 			0
#define	SEARCH_DIRECTION		1
#define REDLINE_FOUND			2
#define BLUELINE_FOUND 			3
#define AVOID_RIGHT_OBSTACLE	4
#define AVOID_LEFT_OBSTACLE		5
#define FLAG_FOUND				6
#define GAME_OVER				7
#define END_PROGRAMM			8

//DEFINE CONSTANT FOR CONTROL
// in millimeters, for TOF
#define FREE_WAY 				100		//DISTANCE MIN INDIQUANT UNE NOUVELLE DIRECTION
#define DIST_SHOOTING_MIN		50		//DISTANCE MIN DE LAQUELLE ON PEUT TIRER
#define DIST_SHOOTING_MAX		90		//DISTANCE MAX DE LAQUELLE ON PEUT TIRER
#define SECURITY_DIST_TOF		40		//DISTANCE A LAQUELLE SE PLACER POUR CHERCHER NEW_DIRECTION

//For the IR proximity sensors
#define SECURITY_DIST_IR		400     //DISTANCE DE SECURITE AVEC LES MURS

//Different speed
#define SPEED_CORRECTION		100		//UNITS ????
#define SPEED_MOTOR				400
#define TURNING_SPEED			600

#define QUARTER_TURN_TIME		550

#define OFF						0
#define ON 						1

void control_robot_start(void);

#endif /* CONTROL_ROBOT_H_ */