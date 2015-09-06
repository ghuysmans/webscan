#pragma once

#define WINVER 0x0600
#pragma comment(lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <windows.h>
#include <WS2tcpip.h>
#define ssize_t int
#define stricmp _stricmp
#define SHUT_RDWR SD_BOTH
#define SHUT_WR SD_SEND
#define SHUT_RD SD_RECEIVE