#include "wifi_task.h"
#include "esp_smartconfig.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "webserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_ESP32_WIFI_SUPPORT

/* 宏定义WiFi名称和密码 */
#define MY_WIFI_SSID CONFIG_WIFI_SSID
#define MY_WIFI_PASSWD CONFIG_WIFI_PASSWORD

/* 宏定义WiFi连接事件标志位、连接失败标志位及智能配网标志位 */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define SMART_CONFIG_BIT BIT2

/* 定义一个WiFi连接事件标志组句柄 */
static EventGroupHandle_t wifi_event_group_handler;

/* 系统事件循环处理函数 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    static int retry_num = 0; /* 记录wifi重连次数 */

    if (event_base == WIFI_EVENT) {
        /* 系统事件为WiFi事件 */

        if (event_id == WIFI_EVENT_STA_START) {
            /* 事件id为STA开始 */
            esp_wifi_connect();

        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            /* 事件id为失去STA连接 */
            esp_wifi_connect();
            retry_num++;
            printf("retry to connect to the AP %d times. \n", retry_num);
            if (retry_num > 10) /* WiFi重连次数大于10 */
            {
                /* 将WiFi连接事件标志组的WiFi连接失败事件位置1 */
                xEventGroupSetBits(wifi_event_group_handler, WIFI_FAIL_BIT);
            }

#ifdef CONFIG_ESP32_WEBSERVER
            stop_webserver();
#endif // CONFIG_ESP32_WEBSERVER

            /* 清除WiFi连接成功标志位 */
            xEventGroupClearBits(wifi_event_group_handler, WIFI_CONNECTED_BIT);
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        /* 系统事件为ip地址事件，且事件id为成功获取ip地址 */
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data; /* 获取IP地址信息*/
        printf("got ip:%d.%d.%d.%d \n", IP2STR(&event->ip_info.ip)); /* 打印ip地址*/
        retry_num = 0; /* WiFi重连次数清零 */
        /* 将WiFi连接事件标志组的WiFi连接成功事件位置1 */

#ifdef CONFIG_ESP32_WEBSERVER
        start_webserver();
#endif // CONFIG_ESP32_WEBSERVER

        xEventGroupSetBits(wifi_event_group_handler, WIFI_CONNECTED_BIT);

    } else if (event_base == SC_EVENT) {
        /* 系统事件为智能配网事件 */
        if (event_id == SC_EVENT_SCAN_DONE) {
            /* 开始扫描智能配网设备端 */
            printf("Scan done\n");

        } else if (event_id == SC_EVENT_FOUND_CHANNEL) {
            /* 得到了智能配网通道 */
            printf("Found channel \n");

        } else if (event_id == SC_EVENT_GOT_SSID_PSWD) {
            /* 得到了智能配网设备提供的ssid和password */
            printf("smartconfig got SSID and password\n");
            /* 获取智能配网设备端提供的数据信息 */
            smartconfig_event_got_ssid_pswd_t* evt = (smartconfig_event_got_ssid_pswd_t*)event_data;
            /* 定义WiFi配置结构体和用了结收ssid和password的数组 */
            wifi_config_t wifi_config;
            uint8_t ssid[33] = { 0 };
            uint8_t password[65] = { 0 };

            bzero(&wifi_config, sizeof(wifi_config_t)); /* 将结构体数据清零 */
            /* 将智能配网设备发送来的WiFi的ssid、password及MAC地址复制到wifi_config */
            memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
            memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
            wifi_config.sta.bssid_set = evt->bssid_set;
            if (wifi_config.sta.bssid_set == true) {
                memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
            }

            /* 打印WiFi名称和密码 */
            memcpy(ssid, evt->ssid, sizeof(evt->ssid));
            memcpy(password, evt->password, sizeof(evt->password));
            printf("SSID:%s \n", ssid);
            printf("PASSWORD:%s \n", password);

            /* 根据得到的WiFi名称和密码连接WiFi*/
            ESP_ERROR_CHECK(esp_wifi_disconnect());
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());

        } else if (event_id == SC_EVENT_SEND_ACK_DONE) {
            /* 智能配网成功，已经给智能配网设备发送应答*/
            xEventGroupSetBits(wifi_event_group_handler, SMART_CONFIG_BIT);
        }
    }
}

