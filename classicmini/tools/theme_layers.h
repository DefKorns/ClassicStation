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
#include <cmath>
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
		int ratingMargin = 3;
		int frameScale = 2;
		int rowWidth = 538;
		int wideWidth = 0;
		int rowHeight = 57;
		int selectorWidth = 546;
		int selectorHeight = 66;
		int titleWidth = 1004;
		int titleHeight = 53;
		int cardScale = 3;
		int chipWidth = 92;
		int chipHeight = 29;
		Rect cardWindow{49, 5, 148, 204};
		int cardExtraTop = 56;
		int barScale = 3;
		int labelWidth = 596;
		int detailCardWidth = 261;
		int detailCardHeight = 388;
		int dividerWidth = 305;
		int labelHeight = 30;
		int screenWidth = 1280;
		int background = -1;
		Image wall;
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

	inline Image solid(int width, int height, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
	{
		Image image(width, height);
		for(int y = 0; y < height; ++y)
			for(int x = 0; x < width; ++x)
			{
				unsigned char* pixel = image.at(x, y);
				pixel[0] = red;
				pixel[1] = green;
				pixel[2] = blue;
				pixel[3] = alpha;
			}
		return image;
	}

	inline Image pad(const Image& source, int margin)
	{
		if(margin < 0)
			throw std::invalid_argument("negative padding");

		Image result(source.width() + 2 * margin, source.height() + 2 * margin);
		for(int row = 0; row < source.height(); ++row)
			std::copy_n(source.at(0, row), static_cast<std::size_t>(source.width()) * 4, result.at(margin, row + margin));
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

	inline bool differsFromBackground(const unsigned char* pixel, const unsigned char* background, int threshold = 60)
	{
		return std::abs(pixel[0] - background[0]) + std::abs(pixel[1] - background[1]) + std::abs(pixel[2] - background[2]) > threshold;
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

	inline Rect findBakedLogo(const Image& bar, int rows, int xLimit, int xStart = 0, int backgroundX = 0, int threshold = 60)
	{
		const int limitX = std::min(xLimit, bar.width());
		const int limitY = std::min(rows, bar.height());
		int minX = limitX;
		int maxX = -1;
		int minY = limitY;
		int maxY = -1;
		for(int y = 0; y < limitY; ++y)
		{
			const unsigned char* background = bar.at(backgroundX, y);
			for(int x = xStart; x < limitX; ++x)
			{
				const unsigned char* pixel = bar.at(x, y);
				if(!differsFromBackground(pixel, background, threshold))
					continue;
				minX = std::min(minX, x);
				maxX = std::max(maxX, x);
				minY = std::min(minY, y);
				maxY = std::max(maxY, y);
			}
		}
		return maxX < 0 ? Rect{0, 0, 0, 0} : Rect{minX, minY, maxX - minX + 1, maxY - minY + 1};
	}

	inline Rect bakedLogoArea(const Image& bar, Family family)
	{
		if(family == Family::A)
			return findBakedLogo(bar, 22, 340);
		const Rect logo = findBakedLogo(bar, 21, 340, 29, 28, 150);
		return logo.h > 0 && logo.h < 21 && logo.w < 300 ? logo : Rect{0, 0, 0, 0};
	}

	inline void clearArea(Image& image, const Rect& area, int sourceX = 0)
	{
		for(int y = area.y; y < area.y + area.h; ++y)
			for(int x = area.x; x < area.x + area.w; ++x)
				std::copy_n(image.at(sourceX, y), 4, image.at(x, y));
	}

	inline Image ledOverlay(const Image& bar, float hue)
	{
		Image result(bar.width(), bar.height());
		const int first = bar.width() * 3 / 5;
		const int last = std::min(bar.height(), 24);
		bool found = false;
		for(int y = 0; y < last; ++y)
			for(int x = first; x < bar.width(); ++x)
			{
				const unsigned char* pixel = bar.at(x, y);
				const int red = pixel[0];
				const int green = pixel[1];
				const int blue = pixel[2];
				if(red < green + 25 || red < blue + 10 || pixel[3] == 0)
					continue;
				const int top = std::max(red, std::max(green, blue));
				const int bottom = std::min(red, std::min(green, blue));
				const float saturation = top == 0 ? 0.0f : static_cast<float>(top - bottom) / static_cast<float>(top);
				const float value = static_cast<float>(top) / 255.0f;
				const float chroma = value * saturation;
				const float sector = hue / 60.0f;
				const float second = chroma * (1.0f - std::fabs(std::fmod(sector, 2.0f) - 1.0f));
				float r1 = 0.0f;
				float g1 = 0.0f;
				float b1 = 0.0f;
				if(sector < 1.0f) { r1 = chroma; g1 = second; }
				else if(sector < 2.0f) { r1 = second; g1 = chroma; }
				else if(sector < 3.0f) { g1 = chroma; b1 = second; }
				else if(sector < 4.0f) { g1 = second; b1 = chroma; }
				else if(sector < 5.0f) { r1 = second; b1 = chroma; }
				else { r1 = chroma; b1 = second; }
				const float shift = value - chroma;
				unsigned char* out = result.at(x, y);
				out[0] = static_cast<unsigned char>((r1 + shift) * 255.0f + 0.5f);
				out[1] = static_cast<unsigned char>((g1 + shift) * 255.0f + 0.5f);
				out[2] = static_cast<unsigned char>((b1 + shift) * 255.0f + 0.5f);
				out[3] = 255;
				found = true;
			}
		return found ? result : Image();
	}

	inline Image cardFrame(const Image& card, const Rect& window, int extraTop, bool punch = true)
	{
		if(window.w <= 0 || window.h <= 0 || window.x < 0 || window.y < 0 || window.x + window.w > card.width() || window.y + window.h > card.height() || extraTop < 0)
			throw std::out_of_range("card window outside the card");

		Image result(card.width(), card.height() + extraTop);
		for(int y = 0; y < card.height(); ++y)
			std::copy_n(card.at(0, y), static_cast<std::size_t>(card.width()) * 4, result.at(0, y + extraTop));
		for(int y = window.y; punch && y < window.y + window.h; ++y)
			for(int x = window.x; x < window.x + window.w; ++x)
				std::fill_n(result.at(x, y + extraTop), 4, static_cast<unsigned char>(0));
		return result;
	}

	inline Image badgeShape(const Image& badge, int width, int height, int notch, const unsigned char* fixed = nullptr)
	{
		Image result(width, height);
		const unsigned char* colour = fixed != nullptr ? fixed : badge.at(badge.width() / 2, 1);
		for(int y = 0; y < height; ++y)
			for(int x = 0; x < width; ++x)
			{
				const bool cornerX = x < notch || x >= width - notch;
				const bool cornerY = y < notch || y >= height - notch;
				std::copy_n(colour, 4, result.at(x, y));
				if(cornerX && cornerY)
					result.at(x, y)[3] = 0;
			}
		return result;
	}

	inline Image mirrored(const Image& source)
	{
		Image result(source.width(), source.height());
		for(int y = 0; y < source.height(); ++y)
			for(int x = 0; x < source.width(); ++x)
				std::copy_n(source.at(x, y), 4, result.at(source.width() - 1 - x, y));
		return result;
	}

	inline int cursorCornerRadius(const Image& piece)
	{
		int top = -1;
		int left = piece.width();
		for(int y = 0; y < piece.height(); ++y)
			for(int x = 0; x < piece.width(); ++x)
				if(piece.at(x, y)[3] > 0)
				{
					if(top < 0)
						top = y;
					left = std::min(left, x);
				}
		if(top < 0)
			return 0;
		for(int x = 0; x < piece.width(); ++x)
			if(piece.at(x, top)[3] > 0)
				return x - left;
		return 0;
	}

	inline Image cursorRing(const std::vector<Image>& pieces, int width, int height)
	{
		Image result(width, height);
		const int cell = pieces[0].width();
		const auto put = [&](const Image& piece, int x, int y) {
			for(int py = 0; py < piece.height(); ++py)
				for(int px = 0; px < piece.width(); ++px)
					if(piece.at(px, py)[3] != 0)
						std::copy_n(piece.at(px, py), 4, result.at(x + px, y + py));
		};
		for(int x = cell; x < width - cell; ++x)
		{
			const int source = (x - cell) % cell;
			for(int py = 0; py < cell; ++py)
			{
				if(pieces[1].at(source, py)[3] != 0)
					std::copy_n(pieces[1].at(source, py), 4, result.at(x, py));
				if(pieces[7].at(source, py)[3] != 0)
					std::copy_n(pieces[7].at(source, py), 4, result.at(x, height - cell + py));
			}
		}
		for(int y = cell; y < height - cell; ++y)
		{
			const int source = (y - cell) % cell;
			for(int px = 0; px < cell; ++px)
			{
				if(pieces[3].at(px, source)[3] != 0)
					std::copy_n(pieces[3].at(px, source), 4, result.at(px, y));
				if(pieces[5].at(px, source)[3] != 0)
					std::copy_n(pieces[5].at(px, source), 4, result.at(width - cell + px, y));
			}
		}
		put(pieces[0], 0, 0);
		put(pieces[2], width - cell, 0);
		put(pieces[6], 0, height - cell);
		put(pieces[8], width - cell, height - cell);
		return result;
	}

	inline const unsigned char* firstOpaque(const Image& source)
	{
		for(int y = 0; y < source.height(); ++y)
			for(int x = 0; x < source.width(); ++x)
				if(source.at(x, y)[3] == 255)
					return source.at(x, y);
		throw std::runtime_error("sprite without an opaque pixel");
	}

	inline Image heartIcon(bool filled, int scale, int canvas)
	{
		static const char* const shape[] = {
			"..ooo...ooo..",
			".orrrooorrro.",
			"orrhrrrrrrrro",
			"orhhrrrrrrrdo",
			"orrrrrrrrrrdo",
			"orrrrrrrrrrdo",
			".orrrrrrrrdo.",
			"..orrrrrrdo..",
			"...orrrrdo...",
			"....orrdo....",
			".....oro.....",
			"......o......"};
		constexpr int columns = 13;
		constexpr int rows = 12;

		const auto colour = [filled](char code, unsigned char* out)
		{
			static const unsigned char full[4][3] = {{22, 14, 20}, {226, 28, 28}, {255, 214, 214}, {170, 16, 26}};
			static const unsigned char empty[4][3] = {{22, 14, 20}, {34, 34, 48}, {70, 70, 96}, {24, 24, 34}};
			const int index = code == 'o' ? 0 : code == 'r' ? 1 : code == 'h' ? 2 : 3;
			const unsigned char* source = (filled ? full : empty)[index];
			out[0] = source[0];
			out[1] = source[1];
			out[2] = source[2];
			out[3] = 255;
		};

		Image icon(canvas, canvas);
		const int left = (canvas - (columns + 1) * scale) / 2;
		const int top = (canvas - (rows + 1) * scale) / 2;
		const auto paint = [&](int column, int row, const unsigned char* rgba)
		{
			for(int dy = 0; dy < scale; ++dy)
				for(int dx = 0; dx < scale; ++dx)
					std::copy_n(rgba, 4, icon.at(left + column * scale + dx, top + row * scale + dy));
		};

		static const unsigned char shadow[4] = {0, 0, 0, 90};
		for(int row = 0; row < rows; ++row)
			for(int column = 0; column < columns; ++column)
				if(shape[row][column] != '.')
					paint(column + 1, row + 1, shadow);
		for(int row = 0; row < rows; ++row)
			for(int column = 0; column < columns; ++column)
				if(shape[row][column] != '.')
				{
					unsigned char rgba[4];
					colour(shape[row][column], rgba);
					paint(column, row, rgba);
				}
		return icon;
	}

	inline Image centered(const Image& source, int width, int height)
	{
		if(width < source.width() || height < source.height())
			throw std::invalid_argument("canvas smaller than the image");

		Image result(width, height);
		const int left = (width - source.width()) / 2;
		const int top = (height - source.height()) / 2;
		for(int row = 0; row < source.height(); ++row)
			std::copy_n(source.at(0, row), static_cast<std::size_t>(source.width()) * 4, result.at(left, top + row));
		return result;
	}

	inline Image networkIcon()
	{
		constexpr int size = 24;
		static const unsigned char black[4] = {0, 0, 0, 255};
		static const unsigned char white[4] = {255, 255, 255, 255};
		static const unsigned char blue[4] = {0x60, 0xA0, 0xFF, 255};

		Image icon(size, size);
		for(int y = 0; y < size; ++y)
			for(int x = 0; x < size; ++x)
			{
				const double px = x + 0.5;
				const double py = y + 0.5;
				const double outer = std::hypot(px - 12.0, py - 12.0);
				const double dx = px - 11.5;
				const double dy = py - 11.5;
				const double inner = std::hypot(dx, dy);
				const unsigned char* colour = nullptr;
				if(outer <= 9.4)
					colour = black;
				if(inner <= 8.3)
					colour = white;
				if(inner <= 6.8)
				{
					colour = blue;
					const double meridian = (dx * dx) / (3.4 * 3.4) + (dy * dy) / (6.8 * 6.8);
					if(std::abs(dx) < 0.75 || std::abs(dy) < 0.75 || std::abs(std::abs(dy) - 3.9) < 0.6 || std::abs(meridian - 1.0) < 0.2)
						colour = white;
				}
				if(colour)
					std::copy_n(colour, 4, icon.at(x, y));
			}
		return icon;
	}

	struct FrameSpec
	{
		const char* sprite;
		Insets insets;
	};

	inline void fillEnclosedGaps(Image& image)
	{
		const int width = image.width();
		const int height = image.height();
		std::vector<unsigned char> outside(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);
		std::vector<int> pending;
		const auto visit = [&](int x, int y)
		{
			const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
			if(outside[index] != 0 || image.at(x, y)[3] != 0)
				return;
			outside[index] = 1;
			pending.push_back(y * width + x);
		};
		for(int x = 0; x < width; ++x)
		{
			visit(x, 0);
			visit(x, height - 1);
		}
		for(int y = 0; y < height; ++y)
		{
			visit(0, y);
			visit(width - 1, y);
		}
		while(!pending.empty())
		{
			const int at = pending.back();
			pending.pop_back();
			const int x = at % width;
			const int y = at / width;
			if(x > 0)
				visit(x - 1, y);
			if(x + 1 < width)
				visit(x + 1, y);
			if(y > 0)
				visit(x, y - 1);
			if(y + 1 < height)
				visit(x, y + 1);
		}
		const unsigned char* fill = image.at(width / 2, height / 2);
		const unsigned char colour[4] = {fill[0], fill[1], fill[2], 255};
		for(int y = 0; y < height; ++y)
			for(int x = 0; x < width; ++x)
				if(image.at(x, y)[3] == 0 && outside[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] == 0)
					std::copy(colour, colour + 4, image.at(x, y));
	}

	inline FrameSpec rowFrameSpec(Family family)
	{
		return family == Family::B ? FrameSpec{"caption_title", Insets{7, 7, 7, 7}} : FrameSpec{"resume_card", Insets{6, 6, 6, 6}};
	}

	inline FrameSpec selectorFrameSpec()
	{
		return FrameSpec{"btn_init_on", Insets{6, 6, 6, 6}};
	}

	inline std::vector<std::string> requiredSprites(Family family)
	{
		return {"icon_Resume_on", "icon_Resume_off", "icon_Resume_lock", rowFrameSpec(family).sprite, selectorFrameSpec().sprite, "caption_title", "game_card", "game_card_active", "menubar_U", "menubar_L"};
	}

	inline std::vector<Layer> buildLayers(const Image& sheet, const Atlas& atlas, Family family, const Options& requested)
	{
		if(family == Family::Unknown)
			throw std::invalid_argument("no layers for an unknown atlas family");

		Options options = requested;
		if(family == Family::B)
		{
			options.cardWindow = Rect{12, 11, 228, 204};
			options.cardExtraTop = 50;
		}

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
			if(!options.wall.empty())
				layers.push_back({"wall", scale(options.wall, options.barScale)});
		}
		layers.push_back({"rating_filled", pad(scale(sprite(sheet, atlas, "icon_Resume_on"), options.iconScale), options.ratingMargin)});
		layers.push_back({"rating_empty", pad(scale(sprite(sheet, atlas, "icon_Resume_off"), options.iconScale), options.ratingMargin)});

		const auto frame = [&](const FrameSpec& spec, int width, int height)
		{
			const Image source = scale(sprite(sheet, atlas, spec.sprite), options.frameScale);
			const Insets insets{spec.insets.left * options.frameScale, spec.insets.top * options.frameScale, spec.insets.right * options.frameScale, spec.insets.bottom * options.frameScale};
			return ninePatch(source, insets, width, height);
		};
		layers.push_back({"list_row", frame(rowFrameSpec(family), options.rowWidth, options.rowHeight)});
		const auto selector = [&](int width)
		{
			Image image = frame(selectorFrameSpec(), width, options.selectorHeight);
			if(family == Family::A)
				fillEnclosedGaps(image);
			return image;
		};
		layers.push_back({"list_selector", selector(options.selectorWidth)});
		if(options.wideWidth > 0)
		{
			layers.push_back({"list_row_wide", frame(rowFrameSpec(family), options.wideWidth, options.rowHeight)});
			layers.push_back({"list_selector_wide", selector(options.wideWidth + options.selectorWidth - options.rowWidth)});
		}
		layers.push_back({"title_bar", frame(FrameSpec{"caption_title", Insets{5, 5, 5, 5}}, options.titleWidth, options.titleHeight)});
		layers.push_back({"card", cardFrame(scale(sprite(sheet, atlas, "game_card"), options.cardScale), options.cardWindow, options.cardExtraTop, false)});
		layers.push_back({"card_active", cardFrame(scale(sprite(sheet, atlas, "game_card_active"), options.cardScale), options.cardWindow, options.cardExtraTop, false)});
		layers.push_back({"menubar_top", centerCrop(scale(sprite(sheet, atlas, "menubar_U"), options.barScale), options.screenWidth)});
		Image bottomBar = sprite(sheet, atlas, "menubar_L");
		const Rect baked = bakedLogoArea(bottomBar, family);
		if(baked.w > 0)
			clearArea(bottomBar, baked, family == Family::A ? 0 : 28);
		layers.push_back({"menubar_bottom", centerCrop(scale(bottomBar, options.barScale), options.screenWidth)});
		layers.push_back({"cover_card", ninePatch(scale(sprite(sheet, atlas, "game_card"), options.cardScale), Insets{4 * options.cardScale, 4 * options.cardScale, 4 * options.cardScale, 18 * options.cardScale}, options.detailCardWidth, options.detailCardHeight)});
		layers.push_back({"label_bar", frame(FrameSpec{"caption_title", Insets{3, 3, 3, 3}}, options.labelWidth, options.labelHeight)});
		for(const char* slot : {"empty:icon_Resume_off", "saved:icon_Resume_on", "locked:icon_Resume_lock"})
		{
			const std::string spec = slot;
			const std::size_t colon = spec.find(':');
			layers.push_back({"slot_" + spec.substr(0, colon), pad(scale(sprite(sheet, atlas, spec.substr(colon + 1)), options.iconScale), options.ratingMargin)});
		}
		if(atlas.find("icon_Resume_off_active.png") != atlas.end() && atlas.find("icon_Resume_on_active.png") != atlas.end() && atlas.find("icon_Resume_lock_active.png") != atlas.end())
			for(const char* slot : {"empty:icon_Resume_off_active", "saved:icon_Resume_on_active", "locked:icon_Resume_lock_active"})
			{
				const std::string spec = slot;
				const std::size_t colon = spec.find(':');
				layers.push_back({"slot_" + spec.substr(0, colon) + "_active", pad(scale(sprite(sheet, atlas, spec.substr(colon + 1)), options.iconScale), options.ratingMargin)});
			}
		layers.push_back({"heart_filled", heartIcon(true, 2, 34)});
		layers.push_back({"heart_empty", heartIcon(false, 2, 34)});
		layers.push_back({"rating_heart_filled", heartIcon(true, 2, 30)});
		layers.push_back({"rating_heart_empty", heartIcon(false, 2, 30)});
		layers.push_back({"divider", solid(options.dividerWidth, 2, 255, 255, 255, 220)});
		const auto has = [&](const char* name) { return atlas.find(std::string(name) + ".png") != atlas.end(); };
		if(has("resumebg") && has("resumebg_tail") && has("icon_Resume") && has("icon_numer_1") && has("icon_numer_4") && has("resume_card") && has("resume_card_active") && has("resume_card_empty") && has("icon_lock"))
		{
			constexpr int panelWidth = 1284;
			constexpr int tailRise = 45;
			Image panel(panelWidth, 354 + tailRise);
			const auto place = [&panel](const Image& source, int centerX, int centerY)
			{
				const int left = centerX - source.width() / 2;
				const int top = centerY - source.height() / 2;
				for(int y = 0; y < source.height(); ++y)
					for(int x = 0; x < source.width(); ++x)
					{
						const int px = left + x;
						const int py = top + y;
						if(px < 0 || py < 0 || px >= panel.width() || py >= panel.height())
							continue;
						const unsigned char* from = source.at(x, y);
						unsigned char* to = panel.at(px, py);
						const int alpha = from[3];
						for(int channel = 0; channel < 3; ++channel)
							to[channel] = static_cast<unsigned char>((from[channel] * alpha + to[channel] * (255 - alpha)) / 255);
						to[3] = static_cast<unsigned char>(alpha + to[3] * (255 - alpha) / 255);
					}
			};
			for(int y = tailRise + 15; y < tailRise + 354 - 15; ++y)
				for(int x = 15; x < panelWidth - 15; ++x)
				{
					unsigned char* pixel = panel.at(x, y);
					pixel[0] = pixel[1] = pixel[2] = 0;
					pixel[3] = 255;
				}
			place(scale(sprite(sheet, atlas, "resumebg"), 3), panelWidth / 2, tailRise + 177);
			place(scale(sprite(sheet, atlas, "resumebg_tail"), 3), panelWidth / 2, 27);
			place(scale(sprite(sheet, atlas, "icon_Resume"), 3), 482, 78);
			static const char* const numbers[] = {"icon_numer_1", "icon_numer_2", "icon_numer_3", "icon_numer_4"};
			for(int slot = 0; slot < 4; ++slot)
				place(scale(sprite(sheet, atlas, numbers[slot]), 3), 128 + slot * 260, 151);
			layers.push_back({"suspend_panel", panel});
			layers.push_back({"suspend_cell", scale(sprite(sheet, atlas, "resume_card"), 3)});
			layers.push_back({"suspend_cell_selected", scale(sprite(sheet, atlas, "resume_card_active"), 3)});
			const Image emptyCell = scale(sprite(sheet, atlas, "resume_card_empty"), 3);
			Image emptySelected = emptyCell;
			const Image cursorSprite = sprite(sheet, atlas, "cursor_1");
			const unsigned char* ring = firstOpaque(cursorSprite);
			constexpr int ringWidth = 6;
			const int ringRadius = family == Family::B ? 15 : (has("cursor_game_1") ? cursorCornerRadius(sprite(sheet, atlas, "cursor_game_1")) : 0);
			const auto insideRounded = [&](int x, int y, int inset, int radius)
			{
				const int right = emptySelected.width() - 1 - inset;
				const int bottom = emptySelected.height() - 1 - inset;
				if(x < inset || y < inset || x > right || y > bottom)
					return false;
				const int cornerX = x < inset + radius ? inset + radius : (x > right - radius ? right - radius : x);
				const int cornerY = y < inset + radius ? inset + radius : (y > bottom - radius ? bottom - radius : y);
				const int dx = x - cornerX;
				const int dy = y - cornerY;
				return dx * dx + dy * dy <= radius * radius;
			};
			for(int y = 0; y < emptySelected.height(); ++y)
				for(int x = 0; x < emptySelected.width(); ++x)
				{
					if(insideRounded(x, y, ringWidth, std::max(0, ringRadius - ringWidth)))
						continue;
					if(insideRounded(x, y, 0, ringRadius))
						std::copy_n(ring, 4, emptySelected.at(x, y));
					else
						emptySelected.at(x, y)[3] = 0;
				}
			layers.push_back({"suspend_cell_empty", emptyCell});
			layers.push_back({"suspend_cell_empty_selected", emptySelected});
			layers.push_back({"suspend_frame", has("resume_screen_frame") ? sprite(sheet, atlas, "resume_screen_frame") : Image(160, 120)});
			layers.push_back({"suspend_lock", scale(sprite(sheet, atlas, "icon_lock"), 3)});
			if(atlas.find("icon_trash_0.png") != atlas.end() && atlas.find("icon_trash_2.png") != atlas.end())
			{
				layers.push_back({"suspend_trash", scale(sprite(sheet, atlas, "icon_trash_0"), 3)});
				layers.push_back({"suspend_trash_open", scale(sprite(sheet, atlas, "icon_trash_2"), 3)});
			}
		}
		if(atlas.find("icon_1P.png") != atlas.end() && atlas.find("icon_1P-2P.png") != atlas.end())
		{
			layers.push_back({"players_one", scale(sprite(sheet, atlas, "icon_1P"), 3)});
			layers.push_back({"players_many", scale(sprite(sheet, atlas, "icon_1P-2P"), 3)});
			const bool activeSprites = atlas.find("icon_1P_active.png") != atlas.end() && atlas.find("icon_1P-2P_active.png") != atlas.end();
			layers.push_back({"players_one_active", scale(sprite(sheet, atlas, activeSprites ? "icon_1P_active" : "icon_1P"), 3)});
			layers.push_back({"players_many_active", scale(sprite(sheet, atlas, activeSprites ? "icon_1P-2P_active" : "icon_1P-2P"), 3)});
		}
		if(has("Icon_Save"))
			layers.push_back({"save_icon", scale(sprite(sheet, atlas, "Icon_Save"), 3)});
		if(atlas.find("icon_1P.png") != atlas.end())
		{
			static const unsigned char inactiveChip[4] = {0x80, 0x80, 0x80, 0xff};
			static const unsigned char activeChipColour[4] = {0x3f, 0xbf, 0xff, 0xff};
			const char* activeChip = atlas.find("icon_1P_active.png") != atlas.end() ? "icon_1P_active" : "icon_1P";
			layers.push_back({"system_chip", badgeShape(sprite(sheet, atlas, "icon_1P"), options.chipWidth, options.chipHeight, 3, family == Family::B ? inactiveChip : nullptr)});
			layers.push_back({"system_chip_active", badgeShape(sprite(sheet, atlas, activeChip), options.chipWidth, options.chipHeight, 3, family == Family::B ? activeChipColour : nullptr)});
		}
		{
			const struct { const char* layer; const char* sprite; int factor; } floating[] = {
				{"floating_card", "resume_card_float_home", 3}, {"floating_card_active", "resume_card_active_home", 3},
				{"floating_frame", "resume_screen_frame_home", 1}, {"floating_wing_a", "resume_wingM_1", 3}, {"floating_wing_b", "resume_wingM_2", 3},
				{"floating_arrow_a", "icon_resume_direction_down", 3}, {"floating_arrow_b", "icon_resume_direction_down_2", 3},
				{"floating_card_small", "resume_card_float_mini", 3}, {"floating_frame_small", "resume_screen_frame_mini", 1},
				{"floating_icon_back", "icon_resume_btn_B", 3}, {"floating_wing_small_a", "resume_wingS_1", 3}, {"floating_wing_small_b", "resume_wingS_2", 3},
				{"floating_large_card", "resume_card_float", 3}, {"floating_large_active", "resume_card_active", 3}, {"floating_large_frame", "resume_screen_frame", 1},
				{"floating_large_wing_a", "resume_wingL_1", 3}, {"floating_large_wing_b", "resume_wingL_2", 3},
				{"floating_balloon", "registration_frame", 3}, {"floating_balloon_icon", family == Family::B ? "icon_Y_light" : "registration_icon", 3},
				{"floating_arrow_left", "arrow_left", 3}, {"floating_arrow_right", "arrow_right", 3},
				{"thumbnail_arrow_a", "curor_thumbnail_1", 3}, {"thumbnail_arrow_b", "curor_thumbnail_2", 3}, {"thumbnail_arrow_c", "curor_thumbnail_3", 3},
				{"warning_popup", "popup_warning", 3}, {"warning_icon", family == Family::B ? "icon_exclamation" : "icon_casette_nes", 3},
				{"floating_smoke_1", "smoke_64_1", 3}, {"floating_smoke_2", "smoke_64_2", 3}, {"floating_smoke_3", "smoke_64_3", 3}, {"floating_smoke_4", "smoke_64_4", 3}, {"floating_smoke_5", "smoke_64_5", 3},
				{"floating_smoke_small_1", "smoke_32_1", 3}, {"floating_smoke_small_2", "smoke_32_2", 3}, {"floating_smoke_small_3", "smoke_32_3", 3}, {"floating_smoke_small_4", "smoke_32_4", 3}, {"floating_smoke_small_5", "smoke_32_5", 3}
			};
			struct Blank { const char* layer; int width; int height; };
			const Blank blanks[] = {
				{"floating_frame", 120, 90}, {"floating_frame_small", 80, 60}, {"floating_large_frame", 160, 120}
			};
			const auto blankFor = [&](const char* layer) -> const Blank* {
				if(family != Family::B)
					return nullptr;
				for(const auto& blank : blanks)
					if(std::string(blank.layer) == layer)
						return &blank;
				return nullptr;
			};
			bool complete = true;
			for(const auto& entry : floating)
				complete = complete && (blankFor(entry.layer) != nullptr || atlas.find(std::string(entry.sprite) + ".png") != atlas.end());
			for(int index = 1; index <= 9; ++index)
				complete = complete && atlas.find("cursor_game_" + std::to_string(index) + ".png") != atlas.end();
			if(complete)
			{
				std::vector<Image> pieces;
				for(int index = 1; index <= 9; ++index)
					pieces.push_back(sprite(sheet, atlas, "cursor_game_" + std::to_string(index)));
				layers.push_back({"cursor_ring", cursorRing(pieces, 224, 200)});
			}
			if(complete)
				for(const auto& entry : floating)
				{
					const auto* blank = blankFor(entry.layer);
				Image image = blank != nullptr ? Image(blank->width, blank->height) : scale(sprite(sheet, atlas, entry.sprite), entry.factor);
				if(std::string(entry.layer).compare(0, 15, "thumbnail_arrow") == 0)
				{
					Image upright(image.width(), image.height());
					for(int y = 0; y < image.height(); ++y)
						std::copy_n(image.at(0, y), static_cast<std::size_t>(image.width()) * 4, upright.at(0, image.height() - 1 - y));
					image = upright;
				}
					layers.push_back({entry.layer, image});
					if(std::string(entry.layer).find("wing") != std::string::npos)
					{
						std::string left = entry.layer;
						left.insert(left.size() - 2, "_left");
						layers.push_back({left, mirrored(image)});
					}
				}
		}
		layers.push_back({"system_card", scale(sprite(sheet, atlas, "game_card"), options.cardScale)});
		layers.push_back({"system_card_active", scale(sprite(sheet, atlas, "game_card_active"), options.cardScale)});
		{
			const Image led = ledOverlay(sprite(sheet, atlas, "menubar_L"), 38.0f);
			if(!led.empty())
				layers.push_back({"led_overclock", centerCrop(scale(led, options.barScale), options.screenWidth)});
		}
		return layers;
	}

	struct UiIcon
	{
		std::string path;
		Image image;
	};

	inline void blendOver(Image& target, const Image& source, int left, int top)
	{
		for(int y = 0; y < source.height(); ++y)
			for(int x = 0; x < source.width(); ++x)
			{
				const int px = left + x;
				const int py = top + y;
				if(px < 0 || py < 0 || px >= target.width() || py >= target.height())
					continue;
				const unsigned char* from = source.at(x, y);
				unsigned char* to = target.at(px, py);
				const int alpha = from[3];
				for(int channel = 0; channel < 3; ++channel)
					to[channel] = static_cast<unsigned char>((from[channel] * alpha + to[channel] * (255 - alpha)) / 255);
				to[3] = static_cast<unsigned char>(alpha + to[3] * (255 - alpha) / 255);
			}
	}

	inline Image stretched(const Image& source, int width, int height)
	{
		Image result(width, height);
		for(int y = 0; y < height; ++y)
			for(int x = 0; x < width; ++x)
				std::copy_n(source.at(x * source.width() / width, y * source.height() / height), 4, result.at(x, y));
		return result;
	}

	inline Image squareCursor(const Image& panel, const Image& topLeft, const Image& topRight, const Image& bottomLeft, const Image& bottomRight, int width, int height)
	{
		const int piece = topLeft.width();
		const int half = piece / 2;
		Image result(width + piece, height + piece);
		blendOver(result, stretched(panel, width, height), half, half);

		Image top(result.width() - 2 * piece, piece);
		Image bottom(top.width(), piece);
		Image left(piece, result.height() - 2 * piece);
		Image right(piece, left.height());
		for(int y = 0; y < piece; ++y)
		{
			for(int x = 0; x < top.width(); ++x)
			{
				std::copy_n(topLeft.at(piece - 1, y), 4, top.at(x, y));
				std::copy_n(bottomLeft.at(piece - 1, y), 4, bottom.at(x, y));
			}
		}
		for(int x = 0; x < piece; ++x)
		{
			for(int y = 0; y < left.height(); ++y)
			{
				std::copy_n(bottomLeft.at(x, 0), 4, left.at(x, y));
				std::copy_n(bottomRight.at(x, 0), 4, right.at(x, y));
			}
		}
		blendOver(result, top, piece, 0);
		blendOver(result, bottom, piece, result.height() - piece);
		blendOver(result, left, 0, piece);
		blendOver(result, right, result.width() - piece, piece);
		blendOver(result, topLeft, 0, 0);
		blendOver(result, topRight, result.width() - piece, 0);
		blendOver(result, bottomLeft, 0, result.height() - piece);
		blendOver(result, bottomRight, result.width() - piece, result.height() - piece);
		return result;
	}

	inline Image menuSelected(int width, int height, int border, const unsigned char* ring, const unsigned char* fill)
	{
		Image result(width, height);
		for(int y = 0; y < height; ++y)
			for(int x = 0; x < width; ++x)
			{
				const bool onBorder = x < border || y < border || x >= width - border || y >= height - border;
				std::copy_n(onBorder ? ring : fill, 4, result.at(x, y));
			}
		return result;
	}

	inline std::vector<UiIcon> buildUiIcons(const Image& sheet, const Atlas& atlas)
	{
		std::vector<UiIcon> icons;
		const auto has = [&](const std::string& name) { return atlas.find(name + ".png") != atlas.end(); };
		const auto add = [&](const std::string& path, const std::string& name, int factor, int minWidth, int minHeight)
		{
			if(!has(name))
				return;
			const Image source = scale(sprite(sheet, atlas, name), factor);
			icons.push_back({path, centered(source, std::max(minWidth, source.width()), std::max(minHeight, source.height()))});
		};
		if(has("caption") && has("caption_edge") && has("caption_arrow"))
		{
			const Image body = scale(sprite(sheet, atlas, "caption"), 3);
			const Image edge = scale(sprite(sheet, atlas, "caption_edge"), 3);
			Image caption(body.width() + 2 * edge.width(), body.height());
			blendOver(caption, mirrored(edge), 0, 0);
			blendOver(caption, body, edge.width(), 0);
			blendOver(caption, edge, edge.width() + body.width(), 0);
			icons.push_back({"menu_caption.png", caption});
			icons.push_back({"menu_caption_arrow.png", scale(sprite(sheet, atlas, "caption_arrow"), 3)});
		}
		add("menu_display.png", "btn_display", 3, 0, 0);
		add("menu_system.png", "btn_option", 3, 0, 0);
		icons.push_back({"menu_network.png", scale(networkIcon(), 3)});
		add("menu_library.png", "btn_copyright", 3, 0, 0);
		add("menu_help.png", "manual", 3, 0, 0);
		add("help/button_a.png", "icon_A_light", 3, 30, 30);
		add("help/button_b.png", "icon_B_light", 3, 30, 30);
		add("help/button_x.png", "icon_X_light", 3, 30, 30);
		add("help/button_y.png", "icon_Y_light", 3, 30, 30);
		add("help/button_start.png", "icon_start_light", 3, 30, 30);
		add("help/button_select.png", "icon_select_light", 3, 30, 30);
		add("help/dpad_all.png", "Icon_direction_light", 3, 30, 30);
		add("help/dpad_updown.png", "Icon_direction_Lengthwise_light", 3, 30, 30);
		add("help/dpad_leftright.png", "icon_direction_lateral_light", 3, 30, 30);
		add("help/menu_plus.png", "icon_direction_up_light", 3, 30, 30);
		add("help/plus.png", "icon_direction_down_light", 3, 30, 30);
		if(has("menubar_U_2") && has("btn_menubar_active") && has("cursor_1"))
		{
			const Image band = sprite(sheet, atlas, "menubar_U_2");
			Image strip(480, 120);
			for(int y = 0; y < strip.height(); ++y)
				for(int x = 0; x < strip.width(); ++x)
					std::copy_n(band.at(0, y / 3), 4, strip.at(x, y));
			icons.push_back({"menu_strip.png", strip});
			const Image active = sprite(sheet, atlas, "btn_menubar_active");
			if(has("cursor_3") && has("cursor_7") && has("cursor_9"))
				icons.push_back({"menu_selected.png", squareCursor(active, sprite(sheet, atlas, "cursor_1"), sprite(sheet, atlas, "cursor_3"), sprite(sheet, atlas, "cursor_7"), sprite(sheet, atlas, "cursor_9"), 115, 84)});
			else
				icons.push_back({"menu_selected.png", menuSelected(108, 82, 6, firstOpaque(sprite(sheet, atlas, "cursor_1")), firstOpaque(active))});
		}
		return icons;
	}
}

#endif
