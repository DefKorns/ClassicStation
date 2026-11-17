/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "SuspendPoints.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
	int failures = 0;

	void check(bool condition, const char* what)
	{
		if(!condition)
		{
			std::cerr << "FAIL: " << what << "\n";
			++failures;
		}
	}

	void run(const std::string& command)
	{
		if(std::system(command.c_str()) != 0)
			std::cerr << "command failed: " << command << "\n";
	}

	bool isDirectory(const std::string& path)
	{
		struct stat info;
		return ::stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
	}

	void makePoint(const std::string& dir, const std::string& playtime)
	{
		run("mkdir -p '" + dir + "/rollback'");
		std::ofstream(dir + "/rollback/savestate") << "state of " << dir;
		std::ofstream(dir + "/state.png") << "png";
		std::ofstream(dir + "/state.time") << playtime;
	}

	void makeFilesPoint(const std::string& dir, int slot, const std::string& playtime)
	{
		run("mkdir -p '" + dir + "'");
		const std::string prefix = dir + "/" + std::to_string(slot);
		std::ofstream(prefix + ".state") << "state of " << prefix;
		std::ofstream(prefix + ".state.png") << "png";
		if(!playtime.empty())
			std::ofstream(prefix + ".state_time.txt") << playtime;
	}

	bool exists(const std::string& path)
	{
		struct stat info;
		return ::stat(path.c_str(), &info) == 0;
	}
}

