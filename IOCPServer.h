#pragma once

#include "NetDefine.h"

#include <thread>
#include <vector>

class IOCPServer
{
public:
	IOCPServer() = default;

	virtual ~IOCPServer()
	{
		WSACleanup();
	}

	bool InitSocket()
	{
		WSADATA wsaData;

		BOOL result = WSAStartup(MAKEWORD(2,2), &wsaData);

		if (0 != result)
		{
			printf("[에러] WSAStartup()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == listenSocket)
		{
			printf("[에러] socket()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		printf("소켓 초기화 성공\n");
		return true;
	}

	bool BindAndListen(int inBindPort)
	{
		SOCKADDR_IN serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(inBindPort);
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		BOOL result = bind(listenSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(SOCKADDR_IN));
		if (0 != result)
		{
			printf("[에러] bind()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		result = listen(listenSocket, 5);
		if (0 != result)
		{
			printf("[에러] listen()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		printf("서버 등록 성공..\n");
		return true;
	}

	bool StartServer(const UINT32 inMaxClientCount)
	{
		CreateClient(inMaxClientCount);

		iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, maxWorkerThreadNum);
		if (NULL == iocpHandle)
		{
			printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
			return false;
		}

		//접속된 클라이언트 주소 정보를 저장할 구조체
		bool bRet = CreateWorkerThread();
		if (false == bRet) {
			return false;
		}

		bRet = CreateAcceptThread();
		if (false == bRet) {
			return false;
		}

		printf("서버 시작\n");
		return true;
	}

	void DestroyThread()
	{
		isRun = false;
		CloseHandle(iocpHandle);

		for (auto& thread : workerThreads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}

		closesocket(listenSocket);
		
		if (acceptThread.joinable())
		{
			acceptThread.join();
		}

		printf("서버 종료\n");
	}

	virtual void OnConnect(const UINT32 inClientIndex) {}
	virtual void OnClose(const UINT32 inClientIndex) {}
	virtual void OnRecv(const UINT32 inClientIndex, const UINT32 inSize, char* inData) {}

private:
	void CreateClient(const UINT32 inMaxClientCount)
	{
		for (UINT32 i = 0; i < inMaxClientCount; ++i)
		{
			clientInfos.emplace_back();
			clientInfos[i].index = i;
		}
	}

	bool CreateWorkerThread()
	{
		for (UINT i = 0; i < maxWorkerThreadNum; ++i)
		{
			workerThreads.emplace_back([this]() {WorkerThread(); });
		}

		printf("WokerThread 시작..\n");
		return true;
	}

	bool CreateAcceptThread()
	{
		acceptThread = std::thread([this]() {AcceptThread(); });

		printf("AccepterThread 시작..\n");
		return true;
	}

	bool FindEmptyClientInfo(ClientInfo& outEmptyClient)
	{
		outEmptyClient = ClientInfo();

		for (auto& client : clientInfos)
		{
			if (client.IsValid() == false)
			{
				outEmptyClient = client;

				return true;
			}
		}
	
		return false;
	}

	bool BindIOCompletionPort(ClientInfo& clientInfo)
	{
		auto resultHandleIOCP = CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientInfo.socketClient), iocpHandle, reinterpret_cast<ULONG_PTR>(&clientInfo), 0);

		if (NULL == resultHandleIOCP || iocpHandle != resultHandleIOCP)
		{
			printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
			return false;
		}

		return true;
	}

	bool BindRecv(ClientInfo& inClientInfo)
	{
		DWORD flag = 0;
		DWORD recvNumBytes = 0;

		OverlappedEx& recvOverlappedEx = inClientInfo.recvOverlappedEx;

		recvOverlappedEx.wsaBuf.len = MAX_SOCKBUF;
		recvOverlappedEx.wsaBuf.buf = inClientInfo.recvBuf;
		recvOverlappedEx.operationType = IOOperationType::RECV;

		BOOL result = WSARecv(inClientInfo.socketClient, &(inClientInfo.recvOverlappedEx.wsaBuf),
			1,
			&recvNumBytes,
			&flag,
			reinterpret_cast<LPWSAOVERLAPPED>(&(inClientInfo.recvOverlappedEx)),
			NULL
		);

		if (SOCKET_ERROR == result && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			printf("[에러] WSARecv()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}

	bool SendMsg(ClientInfo& inClientInfo, char* msg, int msgLength)
	{
		DWORD recvNumBytes = 0;

		CopyMemory(inClientInfo.sendBuf, msg, msgLength);

		inClientInfo.sendBuf[msgLength] = '\0';

		OverlappedEx& overlappedEx = inClientInfo.sendOverlappedEx;

		overlappedEx.wsaBuf.len = msgLength;
		overlappedEx.wsaBuf.buf = inClientInfo.sendBuf;
		overlappedEx.operationType = IOOperationType::SEND;

		BOOL result = WSASend(inClientInfo.socketClient, &(overlappedEx.wsaBuf),
			1,
			&recvNumBytes,
			0,
			reinterpret_cast<LPWSAOVERLAPPED>(&(inClientInfo.sendOverlappedEx)),
			NULL);

		if (SOCKET_ERROR == result && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			printf("[에러] WSASend()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}

	void WorkerThread()
	{
		ClientInfo* clientInfo = nullptr;
		BOOL result = TRUE;
		DWORD ioSize = 0;
		LPOVERLAPPED lpOverlapped = nullptr;

		while (true == isRun)
		{
			result = GetQueuedCompletionStatus(iocpHandle, &ioSize, reinterpret_cast<PULONG_PTR>(&clientInfo), &lpOverlapped, INFINITE);

			// IOCP 핸들 닫는 후 처리
			if (TRUE == result && 0 == ioSize && NULL == lpOverlapped)
			{
				isRun = false;
				continue;
			}

			if (NULL == lpOverlapped)
			{
				continue;
			}

			// 소켓 연결이 끊길 때
			if (FALSE == result || (0 == ioSize && TRUE == result))
			{
				printf("socket(%d) 접속 끊김\n", (int)clientInfo->socketClient);
				CloseSocket(*clientInfo);
				continue;
			}

			OverlappedEx* overlappedEx = reinterpret_cast<OverlappedEx*>(lpOverlapped);

			if (IOOperationType::RECV == overlappedEx->operationType)
			{
				if (MAX_SOCKBUF <= ioSize)
				{
					ioSize = MAX_SOCKBUF - 1;
				}

				OnRecv(clientInfo->index, ioSize, clientInfo->recvBuf);

				SendMsg(*clientInfo, clientInfo->recvBuf, ioSize);
				BindRecv(*clientInfo);
			}
			else if (IOOperationType::SEND == overlappedEx->operationType)
			{
				printf("[송신] bytes : %d , msg : %s\n", ioSize, clientInfo->sendBuf);
			}
			else
			{
				printf("socket(%d)에서 예외상황\n", (int)clientInfo->socketClient);
				//throw
			}
		}

	}

	void AcceptThread()
	{
		SOCKADDR_IN clientAddr;
		int lenAddr = sizeof(clientAddr);
		ZeroMemory(&clientAddr, lenAddr);

		while (true == isRun)
		{
			bool result = false;

			ClientInfo emptyClientInfo;

			result = FindEmptyClientInfo(emptyClientInfo);
			if (result == false)
			{
				printf("[에러] Client Full\n");
				return;
			}

			emptyClientInfo.socketClient = accept(listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &lenAddr);
			if (emptyClientInfo.IsValid() == false)
			{
				printf("[에러] Client Full\n");
				return;
			}

			result = BindIOCompletionPort(emptyClientInfo);
//			result = BindRecv();
			if (false == result)
			{
				printf("[에러] Client Full\n");
				return;
			}

			result = BindRecv(emptyClientInfo);
			if (false == result)
			{
				printf("[에러] Client Full\n");
				return;
			}

			OnConnect(emptyClientInfo.index);

			currentConnectClient += 1;
		}

	}

	void CloseSocket(ClientInfo& inClientInfo, bool inIsForce = false)
	{
		struct linger linger = { 0,0 };

		if (true == inIsForce)
		{
			linger.l_onoff = 1;
		}

		shutdown(inClientInfo.socketClient, SD_BOTH);
		setsockopt(inClientInfo.socketClient, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&linger), sizeof(linger));
		closesocket(inClientInfo.socketClient);

		inClientInfo.socketClient = INVALID_SOCKET;

		currentConnectClient -= 1;

		OnClose(inClientInfo.index);
	}

	// 연결되어 있는 클라이언트 리스트
	std::vector<ClientInfo> clientInfos;

	// 대기 소켓
	SOCKET listenSocket = INVALID_SOCKET;

	// 현재 연결
	int currentConnectClient;

	std::vector<std::thread> workerThreads;
	std::thread acceptThread;
	HANDLE iocpHandle = INVALID_HANDLE_VALUE;
	bool isRun = false;
	
	UINT maxWorkerThreadNum = 4;
};
