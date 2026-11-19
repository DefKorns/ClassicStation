/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_COMPONENTS_DEMO_MODE_H
#define ES_APP_COMPONENTS_DEMO_MODE_H

#include "GuiComponent.h"
#include "components/ImageComponent.h"
#include <array>
#include <deque>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <vector>

class Sound;

class DemoMode : public GuiComponent
{
public:
	explicit DemoMode(Window* window);
	~DemoMode() override;

	bool active() const { return mActive; }
	void cancel();
	void update(int deltaTime, bool ready, float coverTop);
	void render(const Transform4x4f& parentTrans) override;
	void renderActors(const Transform4x4f& parentTrans);

private:
	struct Runner
	{
		Runner(Window* window, const std::string& prefix, int frameMs, int delayMs);

		void locate(const std::string& folder);
		bool available() const { return mAvailable; }
		void begin(float startX);
		void advance(int deltaTime, float direction, float step, float limitLow, float limitHigh);
		void draw(const Transform4x4f& parentTrans, float direction, float scale, float feetY);

		std::string prefix;
		std::array<std::string, 3> frames;
		ImageComponent sprite;
		int frameMs;
		int delayMs;
		int waitedMs = 0;
		int elapsedMs = 0;
		int frame = -1;
		float x = 0.0f;
		bool running = false;
		bool finished = false;
		bool mAvailable = false;
	};

	enum class Pose { None, Stay, Walk, Back, Run, Jump };
	enum class Stage { Idle, Hide, Rise, Run, Sink, Show };

	struct Mover
	{
		int wait = 0;
		int jumping = 0;
		int waitJumping = 0;
		int running = 0;
		int walking = 0;
		int backwalking = 0;
		int waitWalking = 0;
		bool placed = false;
		double x = 0.0;
	};

	struct Figure
	{
		bool visible = false;
		bool left = false;
		int pattern = 0;
		double x = 0.0;
		double y = 0.0;
		Pose pose = Pose::None;
		bool working = false;
		std::size_t step = 0;
		std::size_t cycle = 0;
		double base = 0.0;
	};

	struct DotSource
	{
		int x;
		int y;
		unsigned int colour;
	};

	struct Dot
	{
		float x = 0.0f;
		float y = 0.0f;
		unsigned int colour = 0;
		float delay = 0.0f;
		float volume = 0.0f;
		float wait = 0.0f;
		int phase = 0;
	};

	struct Block
	{
		bool visible = false;
		bool rumbling = false;
		int pattern = 0;
		double y = 0.0;
		std::size_t step = 0;
		std::size_t cycle = 0;
		double base = 0.0;
		int frames = 0;
	};

	bool locateBlocks(const std::string& folder);
	bool locateSkit(const std::string& folder);
	void startSkit();
	void advanceSkit(int deltaTime);
	void renderSkit(const Transform4x4f& parentTrans);
	void startBlocks();
	void stepBlocks();
	void actMario();
	void actLuigi();
	bool pose(Figure& figure, Pose next);
	void drive(Mover& mover, Figure& figure);
	void stepBlock();
	void startFade(unsigned int colour);
	bool loadPortraits(const std::string& folder);
	void startPortrait();
	void advancePortrait(float seconds);
	void rumblePortrait();
	void renderPortrait(const Transform4x4f& parentTrans);
	void startTransit(unsigned int colour);
	void advanceTransit(int deltaTime);
	void renderTransit(const Transform4x4f& parentTrans);
	int roll(int low, int high);
	void startPass();
	void advanceStage(int deltaTime);
	void runTicks(int deltaTime);
	float drop() const;

	Runner mMario;
	Runner mLuigi;
	ImageComponent mSky;
	ImageComponent mLogo;
	bool mActive = false;
	bool mBlocks = false;
	bool mTag = false;
	bool mSkit = false;
	bool mSkitLoaded = false;
	int mSkitMs = 0;
	int mSampleIndex = -1;
	int mSampleFrame = 0;
	bool mAmbient = false;
	float mSampleX = 0.0f;
	float mSampleY = 0.0f;
	double mSkyScroll = 0.0;
	std::deque<ImageComponent> mSampleLarge;
	std::deque<ImageComponent> mSampleRing;
	std::shared_ptr<Sound> mSkitSound;
	bool mLoaded = false;
	Stage mStage = Stage::Idle;
	int mStageMs = 0;
	int mPass = 0;
	int mCooldownMs = 0;
	unsigned int mLastIdle = 0;
	float mCoverTop = 0.0f;

	std::deque<ImageComponent> mMarioFrames;
	std::deque<ImageComponent> mLuigiFrames;
	std::deque<ImageComponent> mBlockFrames;
	Mover mMarioMover;
	Mover mLuigiMover;
	Figure mMarioFigure;
	Figure mLuigiFigure;
	Block mBlock;
	bool mSwitchOn = true;
	double mAccumulator = 0.0;
	unsigned int mFadeFrom = 0;
	unsigned int mFadeTo = 0;
	int mFadeMs = 0;
	bool mFading = false;
	unsigned int mShown = 0;
	std::vector<std::vector<DotSource>> mPortraitSources;
	std::vector<Dot> mDots;
	std::size_t mPortraitCursor = 0;
	std::size_t mRumbleStep = 0;
	float mPortraitX = 0.0f;
	float mPortraitY = 0.0f;
	float mPortraitVx = 0.0f;
	float mPortraitVy = 0.0f;
	float mPortraitHold = 0.0f;
	bool mPortraitLive = false;
	bool mPortraitBreaking = false;
	bool mTransit = false;
	int mTransitMs = 0;
	unsigned int mTransitColour = 0;
	std::mt19937 mRandom;
};

#endif // ES_APP_COMPONENTS_DEMO_MODE_H
