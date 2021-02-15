#pragma once

//#include "NetDefine.h"
#include "ClientInfo.h"
#include <thread>
#include <vector>

namespace SBNetLib
{
	class IOCPServer
	{
	public:
		IOCPServer() = default;

		virtual ~IOCPServer()
		{
			WSACleanup();
		}

		bool InitSocket();

		bool BindAndListen(int inBindPort);

		bool StartServer(const UINT32 inMaxClientCount);

		void DestroyThread();

		bool SendMsg(const UINT32 inSessionIndex, const UINT32 dataSize_, char* pData);

		virtual void OnConnect(const UINT32 inClientIndex) {}
		virtual void OnClose(const UINT32 inClientIndex) {}
		virtual void OnRecv(const UINT32 inClientIndex, const UINT32 inSize, char* inData) {}

	/*public:*/
		void CreateClient(const UINT32 inMaxClientCount);

		bool CreateWorkerThread();

		bool FindEmptyClientInfo(SBNetLib::ClientInfo& outEmptyClient);

		SBNetLib::ClientInfo& GetClientInfo(const UINT32 inSessionIndex);

		bool CreateAcceptThread();

		void WorkerThread();

		void AcceptThread();

		void CloseSocket(SBNetLib::ClientInfo& inClientInfo, bool inIsForce = false);

		// ����Ǿ� �ִ� Ŭ���̾�Ʈ ����Ʈ
		std::vector<SBNetLib::ClientInfo> clientInfos;

		// ��� ����
		SOCKET listenSocket = INVALID_SOCKET;

		// ���� ����
		int currentConnectClient;

		std::vector<std::thread> workerThreads;
		std::thread acceptThread;
		HANDLE iocpHandle = INVALID_HANDLE_VALUE;
		bool isRun = false;

		UINT maxWorkerThreadNum = 4;
	};

}