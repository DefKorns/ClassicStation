/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once

#include "FileData.h"
#include "ThemeData.h"
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

class InfoLine
{
public:
	void load(const ThemeData::ThemeElement& element)
	{
		mFields.clear();
		if(element.has("fields"))
		{
			std::istringstream names(element.get<std::string>("fields"));
			for(std::string name; names >> name;)
				mFields.push_back(name);
		}
		if(element.has("separator"))
			mSeparator = element.get<std::string>("separator");
		if(element.has("playersOne"))
			mPlayersOne = element.get<std::string>("playersOne");
		if(element.has("playersMany"))
			mPlayersMany = element.get<std::string>("playersMany");
		mBreakAfter = element.has("breakAfter") ? static_cast<std::size_t>(element.get<float>("breakAfter")) : 0;
	}

	std::string build(const FileData& file) const
	{
		std::string out;
		std::size_t count = 0;
		for(const std::string& field : mFields)
		{
			std::string value = file.metadata.get(field);
			if(field == "releasedate")
				value = (value.size() >= 4 && value != "not-a-date-time") ? value.substr(0, 4) : "";
			else if(field == "players")
				value = (value.empty() || value == "0") ? "" : value + " " + (value == "1" ? mPlayersOne : mPlayersMany);
			else if(value == "unknown")
				value.clear();
			if(value.empty())
				continue;
			out += (out.empty() ? "" : (count == mBreakAfter ? "\n" : mSeparator)) + value;
			++count;
		}
		return out;
	}

private:
	std::vector<std::string> mFields;
	std::string mSeparator = " ";
	std::string mPlayersOne = "player";
	std::string mPlayersMany = "players";
	std::size_t mBreakAfter = 0;
};
