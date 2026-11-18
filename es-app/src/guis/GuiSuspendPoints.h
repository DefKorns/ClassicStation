/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_GUIS_GUI_SUSPEND_POINTS_H
#define ES_APP_GUIS_GUI_SUSPEND_POINTS_H

#include "SuspendPoints.h"
#include "GuiComponent.h"
#include "components/ImageComponent.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include "ThemeData.h"
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class SuspendBox
{
public:
	SuspendBox(Window* window, const std::string& defaultPath, int corner);

	void load(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element);
	void setSize(float width, float height);
	void render(const Transform4x4f& trans);

private:
	NinePatchComponent mPatch;
	ImageComponent mImage;
	bool mFullImage = false;
};

class GuiSuspendPoints : public GuiComponent
{
public:
	GuiSuspendPoints(Window* window, std::string title, std::string code, SuspendPoints points,
		std::function<void(int)> onResume, std::function<void()> onChanged);
	~GuiSuspendPoints() override;
	void loadTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view);

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;
	std::vector<HelpPrompt> getHelpPrompts() override;
	HelpStyle getHelpStyle() override;
	bool dimsBackground() const override { return mDimsBackground; }
private:
	void refresh();
	void saveSelected(SuspendPoints::SaveMode mode);
	void notify(const std::string& text);
	void close();
	void closeAnimated();
	bool animated() const;
	float slideOffset() const;
	void renderSlot(const Transform4x4f& trans, std::size_t index, bool selected, bool hidden);
	void renderLifted(const Transform4x4f& trans);
	void renderFloating(const Transform4x4f& trans);
	void renderWarning(const Transform4x4f& trans);
	void confirmDelete(int slot);
	void enterMove();
	bool anySaved() const;
	int nearestSaved(int from) const;
	void resumeSelected();
	void leaveMove();
	bool moveInput(InputConfig* config, const Input& input);
	struct Rect
	{
		Rect(float x_ = 0, float y_ = 0, float w_ = 0, float h_ = 0, bool set_ = false) : x(x_), y(y_), w(w_), h(h_), set(set_) {}
		float x;
		float y;
		float w;
		float h;
		bool set;
	};

	void layout();
	bool isSaveKey(InputConfig* config, const Input& input) const;
	const SuspendPoint& selectedPoint() const;

	std::shared_ptr<ThemeData> mTheme;
	std::string mView;
	std::string mTitle;
	std::string mCode;
	SuspendPoints mPoints;
	std::function<void(int)> mOnResume;
	std::function<void()> mOnChanged;

	SuspendPoint mFloating;
	std::array<SuspendPoint, SuspendPoints::MaxSlots> mSlots;
	int mSelected = 1;
	bool mHolding = false;
	bool mMoving = false;
	bool mCardFocus = false;
	int mMoveFrom = 1;
	int mTarget = 1;
	int mHeldMs = 0;
	int mNoticeMs = 0;
	int mSlideMs = 0;
	float mShiftProgress = 0.0f;
	bool mStarted = false;
	bool mClosing = false;

	Rect mPanelTheme;
	Rect mCellTheme;
	Rect mCellEmptyTheme;
	Rect mCellEmptyRect;
	bool mHasEmptyCell = false;
	bool mDimsBackground = true;
	float mCardBrightness = 1.0f;
	Rect mShotTheme;
	Rect mLabelTheme;
	Rect mFrameTheme;
	Rect mFrameSelectedTheme;
	Rect mLockTheme;
	Rect mGaugeTheme;
	Rect mNextTheme;
	Rect mTrashTheme;
	Rect mTrashRect;
	Rect mFrameRect;
	Rect mFrameSelectedRect;
	Rect mLockRect;
	Rect mGaugeRect;
	unsigned int mGaugeColor = 0;
	Rect mPanelRect;
	Rect mCellRect;
	float mCellStep = 0;

	SuspendBox mPanel;
	SuspendBox mCell;
	SuspendBox mCellSelected;
	SuspendBox mCellEmpty;
	SuspendBox mCellEmptySelected;
	SuspendBox mFrame;
	SuspendBox mFrameSelected;
	TextComponent mHeading;
	TextComponent mNotice;
	TextComponent mEmptyLabel;
	Vector2f mEmptyLabelBase;
	ImageComponent mLock;
	ImageComponent mTrash;
	ImageComponent mTrashOpen;
	std::vector<std::unique_ptr<ImageComponent>> mFloat;
	ImageComponent mFloatThumbnail;
	TextComponent mFloatLabel;
	TextComponent mFloatTime;
	int mFloatMs = 0;
	ImageComponent mWarning;
	ImageComponent mWarningIcon;
	TextComponent mWarningText;
	bool mHasSaveData = false;
	std::vector<std::unique_ptr<ImageComponent>> mShots;
	std::vector<std::unique_ptr<TextComponent>> mLabels;
};

#endif // ES_APP_GUIS_GUI_SUSPEND_POINTS_H