/**
 * @brief 智能配网 初始化
 *
 */
static void smartconfig_init_start(void)
{
    /* 设置智能配网类型为 AirKiss */
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_AIRKISS));
    /* 通过SMARTCONFIG_START_CONFIG_DEFAULT宏 来获取一个默认的smartconfig配置参数结构体变量*/
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    /* 开始智能配网 */
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
    printf("smartconfig start ....... \n");

    /* 使用事件标志组等待连接建立(WIFI_CONNECTED_BIT)或连接失败(WIFI_FAIL_BIT)事件 */
    EventBits_t uxBits; /* 定义一个事件位变量来接收事件标志组等待函数的返回值 */
    /* 等待事件标志组，退出前清除设置的事件标志，任意置1就会返回*/
    uxBits = xEventGroupWaitBits(wifi_event_group_handler, WIFI_CONNECTED_BIT | SMART_CONFIG_BIT, true, false, portMAX_DELAY);

    if (uxBits & WIFI_CONNECTED_BIT) {
        printf("WiFi Connected to ap ok. \n");
    }

    if (uxBits & SMART_CONFIG_BIT) {
        printf("smartconfig over \n");
        esp_smartconfig_stop(); /* 关闭智能配网 */
    }
}

void wifi_task(void* arg)
{
    /* 创建一个事件标志组 */
    wifi_event_group_handler = xEventGroupCreate();

    /* 创建一个LwIP核心任务，并初始化 LwIP 相关工作。初始化底层TCP/IP堆栈。在应用程序启动时，应该调用此函数一次。*/
    ESP_ERROR_CHECK(esp_netif_init());

    /* 创建一个系统事件任务，并初始化应用程序事件的回调函数。*/
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 创建一个默认的WIFI-STA网络接口，如果初始化错误，此API将中止。*/
    esp_netif_create_default_wifi_sta();

    /* 使用WIFI_INIT_CONFIG_DEFAULT() 来获取一个默认的wifi配置参数结构体变量*/
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    /* 根据cfg参数初始化wifi连接所需要的资源 */
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 将事件处理程序注册到系统默认事件循环，分别是WiFi事件、IP地址事件及smartconfig事件 */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /* 定义WiFi连接的ssid和password参数 */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MY_WIFI_SSID,
            .password = MY_WIFI_PASSWD,
        },
    };
    /* 设置WiFi的工作模式为 STA */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    /* 设置WiFi连接的参数，主要是ssid和password */
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    /* 启动WiFi连接 */
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("wifi_init_sta finished. \n");

    /* 使用事件标志组等待连接建立(WIFI_CONNECTED_BIT)或连接失败(WIFI_FAIL_BIT)事件 */
    EventBits_t bits; /* 定义一个事件位变量来接收事件标志组等待函数的返回值 */
    bits = xEventGroupWaitBits(wifi_event_group_handler, /* 需要等待的事件标志组的句柄 */
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, /* 需要等待的事件位 */
        pdFALSE, /* 为pdFALSE时，在退出此函数之前所设置的这些事件位不变，为pdFALSE则清零*/
        pdFALSE, /* 为pdFALSE时，设置的这些事件位任意一个置1就会返回，为pdFALSE则需全为1才返回 */
        portMAX_DELAY); /* 设置为最长阻塞等待时间，单位为时钟节拍 */

    /* 根据事件标志组等待函数的返回值获取WiFi连接状态 */
    if (bits & WIFI_CONNECTED_BIT) {
        /* WiFi连接成功事件 */
        printf("connected to ap SSID:%s OK \n", MY_WIFI_SSID);

    } else if (bits & WIFI_FAIL_BIT) {
        /* WiFi连接失败事件 */
        printf("Failed to connect to SSID:%s \n", MY_WIFI_SSID);
        smartconfig_init_start(); /* 开启智能配网 */

    } else {
        printf("UNEXPECTED EVENT \n"); /* 没有等待到事件 */
        smartconfig_init_start(); /* 开启智能配网 */
    }

    vEventGroupDelete(wifi_event_group_handler); /* 删除WiFi连接事件标志组，WiFi连接成功后不再需要 */
    vTaskDelete(NULL);
}

#endif // CONFIG_ESP32_WIFI_SUPPORT
