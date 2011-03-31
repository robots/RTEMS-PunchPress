/*
 * puncher.c
 *
 *  Created on: 25.3.2011
 *      Author: demim6am
 */
#include <stdint.h>
#include <rtems.h>
#include <assert.h>
#include <bsp.h>
#include <stdio.h>

#include "punch.h"
#include "main.h"
#include "ports.h"

enum {
	HEAD_UP,
	HEAD_UPING,
	HEAD_DOWN,
	HEAD_DOWNING,
};

rtems_task task_puncher(rtems_task_argument controller_id)
{
	rtems_status_code status;
	rtems_id          period_id;
	rtems_interval    ticks;

	uint8_t head = HEAD_UP;
	uint32_t events;

	struct port_status last;
	struct port_status cur;

	status = rtems_rate_monotonic_create(rtems_build_name( 'P', 'E', 'T', 'P' ), &period_id);

	/* every 0.5ms */
	ticks = rtems_clock_get_ticks_per_second() / 500;
	printf("puncher ticks %d %d\n", ticks, rtems_clock_get_ticks_per_second());

	cur.raw = 0;

	while(1) {
		status = rtems_rate_monotonic_period(period_id, ticks);
		if (status == RTEMS_TIMEOUT) {
			//printf("puncher timeout\n");
//			break;
		}

		last.raw = cur.raw;

		i386_inport_byte(PORT_STATUS, cur.raw);

		if (last.raw != cur.raw) {
			if (cur.fail) {
				printf("Fatal FAIL - System halted !\n");
				rtems_fatal_error_occurred (2);
			}
		}

		/*
		 * this state-machine will transition @0.5ms intervals
		 * takes 2 states to move head -> satisfy specification
		 */
		if (head == HEAD_UP) {
			if (rtems_event_receive(EVENT_PUNCH, RTEMS_NO_WAIT, 0, &events) == RTEMS_SUCCESSFUL) {
				//printf("punching \n");
				i386_outport_byte(PORT_PUNCH, 1);
				head = HEAD_DOWNING;
			}
		} else if (head == HEAD_DOWNING) {
			head = HEAD_DOWN;
		} else if (head == HEAD_DOWN) {
			i386_outport_byte(PORT_PUNCH, 0);
			head = HEAD_UPING;
		} else if ((head == HEAD_UPING) && (cur.head_up)) {
			//printf("punched\n");
			rtems_event_send(controller_id, EVENT_PUNCH_DONE);
			head = HEAD_UP;
		}
	}

	rtems_rate_monotonic_delete(period_id);
	rtems_task_delete(RTEMS_SELF);
}

