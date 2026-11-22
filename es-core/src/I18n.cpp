/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "I18n.h"

#include "Log.h"
#include "resources/ResourceManager.h"
#include <sstream>
#include <unordered_map>

namespace
{
	std::unordered_map<std::string, std::string> sCatalog;
	std::string sLanguage = "en";

	std::string unescape(const std::string& text)
	{
		std::string result;
		for(size_t i = 0; i < text.size(); ++i)
		{
			if(text[i] == '\\' && i + 1 < text.size() && text[i + 1] == 'n')
			{
				result += '\n';
				++i;
			}
			else
				result += text[i];
		}
		return result;
	}
}

void I18n::load(const std::string& language)
{
	sCatalog.clear();
	sLanguage = language.empty() ? "en" : language;
	if(sLanguage == "en")
		return;

	const std::string path = ":/i18n/" + sLanguage + ".txt";
	if(!ResourceManager::getInstance()->fileExists(path))
	{
		LOG(LogWarning) << "No translation file for language " << sLanguage;
		sLanguage = "en";
		return;
	}

	const ResourceData data = ResourceManager::getInstance()->getFileData(path);
	std::istringstream lines(std::string(reinterpret_cast<const char*>(data.ptr.get()), data.length));
	for(std::string line; std::getline(lines, line);)
	{
		if(!line.empty() && line.back() == '\r')
			line.pop_back();
		const size_t tab = line.find('\t');
		if(line.empty() || line[0] == '#' || tab == std::string::npos)
			continue;
		sCatalog[unescape(line.substr(0, tab))] = unescape(line.substr(tab + 1));
	}
}

const std::string& I18n::language()
{
	return sLanguage;
}

std::string I18n::tr(const std::string& text)
{
	if(sCatalog.empty())
		return text;
	const auto found = sCatalog.find(text);
	if(found != sCatalog.end())
		return found->second;
	return text;
}
