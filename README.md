# vc-websocket-server
VC WebSocketServer 是一个使用原生 C/C++ 及 Win32 API 构建的高性能、轻量级 WebSocket 服务器库。旨在为 VC++ 桌面应用（如 MFC、Qt 程序）提供简洁、易于嵌入的 WebSocket 服务器方案。
#C++
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
