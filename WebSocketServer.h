// WebSocketMongoose.h
#ifndef WEBSOCKET_MONGOOSE_H
#define WEBSOCKET_MONGOOSE_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL StartWebSocketServer(int port);
void StopWebSocketServer(void);
BOOL IsWebSocketRunning(void);
int GetWebSocketClientCount(void);

#ifdef __cplusplus
}
#endif

#endif