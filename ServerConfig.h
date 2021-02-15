#pragma once

#include <string>
#include <filesystem>

namespace SBNetLib
{
	class ServerConfig
	{
	public:

		bool ReadConfigFile(const std::wstring& inFilePath);

		std::string host;
		unsigned short port = 0;
		unsigned short  workerNums = 0;
		unsigned short  clientMax = 0;
	};
}
