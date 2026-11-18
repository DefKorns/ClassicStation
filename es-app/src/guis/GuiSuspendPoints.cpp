/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "guis/GuiSuspendPoints.h"

#include "MenuColors.h"
#include "HelpStyle.h"

#include "guis/GuiMsgBox.h"
#include "UiSound.h"
#include "Settings.h"
#include "InputConfig.h"
#include "Window.h"
#include "views/ViewController.h"
#include "components/GridTileComponent.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <utility>

namespace
{
	constexpr int HoldToOverwriteMs = 400;
	constexpr int FloatBobMs = 1800;
	constexpr float FloatBobPixels = 6.0f;
	constexpr int FloatWingMs = 180;
	constexpr float FloatRise = 142.0f;
	enum FloatPart { FloatCard, FloatActive, FloatFrame, FloatWingA, FloatWingB, FloatWingLeftA, FloatWingLeftB, FloatBalloon, FloatBalloonIcon, FloatArrowLeft, FloatArrowRight, FloatRing, FloatPartCount };
	const char* const FloatNames[FloatPartCount] = {
		"suspend_float_card", "suspend_float_active", "suspend_float_frame", "suspend_float_wing_a", "suspend_float_wing_b",
		"suspend_float_wing_left_a", "suspend_float_wing_left_b", "suspend_float_balloon", "suspend_float_balloon_icon",
		"suspend_float_arrow_left", "suspend_float_arrow_right", "suspend_float_ring"
	};
	constexpr int SlideMs = 200;
	constexpr int WallInMs = 250;
	constexpr int MaxStepMs = 33;
	constexpr int NoticeMs = 1500;
	constexpr unsigned int GaugeColor = 0xF0F0F0FF;
	constexpr unsigned int TextColor = 0xF0F0F0FF;
	constexpr unsigned int MutedColor = 0x9A9A9AFF;

	constexpr float PanelLeft = 0.04f;
	constexpr float PanelTop = 0.46f;
	constexpr float PanelWidth = 0.92f;
	constexpr float PanelHeight = 0.40f;
	constexpr float CellsLeft = 0.07f;
	constexpr float CellsTop = 0.56f;
	constexpr float CellWidth = 0.19f;
	constexpr float CellGap = 0.0233f;
	constexpr float CellHeight = 0.27f;
	constexpr float ShotHeight = 0.17f;
	constexpr float Border = 4.0f;
	constexpr int CardCorner = 30;
	constexpr float Bleed = 12.0f;

	std::string clock(long seconds)
	{
		char text[32];
		std::snprintf(text, sizeof text, "%ld:%02ld", seconds / 60, seconds % 60);
		return text;
	}
}

SuspendBox::SuspendBox(Window* window, const std::string& defaultPath, int corner)
	: mPatch(window, defaultPath), mImage(window)
{
	mPatch.setCornerSize(corner, corner);
}

void SuspendBox::load(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element)
{
	const ThemeData::ThemeElement* image = theme->getElement(view, element, "image");
	mFullImage = image != nullptr && image->has("path");
	if(mFullImage)
		mImage.setImage(image->get<std::string>("path"));
	else
		mPatch.applyTheme(theme, view, element, ThemeFlags::PATH);
}

void SuspendBox::setSize(float width, float height)
{
	mPatch.setSize(width, height);
	mImage.setResize(width, height);
}

void SuspendBox::render(const Transform4x4f& trans)
{
	if(mFullImage)
		mImage.render(trans);
	else
		mPatch.render(trans);
}

