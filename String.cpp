#include "String.h"
#include <algorithm>

namespace SBNetLib::Util
{
	void TrimLeft(std::string& inTarget)
	{
		inTarget.erase(
			inTarget.begin(),
			std::find_if(
				inTarget.begin(),
				inTarget.end(),
				[](char character) {
			return std::isspace(character) == false;
		}
			)
		);
	}

	void TrimRight(std::string& inTarget)
	{
		inTarget.erase(
			std::find_if(
				inTarget.rbegin(),
				inTarget.rend(),
				[](char character) {
			return std::isspace(character) == false;
		}
			).base(),
			inTarget.end()
			);
	}

	void Trim(std::string& target)
	{
		TrimLeft(target);
		TrimRight(target);
	}

	void RemoveWhiteSpace(std::string& inTarget)
	{
		inTarget.erase(std::remove_if(inTarget.begin(), inTarget.end(), std::isspace), inTarget.end());
	}

	bool IsNumber(const std::string& inTarget)
	{
		if (inTarget.empty())
		{
			return false;
		}

		return std::all_of(std::cbegin(inTarget), std::cend(inTarget), std::isdigit);
	}

	std::vector<std::string> Split(const std::string& inTarget, std::vector<char> inDelimiters)
	{
		std::vector<std::string> result;

		if (inTarget.empty() || inDelimiters.empty())
		{
			return result;
		}

		for (auto iterStart = inTarget.begin(); iterStart != inTarget.end(); )
		{
			auto endIter = std::find_if(iterStart, inTarget.end(), [&](const char& character) {
				for (const char& delimiter : inDelimiters)
				{
					if (character == delimiter)
					{
						return true;
					}
				}

				return false;
			});

			std::string newResult(iterStart, endIter);
			if (!newResult.empty())
			{
				result.push_back(std::move(newResult));
			}

			if (endIter != inTarget.end())
			{
				++endIter;
			}

			iterStart = endIter;
		}

		return result;
	}
}