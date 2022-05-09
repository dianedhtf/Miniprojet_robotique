/*
 * control_robot.c
 *  Author: Diane d'Haultfoeuille and Viviane Blanc
 */

//voir ce qui est utile en include
//BIEN REVOIR LES TEMPS DE SLEEP
//CE QU'ON POURRa ajouter ==> s'il détecte une ligne, s'assurer qu'il tire bien dessus
//ATTENTION CA COMMENCE TJS A CLOGNOTER EN ROUGE
//VERIFIER QU'IL SE COCGNE PAS UN MUR

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
#include <main.h>				//Obligatoire ??
#include <sensors/proximity.h>
#include <sensors/VL53L0X/VL53L0X.h>

#include <msgbus/messagebus.h>

//DEFINE DIFFERENT STATES OF THE ROBOT
#define SEARCH_LINE 			0		//est-ce qu'on ne changerait pas les 2, pour commencer avec new_direction ??
#define	SEARCH_DIRECTION		1
#define REDLINE_FOUND			2
#define BLUELINE_FOUND 			3
//#define AVOID_OBSTACLE_RIGHT	4
//#define AVOID_OBSTACLE_LEFT		5
#define NO_INSTRUCTIONS 		5

//DEFINE CONSTANT FOR CONTROL ==> DISTANCE A VERIFIER
#define FREE_WAY 				200		//DISTANCE MIN INDIQUANT UNE NOUVELLE DIRECTION
#define DIST_SHOOTING_MIN		60		//DISTANCE MIN A PARTIR DE LAQUELLE ON PEUT TIRER
#define DIST_SHOOTING_MAX		80		//DISTANCE MAX A PARTIR DE LAQUELLE ON PEUT TIRER
#define DIST_MIN_MUR			60		//DISTANCE A LAQUELLE SE PLACER POUR CHERCHER NEW_DIRECTION
#define SECURITY_DISTANCE		300     //DISTANCE DE SECURITE AVEC LES MURS
#define SPEED_CORRECTION		300
#define SPEED_MOTOR				400
#define TURNING_SPEED			300

//Static global variables for the different states
static int state_robot = 0;
static bool ready_to_search_direction = false;
static bool ready_to_search_line = true;
//static bool right_obstacle_detected = false;
//static bool left_obstacle_detected = false;

//Static global variables for distance sensors ==> A DECLARER EN STATIC ?? OU SIMPLEMENT DANS UNE FONCTION NORMALE ??
//PAS DE CORRESPONDANCE ENTRE IRnumber et get_prox(number)
static int right_speed_correction = 0;
static int left_speed_correction = 0;

//semaphore
static BSEMAPHORE_DECL(detect_obstacle_done, TRUE);

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

	motor_stop();					//utile ??
	chThdSleepMilliseconds(500);	//utile ?

	//ON TOURNE DE 90° A DROITE
	set_speed_motor(TURNING_SPEED,-TURNING_SPEED);
	chThdSleepMilliseconds(1050);
	motor_stop();
	chThdSleepMilliseconds(500);

	if (VL53L0X_get_dist_mm() > FREE_WAY) {
		return true; //On doit sortir de la boucle
	} else {
		//ON TOURNE DE 180° POUR TROUVER UNE NOUVELLE DIRECTION
		set_speed_motor(-TURNING_SPEED, TURNING_SPEED);
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
	while (VL53L0X_get_dist_mm() > distance_max) {
		set_speed_motor(SPEED_MOTOR, SPEED_MOTOR);
	}

	while (VL53L0X_get_dist_mm() < distance_min) {
		set_speed_motor(-SPEED_MOTOR, -SPEED_MOTOR);
	}
		motor_stop();
}

int detection_obstacle(int proximity_sensor_1, int proximity_sensor_2, led_name_t led_number){
	if ((proximity_sensor_1 < SECURITY_DISTANCE) && (proximity_sensor_2 < SECURITY_DISTANCE)) { //Rien à faire
		return 0;
	} else if ((proximity_sensor_1> SECURITY_DISTANCE-50) || (proximity_sensor_2> SECURITY_DISTANCE-50)){ //On doit corriger
		set_led(led_number, 1);
		chThdSleepMilliseconds(200);
		set_led(led_number,0);
		chThdSleepMilliseconds(200);
		return SPEED_CORRECTION;
	}
}

//void detection_obstacle_all_sensors(void){
//
//	//Different proximity sensors
//	int right_dist_IR3 = get_prox(2);
//	int right_dist_IR2 = get_prox(1);
//	int left_dist_IR6 = get_prox(5);
//	int left_dist_IR7 = get_prox(6);
//
//	right_speed_correction = detection_obstacle(right_dist_IR3, right_dist_IR2);
//	left_speed_correction = detection_obstacle(left_dist_IR6, left_dist_IR7);
//
//
////    if(dist_TOF >50 && dist_TOF<60 ){
////    	speed_robot = search_a_way();
////    	}
//}

