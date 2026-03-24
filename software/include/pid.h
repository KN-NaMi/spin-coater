#ifndef PID_H
#define PID_H

struct pid_controller {
	double dt;
	double kp, ki, kd;
	double it;
	double err, prev_err;




	double P, I, D;
};

struct pid_controller init_pid(double dt, double kp, double ki, double kd);
double                run_pid(struct pid_controller *pid, double current, double target);
void                  reset_pid(struct pid_controller *pid);

#endif

