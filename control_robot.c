/*
 * control_robot.c
 * Author: Diane d'Haultfoeuille and Viviane Blanc

A CHECKER : les include sont-ils tous utiles ? Temps pour les chSleep
*/

#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <leds.h>
#include <motors.h>
#include <process_image.h>
#include <sensors/proximity.h>
#include <sensors/VL53L0X/VL53L0X.h>
#include <main.h>	

#include <msgbus/messagebus.h>			

//DEFINE DIFFERENT STATES OF THE ROBOT
#define SEARCH_LINE 			0
#define	SEARCH_DIRECTION		1
#define REDLINE_FOUND			2
#define BLUELINE_FOUND 			3
#define AVOID_OBSTACLE			4
#define NO_INSTRUCTIONS 		5

//DEFINE CONSTANT FOR CONTROL ==> DISTANCE A VERIFIER
#define FREE_WAY 			200		//DISTANCE MIN INDIQUANT UNE NOUVELLE DIRECTION
#define DIST_SHOOTING_MIN		50		//DISTANCE MIN A PARTIR DE LAQUELLE ON PEUT TIRER
#define DIST_SHOOTING_MAX		80		//DISTANCE MAX A PARTIR DE LAQUELLE ON PEUT TIRER
#define DIST_MIN_MUR			60		//DISTANCE A LAQUELLE SE PLACER POUR CHERCHER NEW_DIRECTION

//Static global variables			
static bool ready_to_search_line = true;
static bool ready_to_search_new_direction = false;

uint8_t get_mode(void) {

	if (ready_to_search_line == true){
		if (get_blueline_not_found() == 0) {
			return REDLINE_FOUND;
		} else if (get_redline_not_found() == 0) {
			return BLUELINE_FOUND;
		} else {
			return SEARCH_LINE;
	} else if (ready_to_search_direction == true) {
			return SEARCH_DIRECTION;
	}
}

/********************* FONCTIONS FOR MOTOR **********************************/

void set_speed_motor(int speed_l, int speed_r) {
	left_motor_set_speed(speed_l);
	right_motor_set_speed(speed_r);
}

void motor_stop(void){
	set_speed_motor(0,0);
}
/****************************************************************************/

/********************* FONCTIONS FOR LEDS ***********************************/
void blink_front_led(void) {

	for (int i=0; i<3; ++i) {
		set_front_led(1);
		chThdSleepMilliseconds(500);
		set_front_led(0);
		chThdSleepMilliseconds(500);
	}
}

void active_body_led(void){
	set_body_led(1);
	chThdSleepMilliseconds(500);		//Met en pause et permet à d'autres taches de fonctionner en même temps
	set_body_led(0);
}

void all_leds_off(void){
	set_body_led(0);
	set_front_led(0);
}

/*****************************************************************************/

/******************************** ACTIONS ************************************/
bool search_new_direction(void){

	motor_stop();				//utile ??
	chThdSleepMilliseconds(500);	//utile ?

	//ON TOURNE DE 90° A DROITE
	set_speed_motor(300,-300);
	chThdSleepMilliseconds(1050);
	motor_stop();
	chThdSleepMilliseconds(500);

	if (VL53L0X_get_dist_mm() > FREE_WAY) {
		return true; //On doit sortir de la boucle
	} else {
		//ON TOURNE DE 180° POUR TROUVER UNE NOUVELLE DIRECTION
		set_speed_motor(-300, 300);
		chThdSleepMilliseconds(2150);
		motor_stop();
		if (VL53L0X_get_dist_mm() > FREE_WAY) {
			return true;
		} else {
			return false; //ON FAIT QUOI AVEC ??
		}
	}
}

void adjust_position(uint8_t distance_max, uint8_t distance_min) {
	motor_stop();
	if (VL53L0X_get_dist_mm() > distance_max) {
		do {
			set_speed_motor(400, 400);
		} while (VL53L0X_get_dist_mm() > distance_max);
	} else if (VL53L0X_get_dist_mm() < distance_min) {
		do {
			set_speed_motor(-400, -400);
		} while (VL53L0X_get_dist_mm() < distance_min);
	} else {
		motor_stop();
	}
}

/***************************************************************************/

//Marche pas très bien == a cause de la lumiere
static THD_WORKING_AREA(waControlRobot, 256);
static THD_FUNCTION(ControlRobot, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    while(1) {

    	switch(get_mode()) {

		case SEARCH_LINE:
			ready_to_search_direction = false;
			ready_to_search_line = true;

			all_leds_off();
			set_speed_motor(400,400);			//MOVE STRAIGHT AWAY //EST CE QU'ON IMPOSE UNE VITESSE = SPEED_ROBOT
    	   	 break;

		case SEARCH_DIRECTION:
			ready_to_search_direction = false;
			ready_to_search_line = search_new_direction();
		break;

    		case REDLINE_FOUND:
    			ready_to_search_line = false;			//Donc garde normalement l'etat de get_blue/RED_line donc normalement ne va plus détecter une ligne
			ready_to_search_direction = true;
			motor_stop();    			
			blink_front_led();
		break;

    	       case BLUELINE_FOUND:
    	    	   	ready_to_search_line = false;
			ready_to_search_direction = true;
			motor_stop();
    	    	   	active_body_led();
    	    	break;

		case AVOID_OBSTACLE:
    	    		ready_to_search_line = false;
    	    		motor_stop();
    	    	break;

    	    	case NO_INSTRUCTIONS:
    	    		motor_stop();
    	    	break;
    	}
    }
}

void control_robot_start(void) {
	chThdCreateStatic(waControlRobot, sizeof(waControlRobot), NORMALPRIO, ControlRobot, NULL);
}

