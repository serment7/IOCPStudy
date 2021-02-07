#pragma once

#include <vector>
#include <string>

namespace SBNetLib::Util
{
	void TrimLeft(std::string& inTarget);
	void TrimRight(std::string& inTarget);
	void Trim(std::string& target);
	void RemoveWhiteSpace(std::string& inTarget);
	bool IsNumber(const std::string& inTarget);
	std::vector<std::string> Split(const std::string& inTarget, std::vector<char> inDelimiters);
}