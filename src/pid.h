#ifndef PID_H_
#define PID_H_

enum {
	TERM_P,
	TERM_I,
	TERM_D
};

enum limits {
	LIMIT_I,
	LIMIT_D,
	LIMIT_E,
	LIMIT_O
};

enum {
	AXIS_X,
	AXIS_Y,
};

rtems_task task_pid(rtems_task_argument ignored);

#endif