GuiSuspendPoints::GuiSuspendPoints(Window* window, std::string title, std::string code, SuspendPoints points,
	std::function<void(int)> onResume, std::function<void()> onChanged)
	: GuiComponent(window), mTitle(std::move(title)), mCode(std::move(code)), mPoints(std::move(points)),
	  mOnResume(std::move(onResume)), mOnChanged(std::move(onChanged)),
	  mPanel(window, ":/suspend_card.png", CardCorner), mCell(window, ":/suspend_card.png", CardCorner),
	  mCellSelected(window, ":/suspend_card_sel.png", CardCorner),
	  mCellEmpty(window, ":/suspend_card.png", CardCorner), mCellEmptySelected(window, ":/suspend_card_sel.png", CardCorner),
	  mFrame(window, ":/suspend_frame.png", CardCorner), mFrameSelected(window, ":/suspend_frame_sel.png", CardCorner),
	  mHeading(window, "", Font::get(FONT_SIZE_MEDIUM), TextColor),
	  mNotice(window, "", Font::get(FONT_SIZE_SMALL), MutedColor, ALIGN_RIGHT),
	  mEmptyLabel(window, "No Data", Font::get(FONT_SIZE_SMALL), TextColor, ALIGN_CENTER),
	  mLock(window), mTrash(window), mTrashOpen(window), mFloatThumbnail(window),
	  mWarning(window), mWarningIcon(window),
	  mWarningText(window, "If you continue from a saved suspend point, your in-game save data will be overwritten.", Font::get(FONT_SIZE_SMALL), 0x000000FF, ALIGN_LEFT),
	  mFloatLabel(window, "", Font::get(FONT_SIZE_SMALL), 0x000000FF, ALIGN_LEFT),
	  mFloatTime(window, "", Font::get(FONT_SIZE_SMALL), 0xFFFFFFFF, ALIGN_RIGHT)
{
	for(int part = 0; part < FloatPartCount; ++part)
		mFloat.emplace_back(new ImageComponent(window));
	mLock.setImage(":/suspend_lock.png");
	for(int slot = 0; slot < SuspendPoints::MaxSlots; ++slot)
	{
		mShots.emplace_back(new ImageComponent(window));
		mLabels.emplace_back(new TextComponent(window, "", Font::get(FONT_SIZE_SMALL), TextColor, ALIGN_CENTER));
	}

	layout();
	refresh();
	mCardFocus = mFloating.valid;
	if(!mCardFocus && nearestSaved(1) != 0)
		mSelected = nearestSaved(1);
	updateHelpPrompts();
	ImageComponent::setBackdropVisible(1.0f);
}

GuiSuspendPoints::~GuiSuspendPoints()
{
	ImageComponent::setScrollCoast(1.0f);
	ImageComponent::setBackdropVisible(1.0f);
	GridTileComponent::setUnselectedBrightness(1.0f);
}

void GuiSuspendPoints::loadTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view)
{
	mTheme = theme;
	mView = view;
	constexpr unsigned int textProperties = ThemeFlags::COLOR | ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE | ThemeFlags::ALIGNMENT;
	const float width = static_cast<float>(Renderer::getScreenWidth());
	const float height = static_cast<float>(Renderer::getScreenHeight());

	const auto rectOf = [&](const char* name, const char* type) {
		const ThemeData::ThemeElement* elem = theme->getElement(view, name, type);
		if(elem == nullptr || !elem->has("pos") || !elem->has("size"))
			return Rect();
		const Vector2f pos = elem->get<Vector2f>("pos");
		const Vector2f size = elem->get<Vector2f>("size");
		return Rect(pos.x() * width, pos.y() * height, size.x() * width, size.y() * height, true);
	};
	const auto boxRect = [&](const char* name) -> Rect {
		const ThemeData::ThemeElement* elem = theme->getElement(view, name, "");
		return elem != nullptr && elem->type == "image" ? rectOf(name, "image") : rectOf(name, "ninepatch");
	};

	mPanel.load(theme, view, "suspend_panel");
	mCell.load(theme, view, "suspend_cell");
	mCellSelected.load(theme, view, "suspend_cell_selected");
	if(const ThemeData::ThemeElement* backdrop = theme->getElement(view, "suspend_backdrop", "container"))
		if(backdrop->has("visible"))
			mDimsBackground = backdrop->get<bool>("visible");
	mHasEmptyCell = theme->getElement(view, "suspend_cell_empty", "image") != nullptr;
	mCellEmpty.load(theme, view, "suspend_cell_empty");
	mCellEmptySelected.load(theme, view, mHasEmptyCell && theme->getElement(view, "suspend_cell_empty_selected", "image") ? "suspend_cell_empty_selected" : "suspend_cell_empty");
	mFrame.load(theme, view, "suspend_frame");
	mFrameSelected.load(theme, view, "suspend_frame_selected");
	mPanelTheme = boxRect("suspend_panel");
	mCellTheme = boxRect("suspend_cell");
	mCellEmptyTheme = boxRect("suspend_cell_empty");
	mShotTheme = rectOf("suspend_shot", "image");
	mLabelTheme = rectOf("suspend_label", "text");
	mFrameTheme = boxRect("suspend_frame");
	mFrameSelectedTheme = boxRect("suspend_frame_selected");
	mLockTheme = rectOf("suspend_lock", "image");
	if(const ThemeData::ThemeElement* lock = theme->getElement(view, "suspend_lock", "image"))
		if(lock->has("path"))
			mLock.setImage(lock->get<std::string>("path"));
	mTrashTheme = rectOf("suspend_trash", "image");
	if(const ThemeData::ThemeElement* trash = theme->getElement(view, "suspend_trash", "image"))
		if(trash->has("path"))
			mTrash.setImage(trash->get<std::string>("path"));
	if(const ThemeData::ThemeElement* trashOpen = theme->getElement(view, "suspend_trash_open", "image"))
		if(trashOpen->has("path"))
			mTrashOpen.setImage(trashOpen->get<std::string>("path"));
	for(int part = 0; part < FloatPartCount; ++part)
		if(const ThemeData::ThemeElement* elem = theme->getElement(view, FloatNames[part], "image"))
			if(elem->has("path"))
				mFloat[part]->setImage(elem->get<std::string>("path"));
	if(const ThemeData::ThemeElement* elem = theme->getElement(view, "suspend_warning", "image"))
		if(elem->has("path"))
			mWarning.setImage(elem->get<std::string>("path"));
	if(const ThemeData::ThemeElement* elem = theme->getElement(view, "suspend_warning_icon", "image"))
		if(elem->has("path"))
			mWarningIcon.setImage(elem->get<std::string>("path"));
	mWarningText.applyTheme(theme, view, "suspend_warning_text", textProperties);
	mFloatLabel.applyTheme(theme, view, "suspend_float_label", textProperties);
	mFloatTime.applyTheme(theme, view, "suspend_float_time", textProperties);
	mGaugeTheme = rectOf("suspend_gauge", "image");
	mNextTheme = rectOf("suspend_cell_next", "image");
	mGaugeColor = GaugeColor;
	if(const ThemeData::ThemeElement* gauge = theme->getElement(view, "suspend_gauge", "image"))
		if(gauge->has("color"))
			mGaugeColor = gauge->get<unsigned int>("color");

	layout();

	mHeading.applyTheme(theme, view, "suspend_title", textProperties | ThemeFlags::POSITION | ThemeFlags::SIZE);
	mNotice.applyTheme(theme, view, "suspend_notice", textProperties | ThemeFlags::POSITION | ThemeFlags::SIZE);
	mEmptyLabel.applyTheme(theme, view, "suspend_label_empty", textProperties | ThemeFlags::POSITION | ThemeFlags::SIZE);
	mEmptyLabelBase = Vector2f(mEmptyLabel.getPosition().x(), mEmptyLabel.getPosition().y());
	for(auto& label : mLabels)
		label->applyTheme(theme, view, "suspend_label", textProperties);
}

