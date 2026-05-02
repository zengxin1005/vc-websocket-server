// HttpServer.cpp
#include "HttpServer.h"
#include "mongoose.h"
#include <windows.h>
#include <stdio.h>
#include <string>
#include <map>

#pragma comment(lib, "ws2_32.lib")

// ========== 全局变量 ==========
static struct mg_mgr g_mgr;
static volatile BOOL g_running = FALSE;
static HANDLE g_hThread = NULL;
static CRITICAL_SECTION g_cs;
static int g_port = 8080;
static char g_doc_root[MAX_PATH] = ".";
static struct mg_serve_http_opts g_http_opts;
static HttpHandlerCallback g_custom_callback = NULL;

// ========== 辅助函数 ==========
static void init_lock(void) {
    InitializeCriticalSection(&g_cs);
}
static void cleanup_lock(void) {
    DeleteCriticalSection(&g_cs);
}

// ========== 事件处理函数 ==========
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *) ev_data;
    
    switch (ev) {
        case MG_EV_HTTP_REQUEST: {
            BOOL handled = FALSE;
            char uri[256];
            snprintf(uri, sizeof(uri), "%.*s", (int)hm->uri.len, hm->uri.p);
            
            // 调用自定义回调（如果已设置）
            if (g_custom_callback != NULL) {
                char *response = NULL;
                size_t response_len = 0;
                char body_buf[4096];
                // 提取请求体（仅用于 POST/PUT）
                std::string body;
                if (hm->body.len > 0) {
                    body.assign(hm->body.p, hm->body.len);
                }
                handled = g_custom_callback(uri, 
                            (char*)hm->method.p, 
                            body.c_str(),
                            &response, &response_len);
                if (handled && response != NULL && response_len > 0) {
                    mg_printf(nc, "HTTP/1.1 200 OK\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: %d\r\n"
                              "\r\n%s",
                              (int)response_len, response);
                    free(response);
                } else if (handled) {
                    // 自定义回调已经发送响应，什么也不做
                }
            }
            
            // 如果未处理，回退到静态文件服务
            if (!handled) {
                mg_serve_http(nc, hm, g_http_opts);
                nc->flags |= MG_F_SEND_AND_CLOSE;
            }
            break;
        }
        default:
            break;
    }
}

// ========== 服务器线程 ==========
static DWORD WINAPI HttpThreadProc(LPVOID lpParam) {
    struct mg_connection *nc;
    mg_mgr_init(&g_mgr, NULL);
    
    // 设置 HTTP 服务器选项
    memset(&g_http_opts, 0, sizeof(g_http_opts));
    g_http_opts.document_root = g_doc_root;
    g_http_opts.enable_directory_listing = "yes";
    
    char portStr[32];
    sprintf_s(portStr, sizeof(portStr), "%d", g_port);
    nc = mg_bind(&g_mgr, portStr, ev_handler);
    if (nc == NULL) {
        printf("Failed to bind to port %d\n", g_port);
        return 1;
    }
    mg_set_protocol_http_websocket(nc);
    
    printf("========================================\n");
    printf("HTTP Server Started\n");
    printf("Port: %d\n", g_port);
    printf("Document Root: %s\n", g_doc_root);
    printf("========================================\n");
    
    while (g_running) {
        mg_mgr_poll(&g_mgr, 200);
    }
    
    mg_mgr_free(&g_mgr);
    return 0;
}

// ========== 公共接口实现 ==========
BOOL StartHttpServer(int port, const char* document_root) {
    if (g_running) return TRUE;
    
    g_port = port;
    if (document_root != NULL && document_root[0] != '\0') {
        strncpy_s(g_doc_root, sizeof(g_doc_root), document_root, _TRUNCATE);
    } else {
        // 默认使用当前 exe 所在目录
        GetModuleFileNameA(NULL, g_doc_root, sizeof(g_doc_root));
        char* last_slash = strrchr(g_doc_root, '\\');
        if (last_slash != NULL) *last_slash = '\0';
    }
    
    g_running = TRUE;
    init_lock();
    
    g_hThread = CreateThread(NULL, 0, HttpThreadProc, NULL, 0, NULL);
    if (g_hThread == NULL) {
        g_running = FALSE;
        cleanup_lock();
        return FALSE;
    }
    
    // 等待服务器启动
    Sleep(500);
    return TRUE;
}

void StopHttpServer(void) {
    if (!g_running) return;
    g_running = FALSE;
    if (g_hThread != NULL) {
        WaitForSingleObject(g_hThread, 5000);
        CloseHandle(g_hThread);
        g_hThread = NULL;
    }
    cleanup_lock();
    printf("HTTP server stopped\n");
}

BOOL IsHttpServerRunning(void) {
    return g_running;
}

void SetHttpHandler(HttpHandlerCallback callback) {
    EnterCriticalSection(&g_cs);
    g_custom_callback = callback;
    LeaveCriticalSection(&g_cs);
}

int GetHttpServerPort(void) {
    return g_port;
}