int main()
{
	char pattern[] = "/tmp/sp_test_XXXXXX";
	const std::string root = ::mkdtemp(pattern);
	const std::string profile = root + "/profile";
	const std::string vol = root + "/volatile";
	const SuspendPoints points(profile, vol);

	check(!points.floating().valid, "no floating at first");
	check(points.save("CLV-A", 1, SuspendPoints::SaveMode::KeepExisting) == SuspendPoints::SaveResult::NoFloating, "save without floating");

	makePoint(vol + "/CLV-A/suspendpoint0", "120");
	run("mkdir -p '" + vol + "/CLV-B/suspendpoint0'");
	const SuspendPoint floating = points.floating();
	check(floating.valid && floating.code == "CLV-A" && floating.playtime == 120, "floating found, empty one ignored");

	check(points.save("CLV-B", 1, SuspendPoints::SaveMode::KeepExisting) == SuspendPoints::SaveResult::WrongGame, "floating belongs to another game");
	check(points.save("CLV-A", 0, SuspendPoints::SaveMode::KeepExisting) == SuspendPoints::SaveResult::BadSlot, "slot 0 rejected");
	check(points.save("CLV-A", 5, SuspendPoints::SaveMode::KeepExisting) == SuspendPoints::SaveResult::BadSlot, "slot 5 rejected");
	check(points.save("CLV-A", 2, SuspendPoints::SaveMode::KeepExisting) == SuspendPoints::SaveResult::Saved, "save into empty slot");
	check(!points.floating().valid, "floating consumed");
	check(points.get("CLV-A", 2).valid && points.get("CLV-A", 2).playtime == 120, "slot 2 readable");
	check(!points.get("CLV-A", 1).valid, "slot 1 still empty");

	makePoint(vol + "/CLV-A/suspendpoint0", "300");
	check(points.save("CLV-A", 2, SuspendPoints::SaveMode::KeepExisting) == SuspendPoints::SaveResult::Occupied, "occupied without overwrite");
	check(points.lock("CLV-A", 2) && points.get("CLV-A", 2).locked, "lock slot");
	check(points.save("CLV-A", 2, SuspendPoints::SaveMode::Overwrite) == SuspendPoints::SaveResult::Locked, "locked slot refuses overwrite");
	check(points.unlock("CLV-A", 2) && !points.get("CLV-A", 2).locked, "unlock slot");
	check(points.save("CLV-A", 2, SuspendPoints::SaveMode::Overwrite) == SuspendPoints::SaveResult::Saved, "overwrite unlocked slot");
	check(points.get("CLV-A", 2).playtime == 300, "overwritten content");
	check(points.get("CLV-A", 2).thumbnail == profile + "/CLV-A/suspendpoint2/state.png", "thumbnail path");

	makePoint(vol + "/CLV-A/suspendpoint0", "500");
	check(points.save("CLV-A", 4, SuspendPoints::SaveMode::KeepExisting) == SuspendPoints::SaveResult::Saved, "save into slot 4");
	check(points.swap("CLV-A", 2, 4), "swap two slots");
	check(points.get("CLV-A", 2).playtime == 500 && points.get("CLV-A", 4).playtime == 300, "swapped contents");
	check(points.swap("CLV-A", 4, 3), "move into an empty slot");
	check(!points.get("CLV-A", 4).valid && points.get("CLV-A", 3).playtime == 300, "moved contents");

	const auto slots = points.slots("CLV-A");
	check(!slots[0].valid && slots[1].valid && slots[2].valid && !slots[3].valid, "slots() reflects slots 2 and 3");

	run("mkdir -p '" + profile + "/CLV-A/suspendpoint1'");
	check(!points.get("CLV-A", 1).valid, "directory without state.png/state.time is invalid");

	check(points.remove("CLV-A", 2) && !points.get("CLV-A", 2).valid, "remove slot");

	makePoint(profile + "/CLV-A/suspendpoint3.old", "777");
	run("rm -rf '" + profile + "/CLV-A/suspendpoint3'");
	check(points.get("CLV-A", 3).valid && points.get("CLV-A", 3).playtime == 777, "interrupted replacement restores the previous point");
	check(!isDirectory(profile + "/CLV-A/suspendpoint3.old"), "restored point no longer left as .old");

	makePoint(profile + "/CLV-A/suspendpoint3.old", "1");
	check(points.get("CLV-A", 3).playtime == 777, "a stale .old does not replace a present point");

	makePoint(vol + "/CLV-A/suspendpoint0", "900");
	check(points.save("CLV-A", 3, SuspendPoints::SaveMode::Overwrite) == SuspendPoints::SaveResult::Saved, "overwrite with a stale .old around");
	check(points.get("CLV-A", 3).playtime == 900, "overwritten content after a stale .old");
	check(!isDirectory(profile + "/CLV-A/suspendpoint3.old") && !isDirectory(profile + "/CLV-A/suspendpoint3.new"), "no .old or .new left after a save");

	makePoint(vol + "/CLV-A/suspendpoint0", "1");
	check(points.discardFloating() && !points.floating().valid, "discard floating");

	makeFilesPoint(vol + "/CLV-S", 0, "15");
	const SuspendPoint stockFloating = points.floating();
	check(stockFloating.valid && stockFloating.files && stockFloating.code == "CLV-S" && stockFloating.playtime == 15, "floating in the stock file layout");
	check(points.save("CLV-S", 1, SuspendPoints::SaveMode::KeepExisting) == SuspendPoints::SaveResult::Saved, "save a file-layout point");
	check(!exists(vol + "/CLV-S/0.state") && !exists(vol + "/CLV-S/0.state.png") && !exists(vol + "/CLV-S/0.state_time.txt"), "floating files consumed");
	const SuspendPoint filed = points.get("CLV-S", 1);
	check(filed.valid && filed.files && filed.playtime == 15, "file-layout slot readable");
	check(filed.thumbnail == profile + "/CLV-S/1.state.png", "file-layout thumbnail path");
	check(exists(profile + "/CLV-S/1.state") && exists(profile + "/CLV-S/1.state.png") && exists(profile + "/CLV-S/1.state_time.txt"), "stock file names in the profile");
	check(!isDirectory(profile + "/CLV-S/suspendpoint1"), "no directory made for a file-layout point");

	makeFilesPoint(profile + "/CLV-T", 2, "11");
	const auto stockSlots = points.slots("CLV-T");
	check(!stockSlots[0].valid && stockSlots[1].valid && stockSlots[1].files && stockSlots[1].playtime == 11, "slots() reads a point made by the stock menu");
	makeFilesPoint(profile + "/CLV-T", 3, "");
	check(points.get("CLV-T", 3).valid && points.get("CLV-T", 3).playtime == 0, "missing state_time.txt gives a zero play time");

	makeFilesPoint(vol + "/CLV-S", 0, "40");
	check(points.save("CLV-S", 1, SuspendPoints::SaveMode::KeepExisting) == SuspendPoints::SaveResult::Occupied, "file-layout slot occupied");
	check(points.lock("CLV-S", 1) && points.get("CLV-S", 1).locked && exists(profile + "/CLV-S/1.es_locked"), "lock a file-layout slot");
	check(points.save("CLV-S", 1, SuspendPoints::SaveMode::Overwrite) == SuspendPoints::SaveResult::Locked, "locked file-layout slot refuses overwrite");
	check(points.unlock("CLV-S", 1) && !points.get("CLV-S", 1).locked && !exists(profile + "/CLV-S/1.es_locked"), "unlock a file-layout slot");
	check(points.save("CLV-S", 1, SuspendPoints::SaveMode::Overwrite) == SuspendPoints::SaveResult::Saved && points.get("CLV-S", 1).playtime == 40, "overwrite a file-layout slot");

	makeFilesPoint(vol + "/CLV-S", 0, "60");
	check(points.save("CLV-S", 2, SuspendPoints::SaveMode::KeepExisting) == SuspendPoints::SaveResult::Saved, "second file-layout slot");
	check(points.swap("CLV-S", 1, 2) && points.get("CLV-S", 1).playtime == 60 && points.get("CLV-S", 2).playtime == 40, "swap two file-layout slots");
	check(points.swap("CLV-S", 2, 4) && !points.get("CLV-S", 2).valid && points.get("CLV-S", 4).playtime == 40, "move a file-layout slot into an empty one");
	makePoint(profile + "/CLV-S/suspendpoint3", "300");
	check(points.swap("CLV-S", 3, 4) && points.get("CLV-S", 3).files && points.get("CLV-S", 3).playtime == 40 && !points.get("CLV-S", 4).files && points.get("CLV-S", 4).playtime == 300, "swap a directory slot with a file slot");

	makePoint(vol + "/CLV-S/suspendpoint0", "900");
	check(points.save("CLV-S", 1, SuspendPoints::SaveMode::Overwrite) == SuspendPoints::SaveResult::Saved, "directory point over a file slot");
	check(!points.get("CLV-S", 1).files && points.get("CLV-S", 1).playtime == 900 && !exists(profile + "/CLV-S/1.state"), "file slot replaced by a directory point");
	makeFilesPoint(vol + "/CLV-S", 0, "42");
	check(points.save("CLV-S", 1, SuspendPoints::SaveMode::Overwrite) == SuspendPoints::SaveResult::Saved, "file point over a directory slot");
	check(points.get("CLV-S", 1).files && points.get("CLV-S", 1).playtime == 42 && !isDirectory(profile + "/CLV-S/suspendpoint1"), "directory slot replaced by a file point");

	check(points.remove("CLV-S", 1) && !points.get("CLV-S", 1).valid && !exists(profile + "/CLV-S/1.state.png"), "remove a file-layout slot");
	makeFilesPoint(vol + "/CLV-S", 0, "1");
	check(points.discardFloating() && !points.floating().valid && !exists(vol + "/CLV-S/0.state"), "discard a file-layout floating point");

	makePoint(vol + "/CLV-OLD/suspendpoint0", "5");
	run("touch -d '2026-01-01 00:00:00' '" + vol + "/CLV-OLD/suspendpoint0/state.time'");
	makeFilesPoint(vol + "/CLV-NEW", 0, "6");
	check(points.floating().code == "CLV-NEW", "the newest floating point wins across layouts");
	check(points.discardFloating() && points.floating().code == "CLV-OLD" && points.discardFloating(), "discard walks through both layouts");

	run("rm -rf '" + root + "'");
	if(failures == 0)
		std::cout << "all suspend point checks passed\n";
	return failures == 0 ? 0 : 1;
}
