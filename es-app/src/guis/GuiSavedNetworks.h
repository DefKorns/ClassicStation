/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_GUIS_GUI_SAVED_NETWORKS_H
#define ES_APP_GUIS_GUI_SAVED_NETWORKS_H

#include "components/MenuComponent.h"
#include "GuiComponent.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

struct SavedNetwork
{
	std::string name;
	bool connected;
};

class GuiSavedNetworks : public GuiComponent
{
public:
	static void open(Window* window);
	static void openCurrent(Window* window);

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	std::vector<HelpPrompt> getHelpPrompts() override;
	HelpStyle getHelpStyle() override;

private:
	using Action = std::function<void(GuiSavedNetworks*)>;

	struct Row
	{
		std::string label;
		std::string detail;
		Action action;
		Action onHold;
	};

	GuiSavedNetworks(Window* window, const std::string& title, const std::vector<Row>& rows);

	static std::vector<SavedNetwork> parseSaved(const std::string& toolOutput);
	static void openActions(Window* window, const std::string& name, bool connected);
	static void openRestore(Window* window);
	static void runTool(Window* window, const std::string& arguments, const std::string& name, const std::function<void()>& then);
	static void closeAndRun(GuiSavedNetworks* page, std::function<void()> next);

	void addRow(const Row& row);

	MenuComponent mMenu;
	std::vector<Row> mRows;
	bool mHolding = false;
	int mHeldMs = 0;
};

#endif // ES_APP_GUIS_GUI_SAVED_NETWORKS_H
