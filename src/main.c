#include <stdint.h>
#include <rtems.h>
#include <assert.h>
#include <bsp.h>
#include <stdio.h>

#include "encoder.h"
#include "punch.h"
#include "ports.h"
#include "main.h"
#include "pid.h"

enum {
	TASK_CTRL=0,
	TASK_ENC,
	TASK_PUNCH,
	TASK_PID
};

/* note: all positions are in encoder ticks !!!! */

uint8_t PunchPress_mode = MODE_INIT;

// limit position
struct position PunchPress_limit = {LIMIT_X, LIMIT_Y};

// target position 
struct position PunchPress_target = {0, 0};

// actual position of head
struct position PunchPress_actual;
rtems_id PunchPress_actual_sem;


void rtems_wait_for_event (rtems_event_set event)
{
	rtems_status_code sc = RTEMS_SUCCESSFUL;
	rtems_event_set   out = 0;

//	printf("waiting %x %08x\n", (uint32_t)event, rtems_task_self());
	sc = rtems_event_receive (RTEMS_ALL_EVENTS/*event*/, RTEMS_EVENT_ANY/*ALL*/ | RTEMS_WAIT,
		RTEMS_NO_TIMEOUT, &out);

//	printf("got %d\n", (uint32_t)out);

	if (sc != RTEMS_SUCCESSFUL || out != event)
		rtems_fatal_error_occurred (1);
}

rtems_task task_controller(rtems_task_argument tasks_pointer) {
	rtems_status_code status;
	rtems_id          period_id;
	rtems_interval    ticks;

	rtems_id *tasks = tasks_pointer;

	uint8_t but_last = 0;
	uint8_t but_cur = 0;
	uint8_t stop = 0;
	uint8_t homing = 0;
	uint32_t events;
	status = rtems_rate_monotonic_create(
			rtems_build_name( 'P', 'E', 'R', '1' ),
			&period_id
	);

	ticks = rtems_clock_get_ticks_per_second() / 100;

	printf("main ticks %d %d\n", ticks, rtems_clock_get_ticks_per_second());

	while (1) {
		status = rtems_rate_monotonic_period( period_id, ticks );
/*		if (status == RTEMS_TIMEOUT) {
			printf("main timeout\n");
		}
*/
		if (PunchPress_mode == MODE_INIT) {
			PunchPress_mode = MODE_HOMING;
			homing = 0;

			i386_outport_byte(LED_PORT, 1);
			i386_outport_byte(PORT_PWR_X, -HOMING_SPEED);
			i386_outport_byte(PORT_PWR_Y, -HOMING_SPEED);
		} else if (PunchPress_mode == MODE_HOMING) {
			if (rtems_event_receive(0xF0, RTEMS_NO_WAIT | RTEMS_EVENT_ANY, 0, &events) == RTEMS_SUCCESSFUL) {
				if (events & EVENT_SAFE_L_ON) {
					i386_outport_byte(PORT_PWR_X, HOMING_SPEED_SLOW);
				}

				if (events & EVENT_SAFE_L_OFF) {
					i386_outport_byte(PORT_PWR_X, 0);
					homing |= 1;
				}

				if (events & EVENT_SAFE_T_ON) {
					i386_outport_byte(PORT_PWR_Y, HOMING_SPEED_SLOW);
				}

				if (events & EVENT_SAFE_T_OFF) {
					i386_outport_byte(PORT_PWR_Y, 0);
					homing |= 2;
				}

			}

			if (homing == 3) {
				// reset position to 0,0
				encoder_reset();

				// activate pid controller
				rtems_event_send(tasks[TASK_PID], EVENT_PID_START | EVENT_TARGET_NEW);
				rtems_wait_for_event(EVENT_TARGET_REACHED);

				// switch to manual control
				PunchPress_mode = MODE_MANUAL;
				printf("Homing done\n");
			}
		} if (PunchPress_mode == MODE_MANUAL) {
			i386_outport_byte(LED_PORT, 4);
			but_last = but_cur;
			i386_inport_byte(LED_PORT, but_cur);

			if (rtems_event_receive(EVENT_TARGET_REACHED, RTEMS_NO_WAIT, 0, &events) == RTEMS_SUCCESSFUL) {
				printf("target reached\n");
				stop = 1;				
			} 

			if (but_last == 0) {
				if (but_cur & BUTTON_LEFT) {
					if (PunchPress_target.x > MANUAL_STEP) { 
						PunchPress_target.x -= MANUAL_STEP;
					} else {
						PunchPress_target.x = 0;
					}
					stop = 0;
					rtems_event_send(tasks[TASK_PID], EVENT_TARGET_NEW);
				}

				if (but_cur & BUTTON_RIGHT) {
					if ((PunchPress_limit.x - PunchPress_target.x) > MANUAL_STEP) { 
						PunchPress_target.x += MANUAL_STEP;
					} else {
						PunchPress_target.x = PunchPress_limit.x;
					}
					stop = 0;
					rtems_event_send(tasks[TASK_PID], EVENT_TARGET_NEW);
				}

				if (but_cur & BUTTON_UP) {
					if (PunchPress_target.y > MANUAL_STEP) { 
						PunchPress_target.y -= MANUAL_STEP;
					} else {
						PunchPress_target.y = 0;
					}
					stop = 0;
					rtems_event_send(tasks[TASK_PID], EVENT_TARGET_NEW);
				}

				if (but_cur & BUTTON_DOWN) {
					if ((PunchPress_limit.y - PunchPress_target.y) > MANUAL_STEP) { 
						PunchPress_target.y += MANUAL_STEP;
					} else {
						PunchPress_target.y = PunchPress_limit.y;
					}
					stop = 0;
					rtems_event_send(tasks[TASK_PID], EVENT_TARGET_NEW);
				}

				if (but_cur & BUTTON_PUNCH) {
					if (stop == 1) {
						rtems_event_send(tasks[TASK_PUNCH], EVENT_PUNCH);
						rtems_wait_for_event(EVENT_PUNCH_DONE);
					}
				}

				if (but_cur & BUTTON_AUTO) {
					PunchPress_mode = MODE_AUTO;
				}

				/* debug option */
				if (but_cur & 0x40) {
					rtems_cpu_usage_report();
				}
			}
			
		} else if (PunchPress_mode == MODE_AUTO) {
			i386_outport_byte(LED_PORT, 8);

			//
			// do some fun stuff
			//

			PunchPress_mode = MODE_MANUAL;
		}
	}
}

