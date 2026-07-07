/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-03     20465       the first version
 */
#ifndef APPLICATIONS_MODULES_RFID_MANAGER_H_
#define APPLICATIONS_MODULES_RFID_MANAGER_H_

#include <rtthread.h>

/**
 * @brief 初始化 RFID 读卡模块
 * @return rt_err_t RT_EOK 表示成功，其他表示失败
 */
rt_err_t rfid_manager_init(void);

/**
 * @brief 检测并读取当前靠近的 RFID 卡片 UID 字符串
 * @param uid_buf 用于存储读取到的十六进制 UID 字符串缓冲区
 * @param buf_len 缓冲区长度（建议至少为 9 字节）
 * @return rt_err_t RT_EOK 表示成功读取，-RT_ERROR 表示未检出卡片或卡片类型不匹配，-RT_EINVAL 表示参数非法
 */
rt_err_t rfid_manager_read_uid(char *uid_buf, rt_size_t buf_len);

#endif /* APPLICATIONS_MODULES_RFID_MANAGER_H_ */