void GuiSuspendPoints::layout()
{
	const float width = static_cast<float>(Renderer::getScreenWidth());
	const float height = static_cast<float>(Renderer::getScreenHeight());
	const float count = static_cast<float>(mShots.size());

	setSize(width, height);
	mHeading.setPosition(width * (PanelLeft + 0.025f), height * (PanelTop + 0.015f));
	mHeading.setSize(width * 0.6f, height * 0.06f);
	mNotice.setPosition(width * (PanelLeft + PanelWidth - 0.325f), height * (PanelTop + 0.015f));
	mNotice.setSize(width * 0.3f, height * 0.06f);

	mPanelRect = mPanelTheme.set ? mPanelTheme : Rect(width * PanelLeft - Bleed, height * PanelTop - Bleed,
		width * PanelWidth + 2 * Bleed, height * PanelHeight + 2 * Bleed);
	mCellRect = mCellTheme.set ? mCellTheme : Rect(width * CellsLeft - Bleed, height * CellsTop - Bleed,
		width * CellWidth + 2 * Bleed, height * CellHeight + 2 * Bleed);
	if(mNextTheme.set)
		mCellStep = mNextTheme.x - mCellRect.x;
	else if(mCellTheme.set)
	{
		const float margin = mCellRect.x - mPanelRect.x;
		const float lastLeft = mPanelRect.x + mPanelRect.w - margin - mCellRect.w;
		mCellStep = count > 1 ? (lastLeft - mCellRect.x) / (count - 1) : 0.0f;
	}
	else
		mCellStep = width * (CellWidth + CellGap);

	const Rect shot = mShotTheme.set ? mShotTheme : Rect(width * CellsLeft + Border, height * CellsTop + Border,
		width * CellWidth - 2 * Border, height * ShotHeight);
	const Rect label = mLabelTheme.set ? mLabelTheme : Rect(width * CellsLeft, height * (CellsTop + ShotHeight + 0.01f),
		width * CellWidth, height * 0.07f);

	mCellEmptyRect = mCellEmptyTheme.set ? mCellEmptyTheme : mCellRect;
	mFrameRect = mFrameTheme.set ? mFrameTheme : mCellRect;
	mFrameSelectedRect = mFrameSelectedTheme.set ? mFrameSelectedTheme : mFrameRect;
	mLockRect = mLockTheme.set ? mLockTheme : Rect(mCellRect.x + mCellRect.w - Bleed - height * 0.06f,
		mCellRect.y + Bleed + height * 0.01f, height * 0.05f, height * 0.05f);
	mGaugeRect = mGaugeTheme.set ? mGaugeTheme : Rect(mCellRect.x + Bleed,
		mCellRect.y + mCellRect.h - Bleed - Border * 2, mCellRect.w - 2 * Bleed, Border * 2);
	mLock.setResize(mLockRect.w, mLockRect.h);
	mTrashRect = mTrashTheme.set ? mTrashTheme : Rect(mPanelRect.x + mPanelRect.w - height * 0.2f, mCellRect.y, height * 0.1f, height * 0.2f);
	mTrash.setResize(mTrashRect.w, mTrashRect.h);
	mTrashOpen.setResize(mTrashRect.w, mTrashRect.h);

	mPanel.setSize(mPanelRect.w, mPanelRect.h);
	mCell.setSize(mCellRect.w, mCellRect.h);
	mCellSelected.setSize(mCellRect.w, mCellRect.h);
	mCellEmpty.setSize(mCellEmptyRect.w, mCellEmptyRect.h);
	mCellEmptySelected.setSize(mCellEmptyRect.w, mCellEmptyRect.h);
	mFrame.setSize(mFrameRect.w, mFrameRect.h);
	mFrameSelected.setSize(mFrameSelectedRect.w, mFrameSelectedRect.h);

	for(std::size_t index = 0; index < mShots.size(); ++index)
	{
		const float shift = static_cast<float>(index) * mCellStep;
		mShots[index]->setPosition(shot.x + shift, shot.y);
		mShots[index]->setResize(shot.w, shot.h);
		mLabels[index]->setPosition(label.x + shift, label.y);
		mLabels[index]->setSize(label.w, label.h);
	}
}

