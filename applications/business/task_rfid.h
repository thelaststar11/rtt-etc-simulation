/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-03     20465       the first version
 */
#ifndef APPLICATIONS_BUSINESS_TASK_RFID_H_
#define APPLICATIONS_BUSINESS_TASK_RFID_H_

#include <rtthread.h>

/**
 * @brief 初始化并启动 RFID 扫卡任务线程
 * @param mq 业务接收消息队列句柄
 * @return rt_err_t RT_EOK 表示成功启动，其他表示失败
 */
rt_err_t task_rfid_init(rt_mq_t mq);

#endif /* APPLICATIONS_BUSINESS_TASK_RFID_H_ */
