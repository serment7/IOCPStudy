// SBNetLib.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "SBNetLib.h"
#include <iostream>
#include <string>

const UINT16 SERVER_PORT = 5000;
const UINT16 MAX_CLIENT = 100;

int main()
{
	SBNetLib::ServerConfig serverConfig;

	SBNetLib::EchoServer server;

	server.InitSocket();
	server.BindAndListen(SERVER_PORT);
	server.Run(MAX_CLIENT);

	printf("아무 키나 누를 때까지 대기합니다\n");

	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}


	server.End();

	return 0;
}
