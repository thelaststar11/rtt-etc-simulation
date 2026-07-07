/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-03     20465       the first version
 */
#include <rtthread.h>
#include <stdlib.h>
#include <string.h>
#include "paho_mqtt.h"
#include "cjson.h"
#include "etc_types.h"
#include "mqtt_manager.h"

#define DBG_TAG "mqtt_mgr"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* ==================== OneNET 参数配置 ==================== */
// OneNET MQTT 服务器地址 (端口 1883)
#define MQTT_URI           "tcp://7fCY77B9m6.mqtts.acc.cmcconenet.cn:1883"

// 请替换为你 OneNET 上的实际信息
#define MQTT_CLIENT_ID     "etc_device"    // 设备名称 (DeviceName)
#define MQTT_USERNAME      "7fCY77B9m6"     // 产品ID (ProductID)
#define MQTT_PASSWORD      "version=2018-10-31&res=products%2F7fCY77B9m6%2Fdevices%2Fetc_device&et=1830268800&method=sha1&sign=zPyegbigLKidPo4IiiDWPgKFvxI%3D"          // 使用 OneNET Token 工具生成的鉴权 Token

// OneNET 物模型属性上报与设置 Topic ($sys/{pid}/{device-name}/thing/property/...)
#define MQTT_PUB_TOPIC     "$sys/7fCY77B9m6/etc_device/thing/property/post"
#define MQTT_SUB_TOPIC     "$sys/7fCY77B9m6/etc_device/thing/property/set"

static MQTTClient client;
static rt_mq_t business_mq = RT_NULL;
static rt_bool_t is_started = RT_FALSE;

/* 接收云端属性设置的解析回调 */
static void mqtt_sub_callback(MQTTClient *c, MessageData *msg_data)
{
    if (msg_data == RT_NULL || msg_data->message == RT_NULL) return;

    char *payload = rt_malloc(msg_data->message->payloadlen + 1);
    if (payload == RT_NULL) return;

    rt_memcpy(payload, msg_data->message->payload, msg_data->message->payloadlen);
    payload[msg_data->message->payloadlen] = '\0';

    cJSON *root = cJSON_Parse(payload);
    rt_free(payload);

    if (root == RT_NULL) return;

    cJSON *params = cJSON_GetObjectItem(root, "params");
    if (params != RT_NULL)
    {
        /* 仅解析物模型中已定义的内容并打印输出，不产生多余事件 */
        cJSON *balance = cJSON_GetObjectItem(params, "balance");
        if (balance) LOG_I("Cloud set balance: %f", balance->valuedouble);

        cJSON *fee = cJSON_GetObjectItem(params, "fee");
        if (fee) LOG_I("Cloud set fee: %f", fee->valuedouble);

        cJSON *status = cJSON_GetObjectItem(params, "status");
        if (status) LOG_I("Cloud set status: %d", status->valueint);

        cJSON *plate = cJSON_GetObjectItem(params, "plate_num");
        if (plate) LOG_I("Cloud set plate_num: %s", plate->valuestring);

        cJSON *type = cJSON_GetObjectItem(params, "vehicle_type");
        if (type) LOG_I("Cloud set vehicle_type: %s", type->valuestring);
    }

    cJSON_Delete(root);
}

/* MQTT 连接成功回调 */
static void mqtt_online_callback(MQTTClient *c)
{
    LOG_I("MQTT Client Online.");

    /* 触发已有的网络就绪事件，通知主业务线程 */
    if (business_mq != RT_NULL)
    {
        etc_msg_t msg;
        rt_memset(&msg, 0, sizeof(etc_msg_t));
        msg.event_id = ETC_EVENT_NET_STATUS_UP;

        rt_mq_send(business_mq, &msg, sizeof(etc_msg_t));
    }
}

static void mqtt_offline_callback(MQTTClient *c)
{
    LOG_W("MQTT Client Offline.");
}

