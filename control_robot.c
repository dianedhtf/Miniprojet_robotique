/*
 * control_robot.c
 *  Author: Diane d'Haultfoeuille and Viviane Blanc
 */

#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <leds.h>
#include <motors.h>
#include <process_image.h>
#include <main.h>				//Obligatoire ??

#define SEARCH_LINE 			0
#define	SEARCH_DIRECTION		1
#define REDLINE_FOUND			2
#define BLUELINE_FOUND 			3

//static uint8_t state_robot = 0; 			//besoin d'initialiser ?
static bool ready_to_search_line = true;
//static bool change_mode = true;
//static bool new_direction_find = true;
//static bool ready_to_search_new_direction = true;
//static bool end_line = false;

//void set_state_robot(uint8_t new_state) {
//	state_robot = new_state;
//}

//bool get_ready_to_search_line (void){
//	return ready_to_search_line;
//}

uint8_t get_mode(void) {

	if ((get_blueline_not_found() == 0) && (ready_to_search_line == true)){
			return REDLINE_FOUND;
	} else if ((get_redline_not_found() == 0) && (ready_to_search_line == true)){
		return BLUELINE_FOUND;
	} else {
		return SEARCH_LINE;
	}
}


//	if ((get_blueline_not_found() == 0) && (ready_to_search_line == true)){
////		set_front_led(1);
////		set_body_led(0);
////		ready_to_search_line = false;
//		return REDLINE_FOUND;			//car on devra ajuster la position après pour rester à une distance minimale
//	} else if (() && (ready_to_search_line == true) && (change_mode == true)) {
////		set_body_led(1);
////		set_front_led(0);
////		ready_to_search_line = false;
//		return BlUELINE_FOUND;
//	} else if ((get_blueline_not_found() == 0) && (ready_to_search_line == false) && (obstacle == true) {
//		return OBSTACLE;
//	}
		//		set_front_led(1);{
//	} else if (end_line == true) {
//		return SEARCH_DIRECTION;
//	} else if ((new_direction_find == true) && (ready_to_search_new_direction == true)) {
//		return SEARCH_LINE;
//	} else {

void set_speed_motor(int speed_l, int speed_r) {
	left_motor_set_speed(speed_l);
	right_motor_set_speed(speed_r);
}

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


//void set_pos_motor(int position) {
//	left_motor_set_pos(position);
//	right_motor_set_speed(position);
//}

//Marche pas très bien == a cause de la lumiere
static THD_WORKING_AREA(waControlRobot, 256);
static THD_FUNCTION(ControlRobot, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    while(1) {

    	switch(get_mode()) {

    		case REDLINE_FOUND:
    	    	//FAIT CLIGNOTER LA FRONT LED//
//    			chprintf((BaseSequentialStream *)&SD3, "REDLINE_FOUND= %dus \r \n ",get_mode());
    			ready_to_search_line = false;			//Donc garde normalement l'etat de get_blue/RED_line donc normalement ne va plus détecter une ligne
    			blink_front_led();

    		break;

    	       case SEARCH_LINE:
    	    	   all_leds_off();
    	    	   ready_to_search_line = true;
    	    	   set_speed_motor(200,200);			//avance tout droit

    	    	   //Pour le test
    	    	   chThdSleepMilliseconds(500);
    	    	   set_speed_motor(0,0);
    	    	   break;


    	       case BLUELINE_FOUND:
//    	    	   chprintf((BaseSequentialStream *)&SD3, "BLUELINE_FOUND= %dus \r \n ",get_mode());
    	    	   ready_to_search_line = false;
    	    	   active_body_led();
    	    	   break;
    	}
    }
}

void control_robot_start(void) {
	chThdCreateStatic(waControlRobot, sizeof(waControlRobot), NORMALPRIO, ControlRobot, NULL);
}

