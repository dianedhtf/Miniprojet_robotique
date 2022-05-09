/*
 * control_robot.h
 *
 *  Author: Diane d'Haultfoeuille and Viviane Blanc
 */

#ifndef CONTROL_ROBOT_H_
#define CONTROL_ROBOT_H_

void set_speed_motor(int speed_l, int speed_r);
void motor_stop(void);

void control_robot_start(void);
void detect_obstacle_start(void);

#endif /* CONTROL_ROBOT_H_ */
