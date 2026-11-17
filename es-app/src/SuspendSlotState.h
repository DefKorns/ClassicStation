/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_SUSPEND_SLOT_STATE_H
#define ES_APP_SUSPEND_SLOT_STATE_H

#include "SuspendPoints.h"

#include <array>

enum class SlotState
{
	Empty,
	Saved,
	Locked
};

using SlotStates = std::array<SlotState, SuspendPoints::MaxSlots>;

inline SlotState slotState(const SuspendPoint& point)
{
	if(!point.valid)
		return SlotState::Empty;
	return point.locked ? SlotState::Locked : SlotState::Saved;
}

inline SlotStates slotStates(const std::array<SuspendPoint, SuspendPoints::MaxSlots>& points)
{
	SlotStates states = {{SlotState::Empty, SlotState::Empty, SlotState::Empty, SlotState::Empty}};
	for(std::size_t i = 0; i < points.size(); ++i)
		states[i] = slotState(points[i]);
	return states;
}

#endif // ES_APP_SUSPEND_SLOT_STATE_H
