/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_VIEWS_EXTRA_BUTTONS_H
#define ES_APP_VIEWS_EXTRA_BUTTONS_H

#include "InputManager.h"

#include <cstdlib>
#include <string>

inline bool isFamicomPad()
{
	const char* pad = std::getenv("ES_PAD");
	return pad != nullptr && std::string(pad) == "nes";
}

inline bool hasExtraButtons()
{
	return !isFamicomPad() || InputManager::getInstance()->hasJoystickOtherThan("Nintendo Clovercon");
}

#endif // ES_APP_VIEWS_EXTRA_BUTTONS_H
