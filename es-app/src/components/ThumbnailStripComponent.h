/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_COMPONENTS_THUMBNAIL_STRIP_COMPONENT_H
#define ES_APP_COMPONENTS_THUMBNAIL_STRIP_COMPONENT_H

#include "components/ImageComponent.h"
#include "GuiComponent.h"
#include "ThemeData.h"

#include <memory>
#include <string>
#include <vector>

class ThumbnailStripComponent : public GuiComponent
{
public:
	explicit ThumbnailStripComponent(Window* window);

	void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view);

	void setEntries(std::vector<std::string> paths);
	void setPosition(float index);

	bool enabled() const { return mEnabled; }

	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;

private:
	static constexpr int MaxReach = 16;
	static constexpr int FadeItems = 3;
	static constexpr int Slots = 2 * MaxReach + 1;
	static constexpr int ArrowFrames = 3;

	std::vector<std::string> mPaths;
	std::vector<std::unique_ptr<ImageComponent>> mImages;
	std::vector<int> mAssigned;
	std::vector<float> mWidths;
	std::vector<std::unique_ptr<ImageComponent>> mArrow;

	std::vector<float> mCentres;
	float mTotalWidth = 0.0f;
	bool mTotalDirty = true;
	bool mEnabled = false;
	float mPosition = 0.0f;
	Vector2f mCentre;
	Vector2f mThumbSize;
	int mFullCount = 21;
	int mElapsedMs = 0;
};

#endif // ES_APP_COMPONENTS_THUMBNAIL_STRIP_COMPONENT_H
