/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "components/DemoMode.h"

#include "renderers/Renderer.h"
#include "resources/ResourceManager.h"
#include "Settings.h"
#include "ThemeData.h"
#include "Window.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace
{
	constexpr unsigned int StartMs = 60000;
	constexpr int CooldownMs = 20000;
	constexpr int LuigiDelayMs = 700;
	constexpr int MarioFrameMs = 50;
	constexpr int LuigiFrameMs = 50;
	constexpr float WalkSpeed = 350.0f;
	constexpr float SpriteWidth = 72.0f;
	constexpr float SpriteHeight = 96.0f;
	constexpr float GroundFeetY = 630.0f;
	constexpr float DesignHeight = 720.0f;

	constexpr double TickMs = 1000.0 / 60.0;
	constexpr int MaxTicks = 8;
	constexpr int FadeMs = 250;
	constexpr int StageMs = 400;
	constexpr float StageTravel = 600.0f;
	constexpr double BlockHeight = 192.0;
	constexpr double HitReach = 54.856;
	constexpr double WalkStep = 4.0;
	constexpr double RunStep = 8.0;
	constexpr double BackStep = 0.8;
	constexpr double Edge = 640.0;
	constexpr double ApproachGap = 128.0;
	constexpr double LuigiGap = 256.0;
	constexpr double LuigiNear = 160.0;
	constexpr double LuigiHalt = 480.0;
	constexpr double MarioTurn = -320.0;
	constexpr float SpriteSize = 64.0f;
	constexpr unsigned int MarioColour = 0xAF4A46FF;
	constexpr unsigned int LuigiColour = 0x527E20FF;

	const std::vector<int> WalkTable = {1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2};
	const std::vector<int> RunTable = {1, 1, 1, 2, 2, 2, 3, 3, 3, 2, 2, 2};
	const std::vector<int> BackTable = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
	const std::vector<double> JumpRise = {12, 10, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 6, 6, 6, 5, 5, 0, 0, -5, -5, -6, -6, -6, -7, -7, -7, -7,
		-8, -8, -8, -8, -8, -8, -8, -8, -10, -12};
	const std::vector<int> BlockTable = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1};
	const std::vector<double> RumbleRise = {4, 12, 8, 4, 0, 0, 0, -4, -4, -8, -8, -4, -4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	const std::vector<int> RumblePattern = {1, 1, 2, 2, 1, 1, 0, 0};

	std::string spriteFolder()
	{
		const std::map<std::string, ThemeSet> sets = ThemeData::getThemeSets();
		const auto set = sets.find(Settings::getInstance()->getString("ThemeSet"));
		return set == sets.end() ? std::string() : set->second.path + "/demo/";
	}

	float screenScale()
	{
		return static_cast<float>(Renderer::getScreenHeight()) / DesignHeight;
	}

	float eased(float amount)
	{
		const float once = 1.0f - std::cos(amount * 1.5707963f);
		return 1.0f - std::cos(once * 1.5707963f);
	}

	unsigned int mix(unsigned int from, unsigned int to, float amount)
	{
		unsigned int result = 0;
		for(int shift = 24; shift >= 0; shift -= 8)
		{
			const float a = static_cast<float>((from >> shift) & 0xFF);
			const float b = static_cast<float>((to >> shift) & 0xFF);
			result |= static_cast<unsigned int>(a + (b - a) * amount + 0.5f) << shift;
		}
		return result;
	}
}

DemoMode::Runner::Runner(Window* window, const std::string& prefix_, int frameMs_, int delayMs_)
	: prefix(prefix_), sprite(window), frameMs(frameMs_), delayMs(delayMs_)
{
	sprite.setOrigin(0.5f, 1.0f);
}

void DemoMode::Runner::locate(const std::string& folder)
{
	mAvailable = !folder.empty();
	for(std::size_t index = 0; index < frames.size(); ++index)
	{
		frames[index] = folder + prefix + "0" + std::to_string(index + 1) + ".png";
		mAvailable = mAvailable && ResourceManager::getInstance()->fileExists(frames[index]);
	}
	frame = -1;
}

