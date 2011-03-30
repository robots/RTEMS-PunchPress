#ifndef MAIN_H_
#define MAIN_H_

#define ENC_TICKS_PER_MM 4

#define LIMIT_X_MM        1500
#define LIMIT_Y_MM        1000

#define LIMIT_X           (LIMIT_X_MM * ENC_TICKS_PER_MM)
#define LIMIT_Y           (LIMIT_Y_MM * ENC_TICKS_PER_MM)

// homing speeds
#define HOMING_SPEED      40
#define HOMING_SPEED_SLOW 20

// manual control options (1mm/button press)
#define MANUAL_STEP       (1*ENC_TICKS_PER_MM)

// events
#define EVENT_SAFE_R_ON       0x0001
#define EVENT_SAFE_R_OFF      0x0002
#define EVENT_SAFE_B_ON       0x0004
#define EVENT_SAFE_B_OFF      0x0008
#define EVENT_SAFE_L_ON       0x0010
#define EVENT_SAFE_L_OFF      0x0020
#define EVENT_SAFE_T_ON       0x0040
#define EVENT_SAFE_T_OFF      0x0080
#define EVENT_PUNCH           0x0100
#define EVENT_PUNCH_DONE      0x0200
#define EVENT_TARGET_REACHED  0x0400
#define EVENT_TARGET_NEW      0x0800
#define EVENT_PID_START       0x1000

// button mapping
#define BUTTON_LEFT   0x01
#define BUTTON_RIGHT  0x02
#define BUTTON_UP     0x04
#define BUTTON_DOWN   0x08
#define BUTTON_PUNCH  0x10
#define BUTTON_AUTO   0x80

struct position {
	uint32_t x;
	uint32_t y;
};


enum {
	MODE_INIT,
	MODE_HOMING,
	MODE_MANUAL,
	MODE_AUTO,
};

extern uint8_t PunchPress_mode;
extern struct position PunchPress_target;
extern struct position PunchPress_limit;

extern struct position PunchPress_actual;
extern rtems_id PunchPress_actual_sem;

#endif

