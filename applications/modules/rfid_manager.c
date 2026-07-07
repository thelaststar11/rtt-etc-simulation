/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-03     20465       the first version
 */

#include "rfid_manager.h"
#include <rtthread.h>
#include "mfrc522.h"

rt_err_t rfid_manager_init(void)
{
    /* 初始化硬件引脚 */
    MFRC522(MFRC522_SS_PIN, MFRC522_RST_PIN);

    /* 初始化 MFRC522 芯片 */
    PCD_Init();

    return RT_EOK;
}

rt_err_t rfid_manager_read_uid(char *uid_buf, rt_size_t buf_len)
{
    if (uid_buf == RT_NULL || buf_len < 9) /* 4字节UID需要8个字符加1个結束符 '\0' */
    {
        return -RT_EINVAL;
    }

    /* 寻卡与防冲突 */
    if (!PICC_IsNewCardPresent() || !PICC_ReadCardSerial())
    {
        return -RT_ERROR;
    }

    /* 获取全局 UID 结构体指针并校验卡片类型 */
    Uid *uid = get_uid();
    enum PICC_Type piccType = PICC_GetType(uid->sak);
    if (piccType != PICC_TYPE_MIFARE_1K)
    {
        PICC_HaltA();
        PCD_StopCrypto1();
        return -RT_ERROR;
    }

    /* 将读取到的物理 UID（4字节十六进制）格式化为字符串 */
    rt_snprintf(uid_buf, buf_len, "%02X%02X%02X%02X",
                uid->uidByte[0], uid->uidByte[1], uid->uidByte[2], uid->uidByte[3]);

    /* 挂起卡片与结束加密 */
    PICC_HaltA();
    PCD_StopCrypto1();

    return RT_EOK;
}
