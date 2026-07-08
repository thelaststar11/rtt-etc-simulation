#ifndef APPLICATIONS_BUSINESS_ETC_APP_H_
#define APPLICATIONS_BUSINESS_ETC_APP_H_

#include <rtthread.h>

/* 获取业务层消息队列句柄，供 RFID 等模块初始化时使用 */
rt_mq_t etc_app_get_mq(void);

/* 初始化业务层并启动主业务线程 */
rt_err_t etc_app_init(void);

#endif /* APPLICATIONS_BUSINESS_ETC_APP_H_ */
