/*
** File : 		control_robot.c
** Author : 	Diane d'Haultfoeuille & Viviane Blanc
** Date : 		16 mai 2022
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
#include <control_robot.h>
#include <sensors/proximity.h>
#include <sensors/VL53L0X/VL53L0X.h>
#include <audio/play_melody.h>
#include <audio/audio_thread.h>

#include <msgbus/messagebus.h>

//Static global variables (boolean) to check the robot state
static bool ready_to_search_direction = false;
static bool ready_to_search_line = true;
static bool end_programm = false;
static bool right_obstacle_detected = false;
static bool left_obstacle_detected = false;

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
void active_body_led(void){
	set_body_led(ON);
	chThdSleepMilliseconds(500);
	set_body_led(OFF);
}

void all_leds_off(void){
	set_body_led(OFF);
	set_front_led(OFF);
	clear_leds();
}

void set_all_leds(void){
	for(int i=0; i<4; i++) {
		set_led(i, 1);
	}
}

void circle_leds(void) {
	for (int j=0; j<3; j++) {
		for(int i=0; i<4; i++) {
			set_led(i, 1);
			chThdSleepMilliseconds(200);
			set_led(i,0);
		}
	}
}
/*****************************************************************************/

/*************************** OTHERS ACTIONS **********************************/
void shooting(void) {
	for (int i=0; i<3; ++i) {
		set_front_led(ON);
		playNote(NOTE_D4, 500);
		set_front_led(OFF);
		stopCurrentMelody();
		chThdSleepMilliseconds(500);
	}
}

bool search_new_direction(void){
	motor_stop();

	//QUARTER TURN
	set_speed_motor(TURNING_SPEED,-TURNING_SPEED);
	chThdSleepMilliseconds(QUARTER_TURN_TIME);
	motor_stop();
	chThdSleepMilliseconds(200);

	if (VL53L0X_get_dist_mm() >= FREE_WAY) {					//the robot has found a new direction
		return true;
	} else {
		//HALF TURN
		set_speed_motor(-TURNING_SPEED, TURNING_SPEED);
		chThdSleepMilliseconds(2*QUARTER_TURN_TIME);
		motor_stop();
		if (VL53L0X_get_dist_mm() >= FREE_WAY) {				//the robot has found a new direction
			return true;
		} else {
			return false;									//new direction not found
		}
	}
}

bool detection_obstacle(uint16_t sensor_1, uint16_t sensor_2, led_name_t led_number){
	if ((sensor_1 <= SECURITY_DIST_IR) && (sensor_2 <= SECURITY_DIST_IR)) {					//no obstacle detected
		set_led(led_number, OFF);
		return false;
	} else if ((sensor_1 > SECURITY_DIST_IR-50) || (sensor_2 > SECURITY_DIST_IR-50)){			//obstacle detected
		set_led(led_number, ON);
		return true;
	}
}

void get_obstacle(void) {

    //Define different proximity sensors
	uint16_t right_dist_IR2 = get_prox(1);
	uint16_t right_dist_IR3 = get_prox(2);
	uint16_t left_dist_IR6 = get_prox(5);
	uint16_t left_dist_IR7 = get_prox(6);

    right_obstacle_detected = detection_obstacle(right_dist_IR3, right_dist_IR2, LED3);
    left_obstacle_detected = detection_obstacle(left_dist_IR6, left_dist_IR7, LED7);
}

void adjust_position(uint8_t distance_max, uint8_t distance_min) {
	motor_stop();

	while (VL53L0X_get_dist_mm() >= distance_max) {
		get_obstacle();
		if (right_obstacle_detected == true) {
			set_speed_motor(SPEED_MOTOR, SPEED_MOTOR + SPEED_CORRECTION);				//turn left
		} else if (left_obstacle_detected == true) {
			set_speed_motor(SPEED_MOTOR + SPEED_CORRECTION, SPEED_MOTOR);				//turn right
		} else {
			set_speed_motor(SPEED_MOTOR, SPEED_MOTOR);									//move forward
		}
	}

	while (VL53L0X_get_dist_mm() < distance_min) {
		get_obstacle();
		if (right_obstacle_detected == true) {
			set_speed_motor(-SPEED_MOTOR, -SPEED_MOTOR - SPEED_CORRECTION);				//turn left
		} else if (left_obstacle_detected == true) {
			set_speed_motor(-SPEED_MOTOR - SPEED_CORRECTION, -SPEED_MOTOR);				//turn right
		} else {
			set_speed_motor(-SPEED_MOTOR, -SPEED_MOTOR);								//back up
		}
	}
		motor_stop();
}

