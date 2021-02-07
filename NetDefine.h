#pragma once

#pragma comment(lib,"ws2_32")

#include <WinSock2.h>
#include <WS2tcpip.h>

#define MAX_SOCKBUF 1024
#define SIZE_IPBUF 32;

enum class IOOperationType
{
	RECV,
	SEND
};

struct OverlappedEx
{
	WSAOVERLAPPED		wsaOverlapped;
	SOCKET				socketClient;
	WSABUF				wsaBuf;
	IOOperationType		operationType;
};

struct ClientInfo
{
	UINT32 index = 0;
	SOCKET socketClient = NULL;
	OverlappedEx recvOverlappedEx;
	OverlappedEx sendOverlappedEx;
	char				recvBuf[MAX_SOCKBUF] = {0,};
	char				sendBuf[MAX_SOCKBUF] = { 0, };

	ClientInfo()
	{
		ZeroMemory(&recvOverlappedEx, sizeof(recvOverlappedEx));
		ZeroMemory(&sendOverlappedEx, sizeof(sendOverlappedEx));
		socketClient = INVALID_SOCKET;
	}

	bool IsValid()
	{
		return socketClient != INVALID_SOCKET;
	}
};