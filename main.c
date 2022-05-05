#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "ch.h"
#include "hal.h"
#include "memory_protection.h"
#include <usbcfg.h>
#include <main.h>
#include <motors.h>
#include <camera/po8030.h>
#include <chprintf.h>

#include <pi_regulator.h>
#include <process_image.h>
#include <sensors/proximity.h>
#include <sensors/VL53L0X/VL53L0X.h>
#include <audio/play_melody.h>
#include <audio/audio_thread.h>
#include <leds.h>

#include <msgbus/messagebus.h>

messagebus_t bus;
MUTEX_DECL(bus_lock);
CONDVAR_DECL(bus_condvar);

void SendUint8ToComputer(uint8_t* data, uint16_t size) 
{
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)"START", 5);
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)&size, sizeof(uint16_t));
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)data, size);
}

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
	dac_start();
	//Initie la musique
	playMelodyStart();
	chThdSleepMilliseconds(500);


	//playMelody(MARIO_FLAG,0,0);
	//waitMelodyHasFinished();

	//playMelody(SANDSTORMS,0,0);
	//waitMelodyHasFinished();

	//init de capteur de proximité frontal
	VL53L0X_start();


	//define the message bus to communicate with the proximity
	//proximity_start(); // ne pas décommenter (à enlever au final)
	messagebus_init(&bus, &bus_lock, &bus_condvar);
	proximity_start();
	messagebus_topic_t *proximity_topic = messagebus_find_topic_blocking(&bus, "/proximity");
    proximity_msg_t proximity_values;



	//stars the threads for the pi regulator and the processing of the image
	//pi_regulator_start();
	//process_image_start();

    //INT OK ? OU MIEUX UINT16_t ?
	int speed_robot = 0;
	int correc_r=0;
	int correc_l=0;
	bool HasAWay = false;

    /* Infinite loop. */
    while (1) {
    	//waits 0,1 second
        chThdSleepMilliseconds(100);

        //playMelody(WE_ARE_THE_CHAMPIONS,0,0);
        //waitMelodyHasFinished();

        int Dist_dr3 = get_prox(2);
        int Dist_dr2 = get_prox(1);
        int Dist_ga6 = get_prox(5);
        int Dist_ga7 = get_prox(6);
        //pour Hterme
//        chprintf((BaseSequentialStream *)&SD3, "dist_ga6 = %dus\n       ",Dist_ga6);
//        chprintf((BaseSequentialStream *)&SD3, "dist_dr3 = %dus\n       ",Dist_dr3);


        // Distance limite de proximité avec le mur ATTENTION sensible au variation de lumière
        int DistMurOk = 300;
//
//        // Détection du mur de droite avec capteur IR2 et 3
//               if (Dist_dr3<DistMurOk ){
//               	correc_r=0;
//               }
//               if(Dist_dr3>DistMurOk-50  ){
//               	correc_r=200;
//               }
//
//               //Détection du mur de gauche avec capteur IR6 et 7
//               if (Dist_ga6<DistMurOk ){
//                       	correc_l=0;
//                       }
//               if(Dist_ga6>DistMurOk-50 ){
//               		correc_l=200;
//                       }


        // Détection du mur de droite avec capteur IR2 et 3
        if (Dist_dr3<DistMurOk && Dist_dr2<DistMurOk){
        	correc_r=0;
        }
        if(Dist_dr3>DistMurOk-50 || Dist_dr2>DistMurOk-50 ){
        	correc_r=300;
        }

        //Détection du mur de gauche avec capteur IR6 et 7
        if (Dist_ga6<DistMurOk && Dist_ga7<DistMurOk){
                	correc_l=0;
                }
        if(Dist_ga6>DistMurOk-50 || Dist_ga7>DistMurOk-50){
        		correc_l=300;
                }


  		int dist_TOF = VL53L0X_get_dist_mm();
        //Détection du mur frontal
		if(dist_TOF <= 40){
			speed_robot=-400;
		}

        if(dist_TOF >50 && dist_TOF<60 ){
        	speed_robot = 0;
        	speed_robot = search_a_way();
        	if(speed_robot==400){ HasAWay = true;}
        	}

        if(dist_TOF>60){
        	speed_robot = 400;
                }


        left_motor_set_speed(speed_robot + correc_l);
        right_motor_set_speed(speed_robot + correc_r);


/*
		if(dist_TOF<60){
			left_motor_set_speed(-200);
			chThdSleepMilliseconds(1000);
			left_motor_set_speed(200);
			chThdSleepMilliseconds(1000);
			left_motor_set_speed(0);
		}
*/
/*
		// Say Coucou
		if(dist_TOF<60){
			left_motor_set_speed(200);
			right_motor_set_speed(-200);
			chThdSleepMilliseconds(1000);
			left_motor_set_speed(-200);
			right_motor_set_speed(200);
			chThdSleepMilliseconds(2000);
			left_motor_set_speed(-200);
			right_motor_set_speed(200);
			chThdSleepMilliseconds(2000);
			left_motor_set_speed(200);
			right_motor_set_speed(-200);
			chThdSleepMilliseconds(1000);
			left_motor_set_speed(0);
			right_motor_set_speed(0);

		}
		*/

    }

}


#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}



int search_a_way(void){

	left_motor_set_speed(0);
	right_motor_set_speed(0);
	//recherche d'un nouveau chemin
	chThdSleepMilliseconds(500);
	set_led(LED1,1);
	//tourne à droite de 90°
	left_motor_set_speed(300);
	right_motor_set_speed(-300);
	chThdSleepMilliseconds(1050);
	left_motor_set_speed(0);
	right_motor_set_speed(0);
	chThdSleepMilliseconds(500);

	int dist_TOF = VL53L0X_get_dist_mm();
	//si il y a un chemin, go. Sinon, tourne à gauche de 180°
	if(dist_TOF >100){
		int speed_robot = 400;
		set_led(LED1,0);
		return speed_robot;
	}
	  else{
	    left_motor_set_speed(-300);
	    right_motor_set_speed(300);
	    chThdSleepMilliseconds(2150);
	    left_motor_set_speed(0);
	    right_motor_set_speed(0);
	    set_led(LED1,0);
	    if(dist_TOF >100){
	       left_motor_set_speed(0);
	       right_motor_set_speed(0);
	       set_led(LED1,1);
	       set_led(LED2,1);
		   set_led(LED3,1);
		   chThdSleepMilliseconds(100000);
		   return 0;
       }

   }
}



