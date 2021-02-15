#pragma once

#include "NetDefine.h"
#include <stdio.h>

namespace SBNetLib
{
	class ClientInfo
	{
	public:
		ClientInfo()
		{
			ZeroMemory(&recvOverlappedEx, sizeof(recvOverlappedEx));
			//ZeroMemory(&sendOverlappedEx, sizeof(sendOverlappedEx));
			socketClient = INVALID_SOCKET;
		}

		void Init(const UINT32 inClientIndex)
		{
			clientIndex = inClientIndex;
		}

		UINT32 GetIndex()
		{
			return clientIndex;
		}

		SOCKET GetSocket()
		{
			return socketClient;
		}

		char* GetRecvBuffer()
		{
			return recvBuf;
		}

		bool IsValid()
		{
			return socketClient != INVALID_SOCKET;
		}

		bool OnConnect(HANDLE inIOCPHandle, SOCKET inSocketClient)
		{
			socketClient = inSocketClient;
			Clear();

			if (false == BindIOCompletionPort(inIOCPHandle))
			{
				return false;
			}

			return BindRecv();
		}

		void Close(bool bIsForce = false)
		{
			struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����

		// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
			if (true == bIsForce)
			{
				stLinger.l_onoff = 1;
			}

			//socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
			shutdown(socketClient, SD_BOTH);

			//���� �ɼ��� �����Ѵ�.
			setsockopt(socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

			//���� ������ ���� ��Ų��. 
			closesocket(socketClient);
			socketClient = INVALID_SOCKET;
		}

		void Clear()
		{

		}

		bool BindIOCompletionPort(HANDLE inIOCPHandle)
		{
			auto hIOCP = CreateIoCompletionPort((HANDLE)GetSocket()
				, inIOCPHandle
				, (ULONG_PTR)(this)
				, 0);

			if (inIOCPHandle == INVALID_HANDLE_VALUE)
			{
				printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
				return false;
			}

			return true;
		}

		bool BindRecv()
		{
			DWORD dwFlag = 0;
			DWORD dwRecvNumBytes = 0;

			//Overlapped I/O�� ���� �� ������ ������ �ش�.
			recvOverlappedEx.wsaBuf.len = MAX_SOCKBUF;
			recvOverlappedEx.wsaBuf.buf = recvBuf;
			recvOverlappedEx.operationType = SBNetLib::IOOperationType::RECV;

			int nRet = WSARecv(GetSocket(),
				&(recvOverlappedEx.wsaBuf),
				1,
				&dwRecvNumBytes,
				&dwFlag,
				(LPWSAOVERLAPPED) & (recvOverlappedEx),
				NULL);

			//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
			if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
			{
				printf("[����] WSARecv()�Լ� ���� : %d\n", WSAGetLastError());
				return false;
			}

			return true;
		}

		// 1���� �����忡���� ȣ���ؾ� �Ѵ�!
		bool SendMsg(const UINT32 dataSize_, char* pMsg_)
		{
			auto sendOverlappedEx = new OverlappedEx;
			ZeroMemory(sendOverlappedEx, sizeof(sendOverlappedEx));
			sendOverlappedEx->wsaBuf.len = dataSize_;
			sendOverlappedEx->wsaBuf.buf = new char[dataSize_];
			CopyMemory(sendOverlappedEx->wsaBuf.buf, pMsg_, dataSize_);
			sendOverlappedEx->operationType = SBNetLib::IOOperationType::SEND;

			DWORD dwRecvNumBytes = 0;
			int nRet = WSASend(GetSocket(),
				&(sendOverlappedEx->wsaBuf),
				1,
				&dwRecvNumBytes,
				0,
				(LPWSAOVERLAPPED)sendOverlappedEx,
				NULL);

			//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
			if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
			{
				printf("[����] WSASend()�Լ� ���� : %d\n", WSAGetLastError());
				return false;
			}

			return true;
		}

		void SendCompleted(const UINT32 dataSize_)
		{
			printf("[�۽� �Ϸ�] bytes : %d\n", dataSize_);
		}

	private:
		UINT32 clientIndex = 0;
		SOCKET socketClient = NULL;
		OverlappedEx recvOverlappedEx;
		//OverlappedEx sendOverlappedEx;
		char				recvBuf[MAX_SOCKBUF] = { 0, };
		//char				sendBuf[MAX_SOCKBUF] = { 0, };
	};
}