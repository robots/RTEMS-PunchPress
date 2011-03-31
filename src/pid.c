
#include <stdint.h>
#include <rtems.h>
#include <assert.h>
#include <bsp.h>
#include <stdio.h>

#include "ports.h"
#include "main.h"
#include "encoder.h"
#include "pid.h"

#define ABS(x) (((x)<0)?(-(x)):(x))
#define ERROR 4

static int16_t pid_lasterror[2] = {0, 0};
static int16_t pid_ierror[2] = {0, 0};

static int16_t pid_gain[3] = { 6, 0, 0 };
static int16_t pid_limit[4][2] = {
	{-16, 16},
	{-16, 16},
	{-32, 32},
	{-128, 127},
};

static int8_t pid_do(int16_t in, int16_t target, uint8_t axis)
{
	int16_t out;
	int16_t error;
	int16_t pid_derror;

	error = target - in;

	if (error < pid_limit[LIMIT_E][0])
		error = pid_limit[LIMIT_E][0];
	if (error > pid_limit[LIMIT_E][1])
		error = pid_limit[LIMIT_E][1];

	pid_derror = (error - pid_lasterror[axis]); 

	if (pid_derror < pid_limit[LIMIT_D][0])
		pid_derror = pid_limit[LIMIT_D][0];
	if (pid_derror > pid_limit[LIMIT_D][1])
		pid_derror = pid_limit[LIMIT_D][1];

	pid_ierror[axis] += error;

	if (pid_ierror[axis] < pid_limit[LIMIT_I][0])
		pid_ierror[axis] = pid_limit[LIMIT_I][0];
	if (pid_ierror[axis] > pid_limit[LIMIT_I][1])
		pid_ierror[axis] = pid_limit[LIMIT_I][1];

	out  = pid_gain[TERM_P] * error;
	out += pid_gain[TERM_I] * pid_ierror[axis];
	out += pid_gain[TERM_D] * pid_derror;

	pid_lasterror[axis] = error;

	if (out < pid_limit[LIMIT_O][0]) 
		out = pid_limit[LIMIT_O][0];
	if (out > pid_limit[LIMIT_O][1])
		out = pid_limit[LIMIT_O][1];

  return out;
}

rtems_task task_pid(rtems_task_argument controller_id)
{
	rtems_status_code status;
	rtems_id          period_id;
	rtems_interval    ticks;
	uint32_t events;
	uint8_t count = 0;
	uint8_t cnt = 0;
	uint8_t target_reached = 0;
	int8_t pwr_x = 0;
	int8_t pwr_y = 0;

	struct position tgt = {0,0};
	struct position cur = {0,0};


	status = rtems_rate_monotonic_create(rtems_build_name( 'P', 'E', 'R', 'p' ), &period_id);
	ticks = rtems_clock_get_ticks_per_second() / 500;

	printf("pidr ticks %d %d\n", ticks, rtems_clock_get_ticks_per_second());

	//printf("PID thread is waiting \n");
	rtems_event_receive(EVENT_PID_START, RTEMS_EVENT_ALL | RTEMS_WAIT, RTEMS_NO_TIMEOUT, &events);
	//printf("PID thread is starting \n");

	i386_outport_byte(PORT_PWR_X, 0);
	i386_outport_byte(PORT_PWR_Y, 0);

	while(1) {
		status = rtems_rate_monotonic_period( period_id, ticks );
		if (status == RTEMS_TIMEOUT) {
	//		printf("pid timeout\n");
		}

		count ++;
		if (rtems_event_receive(EVENT_TARGET_NEW, RTEMS_NO_WAIT, 0, &events) == RTEMS_SUCCESSFUL) {
			target_reached = 0;
			tgt = PunchPress_target;
			count = 0;
//			printf("new target: x: %06ld y: %06ld  \n", PunchPress_target.x, PunchPress_target.y);
		}

		if ((target_reached == 0) && (count > 5)) {
			count = 0;
			if ((ABS(PunchPress_actual.x - tgt.x) < ERROR) ||	(ABS(PunchPress_actual.y - tgt.y) < ERROR)) {
				target_reached = 1;
				rtems_event_send(controller_id, EVENT_TARGET_REACHED);
			}
		} 

		rtems_semaphore_obtain(PunchPress_actual_sem, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
		cur = PunchPress_actual;
		rtems_semaphore_release(PunchPress_actual_sem);

		pwr_x = pid_do(cur.x, tgt.x, AXIS_X);
		pwr_y = pid_do(cur.y, tgt.y, AXIS_Y);

		i386_outport_byte(PORT_PWR_X, pwr_x);
		i386_outport_byte(PORT_PWR_Y, pwr_y);

	}
}

