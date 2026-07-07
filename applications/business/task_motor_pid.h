/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef APPLICATIONS_BUSINESS_TASK_MOTOR_PID_H_
#define APPLICATIONS_BUSINESS_TASK_MOTOR_PID_H_

#include <rtthread.h>

/**
 * @brief 初始化并启动传送带恒速控制线程
 */
rt_err_t task_motor_pid_init(void);

/**
 * @brief 动态使能/关闭传送带运转
 */
void task_motor_set_running(rt_bool_t run);

/**
 * @brief 动态调整传送带的目标运转速度
 * @param speed_pps 目标速度（每 20ms 的脉冲变化量）
 */
void task_motor_set_target_speed(float speed_rpm);

#endif /* APPLICATIONS_BUSINESS_TASK_MOTOR_PID_H_ */