void DemoMode::Runner::begin(float startX)
{
	x = startX;
	waitedMs = 0;
	elapsedMs = 0;
	frame = -1;
	running = false;
	finished = false;
}

void DemoMode::Runner::advance(int deltaTime, float direction, float step, float limitLow, float limitHigh)
{
	if(finished)
		return;
	if(!running)
	{
		waitedMs += deltaTime;
		if(waitedMs < delayMs)
			return;
		running = true;
	}

	x += direction * step;
	elapsedMs += deltaTime;
	const int next = (elapsedMs / frameMs) % static_cast<int>(frames.size());
	if(next != frame)
	{
		frame = next;
		sprite.setImage(frames[static_cast<std::size_t>(frame)]);
	}
	if((direction > 0.0f && x > limitHigh) || (direction < 0.0f && x < limitLow))
		finished = true;
}

void DemoMode::Runner::draw(const Transform4x4f& parentTrans, float direction, float scale, float feetY)
{
	if(!running || finished || frame < 0)
		return;
	sprite.setResize(SpriteWidth * scale, SpriteHeight * scale);
	sprite.setFlipX(direction > 0.0f);
	sprite.setPosition(x, feetY);
	sprite.render(parentTrans);
}

DemoMode::DemoMode(Window* window)
	: GuiComponent(window), mMario(window, "mario_run", MarioFrameMs, 0), mLuigi(window, "luigi_run", LuigiFrameMs, LuigiDelayMs), mRandom(std::random_device{}())
{
}

DemoMode::~DemoMode()
{
	setDemoProgress(0.0f);
}

void DemoMode::cancel()
{
	if(!mActive)
	{
		mCooldownMs = 0;
		return;
	}

	if(!mBlocks)
	{
		mActive = false;
		mCooldownMs = 0;
		return;
	}

	if(mStage == Stage::Hide)
	{
		mStage = Stage::Show;
		mStageMs = std::max(0, StageMs - mStageMs);
	}
	else if(mStage == Stage::Rise || mStage == Stage::Run)
	{
		mStage = Stage::Sink;
		mStageMs = 0;
		if((mShown & 0xFF) != 0)
			startFade(mShown & 0xFFFFFF00);
	}
}

float DemoMode::drop() const
{
	const float amount = std::min(1.0f, static_cast<float>(mStageMs) / static_cast<float>(StageMs));
	if(mStage == Stage::Rise)
		return (1.0f - eased(amount)) * StageTravel;
	if(mStage == Stage::Sink)
		return eased(amount) * StageTravel;
	return 0.0f;
}

void DemoMode::runTicks(int deltaTime)
{
	mAccumulator += deltaTime;
	for(int tick = 0; mAccumulator >= TickMs && tick < MaxTicks; ++tick)
	{
		mAccumulator -= TickMs;
		stepBlocks();
	}
	mAccumulator = std::min(mAccumulator, TickMs);
}

void DemoMode::advanceStage(int deltaTime)
{
	mStageMs += deltaTime;
	const float amount = std::min(1.0f, static_cast<float>(mStageMs) / static_cast<float>(StageMs));
	const bool done = mStageMs >= StageMs;

	switch(mStage)
	{
	case Stage::Hide:
		setDemoProgress(eased(amount));
		if(done)
		{
			startBlocks();
			mStage = Stage::Rise;
			mStageMs = 0;
		}
		break;
	case Stage::Rise:
		setDemoProgress(1.0f);
		runTicks(deltaTime);
		if(done)
		{
			mStage = Stage::Run;
			mStageMs = 0;
		}
		break;
	case Stage::Run:
		setDemoProgress(1.0f);
		runTicks(deltaTime);
		mStageMs = 0;
		break;
	case Stage::Sink:
		setDemoProgress(1.0f);
		runTicks(deltaTime);
		if(done)
		{
			mStage = Stage::Show;
			mStageMs = 0;
		}
		break;
	case Stage::Show:
		setDemoProgress(1.0f - eased(amount));
		if(done)
		{
			mStage = Stage::Idle;
			mActive = false;
			mCooldownMs = 0;
			setDemoProgress(0.0f);
		}
		break;
	case Stage::Idle:
		break;
	}
}

