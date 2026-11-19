/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "SuspendPoints.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

constexpr int SuspendPoints::MaxSlots;

namespace
{
	constexpr const char* FloatingName = "suspendpoint0";
	constexpr const char* LockedName = "es.locked";
	constexpr const char* FileLockedSuffix = ".es_locked";
	constexpr const char* SwapPrefix = "es-swap";
	constexpr std::array<const char*, 4> FileSuffixes = { ".state", ".state.png", ".state_time.txt", FileLockedSuffix };
	constexpr const char* StagingSuffix = ".new";
	constexpr const char* SwapSuffix = ".swap";
	constexpr const char* PreviousSuffix = ".old";
	constexpr mode_t DirectoryMode = 0755;

	using DirHandle = std::unique_ptr<DIR, int (*)(DIR*)>;

	bool readStat(const std::string& path, struct stat& info) noexcept
	{
		return ::stat(path.c_str(), &info) == 0;
	}

	bool isDirectory(const std::string& path) noexcept
	{
		struct stat info;
		return readStat(path, info) && S_ISDIR(info.st_mode);
	}

	bool isFile(const std::string& path) noexcept
	{
		struct stat info;
		return readStat(path, info) && S_ISREG(info.st_mode);
	}

	long modifiedTime(const std::string& path) noexcept
	{
		struct stat info;
		return readStat(path, info) ? static_cast<long>(info.st_mtime) : 0;
	}

	std::string environmentOr(const char* name, const char* fallback)
	{
		const char* value = std::getenv(name);
		return value != nullptr ? value : fallback;
	}

	std::vector<std::string> listDirectory(const std::string& path)
	{
		std::vector<std::string> names;
		const DirHandle dir(::opendir(path.c_str()), &::closedir);
		if(!dir)
			return names;

		while(const dirent* entry = ::readdir(dir.get()))
		{
			const std::string name = entry->d_name;
			if(name != "." && name != "..")
				names.push_back(name);
		}
		return names;
	}

	bool makeDirectories(const std::string& path)
	{
		if(isDirectory(path))
			return true;

		const std::string::size_type slash = path.find_last_of('/');
		if(slash != std::string::npos && slash > 0 && !makeDirectories(path.substr(0, slash)))
			return false;

		return ::mkdir(path.c_str(), DirectoryMode) == 0 || errno == EEXIST;
	}

	bool removeTree(const std::string& path)
	{
		if(isDirectory(path))
		{
			const std::vector<std::string> names = listDirectory(path);
			const bool emptied = std::all_of(names.begin(), names.end(),
				[&path](const std::string& name) { return removeTree(path + "/" + name); });
			return emptied && ::rmdir(path.c_str()) == 0;
		}
		return ::unlink(path.c_str()) == 0 || errno == ENOENT;
	}

	bool copyFile(const std::string& from, const std::string& to)
	{
		std::ifstream in(from, std::ios::binary);
		std::ofstream out(to, std::ios::binary | std::ios::trunc);
		if(!in || !out)
			return false;

		out << in.rdbuf();
		out.flush();
		return static_cast<bool>(out);
	}

	bool copyTree(const std::string& from, const std::string& to)
	{
		if(!isDirectory(from))
			return copyFile(from, to);

		if(!makeDirectories(to))
			return false;

		const std::vector<std::string> names = listDirectory(from);
		return std::all_of(names.begin(), names.end(),
			[&from, &to](const std::string& name) { return copyTree(from + "/" + name, to + "/" + name); });
	}

	bool moveTree(const std::string& from, const std::string& to)
	{
		if(::rename(from.c_str(), to.c_str()) == 0)
			return true;
		if(errno != EXDEV)
			return false;

		if(!copyTree(from, to))
		{
			static_cast<void>(removeTree(to));
			return false;
		}
		return removeTree(from);
	}

	bool isValidPoint(const std::string& dir)
	{
		return (isFile(dir + "/state") || isDirectory(dir + "/rollback"))
			&& isFile(dir + "/state.png")
			&& isFile(dir + "/state.time");
	}

	std::string filePrefix(const std::string& dir, int slot)
	{
		return dir + "/" + std::to_string(slot);
	}

	bool isFilesPoint(const std::string& dir, int slot)
	{
		const std::string prefix = filePrefix(dir, slot);
		return isFile(prefix + ".state") && isFile(prefix + ".state.png");
	}

	bool hasFileContents(const std::string& prefix)
	{
		return std::any_of(FileSuffixes.begin(), FileSuffixes.end(),
			[&prefix](const char* suffix) { return isFile(prefix + suffix); });
	}

	bool renameFiles(const std::string& from, const std::string& to)
	{
		for(const char* suffix : FileSuffixes)
			if(isFile(from + suffix) && ::rename((from + suffix).c_str(), (to + suffix).c_str()) != 0)
				return false;
		return true;
	}

