#include <rtthread.h>
#include <wlan_mgnt.h>  
#include <wlan_prot.h>
#include <wlan_cfg.h>

#define WIFI_SSID       "iQOO Neo7 SE" 
#define WIFI_PASSWORD   "li15850158873"      

static void wifi_connect_entry(void *parameter)
{
    /* 延时 1.5 秒，保证底层硬件与 RW007 模块就绪 */
    rt_thread_mdelay(1500);

    rt_kprintf("[WIFI] Preparing to connect to hotspot: %s\n", WIFI_SSID);

    /* 开启 WLAN 自动重连机制 */
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

INIT_APP_EXPORT(wifi_auto_connect_init);
