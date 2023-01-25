#include "webserver.h"

#ifdef CONFIG_ESP32_WEBSERVER

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

static httpd_handle_t server = NULL;
static const httpd_uri_t URIs[];

/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t* req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/echo", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Post请求测试
 *
 * @param req
 * @return esp_err_t
 */
static esp_err_t echo_post_handler(httpd_req_t* req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    // 接收Post来的参数
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }

        // 将参数回传到客户端
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        // 打印参数内容
        printf("=========== RECEIVED DATA ==========\r\n");
        printf("%.*s\r\n", ret, buf);
        printf("====================================\r\n");
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief HEELO请求
 *
 * @param req
 * @return esp_err_t
 */
static esp_err_t hello_get_handler(httpd_req_t* req)
{
    // char* buf;
    // size_t buf_len;

#if 0
    // 使用HTTP 工具请求的时候 可以设置自定义请求头
    /* 获取请求头长度 */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* 获取请求头内容 */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            printf("Found header => Host: %s\r\n", buf);
        }
        free(buf);
    }
#endif

#if 0
    // 得到URL后面的参数
    /* http://192.168.1.121/hello?query1=1&query2=2 */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            printf("Found URL query => %s\r\n", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                printf("Found URL query parameter => query1=%s\r\n", param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                printf("Found URL query parameter => query3=%s\r\n", param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                printf("Found URL query parameter => query2=%s\r\n", param);
            }
        }
        free(buf);
    }
#endif

    /* 设置自定义请求头 */
    // httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    // httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* 发送HTTP正文数据 */
    const char* resp_str = (const char*)req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request  headers are lost. Check if HTTP request headers can be read now. */
    // if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
    //     printf("Request headers lost\r\n");
    // }
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const httpd_uri_t URIs[] = {
    // 测试程序
    {
        .uri = "/hello",
        .method = HTTP_GET,
        .handler = hello_get_handler,
        .user_ctx = "Hello ESP32!",
    },
    {
        .uri = "/echo",
        .method = HTTP_POST,
        .handler = echo_post_handler,
        .user_ctx = NULL,
    },

    // 文件操作
    // {
    //     .uri = "/*",
    //     .method = HTTP_GET,
    //     .handler = download_get_handler,
    //     .user_ctx = NULL,
    // },

    // {
    //     .uri = "/upload/*", // Match all URIs of type /upload/path/to/file
    //     .method = HTTP_POST,
    //     .handler = upload_post_handler,
    //     .user_ctx = NULL,
    // },

    // {
    //     .uri = "/delete/*", // Match all URIs of type /delete/path/to/file
    //     .method = HTTP_POST,
    //     .handler = delete_post_handler,
    //     .user_ctx = NULL,
    // },

    //
    // {},
};

/**
 * @brief 注册所有URI
 *
 * @param hd
 */
static void register_basic_handlers(httpd_handle_t hd)
{
    printf("Registering URIs");

    httpd_register_err_handler(hd, HTTPD_404_NOT_FOUND, http_404_error_handler);

    const int basic_handlers_no = sizeof(URIs) / sizeof(httpd_uri_t);
    for (uint32_t i = 0; i < basic_handlers_no; i++) {
        if (httpd_register_uri_handler(hd, &URIs[i]) != ESP_OK) {
            printf("register uri failed for %d", i);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 启动HTTP服务器
 *
 */
void start_webserver(void)
{
    if (NULL == server) {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.lru_purge_enable = true;

        printf("Starting server on port: '%d'\r\n", config.server_port);

        esp_err_t err = httpd_start(&server, &config);
        if (err == ESP_OK) {
            // register_basic_handlers(server);
            // start_file_server("/sdcard", server);
            printf("starting server success!\r\n");

        } else if (err == ESP_ERR_INVALID_ARG) {
            printf("Error starting server! = Null argument\r\n");
        } else if (err == ESP_ERR_HTTPD_ALLOC_MEM) {
            printf("Error starting server! = Failed to allocate memory for instance\r\n");
        } else if (err == ESP_FAIL) {
            printf("Error starting server! = ESP_FAIL\r\n");
        } else if (err == ESP_ERR_HTTPD_TASK) {
            printf("Error starting server! = Failed to launch server task\r\n");
        } else {
            printf("Error starting server!\r\n");
        }
    }
}

/**
 * @brief 停止HTTP服务器
 *
 */
void stop_webserver(void)
{
    if (NULL != server) {
        httpd_stop(server);
        server = NULL;
    }
}

#endif // CONFIG_ESP32_WEBSERVER