uint8_t get_mode(void) {

	if (ready_to_search_line == true) {
		if (right_obstacle_detected == true) {
			return CORRECTION_RIGHT;
		} else if (left_obstacle_detected == true) {
			return CORRECTION_LEFT;
		} else if (VL53L0X_get_dist_mm() < SECURITY_DIST_TOF) {
			return SEARCH_DIRECTION;
		} else {
			if (get_blueline_not_found() == 0 && get_redline_not_found() == 1) {
				return REDLINE_FOUND;
			} else if (get_redline_not_found() == 0 && get_blueline_not_found() == 1) {
				return BLUELINE_FOUND;
			} else if (get_redline_not_found() == 0 && get_blueline_not_found() == 0) {
				return FLAG_FOUND;
			} else {
				return SEARCH_LINE;
			}
		}
	} else {
		if (ready_to_search_direction == true) {
			return SEARCH_DIRECTION;
		} else {
			if (end_programm == true) {
				return END_PROGRAM;
			} else {
				return GAME_OVER;
			}
		}
	}
}

/***************************************************************************/

static THD_WORKING_AREA(waControlRobot, 256);
static THD_FUNCTION(ControlRobot, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    while(1) {

    	get_obstacle();

    	switch(get_mode()) {

			case SEARCH_LINE:
				ready_to_search_direction = false;

				all_leds_off();
				set_speed_motor(SPEED_MOTOR, SPEED_MOTOR);
				break;

			case SEARCH_DIRECTION:
				ready_to_search_direction = false;
				ready_to_search_line = search_new_direction();
				break;

    		case REDLINE_FOUND:
    			ready_to_search_line = false;
    			ready_to_search_direction = true;

    			playMelody(IMPOSSIBLE_MISSION, 2, 0);
    			chThdSleepMilliseconds(200);
    			//Adjust position to be in the shooting area
    			adjust_position(DIST_SHOOTING_MAX, DIST_SHOOTING_MIN);
    			stopCurrentMelody();
    			//simulate the shooting
    			shooting();
    			//adjust position to be in the area for searching new direction
    			adjust_position(SECURITY_DIST_TOF, SECURITY_DIST_TOF);
    			break;

    	    case BLUELINE_FOUND:
    	    	ready_to_search_line = false;
    	    	ready_to_search_direction = true;

				motor_stop();
				//signals that blueline has been found
    	    	active_body_led();
    	    	//adjust position to be in the area for searching new direction
    	    	adjust_position(SECURITY_DIST_TOF, SECURITY_DIST_TOF);
    	    	break;

    	    case CORRECTION_RIGHT:
    	    	ready_to_search_direction = false;
    	    	//reajust the right position
	    		set_speed_motor(SPEED_MOTOR, SPEED_MOTOR + SPEED_CORRECTION);
    	    	break;

    	    case CORRECTION_LEFT:
    	    	ready_to_search_direction = false;
    	    	//reajust the left position
    	    	set_speed_motor(SPEED_MOTOR + SPEED_CORRECTION, SPEED_MOTOR);
    	    	break;

    	    case FLAG_FOUND:
    	    	ready_to_search_line = false;
    	    	ready_to_search_direction = false;
    	    	end_programm = true;

    	    	motor_stop();
    	    	playMelody(WE_ARE_THE_CHAMPIONS, 2, 0);
    	    	chThdSleepMilliseconds(5000);
    	    	stopCurrentMelody();
    	    	//Waits to Give a break
    	    	set_body_led(ON);
    	    	circle_leds();

    	    	//Waits to give a break
    	    	chThdSleepMilliseconds(BREAK_TIME);
    	    	break;

    	    case GAME_OVER:
    	    	ready_to_search_direction = false;
    	    	ready_to_search_line = false;
    	    	end_programm = true;

    	    	motor_stop();
    	    	set_all_leds();
    	    	playMelody(MARIO_DEATH, 2, 0);
    	    	chThdSleepMilliseconds(5000);
    	    	stopCurrentMelody();

    	    	//Waits to Give a break
    	    	chThdSleepMilliseconds(BREAK_TIME);
    	    	break;

    	    case END_PROGRAM:
    	    	end_programm = false;

    	    	all_leds_off();
    	    	motor_stop();

    	    	//Turn over and begin the program
    	    	set_speed_motor(-TURNING_SPEED, TURNING_SPEED);
    	    	chThdSleepMilliseconds(QUARTER_TURN_TIME);
    	    	motor_stop();
    	    	if (VL53L0X_get_dist_mm() >= SECURITY_DIST_TOF) {		//go to search_line
    	    		ready_to_search_direction = false;
    	    		ready_to_search_line = true;
    	    	} else {
    	    		ready_to_search_direction = true;
    	    		ready_to_search_line = false;					//go to search_direction
    	    	}
    	    	break;
    	}
    }
}

void control_robot_start(void) {
	chThdCreateStatic(waControlRobot, sizeof(waControlRobot), NORMALPRIO, ControlRobot, NULL);
}
