

#include "task_motor_pid.h"
#include "motor_manager.h"
#include <stdlib.h> /* 引入 atof 用于解析浮点型参数，abs 用于绝对值计算 */

#define PID_THREAD_STACK_SIZE   2048  /* 🔴 提升至 2048，防止 rt_kprintf 导致栈溢出 */
#define PID_THREAD_PRIORITY     14
#define PID_SAMPLE_TIME_MS      20   /* PID 闭环控制周期：20ms */

/* 增量式 PID 结构体 */
typedef struct {
    float kp;
    float ki;
    float kd;
    rt_int32_t last_error;
    rt_int32_t prev_error;
} pid_ctrl_t;

static struct rt_thread motor_pid_thread;
static rt_uint8_t motor_pid_stack[PID_THREAD_STACK_SIZE];

/* 恒速环增量式 PID 控制参数（基于脉冲数控制） */
static pid_ctrl_t speed_pid = {
    .kp = 0.85f,
    .ki = 0.20f,
    .kd = 0.05f,
    .last_error = 0,
    .prev_error = 0
};

/* ==================== 转速变量定义 ==================== */
static float target_speed_rpm = 22.7f;     /* 默认目标物理转速：272.7 RPM */
static rt_int32_t target_speed_pps = 10;   /* 内部换算出的目标脉冲数 */
static rt_bool_t is_running = RT_FALSE;      /* 默认开机不启动传送带转动 */

static rt_uint32_t sensor_fail_timer = 0; /* 传感器失效计时器 */
static rt_bool_t is_sensor_failed = RT_FALSE; /* 传感器失效标志 */

static void motor_pid_thread_entry(void *parameter)
{
    rt_int32_t last_count = 0;
    float current_duty = 0.0f;

    rt_kprintf("[Motor PID Task] Conveyor speed closed-loop daemon started.\n");

    motor_reset_encoder();
    last_count = motor_get_encoder_count();

    while (1)
    {
        if (is_running)
        {
            rt_int32_t current_count = motor_get_encoder_count();
            rt_int32_t actual_speed = current_count - last_count;
            last_count = current_count;

            /* ==================== 核心修正 1：处理 32767 过零点数学溢出 ==================== */
            if (actual_speed > 16384)  actual_speed -= 32768;
            if (actual_speed < -16384) actual_speed += 32768;

            /* ==================== 核心修正 2：极性翻转开关（防止方向相反导致正反馈狂飙） ==================== */
            /* 🔴 提示：如果修改代码后电机依然猛转然后报错，请将下面的 0 改为 1 */
            #define ENCODER_POLARITY_REVERSE   0
            #if ENCODER_POLARITY_REVERSE
            actual_speed = -actual_speed;
            #endif

                /* 正常闭环 PID 调节 */
                rt_int32_t error = target_speed_pps - actual_speed;

                float delta_u = speed_pid.kp * (error - speed_pid.last_error) +
                                speed_pid.ki * error +
                                speed_pid.kd * (error - 2 * speed_pid.last_error + speed_pid.prev_error);

                speed_pid.prev_error = speed_pid.last_error;
                speed_pid.last_error = error;

                current_duty += delta_u;

                /* 占空比限幅 */
                if (current_duty > 100.0f) current_duty = 100.0f;
                if (current_duty < 0.0f)   current_duty = 0.0f;

                motor_set_speed((rt_int8_t)current_duty);
        }
        else
        {
            motor_brake();
            current_duty = 0.0f;
            speed_pid.last_error = 0;
            speed_pid.prev_error = 0;
            last_count = motor_get_encoder_count();
            is_sensor_failed = RT_FALSE;
            sensor_fail_timer = 0;
        }

        rt_thread_mdelay(PID_SAMPLE_TIME_MS);
    }
}

rt_err_t task_motor_pid_init(void)
{
    rt_err_t result;

    if (motor_manager_init() != RT_EOK)
    {
        return -RT_ERROR;
    }

    result = rt_thread_init(&motor_pid_thread,
                            "motor_pid",
                            motor_pid_thread_entry,
                            RT_NULL,
                            motor_pid_stack,
                            sizeof(motor_pid_stack),
                            PID_THREAD_PRIORITY, 5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&motor_pid_thread);
    }
    return result;
}

void task_motor_set_running(rt_bool_t run)
{
    is_running = run;
}

/* 接收标准转速 RPM，在内部安全转换为脉冲数 */
void task_motor_set_target_speed(float speed_rpm)
{
    target_speed_rpm = speed_rpm;
    target_speed_pps = (rt_int32_t)(speed_rpm * 0.44f);
}


/* ==================== MSH FinSH 调试命令 ==================== */
#ifdef RT_USING_FINSH
#include <finsh.h>

/* 1. 控制电机启停命令 */
static void cmd_motor_run(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: motor_run <0|1>  (0: Stop, 1: Run)\n");
        rt_kprintf("Current state: %s\n", is_running ? "Running" : "Stopped");
        return;
    }

    rt_bool_t run = (rt_bool_t)atoi(argv[1]);
    task_motor_set_running(run);
    rt_kprintf("Motor running state set to: %s\n", run ? "ON" : "OFF");
}
MSH_CMD_EXPORT_ALIAS(cmd_motor_run, motor_run, Set motor run state: 0-Stop 1-Run);

/* 2. 调整目标速度 */
static void cmd_motor_speed(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: motor_speed <rpm_value>\n");
        rt_kprintf("Current target speed: %.1f RPM\n", target_speed_rpm);
        return;
    }

    float speed_rpm = (float)atof(argv[1]);
    task_motor_set_target_speed(speed_rpm);
    rt_kprintf("Motor target speed updated to: %.1f RPM\n", speed_rpm);
}
MSH_CMD_EXPORT_ALIAS(cmd_motor_speed, motor_speed, Set motor target speed in RPM);

/* 3. 查看当前电机与编码器状态命令 */
static void cmd_motor_status(int argc, char **argv)
{
    float actual_rpm = motor_get_speed_rpm();

    rt_kprintf("================= Motor Status =================\n");
    rt_kprintf("Running State   : %s\n", is_running ? "RUNNING" : "STOPPED/BRAKED");
    rt_kprintf("Target Speed    : %.1f RPM\n", target_speed_rpm);
    rt_kprintf("Actual Speed    : %.1f RPM\n", actual_rpm);
    rt_kprintf("Raw Encoder     : %d\n", (int)motor_get_encoder_count());
    rt_kprintf("================================================\n");
}
MSH_CMD_EXPORT_ALIAS(cmd_motor_status, motor_status, Get motor running status in RPM);

/* 4. 动态调整 PID 控制参数命令 */
static void cmd_motor_pid(int argc, char **argv)
{
    if (argc < 4)
    {
        rt_kprintf("Usage: motor_pid <kp> <ki> <kd>\n");
        rt_kprintf("Current PID parameters: Kp=%.3f, Ki=%.3f, Kd=%.3f\n",
                   speed_pid.kp, speed_pid.ki, speed_pid.kd);
        return;
    }

    speed_pid.kp = (float)atof(argv[1]);
    speed_pid.ki = (float)atof(argv[2]);
    speed_pid.kd = (float)atof(argv[3]);

    speed_pid.last_error = 0;
    speed_pid.prev_error = 0;

    rt_kprintf("PID updated successfully: Kp=%.3f, Ki=%.3f, Kd=%.3f\n",
               speed_pid.kp, speed_pid.ki, speed_pid.kd);
}
MSH_CMD_EXPORT_ALIAS(cmd_motor_pid, motor_pid, Set motor PID parameters: kp ki kd);

#endif /* RT_USING_FINSH */
