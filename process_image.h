/*
** File : 		process_image.h
** Author : 	Diane d'Haultfoeuille & Viviane Blanc
** Date : 		16 mai 2022
*/

#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

//constants for process_image in pixels
#define IMAGE_BUFFER_SIZE		640
#define WIDTH_SLOPE				5
#define MIN_LINE_WIDTH			40

uint8_t get_redline_not_found(void);
uint8_t get_blueline_not_found(void);
void process_image_start(void);

#endif /* PROCESS_IMAGE_H_ */