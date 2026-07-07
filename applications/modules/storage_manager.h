/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-03     20465       the first version
 */
#ifndef APPLICATIONS_MODULES_STORAGE_MANAGER_H_
#define APPLICATIONS_MODULES_STORAGE_MANAGER_H_

#include "etc_types.h"

/**
 * @brief 初始化存储模块
 * @return rt_err_t RT_EOK 表示成功，其他表示失败
 */
rt_err_t etc_storage_init(void);

/**
 * @brief 根据卡 UID 读取本地绑定的车辆及余额数据
 * @param uid 输入的物理卡号（UID 字符串）
 * @param data 用于存储读取到数据的结构体指针
 * @return rt_err_t RT_EOK 表示读取成功且存在此卡，-RT_ERROR 表示未找到或读取失败
 */
rt_err_t etc_storage_read_user(const char *uid, etc_data_t *data);

/**
 * @brief 写入/更新特定 UID 的车辆及余额数据
 * @param uid 物理卡号（UID 字符串）
 * @param data 要写入的数据结构体指针
 * @return rt_err_t RT_EOK 表示写入成功
 */
rt_err_t etc_storage_write_user(const char *uid, const etc_data_t *data);

/**
 * @brief 删除特定 UID 的本地数据
 * @param uid 物理卡号（UID 字符串）
 * @return rt_err_t RT_EOK 表示删除成功
 */
rt_err_t etc_storage_delete_user(const char *uid);

/**
 * @brief 注册一些默认的测试卡数据（方便前期无卡或调试时快速验证）
 */
void etc_storage_register_defaults(void);

/**
 * @brief 脱机状态下，将当前未上报的交易原子化归档至脱机交易日志中
 * @param uid 物理卡号（UID 字符串）
 * @param data 交易成功后的最新数据结构体指针
 * @return rt_err_t RT_EOK 表示保存成功，其他表示失败
 */
rt_err_t etc_storage_save_offline_log(const char *uid, const etc_data_t *data);

/**
 * @brief 检索并提取本地缓存的最古老的一条未同步离线记录
 * @param log_out 存储检索到交易日志的结构体指针
 * @param filename_out 存储提取到的日志文件路径字符串
 * @param name_len 路径缓冲区的最大长度
 * @return rt_err_t RT_EOK 表示提取成功，其他表示失败或队列为空
 */
rt_err_t etc_storage_get_oldest_offline_log(etc_log_t *log_out, char *filename_out, rt_size_t name_len);

/**
 * @brief 同步上传成功后，彻底清除本地对应的脱机日志文件
 * @param filename 要删除的日志文件路径
 * @return rt_err_t RT_EOK 表示删除成功
 */
rt_err_t etc_storage_delete_offline_log(const char *filename);

#endif /* APPLICATIONS_MODULES_STORAGE_MANAGER_H_ */