const SuspendPoint& GuiSuspendPoints::selectedPoint() const
{
	return mSlots[static_cast<std::size_t>(mSelected - 1)];
}

bool GuiSuspendPoints::anySaved() const
{
	for(const SuspendPoint& point : mSlots)
		if(point.valid)
			return true;
	return false;
}

int GuiSuspendPoints::nearestSaved(int from) const
{
	int best = 0;
	for(int slot = 1; slot <= SuspendPoints::MaxSlots; ++slot)
		if(mSlots[static_cast<std::size_t>(slot - 1)].valid && (best == 0 || std::abs(slot - from) < std::abs(best - from)))
			best = slot;
	return best;
}

void GuiSuspendPoints::resumeSelected()
{
	const std::function<void(int)> resume = mOnResume;
	const int slot = mCardFocus ? -1 : mSelected;
	UiSound::play("se_sys_click_game.wav");
	close();
	if(resume)
		resume(slot);
}

void GuiSuspendPoints::refresh()
{
	mFloating = mPoints.floating();
	if(mFloating.valid && mFloating.code != mCode)
		mFloating = SuspendPoint();

	mSlots = mPoints.slots(mCode);
	mHeading.setText("Suspend Point List");
	mFloatLabel.setText("Save");
	mHasSaveData = mPoints.hasSaveData(mCode);
	if(mFloating.valid)
	{
		mFloat[FloatFrame]->setColorShift(0xFFFFFF20);
		mFloatTime.setText(clock(mFloating.playtime));
		mFloatThumbnail.setImage(mFloating.thumbnail);
	}

	for(std::size_t index = 0; index < mSlots.size(); ++index)
	{
		const SuspendPoint& point = mSlots[index];
		if(point.valid)
			mShots[index]->setImage(point.thumbnail);
		mLabels[index]->setText(point.valid ? clock(point.playtime) : std::string());
	}
	if(!mFloating.valid)
		mCardFocus = false;
	if(!mCardFocus && !mMoving && !selectedPoint().valid && nearestSaved(mSelected) != 0)
		mSelected = nearestSaved(mSelected);
	updateHelpPrompts();
}

void GuiSuspendPoints::notify(const std::string& text)
{
	mNotice.setText(text);
	mNoticeMs = NoticeMs;
}

void GuiSuspendPoints::close()
{
	delete this;
}

bool GuiSuspendPoints::animated() const
{
	return Settings::getInstance()->getBool("SuspendAnimations");
}

void GuiSuspendPoints::closeAnimated()
{
	if(!animated())
	{
		close();
		return;
	}
	mClosing = true;
	mSlideMs = 0;
}

float GuiSuspendPoints::slideOffset() const
{
	if(!animated())
		return 0.0f;
	const float progress = std::min(1.0f, static_cast<float>(mSlideMs) / static_cast<float>(SlideMs));
	const float height = static_cast<float>(Renderer::getScreenHeight());
	if(mClosing)
		return height * (progress <= 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (progress - 1.0f)));
	return height * (progress >= 1.0f ? 0.0f : std::pow(2.0f, -10.0f * progress));
}

