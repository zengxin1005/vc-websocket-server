# vc-websocket-server
VC WebSocketServer 是一个使用原生 C/C++ 及 Win32 API 构建的高性能、轻量级 WebSocket 服务器库。旨在为 VC++ 桌面应用（如 MFC、Qt 程序）提供简洁、易于嵌入的 WebSocket 服务器方案。
```cpp
BOOL CServerDlg::OnInitDialog() {

    // 启动 WebSocket 服务器
    if (StartWebSocketServer(30301)) {
        PrintText("WebSocket server started on port 30301");
    } else {
        PrintText("WebSocket server failed to start");
    }

    return TRUE;
}

void CServerDlg::OnClose() {
    StopWebSocketServer();
}

//HTTP SERVER
// 自定义请求处理函数
BOOL MyHttpHandler(const char* uri, const char* method, const char* body, 
                   char** response, size_t* response_len) {
    // 只处理 POST /api/login
    if (strcmp(uri, "/api/login") == 0 && strcmp(method, "POST") == 0) {
        // 解析 body 中的参数 (例如 "username=abc&password=123")
        char username[64] = {0}, password[64] = {0};
        mg_get_http_var((const struct mg_str*)body, "username", username, sizeof(username));
        mg_get_http_var((const struct mg_str*)body, "password", password, sizeof(password));
        
        // 业务逻辑...
        const char* result = "{\"code\":0,\"msg\":\"success\"}";
        *response_len = strlen(result);
        *response = (char*)malloc(*response_len + 1);
        strcpy(*response, result);
        return TRUE;
    }
    // 处理其他API...
    return FALSE; // 未处理，将由静态文件服务处理
}

BOOL CServerDlg::OnInitDialog() {

    // 启动 HTTP 服务器（端口 8080，文档根目录为当前 exe 所在目录下的 web 文件夹）
    if (StartHttpServer(8080, "./web")) {
        SetHttpHandler(MyHttpHandler);
        PrintText("HTTP server started on port 8080");
    } else {
        PrintText("HTTP server failed to start");
    }

    return TRUE;
}

void CServerDlg::OnClose() {
   StopHttpServer();
}

