#include "task_mqtt_client.h"
#include "mqtt_manager.h"
#include "storage_manager.h"
#include "etc_types.h"
#include <wlan_mgnt.h>
#include "task_motor_pid.h"
#define MQTT_THREAD_STACK_SIZE  2048
#define MQTT_THREAD_PRIORITY    15

static struct rt_thread mqtt_thread;
static rt_uint8_t mqtt_stack[MQTT_THREAD_STACK_SIZE];
static rt_mq_t local_business_mq = RT_NULL;

static void mqtt_client_thread_entry(void *parameter)
{
    etc_log_t offline_log;
    char log_filepath[64];
    rt_bool_t pre_connected = RT_FALSE;

    rt_kprintf("[MQTT Task] Network daemon started.\n");

    rt_kprintf("[MQTT Task] Waiting for IP address (DHCP)...\n");
    while (rt_wlan_is_ready() == RT_FALSE)  // rt_wlan_is_ready 返回真代表 IP 已分配就绪
    {
        rt_thread_mdelay(500); // 每半秒查询一次
    }
    rt_kprintf("[MQTT Task] Network is ready! Initializing MQTT...\n");

    if (mqtt_manager_init(local_business_mq) == RT_EOK)
    {
        mqtt_manager_start();

    }
    while (1)
    {
        rt_bool_t current_connected = mqtt_manager_is_connected();

        /* 如果断网后重新联机 */
        if (current_connected && !pre_connected)
        {
            rt_kprintf("[MQTT Task] Link restored. Searching for unsynced offline transactions...\n");
        }
        pre_connected = current_connected;

        if (current_connected)
        {
            rt_memset(&offline_log, 0, sizeof(etc_log_t));
            rt_memset(log_filepath, 0, sizeof(log_filepath));

            /* 尝试从 LittleFS 离线交易目录抽取最老的一份交易日志 */
            if (etc_storage_get_oldest_offline_log(&offline_log, log_filepath, sizeof(log_filepath)) == RT_EOK)
            {
                rt_kprintf("[MQTT Task] Uploading cached transaction from: %s\n", log_filepath);

                /* 在线重新补发至云端 */
                if (mqtt_manager_publish(&offline_log.data) == RT_EOK)
                {
                    rt_kprintf("[MQTT Task] Sync success. Erasing cached transaction log.\n");
                    /* 上报成功后原子化删除脱机日志文件 */
                    etc_storage_delete_offline_log(log_filepath);

                    /* 立即循环，加速排空脱机缓存区 */
                    rt_thread_mdelay(150);
                    continue;
                }
            }
        }

        rt_thread_mdelay(1000);  /* 1秒钟心跳和缓存队列扫描期 */
    }
}

rt_err_t task_mqtt_client_init(rt_mq_t mq)
{
    rt_err_t result;
    if (mq == RT_NULL) return -RT_EINVAL;

    local_business_mq = mq;

    result = rt_thread_init(&mqtt_thread,
                            "task_mqtt",
                            mqtt_client_thread_entry,
                            RT_NULL,
                            mqtt_stack,
                            sizeof(mqtt_stack),
                            MQTT_THREAD_PRIORITY, 10);
    if (result == RT_EOK)
    {
        rt_thread_startup(&mqtt_thread);
    }
    return result;
}