bool GuiSuspendPoints::isSaveKey(InputConfig* config, const Input& input) const
{
	return config->isMappedTo("a", input);
}

void GuiSuspendPoints::saveSelected(SuspendPoints::SaveMode mode)
{
	switch(mPoints.save(mCode, mSelected, mode))
	{
	case SuspendPoints::SaveResult::Saved:
		UiSound::play("se_sys_push.wav");
		refresh();
		notify("SAVED");
		if(mOnChanged)
			mOnChanged();
		break;
	case SuspendPoints::SaveResult::Locked:
		UiSound::play("se_sys_attention.wav");
		notify("SLOT IS LOCKED");
		break;
	case SuspendPoints::SaveResult::Occupied:
		UiSound::play("se_sys_attention.wav");
		notify("HOLD TO OVERWRITE");
		break;
	default:
		notify("NOTHING TO SAVE");
		break;
	}
}

void GuiSuspendPoints::confirmDelete(int slot)
{
	mWindow->pushGui(new GuiMsgBox(mWindow, "DELETE THIS SUSPEND POINT?", "YES", [this, slot] {
		UiSound::play("se_sys_delete.wav");
		static_cast<void>(mPoints.remove(mCode, slot));
		mSelected = slot;
		leaveMove();
		refresh();
	}, "NO", nullptr));
}

void GuiSuspendPoints::enterMove()
{
	mMoving = true;
	mMoveFrom = mSelected;
	mTarget = mSelected;
	UiSound::play("se_sys_click.wav");
	updateHelpPrompts();
}

void GuiSuspendPoints::leaveMove()
{
	mMoving = false;
	updateHelpPrompts();
}

bool GuiSuspendPoints::moveInput(InputConfig* config, const Input& input)
{
	if(input.value == 0)
		return true;

	if(config->isMappedLike("left", input))
	{
		mTarget = std::max(1, mTarget - 1);
		UiSound::play("se_sys_cursor.wav");
		updateHelpPrompts();
	}
	else if(config->isMappedLike("right", input))
	{
		mTarget = std::min(SuspendPoints::MaxSlots + 1, mTarget + 1);
		UiSound::play("se_sys_cursor.wav");
		updateHelpPrompts();
	}
	else if(config->isMappedTo("a", input))
	{
		if(mTarget > SuspendPoints::MaxSlots)
		{
			if(mSlots[static_cast<std::size_t>(mMoveFrom - 1)].locked)
				notify("SLOT IS LOCKED");
			else
				confirmDelete(mMoveFrom);
		}
		else
		{
			if(mTarget != mMoveFrom)
			{
				if(mPoints.swap(mCode, mMoveFrom, mTarget))
					UiSound::play("se_sys_click_game.wav");
				else
					notify("COULD NOT MOVE");
			}
			mSelected = mTarget;
			leaveMove();
			refresh();
		}
	}
	else if(config->isMappedTo("select", input) || config->isMappedTo("b", input) || config->isMappedLike("up", input))
	{
		UiSound::play("se_sys_cancel.wav");
		leaveMove();
	}
	return true;
}

bool GuiSuspendPoints::input(InputConfig* config, Input input)
{
	if(mClosing)
		return true;

	if(mMoving)
		return moveInput(config, input);

	if(isSaveKey(config, input))
	{
		if(input.value == 0)
		{
			mHolding = false;
			mHeldMs = 0;
			return true;
		}

		if(!mCardFocus)
		{
			if(selectedPoint().valid)
				resumeSelected();
			return true;
		}

		const SuspendPoint& point = selectedPoint();
		if(!point.valid)
			saveSelected(SuspendPoints::SaveMode::KeepExisting);
		else if(point.locked)
			notify("SLOT IS LOCKED");
		else
		{
			mHolding = true;
			mHeldMs = 0;
		}
		return true;
	}

	if(input.value == 0)
		return true;

	if(config->isMappedLike("left", input) || config->isMappedLike("right", input))
	{
		const bool left = config->isMappedLike("left", input);
		if(mCardFocus)
			mSelected = left ? std::max(1, mSelected - 1) : std::min(SuspendPoints::MaxSlots, mSelected + 1);
		else
		{
			int target = 0;
			for(int slot = mSelected + (left ? -1 : 1); slot >= 1 && slot <= SuspendPoints::MaxSlots; slot += left ? -1 : 1)
				if(mSlots[static_cast<std::size_t>(slot - 1)].valid)
				{
					target = slot;
					break;
				}
			if(target == 0)
				return true;
			mSelected = target;
		}
		UiSound::play("se_sys_cursor.wav");
	}
	else if(config->isMappedTo("start", input))
	{
		if(mCardFocus || selectedPoint().valid)
			resumeSelected();
	}
	else if(config->isMappedLike("down", input))
	{
		if(mCardFocus)
		{
			const int target = nearestSaved(mSelected);
			if(target != 0)
			{
				mCardFocus = false;
				mSelected = target;
				UiSound::play("se_sys_cursor.wav");
				updateHelpPrompts();
			}
		}
		else if(selectedPoint().valid)
		{
			const SuspendPoint& point = selectedPoint();
			UiSound::play(point.locked ? "se_sys_unlock.wav" : "se_sys_lock.wav");
			static_cast<void>(point.locked ? mPoints.unlock(mCode, mSelected) : mPoints.lock(mCode, mSelected));
			refresh();
		}
	}
	else if(config->isMappedTo("select", input))
	{
		if(!mCardFocus && selectedPoint().valid)
			enterMove();
	}
	else if(config->isMappedLike("up", input) && !mCardFocus && mFloating.valid)
	{
		mCardFocus = true;
		UiSound::play("se_sys_cursor.wav");
		updateHelpPrompts();
	}
	else if(config->isMappedLike("up", input) || config->isMappedTo("b", input))
	{
		UiSound::play("se_sys_cancel.wav");
		closeAnimated();
	}

	return true;
}

