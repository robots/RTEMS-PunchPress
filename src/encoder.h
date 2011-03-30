/*
 * encoder.h
 *
 *  Created on: 25.3.2011
 *      Author: demim6am
 */

#ifndef ENCODER_H_
#define ENCODER_H_

extern struct position PunchPress_actual;

void encoder_reset();
rtems_task task_encoder(rtems_task_argument ignored);

#endif /* ENCODER_H_ */
