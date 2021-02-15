#pragma once

#include "IOCPServer.h"
#include "Packet.h"

#include <vector>
#include <deque>
#include <thread>
#include <mutex>

namespace SBNetLib
{
	class EchoServer : public IOCPServer
	{
	public:
		EchoServer() = default;
		virtual ~EchoServer() = default;

		virtual void OnConnect(const UINT32 clientIndex_) override;

		virtual void OnClose(const UINT32 clientIndex_) override;

		virtual void OnRecv(const UINT32 inClientIndex, const UINT32 inSize, char* inData) override;

		void Run(const UINT32 maxClient);

		void End();

	private:
		void ProcessPacket();

		PacketData& DequePacketData();

		bool isRunProcessThread = false;
		std::thread processThread;
		std::mutex lock;
		std::deque<PacketData> packetDataDeque;
	};
}