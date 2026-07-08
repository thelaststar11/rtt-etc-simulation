#ifndef APPLICATIONS_BUSINESS_TASK_MOTOR_PID_H_
#define APPLICATIONS_BUSINESS_TASK_MOTOR_PID_H_

#include <rtthread.h>

rt_err_t task_motor_pid_init(void);

void task_motor_set_running(rt_bool_t run);

void task_motor_set_target_speed(float speed_rpm);

#endif /* APPLICATIONS_BUSINESS_TASK_MOTOR_PID_H_ */
