/*
 * encoder.c
 *
 *  Created on: 25.3.2011
 *      Author: demim6am
 */
#include <stdint.h>
#include <rtems.h>
#include <assert.h>
#include <bsp.h>
#include <stdio.h>


#include "encoder.h"
#include "main.h"
#include "ports.h"

enum {
	ENC_UNDEF,
	ENC_UP,
	ENC_DOWN
};

static struct position enc_actual;


void encoder_reset()
{
	enc_actual.y = 0;
	enc_actual.x = 0;

	rtems_semaphore_obtain(PunchPress_actual_sem, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
	PunchPress_actual = enc_actual;
	rtems_semaphore_release(PunchPress_actual_sem);
}

static int encoder_decode(uint8_t enc_last, uint8_t enc_new)
{
	uint8_t enc_dir = ENC_UP;

	if (enc_last == enc_new)
		return ENC_UNDEF;

	switch (enc_last) {
		case 0: // 00 -> 10
			if (enc_new == 2)
				enc_dir = ENC_DOWN;
			break;
		case 1: // 01 -> 00
			if (enc_new == 0)
				enc_dir = ENC_DOWN;
			break;
		case 2: // 10 -> 11
			if (enc_new == 3)
				enc_dir = ENC_DOWN;
			break;
		case 3: // 11 -> 01
			if (enc_new == 1)
				enc_dir = ENC_DOWN;
			break;
	}
	return enc_dir;
}

rtems_task task_encoder(rtems_task_argument controller_id)
{
	rtems_status_code status;
	rtems_id          period_id;
	rtems_interval    ticks;

	uint8_t enc1;
	uint8_t enc2;

	struct port_position last;
	struct port_position cur;

	status = rtems_rate_monotonic_create(rtems_build_name( 'P', 'E', 'R', 'E' ), &period_id);
	ticks = rtems_clock_get_ticks_per_second() / 500;

	printf("encoder ticks %d %d\n", ticks, rtems_clock_get_ticks_per_second());

	while(1) {
		status = rtems_rate_monotonic_period( period_id, ticks );
		if (status == RTEMS_TIMEOUT) { 
			//printf("puncher timeout\n");
//			break;
		}

		last.raw = cur.raw;

		i386_inport_byte(PORT_POSITION, cur.raw);

		enc1 = encoder_decode(last.enc_x, cur.enc_x);
		enc2 = encoder_decode(last.enc_y, cur.enc_y);

		// TODO: mutex
		if (enc1 == ENC_UP) {
			enc_actual.x ++;
		} else if (enc1 == ENC_DOWN){
			enc_actual.x --;
		}

		if (enc2 == ENC_UP) {
			enc_actual.y ++;
		} else if (enc2 == ENC_DOWN){
			enc_actual.y --;
		}

		// update only if the semaphore is not locked
		if (rtems_semaphore_obtain(PunchPress_actual_sem, RTEMS_NO_WAIT, RTEMS_NO_TIMEOUT) == RTEMS_SUCCESSFUL) {
			PunchPress_actual = enc_actual;
			rtems_semaphore_release(PunchPress_actual_sem);
		}

		if ((PunchPress_mode == MODE_HOMING) || (PunchPress_mode == MODE_HOMING2)) {
			uint32_t event = 0;
			if (last.safe_l != cur.safe_l) {
				event |= cur.safe_l == 0 ? EVENT_SAFE_L_OFF : EVENT_SAFE_L_ON; 
			}
			
			if (last.safe_t != cur.safe_t) {
				event |= cur.safe_t == 0 ? EVENT_SAFE_T_OFF : EVENT_SAFE_T_ON;
			}

			if (event)
				rtems_event_send(controller_id, event);
		}
/*
		if (PunchPress_mode == MODE_HOMING1) {
			if (last.safe_r != cur.safe_r) {
				printf("r event %d\n", cur.safe_r);
				rtems_event_send(controller_id, cur.safe_r == 0 ? EVENT_SAFE_R_OFF : EVENT_SAFE_R_ON);
			}
			
			if (last.safe_b != cur.safe_b) {
				printf("b event %d\n", cur.safe_b);
				rtems_event_send(controller_id, cur.safe_b == 0 ? EVENT_SAFE_B_OFF : EVENT_SAFE_B_ON);
			}
		}
*/
/*
		if ((PunchPress_mode == MODE_MANUAL) || (PunchPress_mode == MODE_AUTO)) {
			// this shouls also work for head going below 0 coordinate (overflow style)
			if ((PunchPress_actual.x > PunchPress_limit.x) || (PunchPress_actual.y > PunchPress_limit.y)) {
				// signal movement to halt, panic machine
				printf("Machine out of bounds :-(\n");
				printf("x: %06ld y: %06ld  \n", PunchPress_actual.x, PunchPress_actual.y);
				printf("x: %06ld y: %06ld  \n", PunchPress_limit.x, PunchPress_limit.y);
				rtems_fatal_error_occurred (2);
			}
		}
*/
		//printf("encx: %06ld ency: %06ld  \n", PunchPress_actual.x, PunchPress_actual.y);
		//printf("x: %01x lx: %01x  y: %01x ly: %01x\n", cur.enc_x, last.enc_x, cur.enc_y, last.enc_y);
	}

	printf("encoder missed\n");

	rtems_rate_monotonic_delete(period_id);
	rtems_task_delete(RTEMS_SELF);
}
