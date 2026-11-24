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
#include <vector>

namespace
{
	struct Template
	{
		std::string prefix;
		std::string suffix;
		std::string translation;
	};

	std::unordered_map<std::string, std::string> sCatalog;
	std::vector<Template> sTemplates;
	std::string sLanguage = "en";
	const std::string Placeholder = "%s";

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
	sTemplates.clear();
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
	loadFromText(std::string(reinterpret_cast<const char*>(data.ptr.get()), data.length));
}

void I18n::loadFromText(const std::string& text)
{
	sCatalog.clear();
	sTemplates.clear();
	std::istringstream lines(text);
	for(std::string line; std::getline(lines, line);)
	{
		if(!line.empty() && line.back() == '\r')
			line.pop_back();
		const size_t tab = line.find('\t');
		if(line.empty() || line[0] == '#' || tab == std::string::npos)
			continue;

		const std::string key = unescape(line.substr(0, tab));
		const std::string translation = unescape(line.substr(tab + 1));
		const size_t slot = key.find(Placeholder);
		if(slot != std::string::npos && key.find(Placeholder, slot + Placeholder.size()) == std::string::npos)
			sTemplates.push_back({ key.substr(0, slot), key.substr(slot + Placeholder.size()), translation });
		else
			sCatalog[key] = translation;
	}
}

const std::string& I18n::language()
{
	return sLanguage;
}

std::string I18n::tr(const std::string& text)
{
	if(sCatalog.empty() && sTemplates.empty())
		return text;
	const auto found = sCatalog.find(text);
	if(found != sCatalog.end())
		return found->second;

	const Template* best = nullptr;
	for(const Template& entry : sTemplates)
	{
		const size_t fixed = entry.prefix.size() + entry.suffix.size();
		if(text.size() < fixed || text.compare(0, entry.prefix.size(), entry.prefix) != 0 ||
			text.compare(text.size() - entry.suffix.size(), entry.suffix.size(), entry.suffix) != 0)
			continue;
		if(best == nullptr || fixed > best->prefix.size() + best->suffix.size())
			best = &entry;
	}
	if(best == nullptr)
		return text;

	const std::string argument = text.substr(best->prefix.size(), text.size() - best->prefix.size() - best->suffix.size());
	std::string result = best->translation;
	const size_t slot = result.find(Placeholder);
	if(slot != std::string::npos)
		result.replace(slot, Placeholder.size(), argument);
	return result;
}