void GuiSuspendPoints::update(int deltaTime)
{
	mFloatMs += deltaTime;
	const int duration = mClosing ? SlideMs : WallInMs;
	const int step = mStarted ? std::min(deltaTime, MaxStepMs) : 0;
	mStarted = true;
	if(mSlideMs < duration)
		mSlideMs += step;
	if(mClosing && mSlideMs >= SlideMs)
	{
		close();
		return;
	}
	if(animated())
	{
		const float progress = std::min(1.0f, static_cast<float>(mSlideMs) / static_cast<float>(duration));
		const float visible = mClosing ? progress : 1.0f - progress * progress * (3.0f - 2.0f * progress);
		ImageComponent::setBackdropVisible(visible);
		GridTileComponent::setUnselectedBrightness(0.5f + 0.5f * visible);
		if(mClosing || mSlideMs < WallInMs)
		{
			ImageComponent::setScrollCoast(visible);
			ViewController::get()->update(deltaTime);
		}
	}
	else
	{
		ImageComponent::setBackdropVisible(0.0f);
		GridTileComponent::setUnselectedBrightness(0.5f);
	}
	if(mNoticeMs > 0)
	{
		mNoticeMs -= deltaTime;
		if(mNoticeMs <= 0)
			mNotice.setText("");
	}

	if(mHolding)
	{
		mHeldMs += deltaTime;
		if(mHeldMs >= HoldToOverwriteMs)
		{
			mHolding = false;
			mHeldMs = 0;
			saveSelected(SuspendPoints::SaveMode::Overwrite);
		}
	}
}

void GuiSuspendPoints::render(const Transform4x4f& parentTrans)
{
	const float width = static_cast<float>(Renderer::getScreenWidth());
	const float height = static_cast<float>(Renderer::getScreenHeight());

	const Transform4x4f base = parentTrans * getTransform();
	Renderer::setMatrix(base);

	Transform4x4f trans = base;
	trans.translate(Vector3f(0.0f, slideOffset(), 0.0f));
	Renderer::setMatrix(trans);
	Transform4x4f panelTrans = trans;
	panelTrans.translate(Vector3f(mPanelRect.x, mPanelRect.y, 0.0f));
	mPanel.render(panelTrans);

	mHeading.render(trans);
	mNotice.render(trans);

	for(std::size_t index = 0; index < mSlots.size(); ++index)
	{
		const bool hidden = mMoving && static_cast<int>(index) + 1 == mMoveFrom;
		const int highlighted = mMoving ? mTarget : (mCardFocus ? 0 : mSelected);
		renderSlot(trans, index, static_cast<int>(index) + 1 == highlighted && !hidden, hidden);
	}

	if(mFloating.valid && !mMoving)
		renderFloating(trans);

	if(mHasSaveData && !mMoving && !mClosing && selectedPoint().valid)
		renderWarning(base);

	if(mMoving)
	{
		Transform4x4f trashTrans = trans;
		trashTrans.translate(Vector3f(mTrashRect.x, mTrashRect.y, 0.0f));
		(mTarget > SuspendPoints::MaxSlots ? mTrashOpen : mTrash).render(trashTrans);
		renderLifted(trans);
	}
}

