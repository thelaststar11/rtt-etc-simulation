/* applications/main.c */
#include <rtthread.h>
#include "storage_manager.h"
#include "etc_app.h"
#include "task_rfid.h"         /* 引入主动扫卡任务 */
#include "task_motor_pid.h"    /* 引入主动传送带恒速闭环控制任务 */
#include "task_mqtt_client.h"  /* 引入主动弹性 MQTT 客户端任务 */

int main(void)
{
    rt_kprintf("========================================\n");
    rt_kprintf("   Simulated ETC System Initializing... \n");
    rt_kprintf("           (Conveyor Belt Mode)         \n");
    rt_kprintf("========================================\n");



    /* 2. 启动核心业务控制层 */
    if (etc_app_init() != RT_EOK)
    {
        rt_kprintf("[Main] Fatal: Core business layer failed to start.\n");
        return -1;
    }
    /* 1. 初始化存储模块并预设测试卡数据 */
    etc_storage_register_defaults();
    /* 3. 获取业务消息队列，供各外设任务交互 */
    rt_mq_t app_mq = etc_app_get_mq();

    /* 4. 启动主动扫卡任务（硬件驱动在任务内部自动完成初始化） */
    if (task_rfid_init(app_mq) != RT_EOK)
    {
        rt_kprintf("[Main] Warning: Failed to launch RFID task.\n");
    }

    /* 5. 启动传送带恒速闭环控制任务（硬件电机与编码器在任务内部自动完成初始化） */
    if (task_motor_pid_init() != RT_EOK)
    {
        rt_kprintf("[Main] Warning: Failed to launch Motor PID task.\n");
    }

    /* 6. 启动脱机弹性 MQTT 客户端任务（MQTT客户端配置及网络轮询在任务内部自动管理） */
    if (task_mqtt_client_init(app_mq) != RT_EOK)
    {
        rt_kprintf("[Main] Warning: Failed to launch MQTT client task.\n");
    }

    rt_kprintf("[Main] All system tasks launched successfully.\n");

    return 0;
}
