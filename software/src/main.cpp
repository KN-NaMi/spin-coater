#include <SimpleFOC.h>
#include "pinout.h"
#include "lcd.h"
#include "scheduler.h"

#define POLE_PAIRS 4 /* number of poles = 8, then pole pair count = 4 ??? */
#define RPM_SMOOTHING 16 /* this number times 20 is the total change in RPM per second */
#define MOTOR_MIN_RPM 100
#define MOTOR_MAX_RPM 4000
#define MENU_TIMEOUT_MS 1000
#define POT_MIN 0
#define POT_MAX 1023
#define OPTIONAL_THRESHOLD 192

BLDCMotor motor = BLDCMotor(POLE_PAIRS);
BLDCDriver6PWM driver = BLDCDriver6PWM(DRIVER_UH, DRIVER_UL, DRIVER_VH, DRIVER_VL, DRIVER_WH, DRIVER_WL);
HallSensor sensor = HallSensor(HALL_U, HALL_V, HALL_W, POLE_PAIRS);

job_status update_timer(void);
job_status smooth_rpm(void);
job_status run_motor(void);
job_status read_hall(void);
job_status update_lcd(void);
job_status heartbeat_led(void);
job_status input(void);

void enter_menu();
uint32_t to_2_sig_digits(uint32_t num);

typedef uint16_t rpm_t;

int timer;
rpm_t set_rpm;
rpm_t target_rpm;
rpm_t current_rpm;
int running;
uint32_t run_ms;
struct lcd lcd(0x27, 16, 4);

void doA(){ sensor.handleA(); }
void doB(){ sensor.handleB(); }
void doC(){ sensor.handleC(); }

struct option {
	char const *text;
	char const *sel;
	char const *fmt;
	int *val;
	int min, max;
	int optional;
};
struct option options[] = {
	{ .text = "timer", .sel = "TIMER", .fmt = "%d s",   .val = &timer, .min = 1, .max = 120, .optional = 1 },
	{ .text = "exit",  .sel = "exit",  .fmt = "(exit)", .val = NULL,   .min = 0, .max = 0,   .optional = 0 },
};

void
setup()
{
	struct sched_job *update_timer_job, *smooth_rpm_job, *run_motor_job, *read_hall_job, *update_lcd_job, *heartbeat_led_job, *input_job;

	init_scheduler();

	sensor.init();
	sensor.enableInterrupts(doA, doB, doC);
	motor.linkSensor(&sensor);

	driver.voltage_power_supply = 12;
	driver.pwm_frequency = 4000;
	driver.dead_zone = 0.05;
	driver.init();
	motor.linkDriver(&driver);

	// Invert logic for PMOS -> Driver specific
	TIM1->CCER |= (TIM_CCER_CC1P | TIM_CCER_CC2P | TIM_CCER_CC3P);
	TIM1->CCER &= ~(TIM_CCER_CC1NP | TIM_CCER_CC2NP | TIM_CCER_CC3NP);

	motor.controller = MotionControlType::velocity;
	motor.PID_velocity.P = 0.002;
	motor.PID_velocity.I = 0.01;
	motor.PID_velocity.D = 0.0005;
	motor.voltage_limit = 6;

	update_timer_job = sched_register(update_timer);
	update_timer_job->interval_us = 1000000;
	update_timer_job->flags = SCHED_RUNNING | SCHED_PRIORITY;
	smooth_rpm_job = sched_register(smooth_rpm);
	smooth_rpm_job->interval_us = 50000;
	smooth_rpm_job->flags = SCHED_RUNNING | SCHED_PRIORITY;
	run_motor_job = sched_register(run_motor);
	run_motor_job->interval_us = 50000;
	run_motor_job->flags = SCHED_RUNNING | SCHED_PRIORITY;
	read_hall_job = sched_register(read_hall);
	read_hall_job->interval_us = 50000;
	read_hall_job->flags = SCHED_RUNNING;
	update_lcd_job = sched_register(update_lcd);
	update_lcd_job->interval_us = 1000000;
	update_lcd_job->flags = SCHED_RUNNING;
	heartbeat_led_job = sched_register(heartbeat_led);
	heartbeat_led_job->interval_us = 500000;
	heartbeat_led_job->flags = SCHED_RUNNING;
	input_job = sched_register(input);
	input_job->interval_us = 100000;
	input_job->flags = SCHED_RUNNING | SCHED_MAY_STALL;

	pinMode(BUTTON_PIN, INPUT_PULLDOWN);
	pinMode(POT_PIN, INPUT);
	pinMode(LED_YELLOW, OUTPUT);
	pinMode(LED_RED, OUTPUT);
	running = 0;
	timer = -1;
	run_ms = 0;
	init_lcd(&lcd, 16, 4);

	motor.init();
	motor.initFOC();
}

void
loop()
{
	motor.loopFOC();
	run_scheduler();
}

job_status
update_timer(void)
{
	uint32_t now;

	now = millis();

	if(run_ms == 0 && running) run_ms = now;
	if(timer != -1 && now - run_ms >= timer*1000) {
		running = 0;
	}
	if(! running) run_ms = 0;

	return JOB_OK;
}

