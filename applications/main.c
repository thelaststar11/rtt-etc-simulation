#include <rtthread.h>
#include "storage_manager.h"
#include "etc_app.h"
#include "task_rfid.h"        
#include "task_motor_pid.h"    
#include "task_mqtt_client.h"  
int main(void)
{
    rt_kprintf("========================================\n");
    rt_kprintf("   Simulated ETC System Initializing... \n");
    rt_kprintf("           (Conveyor Belt Mode)         \n");
    rt_kprintf("========================================\n");



    /* 启动核心业务控制层 */
    if (etc_app_init() != RT_EOK)
    {
        rt_kprintf("[Main] Fatal: Core business layer failed to start.\n");
        return -1;
    }
    /* 初始化存储模块并预设测试卡数据 */
    etc_storage_register_defaults();
    /* 获取业务消息队列，供各外设任务交互 */
    rt_mq_t app_mq = etc_app_get_mq();

    /* 启动主动扫卡任务 */
    if (task_rfid_init(app_mq) != RT_EOK)
    {
        rt_kprintf("[Main] Warning: Failed to launch RFID task.\n");
    }

    /* 启动传送带恒速闭环控制任务 */
    if (task_motor_pid_init() != RT_EOK)
    {
        rt_kprintf("[Main] Warning: Failed to launch Motor PID task.\n");
    }

    /* 启动脱机弹性 MQTT 客户端任务 */
    if (task_mqtt_client_init(app_mq) != RT_EOK)
    {
        rt_kprintf("[Main] Warning: Failed to launch MQTT client task.\n");
    }

    rt_kprintf("[Main] All system tasks launched successfully.\n");

    return 0;
}