void GuiSuspendPoints::renderSlot(const Transform4x4f& trans, std::size_t index, bool selected, bool hidden)
{
	const float shift = static_cast<float>(index) * mCellStep;

	if((hidden || !mSlots[index].valid) && mHasEmptyCell)
	{
		Transform4x4f emptyTrans = trans;
		emptyTrans.translate(Vector3f(mCellEmptyRect.x + shift, mCellEmptyRect.y, 0.0f));
		(selected ? mCellEmptySelected : mCellEmpty).render(emptyTrans);
		if(!hidden)
		{
			mEmptyLabel.setPosition(mEmptyLabelBase.x() + shift, mEmptyLabelBase.y(), 0.0f);
			mEmptyLabel.render(trans);
		}
		return;
	}

	Transform4x4f cellTrans = trans;
	cellTrans.translate(Vector3f(mCellRect.x + shift, mCellRect.y, 0.0f));
	(selected ? mCellSelected : mCell).render(cellTrans);

	if(selected && !hidden)
	{
		ImageComponent& ring = *mFloat[FloatRing];
		ring.setOrigin(0.0f, 0.0f);
		ring.setResize(mCellRect.w + 32.0f, mCellRect.h + 32.0f);
		ring.setPosition(mCellRect.x + shift - 16.0f, mCellRect.y - 16.0f);
		ring.render(trans);
	}

	if(hidden)
		return;

	if(mSlots[index].valid)
		mShots[index]->render(trans);

	const Rect& frame = selected ? mFrameSelectedRect : mFrameRect;
	Transform4x4f frameTrans = trans;
	frameTrans.translate(Vector3f(frame.x + shift, frame.y, 0.0f));
	(selected ? mFrameSelected : mFrame).render(frameTrans);
	mLabels[index]->render(trans);

	if(mSlots[index].locked)
	{
		Transform4x4f lockTrans = trans;
		lockTrans.translate(Vector3f(mLockRect.x + shift, mLockRect.y, 0.0f));
		mLock.render(lockTrans);
	}

	if(selected && mHolding)
	{
		Renderer::setMatrix(trans);
		const float filled = mGaugeRect.w * static_cast<float>(mHeldMs) / static_cast<float>(HoldToOverwriteMs);
		Renderer::drawRect(mGaugeRect.x + shift, mGaugeRect.y, filled, mGaugeRect.h, mGaugeColor, mGaugeColor);
	}
}

void GuiSuspendPoints::renderWarning(const Transform4x4f& trans)
{
	const float centreX = static_cast<float>(Renderer::getScreenWidth()) / 2.0f + 3.0f;
	const float centreY = 222.0f;

	mWarning.setOrigin(0.5f, 0.5f);
	const float widen = std::max(0.0f, static_cast<float>(mWarning.getTextureSize().x()) - 420.0f);
	mWarning.setResize(420.0f + widen, 108.0f);
	mWarning.setPosition(centreX, centreY);
	mWarning.render(trans);

	mWarningIcon.setOrigin(0.5f, 0.5f);
	mWarningIcon.setResize(72.0f, 72.0f);
	mWarningIcon.setPosition(centreX - 162.0f - widen / 2.0f, centreY - 9.0f);
	mWarningIcon.render(trans);

	mWarningText.setLineSpacing(1.2f);
	mWarningText.setSize(310.0f + widen, 0.0f);
	mWarningText.setPosition(centreX - 126.0f - widen / 2.0f, centreY - 9.0f - 42.0f);
	mWarningText.render(trans);
}

