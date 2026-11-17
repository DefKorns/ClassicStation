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
		return {"icon_Resume_on", "icon_Resume_off", "icon_Resume_lock", rowFrameSpec(family).sprite, selectorFrameSpec().sprite, "caption_title", "game_card", "game_card_active", "menubar_U", "menubar_L"};
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
		layers.push_back({"rating_filled", pad(scale(sprite(sheet, atlas, "icon_Resume_on"), options.iconScale), options.ratingMargin)});
		layers.push_back({"rating_empty", pad(scale(sprite(sheet, atlas, "icon_Resume_off"), options.iconScale), options.ratingMargin)});

		const auto frame = [&](const FrameSpec& spec, int width, int height)
		{
			const Image source = scale(sprite(sheet, atlas, spec.sprite), options.frameScale);
			const Insets insets{spec.insets.left * options.frameScale, spec.insets.top * options.frameScale, spec.insets.right * options.frameScale, spec.insets.bottom * options.frameScale};
			return ninePatch(source, insets, width, height);
		};
		layers.push_back({"list_row", frame(rowFrameSpec(family), options.rowWidth, options.rowHeight)});
		layers.push_back({"list_selector", frame(selectorFrameSpec(), options.selectorWidth, options.selectorHeight)});
		if(options.wideWidth > 0)
		{
			layers.push_back({"list_row_wide", frame(rowFrameSpec(family), options.wideWidth, options.rowHeight)});
			layers.push_back({"list_selector_wide", frame(selectorFrameSpec(), options.wideWidth + options.selectorWidth - options.rowWidth, options.selectorHeight)});
		}
		layers.push_back({"title_bar", frame(FrameSpec{"caption_title", Insets{5, 5, 5, 5}}, options.titleWidth, options.titleHeight)});
		layers.push_back({"card", cardFrame(scale(sprite(sheet, atlas, "game_card"), options.cardScale), options.cardWindow, options.cardExtraTop)});
		layers.push_back({"card_active", cardFrame(scale(sprite(sheet, atlas, "game_card_active"), options.cardScale), options.cardWindow, options.cardExtraTop)});
		layers.push_back({"menubar_top", centerCrop(scale(sprite(sheet, atlas, "menubar_U"), options.barScale), options.screenWidth)});
		Image bottomBar = sprite(sheet, atlas, "menubar_L");
		if(family == Family::A)
			clearArea(bottomBar, findBakedLogo(bottomBar, 22, 340));
		layers.push_back({"menubar_bottom", centerCrop(scale(bottomBar, options.barScale), options.screenWidth)});
		layers.push_back({"cover_card", ninePatch(scale(sprite(sheet, atlas, "game_card"), options.cardScale), Insets{4 * options.cardScale, 4 * options.cardScale, 4 * options.cardScale, 18 * options.cardScale}, options.detailCardWidth, options.detailCardHeight)});
		layers.push_back({"label_bar", frame(FrameSpec{"caption_title", Insets{3, 3, 3, 3}}, options.labelWidth, options.labelHeight)});
		for(const char* slot : {"empty:icon_Resume_off", "saved:icon_Resume_on", "locked:icon_Resume_lock"})
		{
			const std::string spec = slot;
			const std::size_t colon = spec.find(':');
			layers.push_back({"slot_" + spec.substr(0, colon), pad(scale(sprite(sheet, atlas, spec.substr(colon + 1)), options.iconScale), options.ratingMargin)});
		}
		layers.push_back({"heart_filled", heartIcon(true, 2, 34)});
		layers.push_back({"heart_empty", heartIcon(false, 2, 34)});
		layers.push_back({"rating_heart_filled", heartIcon(true, 2, 30)});
		layers.push_back({"rating_heart_empty", heartIcon(false, 2, 30)});
		layers.push_back({"divider", solid(options.dividerWidth, 2, 255, 255, 255, 220)});
		const auto has = [&](const char* name) { return atlas.find(std::string(name) + ".png") != atlas.end(); };
		if(has("resumebg") && has("resumebg_tail") && has("icon_Resume") && has("icon_numer_1") && has("icon_numer_4") && has("resume_card") && has("resume_card_active") && has("resume_card_empty") && has("resume_screen_frame") && has("icon_lock"))
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
			for(int y = 0; y < emptySelected.height(); ++y)
				for(int x = 0; x < emptySelected.width(); ++x)
					if(x < 6 || y < 6 || x >= emptySelected.width() - 6 || y >= emptySelected.height() - 6)
						std::copy_n(ring, 4, emptySelected.at(x, y));
			layers.push_back({"suspend_cell_empty", emptyCell});
			layers.push_back({"suspend_cell_empty_selected", emptySelected});
			layers.push_back({"suspend_frame", sprite(sheet, atlas, "resume_screen_frame")});
			layers.push_back({"suspend_lock", scale(sprite(sheet, atlas, "icon_lock"), 3)});
		}
		if(atlas.find("icon_1P.png") != atlas.end() && atlas.find("icon_1P-2P.png") != atlas.end())
		{
			layers.push_back({"players_one", scale(sprite(sheet, atlas, "icon_1P"), 3)});
			layers.push_back({"players_many", scale(sprite(sheet, atlas, "icon_1P-2P"), 3)});
		}
		return layers;
	}

	struct UiIcon
	{
		std::string path;
		Image image;
	};

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
		add("menu_display.png", "btn_display", 3, 0, 0);
		add("menu_system.png", "btn_option", 3, 0, 0);
		icons.push_back({"menu_network.png", scale(networkIcon(), 3)});
		add("menu_library.png", "btn_copyright", 3, 0, 0);
		add("menu_help.png", "manual", 3, 0, 0);
		add("help/button_a.png", "icon_A_light", 3, 30, 30);
		add("help/button_b.png", "icon_B_light", 3, 30, 30);
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
			const Image cursor = sprite(sheet, atlas, "cursor_1");
			icons.push_back({"menu_selected.png", menuSelected(108, 82, 6, firstOpaque(cursor), firstOpaque(active))});
		}
		return icons;
	}
}

#endif
