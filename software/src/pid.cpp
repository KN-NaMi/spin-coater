#include "pid.h"

struct pid_controller
init_pid(double dt, double kp, double ki, double kd)
{
	struct pid_controller pid;

	pid.dt = dt;
	pid.kp = kp;
	pid.ki = ki;
	pid.kd = kd;

	pid.err = 0;
	pid.prev_err = 0;
	pid.it = 0;

	return pid;
}

double
run_pid(struct pid_controller *pid, double current, double target)
{
	pid->err = target - current;

	pid->P = pid->kp * pid->err;
	pid->I = (pid->it += pid->err * pid->ki * pid->dt);
	pid->D = pid->kd * (pid->err - pid->prev_err) / pid->dt;

	pid->prev_err = pid->err;

	return pid->P + pid->I + pid->D;
}

void
reset_pid(struct pid_controller *pid)
{
	pid->prev_err = 0;
	pid->it = 0;
}

