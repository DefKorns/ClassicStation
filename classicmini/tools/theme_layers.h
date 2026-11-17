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
		int titleWidth = 1004;
		int titleHeight = 53;
		int cardScale = 3;
		Rect cardWindow{49, 5, 148, 204};
		int cardExtraTop = 56;
		int barScale = 3;
		int screenWidth = 1280;
		int background = -1;
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

	inline Image centerCrop(const Image& source, int width)
	{
		if(width <= 0 || source.width() <= width)
			return source;
		return crop(source, Rect{(source.width() - width) / 2, 0, width, source.height()});
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

	inline bool differsFromBackground(const unsigned char* pixel, const unsigned char* background)
	{
		return std::abs(pixel[0] - background[0]) + std::abs(pixel[1] - background[1]) + std::abs(pixel[2] - background[2]) > 60;
	}

	inline std::string logoSvg(const Image& bar, const Rect& area)
	{
		std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " + std::to_string(area.w) + " " + std::to_string(area.h) + "\" shape-rendering=\"crispEdges\" fill=\"#FFFFFF\">\n";
		for(int row = 0; row < area.h; ++row)
		{
			const unsigned char* background = bar.at(0, area.y + row);
			int runStart = -1;
			for(int column = 0; column <= area.w; ++column)
			{
				const bool lit = column < area.w && differsFromBackground(bar.at(area.x + column, area.y + row), background);
				if(lit && runStart < 0)
					runStart = column;
				if(!lit && runStart >= 0)
				{
					svg += "<rect x=\"" + std::to_string(runStart) + "\" y=\"" + std::to_string(row) + "\" width=\"" + std::to_string(column - runStart) + "\" height=\"1\"/>\n";
					runStart = -1;
				}
			}
		}
		return svg + "</svg>\n";
	}

	inline Rect findBakedLogo(const Image& bar, int rows, int xLimit)
	{
		const int limitX = std::min(xLimit, bar.width());
		const int limitY = std::min(rows, bar.height());
		int minX = limitX;
		int maxX = -1;
		int minY = limitY;
		int maxY = -1;
		for(int y = 0; y < limitY; ++y)
		{
			const unsigned char* background = bar.at(0, y);
			for(int x = 0; x < limitX; ++x)
			{
				const unsigned char* pixel = bar.at(x, y);
				if(!differsFromBackground(pixel, background))
					continue;
				minX = std::min(minX, x);
				maxX = std::max(maxX, x);
				minY = std::min(minY, y);
				maxY = std::max(maxY, y);
			}
		}
		return maxX < 0 ? Rect{0, 0, 0, 0} : Rect{minX, minY, maxX - minX + 1, maxY - minY + 1};
	}

	inline void clearArea(Image& image, const Rect& area)
	{
		for(int y = area.y; y < area.y + area.h; ++y)
			for(int x = area.x; x < area.x + area.w; ++x)
				std::copy_n(image.at(0, y), 4, image.at(x, y));
	}

	inline Image cardFrame(const Image& card, const Rect& window, int extraTop)
	{
		if(window.w <= 0 || window.h <= 0 || window.x < 0 || window.y < 0 || window.x + window.w > card.width() || window.y + window.h > card.height() || extraTop < 0)
			throw std::out_of_range("card window outside the card");

		Image result(card.width(), card.height() + extraTop);
		for(int y = 0; y < card.height(); ++y)
			std::copy_n(card.at(0, y), static_cast<std::size_t>(card.width()) * 4, result.at(0, y + extraTop));
		for(int y = window.y; y < window.y + window.h; ++y)
			for(int x = window.x; x < window.x + window.w; ++x)
				std::fill_n(result.at(x, y + extraTop), 4, static_cast<unsigned char>(0));
		return result;
	}

	struct FrameSpec
	{
		const char* sprite;
		Insets insets;
	};

	inline FrameSpec rowFrameSpec(Family family)
	{
		return family == Family::B ? FrameSpec{"caption_title", Insets{7, 7, 7, 7}} : FrameSpec{"resume_card", Insets{4, 4, 4, 4}};
	}

	inline FrameSpec selectorFrameSpec()
	{
		return FrameSpec{"btn_init_on", Insets{6, 6, 6, 6}};
	}

	inline std::vector<std::string> requiredSprites(Family family)
	{
		return {"icon_Resume_on", "icon_Resume_off", rowFrameSpec(family).sprite, selectorFrameSpec().sprite, "caption_title", "game_card", "game_card_active", "menubar_U", "menubar_L"};
	}

	inline std::vector<Layer> buildLayers(const Image& sheet, const Atlas& atlas, Family family, const Options& options)
	{
		if(family == Family::Unknown)
			throw std::invalid_argument("no layers for an unknown atlas family");

		std::vector<Layer> layers;
		if(options.background >= 0)
		{
			Image fill(16, 16);
			for(int y = 0; y < fill.height(); ++y)
				for(int x = 0; x < fill.width(); ++x)
				{
					unsigned char* pixel = fill.at(x, y);
					pixel[0] = static_cast<unsigned char>((options.background >> 16) & 0xff);
					pixel[1] = static_cast<unsigned char>((options.background >> 8) & 0xff);
					pixel[2] = static_cast<unsigned char>(options.background & 0xff);
					pixel[3] = 255;
				}
			layers.push_back({"background", fill});
		}
		layers.push_back({"rating_filled", scale(sprite(sheet, atlas, "icon_Resume_on"), options.iconScale)});
		layers.push_back({"rating_empty", scale(sprite(sheet, atlas, "icon_Resume_off"), options.iconScale)});

		const auto frame = [&](const FrameSpec& spec, int width, int height)
		{
			const Image source = scale(sprite(sheet, atlas, spec.sprite), options.frameScale);
			const Insets insets{spec.insets.left * options.frameScale, spec.insets.top * options.frameScale, spec.insets.right * options.frameScale, spec.insets.bottom * options.frameScale};
			return ninePatch(source, insets, width, height);
		};
		layers.push_back({"list_row", frame(rowFrameSpec(family), options.rowWidth, options.rowHeight)});
		layers.push_back({"list_selector", frame(selectorFrameSpec(), options.selectorWidth, options.selectorHeight)});
		layers.push_back({"title_bar", frame(FrameSpec{"caption_title", Insets{5, 5, 5, 5}}, options.titleWidth, options.titleHeight)});
		layers.push_back({"card", cardFrame(scale(sprite(sheet, atlas, "game_card"), options.cardScale), options.cardWindow, options.cardExtraTop)});
		layers.push_back({"card_active", cardFrame(scale(sprite(sheet, atlas, "game_card_active"), options.cardScale), options.cardWindow, options.cardExtraTop)});
		layers.push_back({"menubar_top", centerCrop(scale(sprite(sheet, atlas, "menubar_U"), options.barScale), options.screenWidth)});
		Image bottomBar = sprite(sheet, atlas, "menubar_L");
		if(family == Family::A)
			clearArea(bottomBar, findBakedLogo(bottomBar, 22, 340));
		layers.push_back({"menubar_bottom", centerCrop(scale(bottomBar, options.barScale), options.screenWidth)});
		return layers;
	}
}

#endif
