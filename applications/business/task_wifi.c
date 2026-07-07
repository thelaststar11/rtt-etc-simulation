#include <rtthread.h>
#include <wlan_mgnt.h>  /* RT-Thread 标准 WLAN 框架头文件 */
#include <wlan_prot.h>
#include <wlan_cfg.h>

#define WIFI_SSID       "iQOO Neo7 SE"  // 替换为您手机热点的名称
#define WIFI_PASSWORD   "li15850158873"      // 替换为您手机热点的密码

static void wifi_connect_entry(void *parameter)
{
    /* 延时 1.5 秒，保证底层硬件与 RW007 模块就绪 */
    rt_thread_mdelay(1500);

    rt_kprintf("[WIFI] Preparing to connect to hotspot: %s\n", WIFI_SSID);

    /* 开启 WLAN 自动重连机制（断开后会自动重连） */
    rt_wlan_config_autoreconnect(RT_TRUE);

    /* 阻塞式连接指定的 Wi-Fi 热点 */
    if (rt_wlan_connect(WIFI_SSID, WIFI_PASSWORD) == RT_EOK)
    {
        rt_kprintf("[WIFI] Connection request sent. Waiting for IP...\n");
    }
    else
    {
        rt_kprintf("[WIFI] Connect failed immediately. Check SSID or Password!\n");
    }
}

int wifi_auto_connect_init(void)
{
    /* 创建一个低优先级的临时线程来处理连网，避免阻塞系统启动 */
    rt_thread_t tid = rt_thread_create("wifi_con",
                                       wifi_connect_entry,
                                       RT_NULL,
                                       1536,
                                       20,
                                       10);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    return 0;
}
/* 导出为应用层自动初始化，系统启动时会自动调用此函数 */
INIT_APP_EXPORT(wifi_auto_connect_init);
