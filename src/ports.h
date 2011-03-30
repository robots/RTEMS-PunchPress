/*
 * ports.h
 *
 *  Created on: 25.3.2011
 *      Author: demim6am
 */

#ifndef PORTS_H_
#define PORTS_H_

#include <stdint.h>

#define LED_PORT 0x7060

#define PORT_PWR_X    0x7070
#define PORT_PWR_Y    0x7071
#define PORT_PUNCH    0x7072


/* port 0x7070 in */
#define PORT_POSITION 0x7070
struct port_position {
	uint8_t enc_x:2;
	uint8_t enc_y:2;
	uint8_t safe_l:1;
	uint8_t safe_r:1;
	uint8_t safe_t:1;
	uint8_t safe_b:1;
};

/* port 0x7071 in */
#define PORT_STATUS 0x7071
struct port_status {
	uint8_t head_up:1;
	uint8_t fail:1;
};


#endif /* PORTS_H_ */
