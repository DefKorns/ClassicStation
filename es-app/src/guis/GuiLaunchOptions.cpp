/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "guis/GuiLaunchOptions.h"

#include "components/TextComponent.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "views/ViewController.h"
#include "ConsoleTools.h"
#include "FileData.h"
#include "I18n.h"
#include "MenuColors.h"
#include "SystemData.h"
#include <sstream>

GuiLaunchOptions::GuiLaunchOptions(Window* window, const LaunchChoices& choices, bool withDefault,
	std::function<void(const std::string&)> chosen, std::function<void()> cancelled)
	: GuiComponent(window), mMenu(window, "LAUNCH OPTIONS"), mChosen(std::move(chosen)), mCancelled(std::move(cancelled))
{
	addChild(&mMenu);

	std::vector<std::pair<std::string, std::string>> entries;
	if(withDefault)
		entries.emplace_back("native", I18n::tr("DEFAULT") + " (" + choices.defaultLabel + ")");
	entries.insert(entries.end(), choices.cores.begin(), choices.cores.end());

	const std::string active = choices.current.empty() ? "native" : choices.current;
	for(const auto& entry : entries)
	{
		const bool isCurrent = entry.first == active;
		ComponentListRow row;
		row.addElement(std::make_shared<TextComponent>(window, entry.second, Font::get(FONT_SIZE_MENU), MenuColors::Text), true);
		row.addElement(std::make_shared<TextComponent>(window, isCurrent ? I18n::tr("CURRENT") : "", Font::get(FONT_SIZE_SMALL), MenuColors::Text), false);
		const std::string id = entry.first;
		row.makeAcceptInputHandler([this, id] { choose(id); });
		mMenu.addRow(row, isCurrent);
	}

	mMenu.addButton("CANCEL", "cancel", [this] { cancel(); });
	mMenu.setSubtitle(I18n::tr("SUSPEND POINTS ARE NOT COMPATIBLE BETWEEN EMULATORS"));

	setSize(static_cast<float>(Renderer::getScreenWidth()), static_cast<float>(Renderer::getScreenHeight()));
	mMenu.setPosition((mSize.x() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

std::string GuiLaunchOptions::listCommand(const FileData* game)
{
	const FileData* source = const_cast<FileData*>(game)->getSourceFileData();
	const std::string code = source->metadata.get("code");
	if(code.empty())
		return std::string();
	return "launch list " + source->getSystem()->getName() + " " + code;
}

LaunchChoices GuiLaunchOptions::parse(const std::string& output)
{
	LaunchChoices choices;
	std::istringstream lines(output);
	for(std::string line; std::getline(lines, line);)
	{
		std::istringstream fields(line);
		std::string kind;
		std::string id;
		std::string label;
		std::getline(fields, kind, '\t');
		std::getline(fields, id, '\t');
		std::getline(fields, label, '\t');
		if(kind == "current")
			choices.current = id;
		else if(kind == "default")
		{
			choices.nativeDefault = id == "native";
			choices.defaultLabel = choices.nativeDefault ? I18n::tr("NATIVE") : label;
		}
		else if(kind == "core" && !id.empty())
			choices.cores.emplace_back(id, label.empty() ? id : label);
	}
	return choices;
}

void GuiLaunchOptions::save(const std::string& code, const LaunchChoices& choices, const std::string& chosen)
{
	if(chosen != (choices.current.empty() ? "native" : choices.current))
		ConsoleTools::run("launch set " + code + " " + chosen);
}

void GuiLaunchOptions::choose(const std::string& id)
{
	const std::string selected = id;
	const std::function<void(const std::string&)> chosen = std::move(mChosen);
	delete this;
	if(chosen)
		chosen(selected);
}

void GuiLaunchOptions::cancel()
{
	const std::function<void()> cancelled = std::move(mCancelled);
	delete this;
	if(cancelled)
		cancelled();
}

bool GuiLaunchOptions::input(InputConfig* config, Input input)
{
	if(config->isMappedTo("b", input) && input.value != 0)
	{
		cancel();
		return true;
	}
	return GuiComponent::input(config, input);
}

HelpStyle GuiLaunchOptions::getHelpStyle()
{
	HelpStyle style = HelpStyle();
	style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
	style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system", "menu_help");
	return style;
}

std::vector<HelpPrompt> GuiLaunchOptions::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("a", "launch"));
	prompts.push_back(HelpPrompt("b", "cancel"));
	return prompts;
}
