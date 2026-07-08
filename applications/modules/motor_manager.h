#ifndef APPLICATIONS_MODULES_MOTOR_MANAGER_H_
#define APPLICATIONS_MODULES_MOTOR_MANAGER_H_

#include <rtthread.h>

rt_err_t motor_manager_init(void);

void motor_set_speed(rt_int8_t speed);

void motor_brake(void);

rt_int32_t motor_get_encoder_count(void);

void motor_reset_encoder(void);

float motor_get_speed_rpm(void);


#endif 