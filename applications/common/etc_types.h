/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef APPLICATIONS_COMMON_ETC_TYPES_H_
#define APPLICATIONS_COMMON_ETC_TYPES_H_

#include <rtthread.h>

/* 核心事件定义 */
#define ETC_EVENT_NET_STATUS_UP   1
#define ETC_EVENT_CARD_DETECTED   2

/* ETC 卡用户数据结构 */
typedef struct {
    double balance;         /* 当前余额 */
    double deduct_amount;             /* 上一次扣费额度 */
    rt_int32_t detect_status;      /* 状态标志：0-正常，1-锁定 */
    char plate_num[16];     /* 车牌号 */
    char vehicle_type[16];  /* 车辆类型 */
} etc_data_t;

/* 业务消息队列消息包 */
typedef struct {
    rt_uint32_t event_id;   /* 事件ID */
    char card_uid[16];      /* 卡片UID字符串 */
} etc_msg_t;

/* 掉电容灾离线交易日志结构（固定大小，便于文件原子读写） */
typedef struct {
    char uid[16];           /* 交易卡片 UID */
    etc_data_t data;        /* 交易后的最新数据快照 */
} etc_log_t;

#endif /* APPLICATIONS_COMMON_ETC_TYPES_H_ */
