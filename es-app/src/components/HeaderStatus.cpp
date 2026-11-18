/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "components/HeaderStatus.h"

#include "renderers/Renderer.h"

#include <fstream>

namespace
{
	constexpr int OverclockRefreshMs = 5000;
	const char* const CpuFreqDir = "/sys/devices/system/cpu/cpu0/cpufreq/";

	long readNumber(const std::string& name)
	{
		std::ifstream file(std::string(CpuFreqDir) + name);
		long value = 0;
		file >> value;
		return value;
	}
}

HeaderStatus::HeaderStatus(Window* window)
	: GuiComponent(window), mDisk(window, "/media", ":/menu_disk.svg"), mNand(window, "/var/lib", ":/menu_nand.svg"),
	  mSwap(window, "", ":/menu_swap.svg", DiskSpaceIndicator::Source::Swap), mOverclock(window)
{
	mNand.placeLeftOf(mDisk);
}

void HeaderStatus::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, bool defaultLayout)
{
	mDiskShown = mDisk.applyTheme(theme, view, "diskSpace", "diskSpaceIcon");
	mNandShown = mNand.applyTheme(theme, view, "nandSpace", "nandSpaceIcon");
	mSwapShown = mSwap.applyTheme(theme, view, "swapSpace", "swapSpaceIcon");
	mOverclockDefault = false;
	mOverclockShown = theme->getElement(view, "overclockIcon", "image") != nullptr;
	if(mOverclockShown)
	{
		mOverclock.setImage(":/menu_overclock.svg");
		mOverclock.applyTheme(theme, view, "overclockIcon", ThemeFlags::ALL);
		refreshOverclock();
	}

	if(!mDiskShown && !mNandShown && defaultLayout)
	{
		mDisk.useDefaultLayout();
		mNand.useDefaultLayout();
		mNand.placeLeftOf(mDisk);
		mSwap.useDefaultLayout();
		mSwap.placeLeftOf(mNand);
		mDiskShown = true;
		mNandShown = true;
		mSwapShown = true;
		mOverclockShown = true;
		mOverclockDefault = true;
		mOverclock.setImage(":/menu_overclock.svg");
		mOverclock.setOrigin(0.5f, 0.5f);
		mOverclock.setColorShift(0xD07070FF);
		refreshOverclock();
	}
}

void HeaderStatus::refreshOverclock()
{
	const long maximum = readNumber("cpuinfo_max_freq");
	mOverclockActive = maximum > 0 && readNumber("scaling_max_freq") == maximum && readNumber("cpuinfo_boot_freq") != maximum;
	mOverclockElapsedMs = 0;
	if(mOverclockDefault)
		placeDefaultOverclock();
}

void HeaderStatus::placeDefaultOverclock()
{
	const float height = static_cast<float>(Renderer::getScreenHeight());
	const float size = height * 30.0f / 720.0f;
	const float edge = mSwap.isActive() ? mSwap.getLeftEdge() : mNand.getLeftEdge();
	mOverclock.setResize(size, size);
	mOverclock.setPosition(edge - 22.0f - size / 2, height * 0.075f);
}

void HeaderStatus::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
	if(mSwapShown)
		mSwap.update(deltaTime);
	if(mOverclockShown)
	{
		mOverclockElapsedMs += deltaTime;
		if(mOverclockElapsedMs >= OverclockRefreshMs)
			refreshOverclock();
	}
	if(mDiskShown)
		mDisk.update(deltaTime);
	if(mNandShown)
		mNand.update(deltaTime);
}

void HeaderStatus::render(const Transform4x4f& parentTrans)
{
	const Transform4x4f lifted = demoLift(parentTrans);
	if(mDiskShown)
		mDisk.render(lifted);
	if(mNandShown)
		mNand.render(lifted);
	if(mSwapShown)
		mSwap.render(lifted);
	if(mOverclockShown && mOverclockActive)
		mOverclock.render(lifted);
}
