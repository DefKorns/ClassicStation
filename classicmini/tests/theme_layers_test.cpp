/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "theme_layers.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
	using namespace theme_layers;

	int failures = 0;

	void check(bool condition, const char* what)
	{
		if(!condition)
		{
			std::cerr << "FAIL: " << what << "\n";
			++failures;
		}
	}

	Image gradient(int width, int height)
	{
		Image image(width, height);
		for(int y = 0; y < height; ++y)
			for(int x = 0; x < width; ++x)
			{
				unsigned char* pixel = image.at(x, y);
				pixel[0] = static_cast<unsigned char>(x);
				pixel[1] = static_cast<unsigned char>(y);
				pixel[2] = 7;
				pixel[3] = 255;
			}
		return image;
	}

	void parsesAtlasFrames()
	{
		const std::string json = "{\"texture\":{\"id\":\"x\",\"width\":512,\"height\":1024},\"sprites\":{"
		                         "\"a.png\":{\"textureFrame\":[1,2,3,4],\"pivot\":[0,0]},"
		                         "\"Icon_B.png\":{\"textureFrame\":[501,187,10,10],\"pivot\":[5,5]}}}";
		const Atlas atlas = parseAtlas(json);
		check(atlas.size() == 2, "two sprites parsed");
		check(atlas.at("a.png").x == 1 && atlas.at("a.png").h == 4, "first frame values");
		check(atlas.at("Icon_B.png").x == 501 && atlas.at("Icon_B.png").y == 187, "second frame values");
	}

	void rejectsBrokenAtlas()
	{
		bool threw = false;
		try
		{
			parseAtlas("{\"sprites\":{\"a.png\":{\"textureFrame\":[1,x]}}}");
		}
		catch(const std::exception&)
		{
			threw = true;
		}
		check(threw, "a frame without numbers is rejected");
	}

	void detectsFamilies()
	{
		Atlas a;
		a["arrow_down.png"] = Rect{0, 0, 1, 1};
		a["resume_screen_frame.png"] = Rect{0, 0, 1, 1};
		check(detectFamily(a) == Family::A, "family A by its sprites");

		Atlas b;
		b["icon_X_light.png"] = Rect{0, 0, 1, 1};
		b["Icon_Save.png"] = Rect{0, 0, 1, 1};
		check(detectFamily(b) == Family::B, "family B by its sprites");

		check(detectFamily(Atlas{}) == Family::Unknown, "an empty atlas is unknown");
	}

	void cropsAndScales()
	{
		const Image source = gradient(8, 8);
		const Image part = crop(source, Rect{2, 3, 4, 2});
		check(part.width() == 4 && part.height() == 2, "crop size");
		check(part.at(0, 0)[0] == 2 && part.at(0, 0)[1] == 3, "crop origin");
		check(part.at(3, 1)[0] == 5 && part.at(3, 1)[1] == 4, "crop far corner");

		bool threw = false;
		try
		{
			crop(source, Rect{6, 6, 4, 4});
		}
		catch(const std::out_of_range&)
		{
			threw = true;
		}
		check(threw, "crop outside the image throws");

		const Image big = scale(part, 3);
		check(big.width() == 12 && big.height() == 6, "scale size");
		check(big.at(5, 4)[0] == 3 && big.at(5, 4)[1] == 4, "scale is nearest neighbour");
	}

	void ninePatchKeepsCorners()
	{
		const Image source = gradient(10, 10);
		const Image result = ninePatch(source, Insets{2, 3, 4, 1}, 40, 30);
		check(result.width() == 40 && result.height() == 30, "nine-patch size");
		check(result.at(0, 0)[0] == 0 && result.at(0, 0)[1] == 0, "top left corner");
		check(result.at(39, 29)[0] == 9 && result.at(39, 29)[1] == 9, "bottom right corner");
		check(result.at(1, 2)[0] == 1 && result.at(1, 2)[1] == 2, "left and top borders are 1:1");
		check(result.at(37, 28)[0] == 7 && result.at(37, 28)[1] == 8, "right and bottom borders are 1:1");
		check(result.at(20, 15)[0] >= 2 && result.at(20, 15)[0] <= 5, "centre samples the stretched middle");

		bool threw = false;
		try
		{
			ninePatch(source, Insets{5, 5, 5, 5}, 40, 30);
		}
		catch(const std::invalid_argument&)
		{
			threw = true;
		}
		check(threw, "insets without a centre throw");
	}

	void centerCropsToExactWidth()
	{
		const Image source = gradient(20, 4);
		const Image cropped = centerCrop(source, 12);
		check(cropped.width() == 12 && cropped.height() == 4, "centre crop size");
		check(cropped.at(0, 0)[0] == 4 && cropped.at(11, 0)[0] == 15, "centre crop keeps the middle columns");
		check(centerCrop(source, 40).width() == 20, "a narrower image is left alone");
	}

	unsigned be16(const std::vector<unsigned char>& data, std::size_t at)
	{
		return (static_cast<unsigned>(data[at]) << 8) | data[at + 1];
	}

	unsigned long be32(const std::vector<unsigned char>& data, std::size_t at)
	{
		return (static_cast<unsigned long>(be16(data, at)) << 16) | be16(data, at + 2);
	}

	std::size_t tableAt(const std::vector<unsigned char>& font, const char* tag)
	{
		const unsigned count = be16(font, 4);
		for(unsigned i = 0; i < count; ++i)
			if(std::string(font.begin() + 12 + i * 16, font.begin() + 16 + i * 16) == tag)
				return be32(font, 12 + i * 16 + 8);
		return 0;
	}

	void buildsAValidGlyphFont()
	{
		Image block(3, 2);
		for(int y = 0; y < 2; ++y)
			for(int x = 0; x < 3; ++x)
			{
				unsigned char* pixel = block.at(x, y);
				const bool hole = x == 1 && y == 0;
				pixel[0] = hole ? 255 : 117;
				pixel[1] = hole ? 255 : 117;
				pixel[2] = hole ? 255 : 117;
				pixel[3] = 255;
			}

		const std::vector<unsigned char> font = buildPlayersFont(block, block);
		check(be32(font, 0) == 0x00010000, "font is a TrueType file");
		check(be16(font, 4) == 8, "eight tables");
		check(tableAt(font, "head") != 0 && be32(font, tableAt(font, "head") + 12) == 0x5F0F3CF5, "head magic number");
		check(be16(font, tableAt(font, "head") + 18) == 1100, "units per em");
		check(be16(font, tableAt(font, "maxp") + 4) == 3, "notdef plus two glyphs");

		const std::size_t cmap = tableAt(font, "cmap");
		check(be16(font, cmap + 12) == 4, "cmap format 4");
		const unsigned segments = be16(font, cmap + 18) / 2;
		check(segments == 3, "two characters and the terminator");
		check(be16(font, cmap + 26) == '1' && be16(font, cmap + 28) == '2' && be16(font, cmap + 30) == 0xFFFF, "end codes");

		const std::size_t glyf = tableAt(font, "glyf");
		const std::size_t loca = tableAt(font, "loca");
		check(be32(font, loca) == 0 && be32(font, loca + 4) == 0, "notdef is empty");
		check(be16(font, glyf + be32(font, loca + 4)) == 3, "block with a hole is three runs");
		unsigned long sum = 0;
		for(std::size_t i = 0; i < font.size(); i += 4)
			sum = (sum + be32(font, i)) & 0xffffffffUL;
		check(sum == 0xB1B0AFBAUL, "whole file checksum");
	}

	void findsAndClearsBakedLogo()
	{
		Image bar(100, 30);
		for(int y = 0; y < bar.height(); ++y)
			for(int x = 0; x < bar.width(); ++x)
			{
				unsigned char* pixel = bar.at(x, y);
				const bool logo = x >= 20 && x < 50 && y >= 5 && y < 13;
				const bool slot = x >= 80 && y < 10;
				pixel[0] = logo ? 200 : (slot ? 0 : 180);
				pixel[1] = logo ? 30 : (slot ? 0 : 180);
				pixel[2] = logo ? 20 : (slot ? 0 : 180);
				pixel[3] = 255;
			}

		const Rect found = findBakedLogo(bar, 22, 60);
		check(found.x == 20 && found.y == 5 && found.w == 30 && found.h == 8, "logo box found without the slot");
		check(findBakedLogo(bar, 22, 10).w == 0, "no logo inside an empty area");

		const std::string svg = logoSvg(bar, found);
		check(svg.find("viewBox=\"0 0 30 8\"") != std::string::npos, "logo svg has the box size");
		size_t rects = 0;
		for(size_t at = svg.find("<rect"); at != std::string::npos; at = svg.find("<rect", at + 1))
			++rects;
		check(rects == 8, "a solid block is one run per row");

		clearArea(bar, found);
		check(bar.at(30, 8)[0] == 180 && bar.at(30, 8)[1] == 180, "logo pixels take the background colour");
		check(bar.at(85, 5)[0] == 0, "the slot is left alone");
	}

	void buildsLayersForBothFamilies()
	{
		const Image sheet = gradient(250, 210);
		Atlas atlas;
		atlas["icon_Resume_on.png"] = Rect{0, 0, 6, 6};
		atlas["icon_Resume_off.png"] = Rect{6, 0, 6, 6};
		atlas["icon_Resume_lock.png"] = Rect{12, 0, 6, 6};
		atlas["resume_card.png"] = Rect{0, 10, 64, 56};
		atlas["caption_title.png"] = Rect{70, 10, 100, 22};
		atlas["btn_init_on.png"] = Rect{0, 70, 152, 32};
		atlas["game_card.png"] = Rect{0, 110, 82, 90};
		atlas["game_card_active.png"] = Rect{90, 110, 82, 90};
		atlas["menubar_U.png"] = Rect{0, 0, 120, 40};
		atlas["menubar_L.png"] = Rect{0, 40, 120, 40};

		for(const Family family : {Family::A, Family::B})
		{
			const std::vector<Layer> layers = buildLayers(sheet, atlas, family, Options{});
			check(layers.size() == 19, "nineteen layers without the player badges");
			check(layers[0].name == "rating_filled" && layers[0].image.width() == 24 && layers[0].image.height() == 24, "rating icon scaled by 3 with a margin");
			check(layers[2].name == "list_row" && layers[2].image.width() == 538 && layers[2].image.height() == 57, "row frame size");
			check(layers[3].name == "list_selector" && layers[3].image.width() == 546 && layers[3].image.height() == 66, "selector frame size");
			check(layers[4].name == "title_bar" && layers[4].image.width() == 1004 && layers[4].image.height() == 53, "title bar size");
			check(layers[5].name == "card" && layers[5].image.width() == 246 && layers[5].image.height() == 326, "card scaled by 3 with the extra top");
			check(layers[7].name == "menubar_top" && layers[7].image.width() == 360 && layers[7].image.height() == 120, "menu bar scaled by 3");
			check(layers[5].image.at(60, 80)[3] == 0 && layers[5].image.at(5, 100)[3] != 0, "card window is transparent and its border is not");
			check(layers[9].name == "cover_card" && layers[9].image.width() == 261 && layers[9].image.height() == 388, "cover card is a nine-patch of the detail size");
			check(layers[10].name == "label_bar" && layers[10].image.width() == 596 && layers[10].image.height() == 30, "label bar size");
			check(layers[11].name == "slot_empty" && layers[12].name == "slot_saved" && layers[13].name == "slot_locked" && layers[13].image.width() == 24, "three slot icons with the rating margin");
			check(layers[14].name == "heart_filled" && layers[15].name == "heart_empty" && layers[14].image.width() == 34 && layers[14].image.height() == 34, "two hearts on a 34 pixel canvas");
			check(layers[14].image.at(17, 14)[0] > 200 && layers[14].image.at(17, 14)[1] < 60 && layers[15].image.at(17, 14)[0] < 60, "the filled heart is red and the empty one is dark");
			check(layers[14].image.at(0, 0)[3] == 0, "the canvas corner stays transparent");
			check(layers[16].name == "rating_heart_filled" && layers[16].image.width() == 30 && layers[17].name == "rating_heart_empty" && layers[18].name == "divider" && layers[18].image.width() == 305 && layers[18].image.height() == 2, "rating hearts at 30 pixels and the divider");
			const std::vector<UiIcon> icons = buildUiIcons(sheet, atlas);
			check(icons.size() == 1 && icons[0].path == "menu_network.png" && icons[0].image.width() == 72, "only the generated network icon without the button sprites");
			check(icons[0].image.at(0, 0)[3] == 0 && icons[0].image.at(36, 36)[3] == 255, "the network icon is round with transparent corners");
		}

		check(std::string(rowFrameSpec(Family::A).sprite) == "resume_card", "family A row frame");
		check(std::string(rowFrameSpec(Family::B).sprite) == "caption_title", "family B row frame");

		bool threw = false;
		try
		{
			buildLayers(sheet, atlas, Family::Unknown, Options{});
		}
		catch(const std::invalid_argument&)
		{
			threw = true;
		}
		check(threw, "unknown family throws");
	}

	std::string readFile(const std::string& path)
	{
		std::ifstream stream(path, std::ios::binary);
		std::ostringstream content;
		content << stream.rdbuf();
		return content.str();
	}

	void checksRealAtlas(const std::string& directory, const char* file, Family family, std::size_t count)
	{
		const std::string path = directory + "/" + file;
		const std::string json = readFile(path);
		if(json.empty())
		{
			std::cerr << "skipped (missing): " << path << "\n";
			return;
		}
		const Atlas atlas = parseAtlas(json);
		check(atlas.size() == count, (std::string("sprite count of ") + file).c_str());
		check(detectFamily(atlas) == family, (std::string("family of ") + file).c_str());
		for(const std::string& name : requiredSprites(family))
			check(atlas.count(name + ".png") == 1, (name + " in " + file).c_str());
	}
}

int main()
{
	parsesAtlasFrames();
	rejectsBrokenAtlas();
	detectsFamilies();
	cropsAndScales();
	ninePatchKeepsCorners();
	centerCropsToExactWidth();
	buildsAValidGlyphFont();
	findsAndClearsBakedLogo();
	buildsLayersForBothFamilies();

	if(const char* samples = std::getenv("THEME_ATLAS_SAMPLES"))
	{
		checksRealAtlas(samples, "nes.json", Family::A, 170);
		checksRealAtlas(samples, "hvc.json", Family::A, 170);
		checksRealAtlas(samples, "hvcj_packed.json", Family::A, 170);
		checksRealAtlas(samples, "shvc_packed.json", Family::B, 166);
		checksRealAtlas(samples, "snes_eur_packed.json", Family::B, 166);
		checksRealAtlas(samples, "snes_usa_packed.json", Family::B, 166);
	}

	if(failures == 0)
		std::cout << "theme_layers_test: ok\n";
	return failures == 0 ? 0 : 1;
}
