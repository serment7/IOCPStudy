#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace SBNetLib
{
	struct PacketData
	{
		void Set(PacketData& value)
		{
			sessionIndex = value.sessionIndex;
			dataSize = value.dataSize;

			packetData = new char[value.dataSize];
			CopyMemory(packetData, value.packetData, value.dataSize);
		}

		void Set(UINT32 inSessionIndex, UINT32 inDataSize, char* inData)
		{
			sessionIndex = inSessionIndex;
			dataSize = inDataSize;
			packetData = new char[inDataSize];
			CopyMemory(packetData, inData, inDataSize);
		}

		void Release()
		{
			sessionIndex = 0;
			dataSize = 0;
			delete packetData;
		}

		UINT32 sessionIndex = 0;
		UINT32 dataSize = 0;
		char* packetData = nullptr;

	};
}