#define TASKS 4
rtems_task Init(rtems_task_argument ignored) {
	rtems_status_code status;
	rtems_id task_id[TASKS];
	rtems_name task_name[TASKS];
	int i;
	
	task_name[TASK_CTRL] = rtems_build_name( 'C', 'T', 'R', 'L' );
	task_name[TASK_ENC] = rtems_build_name( 'E', 'N', 'C', '1' );
	task_name[TASK_PUNCH] = rtems_build_name( 'P', 'U', 'N', '1' );
	task_name[TASK_PID] = rtems_build_name( 'P', 'I', 'D', '1' );

	status = rtems_task_create(task_name[TASK_CTRL], 2, RTEMS_MINIMUM_STACK_SIZE * 3, RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &task_id[TASK_CTRL]);
	assert( status == RTEMS_SUCCESSFUL );

	status = rtems_task_create(task_name[TASK_ENC], 5, RTEMS_MINIMUM_STACK_SIZE * 3, RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &task_id[TASK_ENC]);
	assert( status == RTEMS_SUCCESSFUL );

	status = rtems_task_create(task_name[TASK_PUNCH], 3, RTEMS_MINIMUM_STACK_SIZE * 3, RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &task_id[TASK_PUNCH]);
	assert( status == RTEMS_SUCCESSFUL );

	status = rtems_task_create(task_name[TASK_PID], 4, RTEMS_MINIMUM_STACK_SIZE * 3, RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &task_id[TASK_PID]);
	assert( status == RTEMS_SUCCESSFUL );

	status = rtems_semaphore_create(rtems_build_name('S', 'e', 'm', '1'), 1, RTEMS_BINARY_SEMAPHORE, RTEMS_NO_PRIORITY, &PunchPress_actual_sem);
	assert( status == RTEMS_SUCCESSFUL );
	
	printf("starting tasks\n");

	status = rtems_task_start( task_id[TASK_CTRL], task_controller, &task_id[0] );
	assert( status == RTEMS_SUCCESSFUL );

	status = rtems_task_start( task_id[TASK_ENC], task_encoder, task_id[TASK_CTRL] );
	assert( status == RTEMS_SUCCESSFUL );

	status = rtems_task_start( task_id[TASK_PUNCH], task_puncher, task_id[TASK_CTRL] );
	assert( status == RTEMS_SUCCESSFUL );

	status = rtems_task_start( task_id[TASK_PID], task_pid, task_id[TASK_CTRL] );
	assert( status == RTEMS_SUCCESSFUL );

	rtems_task_delete(RTEMS_SELF);
}

/**************** START OF CONFIGURATION INFORMATION ****************/

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define CONFIGURE_MAXIMUM_TASKS             5
#define CONFIGURE_MAXIMUM_PERIODS           5

#define CONFIGURE_MICROSECONDS_PER_TICK     1000
#define CONFIGURE_EXTRA_TASK_STACKS (3000 * RTEMS_MINIMUM_STACK_SIZE)

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT
#include <rtems/confdefs.h>

/****************  END OF CONFIGURATION INFORMATION  ****************/
