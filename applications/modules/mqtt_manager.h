#ifndef APPLICATIONS_MODULES_MQTT_MANAGER_H_
#define APPLICATIONS_MODULES_MQTT_MANAGER_H_

#include <rtthread.h>
#include "etc_types.h"

rt_err_t mqtt_manager_init(rt_mq_t mq);

rt_err_t mqtt_manager_start(void);

rt_err_t mqtt_manager_stop(void);

rt_err_t mqtt_manager_publish(const etc_data_t *data);

rt_bool_t mqtt_manager_is_connected(void);

#endif
