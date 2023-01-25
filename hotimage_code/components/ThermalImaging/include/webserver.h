#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <esp_http_server.h>

void start_webserver(void);
void stop_webserver(void);
esp_err_t start_file_server(const char* base_path, httpd_handle_t server);

#ifdef __cplusplus
}
#endif
