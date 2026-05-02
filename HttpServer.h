// HttpServer.h
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// 启动 HTTP 服务器，port：监听端口，document_root：网站根目录（可为 NULL，默认为当前 exe 目录）
BOOL StartHttpServer(int port, const char* document_root);

// 停止 HTTP 服务器
void StopHttpServer(void);

// 检查服务器是否正在运行
BOOL IsHttpServerRunning(void);

// 设置自定义请求处理回调
// 参数：请求 URI，方法，包含表单参数的原文，输出响应内容的指针（需自行 malloc），输出响应大小
// 返回值：TRUE 表示已处理，FALSE 表示未处理（将由静态文件服务处理）
typedef BOOL (*HttpHandlerCallback)(const char* uri, const char* method, const char* body, char** response, size_t* response_len);
void SetHttpHandler(HttpHandlerCallback callback);

// 获取服务器端口
int GetHttpServerPort(void);

#ifdef __cplusplus
}
#endif

#endif