job_status
smooth_rpm(void)
{
	unsigned int pot_value;

	pot_value = analogRead(POT_PIN);
	set_rpm = running ? map(pot_value, POT_MIN, POT_MAX, MOTOR_MIN_RPM, MOTOR_MAX_RPM) : 0;
	if(set_rpm > target_rpm + RPM_SMOOTHING) target_rpm += RPM_SMOOTHING;
	else if(set_rpm + RPM_SMOOTHING < target_rpm) target_rpm -= RPM_SMOOTHING;
	else target_rpm = set_rpm;

	return JOB_OK;
}

job_status
run_motor(void)
{
	float tgt_rad_per_s;

	if(target_rpm == 0) {
		motor.move(0);
	} else {
		tgt_rad_per_s = (float)target_rpm * 2.0 * PI / 60.0;
		motor.move(tgt_rad_per_s);
	}

	return JOB_OK;
}

job_status
read_hall(void)
{
	float signed_rpm = motor.shaft_velocity * 60.0f / (2.0f * PI);

	current_rpm = fabsf(signed_rpm);

	return JOB_OK;
}

job_status
update_lcd(void)
{
	set_cur_lcd(&lcd, 0, 0);
	printf_lcd(&lcd, "SET: %d", (int)to_2_sig_digits(set_rpm));
	wipe_line(&lcd);
	set_cur_lcd(&lcd, 0, 1);
	printf_lcd(&lcd, "TGT: %d", (int)to_2_sig_digits(target_rpm));
	wipe_line(&lcd);
	set_cur_lcd(&lcd, 0, 2);
	printf_lcd(&lcd, "CUR: %d", (int)to_2_sig_digits(current_rpm));
	wipe_line(&lcd);
	set_cur_lcd(&lcd, 0, 3);
	if(timer != -1) {
		if(running) {
			printf_lcd(&lcd, "TMR: %d s", (int)(run_ms - millis())/1000 + timer);
		} else {
			printf_lcd(&lcd, "TMR: %d s", (int)timer);
		}
	} else {
		printf_lcd(&lcd, "MANUAL");
	}
	wipe_line(&lcd);
	flush_lcd(&lcd);

	return JOB_OK;
}

job_status
heartbeat_led(void)
{
	static int led_state = 1;

	if(led_state) {
		digitalWrite(LED_RED, LOW);
	} else {
		digitalWrite(LED_RED, HIGH);
	}
	led_state = !led_state;

	return JOB_OK;
}

job_status
input(void)
{
	static uint32_t press_ms = -1;

	if (digitalRead(BUTTON_PIN) == HIGH) {
		if(press_ms == (uint32_t)-1) {
			press_ms = millis();
		}
	} else {
		if(press_ms != (uint32_t)-1) {
			if(millis() - press_ms >= MENU_TIMEOUT_MS) {
				enter_menu();
				return JOB_STALLED;
			} else {
				running = !running;
			}
		}

		press_ms = (uint32_t)-1;
	}

	return JOB_OK;
}

void
enter_menu()
{
	unsigned int pot_value;
	uint8_t opt;
	struct option selected;
	int editing, pressed;
	uint32_t last_lcd_update_ms;

	running = 0;
	motor.disable();

	editing = 0;
	pressed = 0;
	last_lcd_update_ms = 0;
	while(1) {
		pot_value = analogRead(POT_PIN);
		if(editing) {
			if(! selected.val) break; /* val = NULL means exit button */
			if(selected.optional) {
				if(pot_value <= OPTIONAL_THRESHOLD) *selected.val = -1;
				else {
					*selected.val = map(pot_value, OPTIONAL_THRESHOLD, POT_MAX, selected.min, selected.max);
					*selected.val = constrain(*selected.val, selected.min, selected.max);
				}
			} else {
				*selected.val = map(pot_value, POT_MIN, POT_MAX, selected.min, selected.max);
				*selected.val = constrain(*selected.val, selected.min, selected.max);
			}
		} else {
			opt = map(pot_value, POT_MIN, POT_MAX, 0, sizeof(options)/sizeof(*options) - 1);
			opt = constrain(opt, 0, (sizeof(options)/sizeof(*options)) - 1);
			selected = options[opt];
		}

		if (digitalRead(BUTTON_PIN) == HIGH) {
			pressed = 1;
		} else {
			if(pressed) editing = !editing;
			pressed = 0;
		}

		if(millis() - last_lcd_update_ms >= 1000) {
			set_cur_lcd(&lcd, 0, 0);
			printf_lcd(&lcd, "Options");
			wipe_line(&lcd);
			set_cur_lcd(&lcd, 0, 1);
			printf_lcd(&lcd, "%s", editing ? selected.sel : selected.text);
			wipe_line(&lcd);
			set_cur_lcd(&lcd, 0, 2);
			printf_lcd(&lcd, selected.fmt, selected.val ? *selected.val : 0);
			wipe_line(&lcd);
			set_cur_lcd(&lcd, 0, 3);
			printf_lcd(&lcd, "%d/%d", opt + 1, sizeof(options)/sizeof(*options));
			wipe_line(&lcd);
			last_lcd_update_ms += 1000;
		}

		flush_lcd(&lcd);
	}

	motor.enable();
}

uint32_t
to_2_sig_digits(uint32_t num)
{
	if(num < 100) return num;
	if(num < 1000) return num - num%10;
	if(num < 10000) return num - num%100;
	if(num < 100000) return num - num%1000;
	if(num < 1000000) return num - num%10000;
	if(num < 10000000) return num - num%100000;
	if(num < 100000000) return num - num%1000000;
	if(num < 1000000000) return num - num%10000000;
	return num - num%100000000;
}

