/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef APPLICATIONS_BUSINESS_TASK_MQTT_CLIENT_H_
#define APPLICATIONS_BUSINESS_TASK_MQTT_CLIENT_H_

#include <rtthread.h>

/**
 * @brief 初始化弹性网络任务，绑定主业务队列
 */
rt_err_t task_mqtt_client_init(rt_mq_t mq);

#endif /* APPLICATIONS_BUSINESS_TASK_MQTT_CLIENT_H_ */
