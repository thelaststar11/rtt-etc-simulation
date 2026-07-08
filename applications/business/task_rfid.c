#include "task_rfid.h"
#include "rfid_manager.h"
#include "etc_types.h"

#define RFID_THREAD_STACK_SIZE   2048
#define RFID_THREAD_PRIORITY     15

static rt_thread_t rfid_thread = RT_NULL;
static rt_mq_t rfid_mq = RT_NULL;

static void rfid_thread_entry(void *parameter)
{
    char uid_str[16];
    rt_kprintf("[RFID Task] Thread started, scanning cards...\n");

    while (1)
    {
        rt_memset(uid_str, 0, sizeof(uid_str));

        /* 调用底层的被动读取接口 */
        if (rfid_manager_read_uid(uid_str, sizeof(uid_str)) == RT_EOK)
        {
            etc_msg_t msg;
            rt_memset(&msg, 0, sizeof(etc_msg_t));

            msg.event_id = ETC_EVENT_CARD_DETECTED;
            rt_strncpy(msg.card_uid, uid_str, sizeof(msg.card_uid) - 1);

            /* 将事件打包后发送到主业务队列 */
            if (rfid_mq != RT_NULL)
            {
                if (rt_mq_send(rfid_mq, &msg, sizeof(etc_msg_t)) != RT_EOK)
                {
                    rt_kprintf("[RFID Task] Warning: Message queue is full, lost RFID event.\n");
                }
            }
        }

        rt_thread_mdelay(100); /* 保持 100ms 的检测频率 */
    }
}

rt_err_t task_rfid_init(rt_mq_t mq)
{
    if (mq == RT_NULL)
    {
        return -RT_EINVAL;
    }

    rfid_mq = mq;

    /* 初始化底层的被动 MFRC522 硬件驱动 */
    if (rfid_manager_init() != RT_EOK)
    {
        rt_kprintf("[RFID Task] Error: Failed to initialize RFID hardware.\n");
        return -RT_ERROR;
    }

    /* 创建主动扫卡检测线程 */
    rfid_thread = rt_thread_create("rfid_task",
                                  rfid_thread_entry,
                                  RT_NULL,
                                  RFID_THREAD_STACK_SIZE,
                                  RFID_THREAD_PRIORITY,
                                  10);
    if (rfid_thread != RT_NULL)
    {
        rt_thread_startup(rfid_thread);
        return RT_EOK;
    }

    return -RT_ERROR;
}
