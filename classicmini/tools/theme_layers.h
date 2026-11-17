/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#ifndef CLASSICMINI_THEME_LAYERS_H
#define CLASSICMINI_THEME_LAYERS_H

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace theme_layers
{
	struct Rect
	{
		int x;
		int y;
		int w;
		int h;
	};

	struct Insets
	{
		int left;
		int top;
		int right;
		int bottom;
	};

	class Image
	{
	public:
		Image() = default;
		Image(int width, int height) : mWidth(width), mHeight(height), mPixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0) {}

		int width() const { return mWidth; }
		int height() const { return mHeight; }
		bool empty() const { return mPixels.empty(); }
		std::vector<unsigned char>& pixels() { return mPixels; }
		const std::vector<unsigned char>& pixels() const { return mPixels; }

		unsigned char* at(int x, int y) { return &mPixels[offset(x, y)]; }
		const unsigned char* at(int x, int y) const { return &mPixels[offset(x, y)]; }

	private:
		std::size_t offset(int x, int y) const { return (static_cast<std::size_t>(y) * static_cast<std::size_t>(mWidth) + static_cast<std::size_t>(x)) * 4; }

		int mWidth = 0;
		int mHeight = 0;
		std::vector<unsigned char> mPixels;
	};

	enum class Family
	{
		Unknown,
		A,
		B
	};

	using Atlas = std::map<std::string, Rect>;

	struct Layer
	{
		std::string name;
		Image image;
	};

	struct Options
	{
		int iconScale = 3;
		int frameScale = 2;
		int rowWidth = 538;
		int rowHeight = 57;
		int selectorWidth = 546;
		int selectorHeight = 66;
	};

	inline Atlas parseAtlas(const std::string& json)
	{
		static const std::string frameKey = "\"textureFrame\":[";

		Atlas atlas;
		std::size_t position = json.find("\"sprites\"");
		while(position != std::string::npos && (position = json.find(frameKey, position)) != std::string::npos)
		{
			const std::size_t nameEnd = position >= 3 ? position - 3 : std::string::npos;
			if(nameEnd == std::string::npos || json.compare(nameEnd, 3, "\":{") != 0)
				throw std::runtime_error("unexpected atlas layout near offset " + std::to_string(position));

			const std::size_t nameStart = json.rfind('"', nameEnd - 1);
			if(nameStart == std::string::npos)
				throw std::runtime_error("sprite name without an opening quote");

			std::size_t cursor = position + frameKey.size();
			std::vector<int> values;
			while(values.size() < 4)
			{
				char* end = nullptr;
				const char* begin = json.c_str() + cursor;
				values.push_back(static_cast<int>(std::strtol(begin, &end, 10)));
				if(end == begin)
					throw std::runtime_error("sprite frame without a number near offset " + std::to_string(cursor));
				cursor += static_cast<std::size_t>(end - begin);
				if(cursor < json.size() && json[cursor] == ',')
					++cursor;
			}

			atlas[json.substr(nameStart + 1, nameEnd - nameStart - 1)] = Rect{values[0], values[1], values[2], values[3]};
			position = cursor;
		}
		return atlas;
	}

	inline Family detectFamily(const Atlas& atlas)
	{
		const auto has = [&atlas](const char* name) { return atlas.count(name) != 0; };
		if(has("arrow_down.png") && has("resume_screen_frame.png"))
			return Family::A;
		if(has("icon_X_light.png") && has("Icon_Save.png"))
			return Family::B;
		return Family::Unknown;
	}

	inline const char* familyName(Family family)
	{
		switch(family)
		{
		case Family::A: return "A";
		case Family::B: return "B";
		default: return "unknown";
		}
	}

	inline Image crop(const Image& source, const Rect& area)
	{
		if(area.w <= 0 || area.h <= 0 || area.x < 0 || area.y < 0 || area.x + area.w > source.width() || area.y + area.h > source.height())
			throw std::out_of_range("crop outside the image");

		Image result(area.w, area.h);
		for(int row = 0; row < area.h; ++row)
			std::copy_n(source.at(area.x, area.y + row), static_cast<std::size_t>(area.w) * 4, result.at(0, row));
		return result;
	}

	inline Image scale(const Image& source, int factor)
	{
		if(factor < 1)
			throw std::invalid_argument("scale factor below 1");

		Image result(source.width() * factor, source.height() * factor);
		for(int y = 0; y < result.height(); ++y)
			for(int x = 0; x < result.width(); ++x)
				std::copy_n(source.at(x / factor, y / factor), 4, result.at(x, y));
		return result;
	}

	namespace detail
	{
		inline int sourceIndex(int target, int targetSize, int sourceSize, int before, int after)
		{
			if(target < before)
				return target;
			if(target >= targetSize - after)
				return sourceSize - (targetSize - target);
			const int middle = targetSize - before - after;
			return before + (target - before) * (sourceSize - before - after) / middle;
		}
	}

	inline Image ninePatch(const Image& source, const Insets& insets, int width, int height)
	{
		if(insets.left + insets.right >= source.width() || insets.top + insets.bottom >= source.height())
			throw std::invalid_argument("nine-patch insets leave no centre");
		if(width < insets.left + insets.right + 1 || height < insets.top + insets.bottom + 1)
			throw std::invalid_argument("nine-patch target smaller than its borders");

		Image result(width, height);
		for(int y = 0; y < height; ++y)
		{
			const int sourceY = detail::sourceIndex(y, height, source.height(), insets.top, insets.bottom);
			for(int x = 0; x < width; ++x)
				std::copy_n(source.at(detail::sourceIndex(x, width, source.width(), insets.left, insets.right), sourceY), 4, result.at(x, y));
		}
		return result;
	}

	inline Image sprite(const Image& sheet, const Atlas& atlas, const std::string& name)
	{
		const auto found = atlas.find(name + ".png");
		if(found == atlas.end())
			throw std::runtime_error("the atlas has no sprite '" + name + "'");
		return crop(sheet, found->second);
	}

	inline std::vector<Layer> buildLayers(const Image& sheet, const Atlas& atlas, const Options& options)
	{
		std::vector<Layer> layers;
		layers.push_back({"rating_filled", scale(sprite(sheet, atlas, "icon_Resume_on"), options.iconScale)});
		layers.push_back({"rating_empty", scale(sprite(sheet, atlas, "icon_Resume_off"), options.iconScale)});

		const auto frame = [&](const char* name, const Insets& nativeInsets, int width, int height)
		{
			const Image source = scale(sprite(sheet, atlas, name), options.frameScale);
			const Insets insets{nativeInsets.left * options.frameScale, nativeInsets.top * options.frameScale, nativeInsets.right * options.frameScale, nativeInsets.bottom * options.frameScale};
			return ninePatch(source, insets, width, height);
		};
		layers.push_back({"list_row", frame("resume_card", Insets{4, 4, 4, 4}, options.rowWidth, options.rowHeight)});
		layers.push_back({"list_selector", frame("btn_init_on", Insets{6, 6, 6, 6}, options.selectorWidth, options.selectorHeight)});
		return layers;
	}
}

#endif
