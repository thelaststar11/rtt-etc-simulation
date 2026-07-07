/* applications/business/etc_app.c */

#include "etc_app.h"
#include "etc_types.h"
#include "storage_manager.h"
#include "mqtt_manager.h"
#include "task_motor_pid.h"
#include "motor_manager.h"
#define ETC_APP_MQ_MSG_SZ      sizeof(etc_msg_t)
#define ETC_APP_MQ_MAX_MSGS    8
#define ETC_APP_THREAD_STACK   2048
#define ETC_APP_THREAD_PRIO    12

#define ETC_TOLL_FEE           10.0

static struct rt_messagequeue etc_mq;
static rt_uint8_t etc_mq_pool[ETC_APP_MQ_MSG_SZ * ETC_APP_MQ_MAX_MSGS];

static struct rt_thread etc_thread;
static rt_uint8_t etc_thread_stack[ETC_APP_THREAD_STACK];

static rt_bool_t is_net_online = RT_FALSE;

static void etc_app_thread_entry(void *parameter)
{
    etc_msg_t msg;
    rt_kprintf("[ETC App] Deterministic business logic core running.\n");

    while (1)
    {
        if (rt_mq_recv(&etc_mq, &msg, sizeof(etc_msg_t), RT_WAITING_FOREVER) == RT_EOK)
        {
            switch (msg.event_id)
            {
                case ETC_EVENT_NET_STATUS_UP:
                {
                    is_net_online = RT_TRUE;
                    rt_kprintf("[ETC App] Connection active. Gateway online.\n");
                    task_motor_set_running(RT_TRUE);
                    break;
                }

                case ETC_EVENT_CARD_DETECTED:
                {
                    rt_kprintf("[ETC App] Card detected! Motor STOPPED.\n");
                    motor_brake();
                    /* 1. 刷卡后立即关闭电机运转 */
                    task_motor_set_running(RT_FALSE);

                    etc_data_t user_data;
                    /* 从本地调取用户信息进行业务扣费 */
                    if (etc_storage_read_user(msg.card_uid, &user_data) == RT_EOK)
                    {
                        if (user_data.balance >= ETC_TOLL_FEE)
                        {
                            user_data.balance -= ETC_TOLL_FEE;
                            user_data.deduct_amount = ETC_TOLL_FEE;

                            if (etc_storage_write_user(msg.card_uid, &user_data) == RT_EOK)
                            {
                                rt_kprintf("[ETC App] Charged Successfully. Balance: %d\n", (int)user_data.balance);

                                /* 联机上报或离线保存 */
                                if (is_net_online && mqtt_manager_is_connected())
                                {
                                    mqtt_manager_publish(&user_data);
                                }
                                else
                                {
                                    etc_storage_save_offline_log(msg.card_uid, &user_data);
                                    rt_kprintf("[ETC App] Device offline. Log cached atomically.\n");
                                }
                            }
                        }
                        else
                        {
                            rt_kprintf("[ETC App] Process rejected: Insufficient balance!\n");
                        }
                    }
                    else
                    {
                        rt_kprintf("[ETC App] Process rejected: Card Unregistered!\n");
                    }

                    /* 2. 🔴 无论扣费结果如何、系统是否在线，统一让电机停止转动 3 秒钟（3000ms） */
                    rt_kprintf("[ETC App] Waiting 3 seconds for vehicle/conveyor... \n");
                    rt_thread_mdelay(3000);

                    /* 3. 🔴 延迟结束后重新启动电机运转，恢复到默认状态 */
                    rt_kprintf("[ETC App] Restarting motor...\n");
                    task_motor_set_running(RT_TRUE);

                    break;
                }

                default:
                    break;
            }
        }
    }
}

rt_mq_t etc_app_get_mq(void)
{
    return &etc_mq;
}

rt_err_t etc_app_init(void)
{
    rt_err_t result;

    etc_storage_init();

    result = rt_mq_init(&etc_mq, "etc_mq", etc_mq_pool, ETC_APP_MQ_MSG_SZ, sizeof(etc_mq_pool), RT_IPC_FLAG_FIFO);
    if (result != RT_EOK) return result;

    result = rt_thread_init(&etc_thread, "etc_app", etc_app_thread_entry, RT_NULL, etc_thread_stack, sizeof(etc_thread_stack), ETC_APP_THREAD_PRIO, 10);
    if (result == RT_EOK)
    {
        rt_thread_startup(&etc_thread);
    }
    return result;
}
