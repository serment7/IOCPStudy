#pragma once

#pragma comment(lib,"ws2_32")

#include <WinSock2.h>
#include <WS2tcpip.h>

namespace SBNetLib
{
	const UINT32 MAX_SOCKBUF = 256;
	const UINT32 MAX_SOCK_SENDBUF = 4096;
	const UINT32 MAX_WORKERTHREAD = 4;

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


}