#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define SCHEDULER_MAX_JOBS 32

#define JOB_OK 0
#define JOB_STALLED 1

#define SCHED_RUNNING 1 /* is the job enabled */
#define SCHED_PRIORITY 2 /* should the job be prioritized over others */
#define SCHED_MAY_STALL 4 /* can the job stall for a long time */

typedef int job_status;

struct sched_job {
	uint32_t interval_us;
	uint32_t flags;
};

void init_scheduler();
struct sched_job *sched_register(job_status (*callback)(void));
void run_scheduler();

#endif