void DemoMode::startPass()
{
	const float width = static_cast<float>(Renderer::getScreenWidth());
	const float margin = SpriteWidth * screenScale();
	const float startX = mPass % 2 == 0 ? -margin : width + margin;
	mMario.begin(startX);
	mLuigi.begin(startX);
}

int DemoMode::roll(int low, int high)
{
	return std::uniform_int_distribution<int>(low, high)(mRandom);
}

bool DemoMode::locateBlocks(const std::string& folder)
{
	if(folder.empty())
		return false;
	if(mLoaded)
		return true;

	const auto add = [&](std::deque<ImageComponent>& frames, const std::string& path)
	{
		if(!ResourceManager::getInstance()->fileExists(path))
			return false;
		frames.emplace_back(mWindow);
		frames.back().setOrigin(0.5f, 1.0f);
		frames.back().setImage(path);
		return true;
	};
	const auto load = [&](std::deque<ImageComponent>& frames, const std::string& name, int count)
	{
		for(int index = 0; index < count; ++index)
			if(!add(frames, folder + name + "_" + std::to_string(index) + ".png"))
				return false;
		return true;
	};
	const auto loadRun = [&](std::deque<ImageComponent>& frames, const std::string& name)
	{
		static const int order[] = {1, 1, 2, 3, 1};
		for(const int frame : order)
			if(!add(frames, folder + name + "_run0" + std::to_string(frame) + ".png"))
				return false;
		return true;
	};

	mTag = false;
	if(load(mMarioFrames, "mario", 5) && load(mLuigiFrames, "luigi", 5) && load(mBlockFrames, "block", 3))
	{
		mLoaded = true;
		return true;
	}
	mMarioFrames.clear();
	mLuigiFrames.clear();
	mBlockFrames.clear();

	if(loadRun(mMarioFrames, "mario") && loadRun(mLuigiFrames, "luigi"))
	{
		mTag = true;
		mLoaded = true;
		return true;
	}
	mMarioFrames.clear();
	mLuigiFrames.clear();
	return false;
}

void DemoMode::startBlocks()
{
	mMarioMover = Mover();
	mLuigiMover = Mover();
	mMarioFigure = Figure();
	mLuigiFigure = Figure();
	mBlock = Block();
	mMarioMover.wait = 60 + roll(5, 8) * 60;
	mLuigiMover.wait = 60;
	mSwitchOn = true;
	mAccumulator = 0.0;
	mShown = 0;
	mFading = false;
}

void DemoMode::startFade(unsigned int colour)
{
	mFadeFrom = (mShown & 0xFF) == 0 ? (colour & 0xFFFFFF00) : mShown;
	mFadeTo = colour;
	mFadeMs = 0;
	mFading = true;
}

void DemoMode::actMario()
{
	Mover& m = mMarioMover;
	Mover& l = mLuigiMover;
	if(m.wait > 0)
	{
		--m.wait;
		if(m.wait > 0)
			return;
	}

	if(!m.placed)
	{
		m.placed = true;
		m.x = -Edge;
		m.running = 0;
		m.wait = 0;
		m.walking = 1;
		m.backwalking = 0;
		m.waitWalking = 0;
	}

	static const int jumps[] = {1, 1, 1, 3};
	if(m.running > 0)
	{
		m.x += RunStep;
		if(m.x > Edge)
		{
			--m.running;
			m.x = -Edge;
			if(m.running == 0)
			{
				m.placed = false;
				m.wait = roll(1, 8) * 60;
			}
		}
	}
	else if(m.jumping > 0)
	{
		if(m.waitJumping <= 0)
		{
			--m.jumping;
			mSwitchOn = true;
			if(m.jumping > 0)
			{
				m.wait = 1;
				m.waitJumping = 1;
			}
			else
				m.wait = 30;
		}
	}
	else if(m.walking != 0)
	{
		bool move = true;
		if(l.placed)
		{
			if(l.x - m.x < ApproachGap)
			{
				m.running = 3;
				m.walking = 0;
				l.running = 3;
				l.wait = 0;
			}
			else if(m.x == 0.0)
			{
				if(!mSwitchOn)
				{
					m.jumping = jumps[roll(0, 3)];
					m.waitJumping = 1;
					move = false;
				}
			}
			else if(roll(1, 64) == 1)
			{
				m.wait = roll(1, 2) * 32;
				move = false;
			}
			else if(roll(1, 64) == 1 && m.walking <= 0)
				m.walking = 1;
		}
		else if(mSwitchOn)
		{
			if(m.walking == 1)
			{
				if(m.x > MarioTurn)
					m.walking = -1;
				if(roll(1, 32) == 1)
					m.walking = -1;
			}
		}
		else if(m.x == 0.0)
		{
			m.jumping = jumps[roll(0, 3)];
			m.waitJumping = 1;
			move = false;
		}

		if(m.walking != 0 && move)
		{
			m.x += m.walking * WalkStep;
			if(m.x < -Edge)
			{
				m.placed = false;
				m.wait = roll(1, 8) * 60;
			}
		}
	}
}

