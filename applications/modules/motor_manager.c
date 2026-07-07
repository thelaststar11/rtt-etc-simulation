/* applications/modules/motor_manager.c */

#include "motor_manager.h"
#include <rtdevice.h>

#ifndef GET_PIN
#define GET_PIN(PORT, PIN)  (rt_uint8_t)((((rt_uint8_t)PORT - 'A') * 16) + PIN)
#endif

#define MOTOR_IN2_PIN       GET_PIN('A', 2)   /* PA2: TB67H450 的 IN2 */
#define PWM_DEV_NAME        "pwm2"            /* PA3 对应的 PWM 设备 */
#define PWM_DEV_CHANNEL     4                 /* PWM 通道号 */
#define PWM_PERIOD_NS       50000             /* 20kHz 载波周期 (50us) */
#define ENCODER_DEV_NAME    "pulse4"          /* PB6/PB7 正交编码器 */

#define MOTOR_GEAR_RATIO      30.0f  /* 减速比 1:30 */
#define MOTOR_BASE_PPR        11.0f  /* 磁编码器基本脉冲 11 PPR */
#define ENCODER_MULTIPLIER    4.0f   /* RT-Thread 默认正交解码 4 倍频 */

#define TOTAL_PULSES_PER_REV  (MOTOR_BASE_PPR * MOTOR_GEAR_RATIO * ENCODER_MULTIPLIER)

static rt_int32_t last_calc_count = 0;
static rt_tick_t last_calc_tick = 0;
static float current_calculated_rpm = 0.0f;

static struct rt_device_pwm *pwm_device = RT_NULL;
static rt_device_t encoder_device = RT_NULL;

rt_err_t motor_manager_init(void)
{
    rt_err_t result;

    /* 1. 初始化方向控制引脚 */
    rt_pin_mode(MOTOR_IN2_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(MOTOR_IN2_PIN, PIN_LOW);

    /* 2. 查找并配置 PWM 设备 */
    pwm_device = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
    if (pwm_device == RT_NULL)
    {
        rt_kprintf("[Motor] Error: PWM device %s not found.\n", PWM_DEV_NAME);
        return -RT_ERROR;
    }
    rt_pwm_set(pwm_device, PWM_DEV_CHANNEL, PWM_PERIOD_NS, 0);
    rt_pwm_enable(pwm_device, PWM_DEV_CHANNEL);

    /* 3. 查找并初始化正交编码器 */
    encoder_device = rt_device_find(ENCODER_DEV_NAME);
    if (encoder_device == RT_NULL)
    {
        rt_kprintf("[Motor] Error: Encoder device %s not found.\n", ENCODER_DEV_NAME);
        return -RT_ERROR;
    }
    result = rt_device_open(encoder_device, RT_DEVICE_OFLAG_RDONLY);
    if (result != RT_EOK)
    {
        rt_kprintf("[Motor] Error: Open encoder failed.\n");
        return result;
    }
    rt_device_control(encoder_device, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);

    last_calc_count = 0;
    last_calc_tick = rt_tick_get();
    current_calculated_rpm = 0.0f;

    return RT_EOK;
}

void motor_set_speed(rt_int8_t speed)
{
    if (pwm_device == RT_NULL) return;

    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;

    rt_uint32_t pulse_ns = 0;

    if (speed > 0)
    {
        rt_pin_write(MOTOR_IN2_PIN, PIN_LOW);
        pulse_ns = (PWM_PERIOD_NS * speed) / 100;
        rt_pwm_set(pwm_device, PWM_DEV_CHANNEL, PWM_PERIOD_NS, pulse_ns);
    }
    else if (speed < 0)
    {
        rt_pin_write(MOTOR_IN2_PIN, PIN_HIGH);
        rt_uint8_t abs_speed = (rt_uint8_t)(-speed);
        pulse_ns = (PWM_PERIOD_NS * (100 - abs_speed)) / 100;
        rt_pwm_set(pwm_device, PWM_DEV_CHANNEL, PWM_PERIOD_NS, pulse_ns);
    }
    else
    {
        rt_pin_write(MOTOR_IN2_PIN, PIN_LOW);
        rt_pwm_set(pwm_device, PWM_DEV_CHANNEL, PWM_PERIOD_NS, 0);
    }
}

void motor_brake(void)
{
    if (pwm_device == RT_NULL) return;
    rt_pin_write(MOTOR_IN2_PIN, PIN_HIGH);
    rt_pwm_set(pwm_device, PWM_DEV_CHANNEL, PWM_PERIOD_NS, PWM_PERIOD_NS);
}

rt_int32_t motor_get_encoder_count(void)
{
    rt_int32_t count = 0;
    if (encoder_device != RT_NULL)
    {
        rt_device_read(encoder_device, 0, &count, sizeof(count));
    }
    return count;
}

void motor_reset_encoder(void)
{
    if (encoder_device != RT_NULL)
    {
        rt_device_control(encoder_device, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    }
}

float motor_get_speed_rpm(void)
{
    if (encoder_device == RT_NULL) return 0.0f;

    rt_tick_t now_tick = rt_tick_get();
    rt_int32_t current_count = motor_get_encoder_count();

    if (now_tick != last_calc_tick)
    {
        float dt = (float)(now_tick - last_calc_tick) / RT_TICK_PER_SECOND;
        rt_int32_t delta_count = current_count - last_calc_count;

        float rps = ((float)delta_count / TOTAL_PULSES_PER_REV) / dt;
        current_calculated_rpm = rps * 60.0f;

        last_calc_count = current_count;
        last_calc_tick = now_tick;
    }

    return current_calculated_rpm;
}

/* 🔴 新增：实现彻底关闭并释放物理编码器设备 */
void motor_disable_encoder(void)
{
    if (encoder_device != RT_NULL)
    {
        rt_device_close(encoder_device);
        encoder_device = RT_NULL;
        rt_kprintf("[Motor] Encoder hardware device closed and released.\n");
    }
}