void GuiSuspendPoints::renderFloating(const Transform4x4f& trans)
{
	constexpr float PI = 3.14159265f;
	const float bob = -FloatBobPixels * std::sin(2.0f * PI * static_cast<float>(mFloatMs % FloatBobMs) / static_cast<float>(FloatBobMs));
	const bool wingB = (mFloatMs / FloatWingMs) % 2 == 1;
	const float centreX = mCellRect.x + mCellRect.w / 2.0f + static_cast<float>(mSelected - 1) * mCellStep;
	const float centreY = mCellRect.y - FloatRise + bob - (mCardFocus ? 40.0f : 0.0f);

	const auto draw = [&](ImageComponent& image, float width, float height, float dx, float dy) {
		image.setOrigin(0.5f, 0.5f);
		image.setResize(width, height);
		image.setPosition(centreX + dx, centreY + dy);
		image.render(trans);
	};

	draw(*mFloat[FloatCard], 204.0f, 180.0f, 0.0f, 0.0f);
	if(mCardFocus)
	{
		draw(*mFloat[FloatActive], 192.0f, 168.0f, 0.0f, 0.0f);
		draw(*mFloat[FloatRing], 224.0f, 200.0f, 0.0f, 0.0f);
	}
	mFloatThumbnail.setOrigin(0.5f, 0.5f);
	mFloatThumbnail.setResize(160.0f, 120.0f);
	mFloatThumbnail.setPosition(centreX, centreY - 10.0f);
	mFloatThumbnail.render(trans);
	draw(*mFloat[FloatFrame], 160.0f, 120.0f, 0.0f, -10.0f);
	draw(*mFloat[wingB ? FloatWingLeftB : FloatWingLeftA], 72.0f, 96.0f, -129.0f, -72.0f);
	draw(*mFloat[wingB ? FloatWingB : FloatWingA], 72.0f, 96.0f, 129.0f, -72.0f);

	mFloatTime.setSize(90.0f, 30.0f);
	mFloatTime.setPosition(centreX - 6.0f, centreY + 52.0f);
	mFloatTime.render(trans);

	if(!mCardFocus)
		return;

	const float nudge = 4.0f * std::sin(2.0f * PI * static_cast<float>(mFloatMs % 900) / 900.0f);
	draw(*mFloat[FloatArrowLeft], 48.0f, 66.0f, -144.0f - nudge, 0.0f);
	draw(*mFloat[FloatArrowRight], 48.0f, 66.0f, 144.0f + nudge, 0.0f);

	const Vector2i balloon = mFloat[FloatBalloon]->getTextureSize();
	const Vector2i balloonIcon = mFloat[FloatBalloonIcon]->getTextureSize();
	constexpr float LabelWidth = 63.0f;
	constexpr float LabelGap = 5.0f;
	const float group = static_cast<float>(balloonIcon.x()) + LabelGap + LabelWidth;
	draw(*mFloat[FloatBalloon], static_cast<float>(balloon.x()), static_cast<float>(balloon.y()), 0.0f, 111.0f);
	draw(*mFloat[FloatBalloonIcon], static_cast<float>(balloonIcon.x()), static_cast<float>(balloonIcon.y()), -group / 2.0f + static_cast<float>(balloonIcon.x()) / 2.0f, 120.0f);
	mFloatLabel.setSize(90.0f, 30.0f);
	mFloatLabel.setPosition(centreX + group / 2.0f - LabelWidth, centreY + 105.0f);
	mFloatLabel.render(trans);
}

void GuiSuspendPoints::renderLifted(const Transform4x4f& trans)
{
	constexpr float LiftPixels = 6.0f;
	constexpr float TrashScale = 2.0f / 3.0f;

	const std::size_t from = static_cast<std::size_t>(mMoveFrom - 1);
	Transform4x4f lifted = trans;
	if(mTarget <= SuspendPoints::MaxSlots)
		lifted.translate(Vector3f(static_cast<float>(mTarget - mMoveFrom) * mCellStep, -LiftPixels, 0.0f));
	else
	{
		const float centreX = mCellRect.x + static_cast<float>(from) * mCellStep + mCellRect.w / 2.0f;
		const float centreY = mCellRect.y + mCellRect.h / 2.0f;
		lifted.translate(Vector3f(mTrashRect.x + mTrashRect.w / 2.0f - centreX, mTrashRect.y + mTrashRect.h / 2.0f - centreY - 24.0f, 0.0f));
		lifted.translate(Vector3f(centreX, centreY, 0.0f));
		lifted.scale(Vector3f(TrashScale, TrashScale, 1.0f));
		lifted.translate(Vector3f(-centreX, -centreY, 0.0f));
	}
	renderSlot(lifted, from, true, false);
}

HelpStyle GuiSuspendPoints::getHelpStyle()
{
	HelpStyle style;
	if(mTheme)
	{
		style.applyTheme(mTheme, mView);
		style.applyTheme(mTheme, mView, "suspend_help");
	}
	return style;
}

std::vector<HelpPrompt> GuiSuspendPoints::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	if(mMoving)
	{
		prompts.push_back(HelpPrompt("left/right", "select"));
		prompts.push_back(HelpPrompt("a", mTarget > SuspendPoints::MaxSlots ? "delete" : "move"));
		prompts.push_back(HelpPrompt("b", "back"));
		return prompts;
	}
	if(mCardFocus)
	{
		prompts.push_back(HelpPrompt("left/right", "select"));
		prompts.push_back(HelpPrompt("start", "start game"));
		prompts.push_back(HelpPrompt("b", "back"));
		prompts.push_back(HelpPrompt("a", "save"));
		return prompts;
	}
	if(!anySaved())
	{
		prompts.push_back(HelpPrompt("b", "back"));
		return prompts;
	}
	prompts.push_back(HelpPrompt("down", "lock/unlock"));
	prompts.push_back(HelpPrompt("left/right", "select"));
	prompts.push_back(HelpPrompt("select", "move/delete"));
	prompts.push_back(HelpPrompt("start", "start game"));
	prompts.push_back(HelpPrompt("b", "back"));
	return prompts;
}
