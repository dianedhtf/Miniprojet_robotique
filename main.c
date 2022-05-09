/*
File : main.c
Author : Diane d'Haultfoeuille & Viviane Blanc
Date : 16 mai 2022
*/

/*
File : main.c
Author : Diane d'Haultfoeuille & Viviane Blanc
Date : 16 mai 2022
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
#include <chprintf.h>

#include <process_image.h>
#include <control_robot.h>
#include <msgbus/messagebus.h>
#include <sensors/proximity.h>
#include <sensors/VL53L0X/VL53L0X.h>

//BUS USED FOR THE PROXIMITY FUNCTIONS
//define the message bus to communicate with the proximity
messagebus_t bus;
MUTEX_DECL(bus_lock);
CONDVAR_DECL(bus_condvar);

//a enlever si pas utilisée ==> sauf si ifdef debug
void SendUint8ToComputer(uint8_t* data, uint16_t size) 
{
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)"START", 5);
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)&size, sizeof(uint16_t));
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)data, size);
}

//a enlever si pas utilisée
static void serial_start(void)
{
	static SerialConfig ser_cfg = {
	    115200,
	    0,
	    0,
	    0,
	};

	sdStart(&SD3, &ser_cfg); // UART3.
}

int main(void)
{

    halInit();
    chSysInit();
    mpu_init();

    //starts the serial communication
    serial_start();
    //start the USB communication
    usb_start();
    //starts the camera
    dcmi_start();
	po8030_start();
	//inits the motors
	motors_init();
	//define the message bus to communicate with the proximity
	messagebus_init(&bus, &bus_lock, &bus_condvar);
//	messagebus_topic_t *proximity_topic = messagebus_find_topic_blocking(&bus, "/proximity");
//	proximity_msg_t proximity_values;
//	dac_start();

	//inits IR proximity sensors
	proximity_start();
	VL53L0X_start();

	//starts the threads for the pi regulator and the processing of the image
	process_image_start();
	control_robot_start();
	detect_obstacle_start();

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
