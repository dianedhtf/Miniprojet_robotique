/*
** File : 		main.c
** Author : 	Diane d'Haultfoeuille & Viviane Blanc
** Date : 		16 mai 2022
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ch.h"
#include "hal.h"
#include "memory_protection.h"
#include <usbcfg.h>
#include <main.h>
#include <motors.h>
#include <camera/po8030.h>
#include <selector.h>

#include <process_image.h>
#include <control_robot.h>
#include <msgbus/messagebus.h>
#include <sensors/proximity.h>
#include <sensors/VL53L0X/VL53L0X.h>
#include <audio/play_melody.h>
#include <audio/audio_thread.h>

//Message bus to communicate with the proximity
messagebus_t bus;
MUTEX_DECL(bus_lock);
CONDVAR_DECL(bus_condvar);

int main(void)
{
    halInit();
    chSysInit();
    mpu_init();

    //start the USB communication
    usb_start();
    //starts the camera
    dcmi_start();
	po8030_start();
	//inits the motors
	motors_init();
	//inits the message bus to communicate with the proximity
	messagebus_init(&bus, &bus_lock, &bus_condvar);
	//inits IR proximity sensors
	proximity_start();
	//inits TOF sensor
	VL53L0X_start();
	//inits the microphone
	dac_start();

	//starts the threads
	process_image_start();
	control_robot_start();
	playMelodyStart();

    /* Infinite loop. */
    while (1) {
    	//waits 1 second
        chThdSleepMilliseconds(1000);
    }
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}
