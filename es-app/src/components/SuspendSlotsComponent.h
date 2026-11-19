/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_COMPONENTS_SUSPEND_SLOTS_COMPONENT_H
#define ES_APP_COMPONENTS_SUSPEND_SLOTS_COMPONENT_H

#include "components/ImageComponent.h"
#include "GuiComponent.h"
#include "SuspendSlotState.h"
#include <memory>
#include <string>
#include <vector>

class SuspendSlotsComponent : public GuiComponent
{
public:
	explicit SuspendSlotsComponent(Window* window);

	void setStates(const SlotStates& states);

	void onSizeChanged() override;
	void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

private:
	void layout();
	void refreshImages();
	const std::string& pathFor(SlotState state) const;

	std::vector<std::unique_ptr<ImageComponent>> mIcons;
	SlotStates mStates{};
	std::string mEmptyPath;
	std::string mSavedPath;
	std::string mLockedPath;
};

#endif // ES_APP_COMPONENTS_SUSPEND_SLOTS_COMPONENT_H
