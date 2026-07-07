/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-03     20465       the first version
 */
#ifndef APPLICATIONS_MODULES_MQTT_MANAGER_H_
#define APPLICATIONS_MODULES_MQTT_MANAGER_H_

#include <rtthread.h>
#include "etc_types.h"

/**
 * @brief 初始化 MQTT 模块，并绑定已有消息队列
 * @param mq 业务消息队列句柄（用于发送 ETC_EVENT_NET_STATUS_UP 事件）
 */
rt_err_t mqtt_manager_init(rt_mq_t mq);

/**
 * @brief 启动 MQTT 客户端连接
 */
rt_err_t mqtt_manager_start(void);

/**
 * @brief 停止 MQTT 客户端
 */
rt_err_t mqtt_manager_stop(void);

/**
 * @brief 上报 5 个 ETC 核心属性到云端
 */
rt_err_t mqtt_manager_publish(const etc_data_t *data);

/**
 * @brief 检查 MQTT 是否连接
 */
rt_bool_t mqtt_manager_is_connected(void);

#endif /* APPLICATIONS_MODULES_MQTT_MANAGER_H_ */