uint8_t get_mode(void) {

//	//Different proximity sensors
//	int right_dist_IR3 = get_prox(2);
//	int right_dist_IR2 = get_prox(1);
//	int left_dist_IR6 = get_prox(5);
//	int left_dist_IR7 = get_prox(6);
//
//	set_led(LED3, 1);
//			chThdSleepMilliseconds(200);
//			set_led(LED3,0);
//			chThdSleepMilliseconds(200);

//	chprintf((BaseSequentialStream *)&SD3, "Get_mode test = %dus\n ",SECURITY_DISTANCE);

//	if ((right_dist_IR3> SECURITY_DISTANCE-50) || (right_dist_IR2> SECURITY_DISTANCE-50)) {
//		return AVOID_OBSTACLE_RIGHT;
//	} else if ((left_dist_IR6> SECURITY_DISTANCE-50) || (left_dist_IR7> SECURITY_DISTANCE-50)) {
//		return AVOID_OBSTACLE_LEFT;
	if (VL53L0X_get_dist_mm() < DIST_MIN_MUR) { //Peut être changer ça
		return SEARCH_DIRECTION;
	}

	if (ready_to_search_line == true) {
		if (get_blueline_not_found() == 0) {
			return REDLINE_FOUND;
		} else if (get_redline_not_found() == 0) {
			return BLUELINE_FOUND;
		} else {
			return SEARCH_LINE;
		}
	} else if (ready_to_search_direction == true){
		return SEARCH_DIRECTION;
	} else {
		return NO_INSTRUCTIONS;
	}
}

/***************************************************************************/
static THD_WORKING_AREA(waDetectObstacle, 256);
static THD_FUNCTION(DetectObstacle, arg) {

	chRegSetThreadName(__FUNCTION__);
	    (void)arg;

	    while(1) {

	    //Different proximity sensors
	    int right_dist_IR3 = get_prox(2);
	    int right_dist_IR2 = get_prox(1);
	    int left_dist_IR6 = get_prox(5);
	    int left_dist_IR7 = get_prox(6);

	    right_speed_correction = detection_obstacle(right_dist_IR3, right_dist_IR2, LED3);
	    left_speed_correction = detection_obstacle(left_dist_IR6, left_dist_IR7, LED7);

	    //signals an image has been captured
	    chBSemSignal(&detect_obstacle_done);
	    }
}



static THD_WORKING_AREA(waControlRobot, 256);
static THD_FUNCTION(ControlRobot, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    while(1) {
    	//waits until speed_correction has been updated
    	chBSemWait(&detect_obstacle_done);
    	state_robot = get_mode();					//Même pas obligé, mais jsp comment ne pas faire pour que ça clignote pas au début.

    	switch(state_robot) {

			case SEARCH_LINE:
				ready_to_search_direction = false;
				ready_to_search_line = true;

				all_leds_off();
//				detection_obstacle_all_sensors();
				set_speed_motor(SPEED_MOTOR + left_speed_correction, SPEED_MOTOR + right_speed_correction);			//MOVE STRAIGHT AWAY //EST CE QU'ON IMPOSE UNE VITESSE = SPEED_ROBOT
//				set_speed_motor(SPEED_MOTOR, SPEED_MOTOR);
			break;

			case SEARCH_DIRECTION:
				ready_to_search_direction = false;
				ready_to_search_line = search_new_direction();
			break;

    		case REDLINE_FOUND:
    			ready_to_search_line = false;
    			ready_to_search_direction = true;
    			adjust_position(DIST_SHOOTING_MAX, DIST_SHOOTING_MIN);
    			blink_front_led();
    		break;

    	    case BLUELINE_FOUND:
    	    	ready_to_search_line = false;
    	    	ready_to_search_direction = true;
				motor_stop();
    	    	active_body_led();
    	    	adjust_position(DIST_MIN_MUR, DIST_MIN_MUR);
    	    break;

//    	    case AVOID_OBSTACLE_RIGHT:
//    	    	set_speed_motor(SPEED_MOTOR, SPEED_MOTOR + SPEED_CORRECTION);
//    	    break;
//
//    	    case AVOID_OBSTACLE_LEFT:
//    	    	set_speed_motor(SPEED_MOTOR + SPEED_CORRECTION, SPEED_MOTOR);
//    	    break;

    	    case NO_INSTRUCTIONS:
    	    	motor_stop();
    	    break;
    	}
    }
}

void control_robot_start(void) {
	chThdCreateStatic(waControlRobot, sizeof(waControlRobot), NORMALPRIO, ControlRobot, NULL);
}

void detect_obstacle_start(void) {
	chThdCreateStatic(waDetectObstacle, sizeof(waDetectObstacle), NORMALPRIO, DetectObstacle, NULL);
}