void DemoMode::actLuigi()
{
	Mover& m = mMarioMover;
	Mover& l = mLuigiMover;
	if(l.wait > 0)
	{
		--l.wait;
		if(l.wait > 0)
			return;
	}

	if(!l.placed)
	{
		l.placed = true;
		l.x = Edge;
		l.running = 0;
		l.wait = 0;
		l.walking = -1;
		l.backwalking = 0;
		l.waitWalking = 0;
	}

	const auto wander = [&]
	{
		const int choice = roll(0, 2);
		if(choice == 0)
		{
			l.walking = 0;
			l.backwalking = 0;
			l.waitWalking = roll(1, 2) * 32;
		}
		else if(choice == 1)
		{
			l.walking = 1;
			l.backwalking = 0;
		}
		else
		{
			l.walking = 0;
			l.backwalking = 1;
		}
	};

	if(l.jumping > 0)
	{
		if(l.waitJumping <= 0)
		{
			--l.jumping;
			mSwitchOn = false;
			if(l.jumping > 0)
			{
				l.waitJumping = 1;
				l.running = 1;
			}
		}
	}
	else if(l.running > 0)
	{
		l.x += RunStep;
		if(l.x > Edge)
		{
			--l.running;
			l.x = -Edge;
			if(l.running == 0)
			{
				l.placed = false;
				l.wait = roll(1, 8) * 60;
			}
		}
		else if(!mTag && l.x == 0.0 && l.running == 1 && mSwitchOn && roll(1, 4) == 1)
		{
			l.jumping = 1;
			l.waitJumping = 1;
		}
	}
	else
	{
		bool move = true;
		if(m.placed)
		{
			if(roll(1, 32) == 1)
				wander();
			if(l.walking < 0 && (l.x - m.x < LuigiGap || l.x < LuigiNear))
				wander();
			if(l.x - m.x < ApproachGap)
			{
				m.running = 3;
				l.running = 3;
				m.walking = 0;
				m.waitWalking = 0;
			}
		}
		else if(!mSwitchOn)
		{
			if(l.walking == -1 && l.x < LuigiHalt)
				l.walking = 0;
		}
		else if(!mTag && l.x == 0.0)
		{
			l.jumping = 1;
			l.waitJumping = 1;
			l.running = 1;
			move = false;
		}

		if(l.waitWalking > 0)
			--l.waitWalking;
		else if(l.walking != 0 && move)
		{
			l.x += l.walking * WalkStep;
			if(l.x > Edge || l.x < -Edge)
			{
				l.placed = false;
				l.wait = roll(1, 8) * 60;
			}
		}
		else if(l.backwalking != 0 && move)
		{
			l.x += l.backwalking * BackStep;
			if(l.x > Edge)
			{
				l.placed = false;
				l.wait = roll(1, 8) * 60;
			}
		}
	}
}

