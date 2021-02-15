#include "IOCPServer.h"

namespace SBNetLib
{
	bool IOCPServer::InitSocket()
	{
		WSADATA wsaData;

		BOOL result = WSAStartup(MAKEWORD(2, 2), &wsaData);

		if (0 != result)
		{
			printf("[����] WSAStartup()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == listenSocket)
		{
			printf("[����] socket()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		printf("���� �ʱ�ȭ ����\n");
		return true;
	}

	bool IOCPServer::BindAndListen(int inBindPort)
	{
		SOCKADDR_IN serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(inBindPort);
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		BOOL result = bind(listenSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(SOCKADDR_IN));
		if (0 != result)
		{
			printf("[����] bind()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		result = listen(listenSocket, 5);
		if (0 != result)
		{
			printf("[����] listen()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		printf("���� ��� ����..\n");
		return true;
	}

	bool IOCPServer::StartServer(const UINT32 inMaxClientCount)
	{
		CreateClient(inMaxClientCount);

		iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, maxWorkerThreadNum);
		if (NULL == iocpHandle)
		{
			printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
			return false;
		}

		//���ӵ� Ŭ���̾�Ʈ �ּ� ������ ������ ����ü
		bool bRet = CreateWorkerThread();
		if (false == bRet) {
			return false;
		}

		bRet = CreateAcceptThread();
		if (false == bRet) {
			return false;
		}

		printf("���� ����\n");
		return true;
	}

	void IOCPServer::DestroyThread()
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

		printf("���� ����\n");
	}

	bool IOCPServer::SendMsg(const UINT32 inSessionIndex, const UINT32 dataSize_, char* pData)
	{
		auto pClient = GetClientInfo(inSessionIndex);
		return pClient.SendMsg(dataSize_, pData);
	}


	void IOCPServer::CreateClient(const UINT32 inMaxClientCount)
	{
		for (UINT32 i = 0; i < inMaxClientCount; ++i)
		{
			clientInfos.emplace_back();
			clientInfos[i].Init(i);
		}
	}

	bool IOCPServer::CreateWorkerThread()
	{
		for (UINT i = 0; i < maxWorkerThreadNum; ++i)
		{
			workerThreads.emplace_back([this]() {WorkerThread(); });
		}

		printf("WokerThread ����..\n");
		return true;
	}

	bool IOCPServer::FindEmptyClientInfo(SBNetLib::ClientInfo& outEmptyClient)
	{
		outEmptyClient = SBNetLib::ClientInfo();

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

	SBNetLib::ClientInfo& IOCPServer::GetClientInfo(const UINT32 inSessionIndex)
	{
		return clientInfos[inSessionIndex];
	}

	bool IOCPServer::CreateAcceptThread()
	{
		acceptThread = std::thread([this]() {AcceptThread(); });

		printf("AccepterThread ����..\n");
		return true;
	}

	void IOCPServer::WorkerThread()
	{
		SBNetLib::ClientInfo* clientInfo = nullptr;
		BOOL result = TRUE;
		DWORD ioSize = 0;
		LPOVERLAPPED lpOverlapped = nullptr;

		while (true == isRun)
		{
			result = GetQueuedCompletionStatus(iocpHandle, &ioSize, reinterpret_cast<PULONG_PTR>(&clientInfo), &lpOverlapped, INFINITE);

			// IOCP �ڵ� �ݴ� �� ó��
			if (TRUE == result && 0 == ioSize && NULL == lpOverlapped)
			{
				isRun = false;
				continue;
			}

			if (NULL == lpOverlapped)
			{
				continue;
			}

			// ���� ������ ���� ��
			if (FALSE == result || (0 == ioSize && TRUE == result))
			{
				printf("socket(%d) ���� ����\n", (int)clientInfo->GetSocket());
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

				OnRecv(clientInfo->GetIndex(), ioSize, clientInfo->GetRecvBuffer());

				clientInfo->BindRecv();
			}
			else if (IOOperationType::SEND == overlappedEx->operationType)
			{
				delete[] overlappedEx->wsaBuf.buf;
				delete overlappedEx;
				clientInfo->SendCompleted(ioSize);
			}
			else
			{
				printf("Client Index(%d)���� ���ܻ�Ȳ\n", (int)clientInfo->GetIndex());
				//throw
			}
		}

	}

	void IOCPServer::AcceptThread()
	{
		SOCKADDR_IN clientAddr;
		int lenAddr = sizeof(clientAddr);
		ZeroMemory(&clientAddr, lenAddr);

		while (true == isRun)
		{
			bool result = false;

			SBNetLib::ClientInfo emptyClientInfo;

			result = FindEmptyClientInfo(emptyClientInfo);
			if (result == false)
			{
				printf("[����] Client Full\n");
				return;
			}

			auto newSocket = accept(listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &lenAddr);
			if (INVALID_SOCKET == newSocket)
			{
				continue;
			}

			if (false == emptyClientInfo.OnConnect(iocpHandle, newSocket))
			{
				emptyClientInfo.Close(true);
				return;
			}


			OnConnect(emptyClientInfo.GetIndex());

			currentConnectClient += 1;
		}

	}

	void IOCPServer::CloseSocket(SBNetLib::ClientInfo& inClientInfo, bool inIsForce)
	{
		auto clientIndex = inClientInfo.GetIndex();
		inClientInfo.Close(inIsForce);

		OnClose(clientIndex);
	}
}