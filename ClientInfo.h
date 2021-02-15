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
			struct linger stLinger = { 0, 0 };	// SO_DONTLINGER로 설정

		// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
			if (true == bIsForce)
			{
				stLinger.l_onoff = 1;
			}

			//socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
			shutdown(socketClient, SD_BOTH);

			//소켓 옵션을 설정한다.
			setsockopt(socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

			//소켓 연결을 종료 시킨다. 
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
				printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
				return false;
			}

			return true;
		}

		bool BindRecv()
		{
			DWORD dwFlag = 0;
			DWORD dwRecvNumBytes = 0;

			//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
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

			//socket_error이면 client socket이 끊어진걸로 처리한다.
			if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
			{
				printf("[에러] WSARecv()함수 실패 : %d\n", WSAGetLastError());
				return false;
			}

			return true;
		}

		// 1개의 스레드에서만 호출해야 한다!
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

			//socket_error이면 client socket이 끊어진걸로 처리한다.
			if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
			{
				printf("[에러] WSASend()함수 실패 : %d\n", WSAGetLastError());
				return false;
			}

			return true;
		}

		void SendCompleted(const UINT32 dataSize_)
		{
			printf("[송신 완료] bytes : %d\n", dataSize_);
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