bool DemoMode::pose(Figure& figure, Pose next)
{
	if(figure.pose != next)
	{
		figure.pose = next;
		figure.working = false;
	}
	if(next == Pose::None)
	{
		figure.visible = false;
		return true;
	}
	figure.visible = true;

	const auto cycle = [&](const std::vector<int>& table)
	{
		if(!figure.working)
		{
			figure.working = true;
			figure.cycle = 0;
		}
		figure.pattern = table[figure.cycle];
		figure.cycle = (figure.cycle + 1) % table.size();
		return false;
	};

	switch(next)
	{
	case Pose::Walk:
		return cycle(WalkTable);
	case Pose::Run:
		return cycle(RunTable);
	case Pose::Back:
		return cycle(BackTable);
	case Pose::Jump:
		if(!figure.working)
		{
			figure.working = true;
			figure.step = 0;
			figure.base = figure.y;
		}
		if(figure.step + 1 < JumpRise.size())
		{
			figure.y += JumpRise[figure.step++];
			figure.pattern = 4;
			return false;
		}
		figure.y = figure.base;
		figure.pattern = 0;
		figure.working = false;
		return true;
	default:
		figure.pattern = 0;
		return false;
	}
}

void DemoMode::drive(Mover& mover, Figure& figure)
{
	if(!mover.placed)
	{
		pose(figure, Pose::None);
		return;
	}

	figure.x = mover.x;
	Pose next = Pose::Stay;
	int direction = 0;
	if(mover.wait > 0)
		next = Pose::Stay;
	else if(mover.jumping != 0)
		next = Pose::Jump;
	else if(mover.running != 0)
	{
		next = Pose::Run;
		direction = 1;
	}
	else if(mover.waitWalking > 0)
		next = Pose::Stay;
	else if(mover.backwalking != 0)
	{
		next = Pose::Back;
		direction = mover.backwalking > 0 ? -1 : 1;
	}
	else if(mover.walking != 0)
	{
		next = Pose::Walk;
		direction = mover.walking > 0 ? 1 : -1;
	}

	if(direction != 0)
		figure.left = direction < 0;
	if(pose(figure, next) && figure.pose == Pose::Jump)
		mover.waitJumping = 0;
}

void DemoMode::stepBlock()
{
	if(!mBlock.visible)
	{
		if(++mBlock.frames >= 2)
		{
			mBlock.visible = true;
			mBlock.y = BlockHeight;
		}
		return;
	}

	if(mBlock.rumbling)
	{
		if(mBlock.step + 1 < RumbleRise.size())
		{
			mBlock.y += RumbleRise[mBlock.step++];
			mBlock.pattern = RumblePattern[mBlock.cycle];
			mBlock.cycle = (mBlock.cycle + 1) % RumblePattern.size();
		}
		else
		{
			mBlock.y = mBlock.base;
			mBlock.pattern = 0;
			mBlock.rumbling = false;
			mBlock.step = 0;
			mBlock.cycle = 0;
		}
		return;
	}

	mBlock.pattern = BlockTable[mBlock.cycle];
	mBlock.cycle = (mBlock.cycle + 1) % BlockTable.size();

	unsigned int colour = 0;
	if(mMarioFigure.pose == Pose::Jump && mBlock.y - mMarioFigure.y < HitReach)
		colour = MarioColour;
	if(mLuigiFigure.pose == Pose::Jump && mBlock.y - mLuigiFigure.y < HitReach)
		colour = LuigiColour;
	if(colour != 0)
	{
		startFade(colour);
		mBlock.rumbling = true;
		mBlock.step = 0;
		mBlock.cycle = 0;
		mBlock.base = mBlock.y;
	}
}

void DemoMode::stepBlocks()
{
	actMario();
	drive(mMarioMover, mMarioFigure);
	actLuigi();
	drive(mLuigiMover, mLuigiFigure);
	if(!mTag)
		stepBlock();
}

