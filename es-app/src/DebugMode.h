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

#include <csignal>
#include <cstdio>
#include <unistd.h>

namespace DebugMode
{
	constexpr const char* markerPath = "/var/lib/hakchi/emulationstation/debug";

	inline volatile std::sig_atomic_t& activeFlag()
	{
		static volatile std::sig_atomic_t active = 0;
		return active;
	}

	inline bool isActive() { return activeFlag() != 0; }
	inline void setActive(bool active) { activeFlag() = active ? 1 : 0; }

	constexpr const char* developerPath = "/var/lib/hakchi/emulationstation/developer";

	inline bool markerExists(const char* path) { return access(path, F_OK) == 0; }

	inline bool setMarker(const char* path, bool present)
	{
		if(!present)
			return unlink(path) == 0 || !markerExists(path);

		FILE* marker = fopen(path, "w");
		if(marker == nullptr)
			return false;
		fclose(marker);
		return true;
	}

	inline bool isEnabled() { return markerExists(markerPath); }
	inline bool setEnabled(bool enabled) { return setMarker(markerPath, enabled); }
	constexpr const char* journalPath = "/var/lib/hakchi/emulationstation/crtfx_journal";
	inline bool isJournalEnabled() { return markerExists(journalPath); }
	inline bool setJournalEnabled(bool enabled) { return setMarker(journalPath, enabled); }
	inline bool isUnlocked() { return markerExists(developerPath); }
	inline bool unlock() { return setMarker(developerPath, true); }
}

#endif // ES_APP_DEBUG_MODE_H
