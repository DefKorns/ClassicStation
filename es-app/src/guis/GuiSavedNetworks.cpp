/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "MenuColors.h"
#include "guis/GuiSavedNetworks.h"
#include "I18n.h"

#include "guis/GuiMsgBox.h"
#include "guis/GuiToolRunner.h"
#include "views/ViewController.h"
#include "ConsoleTools.h"
#include "SystemData.h"
#include "Window.h"
#include "renderers/Renderer.h"
#include <sstream>

void GuiSavedNetworks::open(Window* window)
{
	window->pushGui(new GuiToolRunner(window, [] { return ConsoleTools::run("savednetworks"); }, [window](const std::string& output) {
		std::vector<Row> rows;
		for(const SavedNetwork& network : parseSaved(output))
		{
			const std::string name = network.name;
			const bool connected = network.connected;
			rows.push_back({ name, connected ? "CONNECTED" : "", [window, name, connected](GuiSavedNetworks* page) {
				closeAndRun(page, [window, name, connected] { openActions(window, name, connected); });
			} });
		}
		if(rows.empty())
			rows.push_back({ "NO SAVED NETWORKS", "", [](GuiSavedNetworks*) { return; } });
		rows.push_back({ "RESTORE NETWORKS", "SD CARD", [window](GuiSavedNetworks* page) {
			closeAndRun(page, [window] { openRestore(window); });
		} });
		window->pushGui(new GuiSavedNetworks(window, "SAVED NETWORKS", rows));
	}));
}

void GuiSavedNetworks::openCurrent(Window* window)
{
	window->pushGui(new GuiToolRunner(window, [] { return ConsoleTools::run("wifistatus"); }, [window](const std::string& output) {
		std::string name = output.substr(0, output.find('\n'));
		while(!name.empty() && (name.back() == '\r' || name.back() == ' '))
			name.pop_back();
		if(name.empty())
			runTool(window, "reconnect", "", [] { return; });
		else
			openActions(window, name, true);
	}));
}

void GuiSavedNetworks::openActions(Window* window, const std::string& name, bool connected)
{
	std::vector<Row> rows;
	if(connected)
		rows.push_back({ "DISCONNECT", "", [window, name](GuiSavedNetworks* page) {
			closeAndRun(page, [window, name] { runTool(window, "wifidisconnect", name, [] { return; }); });
		} });
	else
		rows.push_back({ "CONNECT", "", [window, name](GuiSavedNetworks* page) {
			closeAndRun(page, [window, name] { runTool(window, "wifiselect", name, [] { return; }); });
		} });
	rows.push_back({ "BACK UP", "SD CARD", [window, name](GuiSavedNetworks* page) {
		closeAndRun(page, [window, name] { runTool(window, "wifibackup", name, [] { return; }); });
	} });
	rows.push_back({ "FORGET", "", [window, name](GuiSavedNetworks* page) {
		auto* confirm = new GuiMsgBox(window, I18n::tr("FORGET") + " " + name + "?", "YES", [window, name, page] {
			closeAndRun(page, [window, name] { runTool(window, "wififorget", name, [window] { open(window); }); });
		}, "NO", nullptr);
		window->pushGui(confirm);
	} });
	auto* page = new GuiSavedNetworks(window, name, rows);
	if(connected)
		page->mMenu.setSubtitle(ConsoleTools::localAddress());
	window->pushGui(page);
}

void GuiSavedNetworks::openRestore(Window* window)
{
	window->pushGui(new GuiToolRunner(window, [] { return ConsoleTools::run("wifibackups"); }, [window](const std::string& output) {
		std::vector<Row> rows;
		std::istringstream lines(output);
		for(std::string line; std::getline(lines, line);)
		{
			while(!line.empty() && line.back() == '\r')
				line.pop_back();
			if(line.empty())
				continue;
			const std::string name = line;
			rows.push_back({ name, "", [window, name](GuiSavedNetworks* page) {
				closeAndRun(page, [window, name] { runTool(window, "wifirestore", name, [window] { open(window); }); });
			} });
		}
		if(rows.empty())
			rows.push_back({ "NO BACKUPS FOUND", "", [](GuiSavedNetworks*) { return; } });
		window->pushGui(new GuiSavedNetworks(window, "RESTORE NETWORKS", rows));
	}));
}

void GuiSavedNetworks::runTool(Window* window, const std::string& arguments, const std::string& name, const std::function<void()>& then)
{
	window->pushGui(new GuiToolRunner(window, [arguments, name] {
		return ConsoleTools::run(arguments, { { "ES_SSID", name } });
	}, [window, then](const std::string& text) {
		window->pushGui(new GuiMsgBox(window, text, "OK", then));
	}));
}

void GuiSavedNetworks::closeAndRun(GuiSavedNetworks* page, std::function<void()> next)
{
	delete page;
	next();
}

GuiSavedNetworks::GuiSavedNetworks(Window* window, const std::string& title, const std::vector<Row>& rows)
	: GuiComponent(window), mMenu(window, title.c_str())
{
	addChild(&mMenu);

	for(const Row& row : rows)
		addRow(row);

	mMenu.addButton("BACK", "go back", [this] { delete this; });
	setSize(static_cast<float>(Renderer::getScreenWidth()), static_cast<float>(Renderer::getScreenHeight()));
	mMenu.setPosition((mSize.x() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

std::vector<SavedNetwork> GuiSavedNetworks::parseSaved(const std::string& toolOutput)
{
	std::vector<SavedNetwork> networks;
	std::istringstream lines(toolOutput);
	std::string line;
	while(std::getline(lines, line))
	{
		const std::size_t tab = line.rfind('\t');
		if(tab == std::string::npos || tab == 0)
			continue;
		networks.push_back({ line.substr(0, tab), line.substr(tab + 1) == "CONNECTED" });
	}
	return networks;
}

void GuiSavedNetworks::addRow(const Row& row)
{
	ComponentListRow listRow;
	listRow.addElement(std::make_shared<TextComponent>(mWindow, I18n::tr(row.label), Font::get(FONT_SIZE_MENU), MenuColors::Text), true);
	if(!row.detail.empty())
		listRow.addElement(std::make_shared<TextComponent>(mWindow, I18n::tr(row.detail), Font::get(FONT_SIZE_SMALL), MenuColors::Dim), false);
	const Action action = row.action;
	listRow.makeAcceptInputHandler([this, action] { action(this); });
	mMenu.addRow(listRow);
}

bool GuiSavedNetworks::input(InputConfig* config, Input input)
{
	if(config->isMappedTo("b", input) && input.value != 0)
	{
		delete this;
		return true;
	}

	return GuiComponent::input(config, input);
}

HelpStyle GuiSavedNetworks::getHelpStyle()
{
	HelpStyle style = HelpStyle();
	style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
	style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system", "menu_help");
	return style;
}

std::vector<HelpPrompt> GuiSavedNetworks::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("b", "back"));
	return prompts;
}