	bool removeFiles(const std::string& prefix)
	{
		return std::all_of(FileSuffixes.begin(), FileSuffixes.end(),
			[&prefix](const char* suffix) { return ::unlink((prefix + suffix).c_str()) == 0 || errno == ENOENT; });
	}

	bool moveFiles(const std::string& from, const std::string& to)
	{
		constexpr std::array<const char*, 3> suffixes = { ".state_time.txt", ".state.png", ".state" };
		const std::string old = ".old";
		for(const char* suffix : suffixes)
			if(isFile(to + suffix) && ::rename((to + suffix).c_str(), (to + suffix + old).c_str()) != 0)
				return false;

		bool moved = true;
		for(const char* suffix : suffixes)
			if(isFile(from + suffix) && !moveTree(from + suffix, to + suffix))
			{
				moved = false;
				break;
			}

		for(const char* suffix : suffixes)
		{
			if(!moved)
			{
				static_cast<void>(::unlink((to + suffix).c_str()));
				static_cast<void>(::rename((to + suffix + old).c_str(), (to + suffix).c_str()));
			}
			else
				static_cast<void>(::unlink((to + suffix + old).c_str()));
		}
		return moved;
	}

	bool replaceDirectory(const std::string& from, const std::string& target)
	{
		const std::string previous = target + PreviousSuffix;
		if(!removeTree(previous))
			return false;

		const bool hadTarget = isDirectory(target);
		if(hadTarget && ::rename(target.c_str(), previous.c_str()) != 0)
			return false;

		if(::rename(from.c_str(), target.c_str()) != 0)
		{
			if(hadTarget)
				static_cast<void>(::rename(previous.c_str(), target.c_str()));
			return false;
		}
		static_cast<void>(removeTree(previous));
		return true;
	}

	void restoreInterrupted(const std::string& target)
	{
		const std::string previous = target + PreviousSuffix;
		if(!isDirectory(target) && isDirectory(previous))
			static_cast<void>(::rename(previous.c_str(), target.c_str()));
	}

	long readNumber(const std::string& file)
	{
		std::ifstream in(file);
		long value = 0;
		in >> value;
		return in ? value : 0;
	}

	SuspendPoint describe(const std::string& code, int slot, const std::string& dir, const std::string& gameDir)
	{
		SuspendPoint point;
		point.slot = slot;
		point.code = code;
		if(isValidPoint(dir))
		{
			point.valid = true;
			point.dir = dir;
			point.thumbnail = dir + "/state.png";
			point.playtime = readNumber(dir + "/state.time");
			point.locked = isFile(dir + "/" + LockedName);
			return point;
		}
		if(isFilesPoint(gameDir, slot))
		{
			const std::string prefix = filePrefix(gameDir, slot);
			point.valid = true;
			point.files = true;
			point.dir = gameDir;
			point.thumbnail = prefix + ".state.png";
			point.playtime = readNumber(prefix + ".state_time.txt");
			point.locked = isFile(prefix + FileLockedSuffix);
			return point;
		}
		return SuspendPoint();
	}

	long pointTime(const SuspendPoint& point)
	{
		if(!point.files)
			return modifiedTime(point.dir + "/state.time");
		const std::string prefix = filePrefix(point.dir, point.slot);
		const long stamped = modifiedTime(prefix + ".state_time.txt");
		return stamped != 0 ? stamped : modifiedTime(prefix + ".state");
	}

	bool validSlot(int slot) noexcept
	{
		return slot >= 1 && slot <= SuspendPoints::MaxSlots;
	}
}

SuspendPoints::SuspendPoints(std::string profileDir, std::string volatileDir)
	: mProfileDir(std::move(profileDir)), mVolatileDir(std::move(volatileDir))
{
}

SuspendPoints SuspendPoints::fromEnvironment()
{
	return SuspendPoints(environmentOr("ES_SP_PROFILE", "/var/lib/clover/profiles/0"),
		environmentOr("ES_SP_VOLATILE", "/var/cache/clover/volatile"));
}

std::string SuspendPoints::codeDir(const std::string& code, int slot) const
{
	return (slot == 0 ? mVolatileDir : mProfileDir) + "/" + code;
}

std::string SuspendPoints::slotDir(const std::string& code, int slot) const
{
	if(slot == 0)
		return mVolatileDir + "/" + code + "/" + FloatingName;
	return mProfileDir + "/" + code + "/suspendpoint" + std::to_string(slot);
}

SuspendPoint SuspendPoints::floating() const
{
	SuspendPoint newest;
	long newestTime = 0;

	for(const std::string& code : listDirectory(mVolatileDir))
	{
		SuspendPoint candidate = describe(code, 0, slotDir(code, 0), codeDir(code, 0));
		if(!candidate.valid)
			continue;

		const long time = pointTime(candidate);
		if(!newest.valid || time > newestTime)
		{
			newestTime = time;
			newest = std::move(candidate);
		}
	}
	return newest;
}

