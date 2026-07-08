#ifndef APPLICATIONS_COMMON_ETC_TYPES_H_
#define APPLICATIONS_COMMON_ETC_TYPES_H_

#include <rtthread.h>

#define ETC_EVENT_NET_STATUS_UP   1
#define ETC_EVENT_CARD_DETECTED   2

/* ETC 卡用户数据结构 */
typedef struct {
    double balance;         
    double deduct_amount;            
    rt_int32_t detect_status;     
    char plate_num[16];    
    char vehicle_type[16];  
} etc_data_t;

/* 业务消息队列消息包 */
typedef struct {
    rt_uint32_t event_id;   
    char card_uid[16];     
} etc_msg_t;

/* 掉电容灾离线交易日志结构 */
typedef struct {
    char uid[16];           
    etc_data_t data;        
} etc_log_t;

#endif