rt_err_t mqtt_manager_init(rt_mq_t mq)
{
    if (mq == RT_NULL) return -RT_EINVAL;
    business_mq = mq;

    rt_memset(&client, 0, sizeof(MQTTClient));
    client.uri = MQTT_URI;
    /* 1. 首先确保初始化了 condata 连接包 */
    MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
    memcpy(&client.condata, &condata, sizeof(condata));

    /* 2. 将连接信息赋值到 condata 子成员中（注意：是 clientID 大写 D） */
    client.condata.clientID.cstring = MQTT_CLIENT_ID;
    client.condata.username.cstring = MQTT_USERNAME;
    client.condata.password.cstring = MQTT_PASSWORD;

    client.buf_size = 1024;
    client.readbuf_size = 1024;
    client.buf = rt_malloc(client.buf_size);
    client.readbuf = rt_malloc(client.readbuf_size);

    if (client.buf == RT_NULL || client.readbuf == RT_NULL)
    {
        if (client.buf) rt_free(client.buf);
        if (client.readbuf) rt_free(client.readbuf);
        return -RT_ENOMEM;
    }

    client.connect_callback = mqtt_online_callback;
    client.online_callback = mqtt_online_callback;
    client.offline_callback = mqtt_offline_callback;

    client.messageHandlers[0].topicFilter = MQTT_SUB_TOPIC;
    client.messageHandlers[0].callback = mqtt_sub_callback;
    client.messageHandlers[0].qos = QOS0;

    return RT_EOK;
}

rt_err_t mqtt_manager_start(void)
{
    if (is_started) return RT_EOK;

    int ret = paho_mqtt_start(&client);
    if (ret == 0)
    {
        is_started = RT_TRUE;
        return RT_EOK;
    }
    return -RT_ERROR;
}

rt_err_t mqtt_manager_stop(void)
{
    if (!is_started) return RT_EOK;

    paho_mqtt_stop(&client);
    is_started = RT_FALSE;
    return RT_EOK;
}

rt_err_t mqtt_manager_publish(const etc_data_t *data)
{
    if (data == RT_NULL) return -RT_EINVAL;
    if (!mqtt_manager_is_connected()) return -RT_ERROR;

    cJSON *root = cJSON_CreateObject();
    if (root == RT_NULL) return -RT_ENOMEM;

    // OneNET 通常需要 id 和 version 字段
    cJSON_AddStringToObject(root, "id", "123");
    cJSON_AddStringToObject(root, "version", "1.0");

    cJSON *params = cJSON_CreateObject();
    if (params == RT_NULL)
    {
        cJSON_Delete(root);
        return -RT_ENOMEM;
    }

    /* OneNET 物模型属性数据格式：{"value": xxx} */
    cJSON *balance_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(balance_obj, "value", data->balance);
    cJSON_AddItemToObject(params, "balance", balance_obj);

    cJSON *deduct_amount_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(deduct_amount_obj, "value", data->deduct_amount);
    cJSON_AddItemToObject(params, "deduct_amount", deduct_amount_obj);

    cJSON *detect_status_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(detect_status_obj, "value", (double)data->detect_status);
    cJSON_AddItemToObject(params, "detect_status", detect_status_obj);

    cJSON *plate_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(plate_obj, "value", data->plate_num);
    cJSON_AddItemToObject(params, "plate_num", plate_obj);

    cJSON *type_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(type_obj, "value", data->vehicle_type);
    cJSON_AddItemToObject(params, "vehicle_type", type_obj);

    cJSON_AddItemToObject(root, "params", params);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str == RT_NULL) return -RT_ENOMEM;
    /* 建议：在此处打印生成的 JSON 报文内容 */
    LOG_D("Publishing payload: %s", json_str);
    int ret = paho_mqtt_publish(&client, QOS1, MQTT_PUB_TOPIC, json_str);
    rt_free(json_str);

    return (ret == 0) ? RT_EOK : -RT_ERROR;
}

rt_bool_t mqtt_manager_is_connected(void)
{
    return (rt_bool_t)client.isconnected;
}
