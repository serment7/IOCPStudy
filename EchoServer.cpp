#include "EchoServer.h"

namespace SBNetLib
{
	void EchoServer::OnConnect(const UINT32 clientIndex_)
	{
		printf("[OnConnect] 클라이언트: Index(%d)\n", clientIndex_);
	}
	void EchoServer::OnClose(const UINT32 clientIndex_)
	{
		printf("[OnClose] 클라이언트: Index(%d)\n", clientIndex_);
	}
	void EchoServer::OnRecv(const UINT32 inClientIndex, const UINT32 inSize, char* inData)
	{
		printf("[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", inClientIndex, inSize);

		PacketData packet;
		packet.Set(inClientIndex, inSize, inData);

		std::lock_guard<std::mutex> guard(lock);
		packetDataDeque.push_back(packet);
	}
	void EchoServer::Run(const UINT32 maxClient)
	{
		isRunProcessThread = true;
		processThread = std::thread([this]() {});

		StartServer(maxClient);
	}
	void EchoServer::End()
	{
		isRunProcessThread = false;

		if (processThread.joinable())
		{
			processThread.join();
		}

		DestroyThread();
	}
	void EchoServer::ProcessPacket()
	{
		while (true == isRunProcessThread)
		{
			auto packetData = DequePacketData();
			if (packetData.dataSize != 0)
			{
				SendMsg(packetData.sessionIndex, packetData.dataSize, packetData.packetData);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}

	PacketData& EchoServer::DequePacketData()
	{
		PacketData packetData;

		std::lock_guard<std::mutex> guard(lock);
		if (packetDataDeque.empty())
		{
			return packetData;
		}

		packetData.Set(packetDataDeque.front());

		packetDataDeque.front().Release();
		packetDataDeque.pop_front();

		return packetData;
	}
}