/* applications/modules/motor_manager.h */

#ifndef APPLICATIONS_MODULES_MOTOR_MANAGER_H_
#define APPLICATIONS_MODULES_MOTOR_MANAGER_H_

#include <rtthread.h>

/* 初始化电机与编码器硬件及驱动驱动 */
rt_err_t motor_manager_init(void);

/**
 * 设置电机速度
 * @param speed: 目标速度百分比，范围 [-100, 100]
 *               正数正转，负数反转，0为滑行停止
 */
void motor_set_speed(rt_int8_t speed);

/* 刹车（强制锁定阻尼） */
void motor_brake(void);

/* 读取当前编码器的累计脉冲数值 */
rt_int32_t motor_get_encoder_count(void);

/* 重置编码器计数值为 0 */
void motor_reset_encoder(void);

/* 获取当前电机输出轴的实际转速（单位：RPM，转/分钟） */
float motor_get_speed_rpm(void);

/* 🔴 新增：彻底关闭并释放物理编码器设备，解除引脚复用 */
void motor_disable_encoder(void);

#endif /* APPLICATIONS_MODULES_MOTOR_MANAGER_H_ */
