/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_APP_DEBUG_MODE_H
#define ES_APP_DEBUG_MODE_H

#include <cstdio>
#include <unistd.h>

namespace DebugMode
{
	constexpr const char* markerPath = "/var/lib/hakchi/emulationstation/debug";

	inline bool isEnabled()
	{
		return access(markerPath, F_OK) == 0;
	}

	inline bool toggle()
	{
		if(isEnabled())
			return unlink(markerPath) != 0;

		FILE* marker = fopen(markerPath, "w");
		if(marker == nullptr)
			return false;
		fclose(marker);
		return true;
	}
}

#endif // ES_APP_DEBUG_MODE_H
