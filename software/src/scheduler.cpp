#include "scheduler.h"

#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

#define INT_DUE 1

struct sched_listing {
	struct sched_job api;
	job_status (*callback)(void);
	uint32_t previous_run;
	uint32_t int_flags;
};

typedef uint16_t job_id;

struct scheduler {
	struct sched_listing jobs[SCHEDULER_MAX_JOBS];
	job_id due[SCHEDULER_MAX_JOBS];
	job_id num_jobs, num_due;
};

static struct scheduler global_scheduler;

static void run_job(job_id id);

void
init_scheduler()
{
	global_scheduler.num_jobs = 0;
	global_scheduler.num_due = 0;
}

struct sched_job *
sched_register(job_status (*callback)(void))
{
	if(global_scheduler.num_jobs >= SCHEDULER_MAX_JOBS) return NULL;

	memset(global_scheduler.jobs + global_scheduler.num_jobs, 0, sizeof(struct sched_listing));
	global_scheduler.jobs[global_scheduler.num_jobs].callback = callback;
	global_scheduler.jobs[global_scheduler.num_jobs].previous_run = micros();

	return &global_scheduler.jobs[global_scheduler.num_jobs++].api;
}

void
run_scheduler()
{
	uint32_t now = micros();
	job_id i;
	job_id prio, prio_stalling, normal;
	job_id checked_job, ran_job;
	uint32_t job_flags;

	for(i = 0; i < global_scheduler.num_jobs; ++i) {
		if(global_scheduler.jobs[i].api.flags & SCHED_RUNNING) {
			if(! (global_scheduler.jobs[i].int_flags & INT_DUE)) {
				if(now - global_scheduler.jobs[i].previous_run >= global_scheduler.jobs[i].api.interval_us) {
					global_scheduler.due[global_scheduler.num_due++] = i;
					global_scheduler.jobs[i].int_flags |= INT_DUE;
				}
			}
		}
	}

	if(global_scheduler.num_due) {
		prio = prio_stalling = normal = (job_id)-1;
		for(i = 0; i < global_scheduler.num_due; ++i) {
			checked_job = global_scheduler.due[i];
			job_flags = global_scheduler.jobs[checked_job].api.flags;
			if(job_flags & SCHED_MAY_STALL) {
				if(job_flags & SCHED_PRIORITY) prio_stalling = i;
			} else {
				if(job_flags & SCHED_PRIORITY) { prio = i; break; }
				else normal = i;
			}
		}

		if(prio != (job_id)-1) ran_job = prio;
		else if(prio_stalling != (job_id)-1) ran_job = prio_stalling;
		else if(normal != (job_id)-1) ran_job = normal;
		else ran_job = 0;

		run_job(global_scheduler.due[ran_job]);
		global_scheduler.due[ran_job] = global_scheduler.due[--global_scheduler.num_due];
	}
}

static void
run_job(job_id id)
{
	job_status status;

	global_scheduler.jobs[id].int_flags &= ~INT_DUE;
	status = global_scheduler.jobs[id].callback();

	switch(status) {
	case JOB_STALLED:
		global_scheduler.jobs[id].previous_run = micros();
		break;
	case JOB_OK:
	default:
		global_scheduler.jobs[id].previous_run += global_scheduler.jobs[id].api.interval_us;
		break;
	}
}