SuspendPoint SuspendPoints::get(const std::string& code, int slot) const
{
	if(!validSlot(slot))
		return SuspendPoint();
	restoreInterrupted(slotDir(code, slot));
	return describe(code, slot, slotDir(code, slot), codeDir(code, slot));
}

std::array<SuspendPoint, SuspendPoints::MaxSlots> SuspendPoints::slots(const std::string& code) const
{
	std::array<SuspendPoint, MaxSlots> result;
	for(std::size_t index = 0; index < result.size(); ++index)
		result[index] = get(code, static_cast<int>(index) + 1);
	return result;
}

SuspendPoints::SaveResult SuspendPoints::save(const std::string& code, int slot, SaveMode mode) const
{
	if(!validSlot(slot))
		return SaveResult::BadSlot;

	const SuspendPoint pending = describe(code, 0, slotDir(code, 0), codeDir(code, 0));
	if(!pending.valid)
		return floating().valid ? SaveResult::WrongGame : SaveResult::NoFloating;

	const SuspendPoint current = get(code, slot);
	if(current.valid)
	{
		if(current.locked)
			return SaveResult::Locked;
		if(mode == SaveMode::KeepExisting)
			return SaveResult::Occupied;
	}

	const std::string target = slotDir(code, slot);
	if(!makeDirectories(mProfileDir + "/" + code))
		return SaveResult::Failed;

	if(pending.files)
	{
		const std::string from = filePrefix(pending.dir, 0);
		const std::string to = filePrefix(codeDir(code, slot), slot);
		static_cast<void>(::unlink((to + FileLockedSuffix).c_str()));
		if(!moveFiles(from, to))
			return SaveResult::Failed;
		static_cast<void>(removeTree(target));
		return SaveResult::Saved;
	}

	const std::string staging = target + StagingSuffix;
	if(!removeTree(staging) || !moveTree(pending.dir, staging))
		return SaveResult::Failed;

	if(!replaceDirectory(staging, target))
	{
		static_cast<void>(removeTree(staging));
		return SaveResult::Failed;
	}
	static_cast<void>(removeFiles(filePrefix(codeDir(code, slot), slot)));
	return SaveResult::Saved;
}

bool SuspendPoints::remove(const std::string& code, int slot) const
{
	return validSlot(slot) && removeTree(slotDir(code, slot)) && removeFiles(filePrefix(codeDir(code, slot), slot));
}

bool SuspendPoints::lock(const std::string& code, int slot) const
{
	const SuspendPoint point = get(code, slot);
	if(!point.valid)
		return false;

	const std::ofstream marker(point.files ? filePrefix(point.dir, slot) + FileLockedSuffix : point.dir + "/" + LockedName);
	return static_cast<bool>(marker);
}

bool SuspendPoints::unlock(const std::string& code, int slot) const
{
	const SuspendPoint point = get(code, slot);
	if(!point.valid)
		return false;

	const std::string marker = point.files ? filePrefix(point.dir, slot) + FileLockedSuffix : point.dir + "/" + LockedName;
	return ::unlink(marker.c_str()) == 0 || errno == ENOENT;
}

bool SuspendPoints::swap(const std::string& code, int first, int second) const
{
	if(first == second || !validSlot(first) || !validSlot(second))
		return false;

	struct Name { std::string directory; std::string prefix; };
	const auto nameOf = [this, &code](int slot) { return Name{ slotDir(code, slot), filePrefix(codeDir(code, slot), slot) }; };
	const auto holds = [](const Name& name) { return isDirectory(name.directory) || hasFileContents(name.prefix); };
	const auto move = [](const Name& from, const Name& to)
	{
		if(isDirectory(from.directory) && ::rename(from.directory.c_str(), to.directory.c_str()) != 0)
			return false;
		return renameFiles(from.prefix, to.prefix);
	};

	const Name a = nameOf(first);
	const Name b = nameOf(second);
	const Name spare{ a.directory + SwapSuffix, codeDir(code, first) + "/" + SwapPrefix };

	const bool hasFirst = holds(a);
	const bool hasSecond = holds(b);
	if(!hasFirst && !hasSecond)
		return false;

	if(hasFirst && !move(a, spare))
		return false;

	if(hasSecond && !move(b, a))
	{
		if(hasFirst)
			static_cast<void>(move(spare, a));
		return false;
	}
	return !hasFirst || move(spare, b);
}

bool SuspendPoints::hasSaveData(const std::string& code) const
{
	for(const char* name : {"/save.sram", "/cartridge.sram"})
	{
		std::ifstream file(mProfileDir + "/" + code + name, std::ios::binary | std::ios::ate);
		if(file && file.tellg() > 0)
			return true;
	}
	return false;
}

bool SuspendPoints::discardFloating() const
{
	const SuspendPoint pending = floating();
	if(!pending.valid)
		return true;
	return pending.files ? removeFiles(filePrefix(pending.dir, 0)) : removeTree(pending.dir);
}
