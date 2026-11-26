/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_GUIS_GUI_LAUNCH_OPTIONS_H
#define ES_APP_GUIS_GUI_LAUNCH_OPTIONS_H

#include "components/MenuComponent.h"
#include "GuiComponent.h"
#include <functional>
#include <string>
#include <utility>
#include <vector>

class FileData;

struct LaunchChoices
{
	std::vector<std::pair<std::string, std::string>> cores;
	std::string current;
	std::string defaultLabel;
	bool nativeDefault = false;
};

class GuiLaunchOptions : public GuiComponent
{
public:
	GuiLaunchOptions(Window* window, const LaunchChoices& choices, bool withDefault, std::function<void(const std::string&)> chosen,
		std::function<void()> cancelled);

	static std::string listCommand(const FileData* game);
	static LaunchChoices parse(const std::string& output);
	static void save(const std::string& code, const LaunchChoices& choices, const std::string& chosen);

	bool input(InputConfig* config, Input input) override;
	std::vector<HelpPrompt> getHelpPrompts() override;
	HelpStyle getHelpStyle() override;

private:
	void choose(const std::string& id);
	void cancel();

	MenuComponent mMenu;
	std::function<void(const std::string&)> mChosen;
	std::function<void()> mCancelled;
};

#endif // ES_APP_GUIS_GUI_LAUNCH_OPTIONS_H
