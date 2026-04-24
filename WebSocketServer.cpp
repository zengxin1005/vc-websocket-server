// WebSocketServer.cpp
#include "WebSocketServer.h"
#include "mongoose.h"
#include <windows.h>
#include <stdio.h>
#include <map>

#pragma comment(lib, "ws2_32.lib")

// ========== 全局变量 ==========
static struct mg_mgr g_mgr;
static volatile BOOL g_running = FALSE;
static HANDLE g_hThread = NULL;
static CRITICAL_SECTION g_cs;
static int g_clientCount = 0;
static int g_port = 30301;
static struct mg_serve_http_opts g_http_server_opts;

// 存储所有 WebSocket 连接
static std::map<struct mg_connection*, BOOL> g_websocket_clients;

// 初始化锁
static void init_lock(void) {
    InitializeCriticalSection(&g_cs);
}

static void cleanup_lock(void) {
    DeleteCriticalSection(&g_cs);
}

// 判断是否是 WebSocket 连接
static int is_websocket(const struct mg_connection *nc) {
    return (nc->flags & MG_F_IS_WEBSOCKET) != 0;
}

// 获取客户端数量
static int get_client_count(void) {
    int count;
    EnterCriticalSection(&g_cs);
    count = (int)g_websocket_clients.size();
    LeaveCriticalSection(&g_cs);
    return count;
}

// 广播消息
static void broadcast(struct mg_connection *nc, const char *msg, size_t len) {
    struct mg_connection *c;
    char buf[500];
    
    snprintf(buf, sizeof(buf), "%p %.*s", nc, (int)len, msg);
    
    EnterCriticalSection(&g_cs);
    for (c = mg_next(&g_mgr, NULL); c != NULL; c = mg_next(&g_mgr, c)) {
        if (is_websocket(c)) {
            mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, buf, strlen(buf));
        }
    }
    LeaveCriticalSection(&g_cs);
}

// 事件处理函数
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *)ev_data;
    struct websocket_message *wm = (struct websocket_message *)ev_data;
    
    switch (ev) {
        case MG_EV_HTTP_REQUEST:
            // HTTP 请求 - 提供静态文件服务
            mg_serve_http(nc, hm, g_http_server_opts);
            nc->flags |= MG_F_SEND_AND_CLOSE;
            break;
            
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
            // 新 WebSocket 连接
            EnterCriticalSection(&g_cs);
            g_websocket_clients[nc] = TRUE;
            g_clientCount = (int)g_websocket_clients.size();
            LeaveCriticalSection(&g_cs);
            
            printf("[%lu] WebSocket client connected, total: %d\n", 
                   GetTickCount(), get_client_count());
            broadcast(nc, "joined", 6);
            break;
            
        case MG_EV_WEBSOCKET_FRAME:
            // 收到 WebSocket 消息
            printf("[%lu] Received: %.*s\n", GetTickCount(), (int)wm->size, (char*)wm->data);
            broadcast(nc, (char*)wm->data, wm->size);
            break;
            
        case MG_EV_CLOSE:
            // 连接关闭
            if (is_websocket(nc)) {
                EnterCriticalSection(&g_cs);
                g_websocket_clients.erase(nc);
                g_clientCount = (int)g_websocket_clients.size();
                LeaveCriticalSection(&g_cs);
                
                printf("[%lu] WebSocket client disconnected, total: %d\n", 
                       GetTickCount(), get_client_count());
                broadcast(nc, "left", 4);
            } else {
                printf("[%lu] HTTP connection closed\n", GetTickCount());
            }
            break;
            
        default:
            break;
    }
}

// WebSocket 服务器线程
static DWORD WINAPI WebSocketThreadProc(LPVOID lpParam) {
    struct mg_connection *nc;
    
    mg_mgr_init(&g_mgr, NULL);
    
    // 配置 HTTP 服务器选项
    g_http_server_opts.document_root = ".";
    
    char portStr[32];
    sprintf_s(portStr, sizeof(portStr), "%d", g_port);
    
    nc = mg_bind(&g_mgr, portStr, ev_handler);
    if (nc == NULL) {
        printf("Failed to bind to port %d\n", g_port);
        return 1;
    }
    
    // 设置 HTTP + WebSocket 协议
    mg_set_protocol_http_websocket(nc);
    
    printf("========================================\n");
    printf("WebSocket Server Started on port %d\n", g_port);
    printf("HTTP:  http://localhost:%d/\n", g_port);
    printf("WebSocket: ws://localhost:%d/\n", g_port);
    printf("========================================\n");
    
    while (g_running) {
        mg_mgr_poll(&g_mgr, 200);
    }
    
    mg_mgr_free(&g_mgr);
    return 0;
}

// ========== 公共接口实现 ==========

BOOL StartWebSocketServer(int port) {
    if (g_running) {
        return TRUE;
    }
    
    g_port = port;
    g_running = TRUE;
    init_lock();
    
    g_hThread = CreateThread(NULL, 0, WebSocketThreadProc, NULL, 0, NULL);
    if (g_hThread == NULL) {
        g_running = FALSE;
        cleanup_lock();
        return FALSE;
    }
    
    // 等待服务器启动
    Sleep(500);
    
    return TRUE;
}

void StopWebSocketServer(void) {
    if (!g_running) {
        return;
    }
    
    g_running = FALSE;
    
    if (g_hThread != NULL) {
        WaitForSingleObject(g_hThread, 5000);
        CloseHandle(g_hThread);
        g_hThread = NULL;
    }
    
    cleanup_lock();
    printf("WebSocket server stopped\n");
}

BOOL IsWebSocketRunning(void) {
    return g_running;
}

int GetWebSocketClientCount(void) {
    return get_client_count();
}