void DemoMode::update(int deltaTime, bool ready, float coverTop)
{
	if(mFading)
	{
		mFadeMs += deltaTime;
		const float amount = std::min(1.0f, static_cast<float>(mFadeMs) / static_cast<float>(FadeMs));
		mShown = mix(mFadeFrom, mFadeTo, amount);
		if(amount >= 1.0f)
			mFading = false;
	}

	if(mBlocks && mStage != Stage::Idle)
		advanceStage(deltaTime);

	if(!Settings::getInstance()->getBool("DemoMode"))
	{
		cancel();
		return;
	}

	const unsigned int idle = mWindow->getTimeSinceLastInput();
	const bool interrupted = idle < mLastIdle;
	mLastIdle = idle;
	if(mActive && (interrupted || !ready))
		cancel();
	if(!mActive)
	{
		if(idle < StartMs || !ready)
			return;
		mCooldownMs -= deltaTime;
		if(mCooldownMs > 0)
			return;
		const std::string folder = spriteFolder();
		if(locateBlocks(folder))
		{
			mBlocks = true;
			mActive = true;
			mStage = Stage::Hide;
			mStageMs = 0;
			return;
		}
		mBlocks = false;
		mMario.locate(folder);
		mLuigi.locate(folder);
		if(!mMario.available() || !mLuigi.available())
		{
			mCooldownMs = CooldownMs;
			return;
		}
		mActive = true;
		mPass = 0;
		startPass();
	}

	if(mBlocks)
		return;

	mCoverTop = coverTop;
	const float scale = screenScale();
	const float width = static_cast<float>(Renderer::getScreenWidth());
	const float margin = SpriteWidth * scale;
	const float direction = mPass % 2 == 0 ? 1.0f : -1.0f;
	const float step = WalkSpeed * scale * static_cast<float>(deltaTime) / 1000.0f;
	mMario.advance(deltaTime, direction, step, -margin, width + margin);
	mLuigi.advance(deltaTime, direction, step, -margin, width + margin);

	if(mMario.finished && mLuigi.finished)
	{
		++mPass;
		if(mPass >= 2)
		{
			mActive = false;
			mCooldownMs = CooldownMs;
			return;
		}
		startPass();
	}
}

void DemoMode::render(const Transform4x4f& parentTrans)
{
	if((mShown & 0xFF) == 0)
		return;

	Renderer::setMatrix(parentTrans);
	Renderer::drawRect(0.0f, 0.0f, static_cast<float>(Renderer::getScreenWidth()), static_cast<float>(Renderer::getScreenHeight()), mShown, mShown);
}

void DemoMode::renderActors(const Transform4x4f& parentTrans)
{
	if(!mActive)
		return;

	const float scale = screenScale();
	if(mBlocks)
	{
		if(mStage == Stage::Hide || mStage == Stage::Show)
			return;
		const float centre = static_cast<float>(Renderer::getScreenWidth()) / 2.0f;
		const float dropped = drop();
		const auto draw = [&](ImageComponent& image, double x, double up, bool left)
		{
			image.setResize((mTag ? SpriteWidth : SpriteSize) * scale, (mTag ? SpriteHeight : SpriteSize) * scale);
			image.setFlipX(mTag ? !left : left);
			image.setPosition(centre + static_cast<float>(x) * scale, (GroundFeetY - static_cast<float>(up) + dropped) * scale);
			image.render(parentTrans);
		};
		if(!mTag && mBlock.visible)
			draw(mBlockFrames[static_cast<std::size_t>(mBlock.pattern)], 0.0, mBlock.y, false);
		if(mLuigiFigure.visible)
			draw(mLuigiFrames[static_cast<std::size_t>(mLuigiFigure.pattern)], mLuigiFigure.x, mLuigiFigure.y, mLuigiFigure.left);
		if(mMarioFigure.visible)
			draw(mMarioFrames[static_cast<std::size_t>(mMarioFigure.pattern)], mMarioFigure.x, mMarioFigure.y, mMarioFigure.left);
		return;
	}

	const float direction = mPass % 2 == 0 ? 1.0f : -1.0f;
	const float feetY = mPass % 2 == 0 ? GroundFeetY * scale : mCoverTop;
	mMario.draw(parentTrans, direction, scale, feetY);
	mLuigi.draw(parentTrans, direction, scale, feetY);
}
