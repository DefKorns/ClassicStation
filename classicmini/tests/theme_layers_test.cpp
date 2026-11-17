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
		for(const char* name : {"icon_Resume_on.png", "icon_Resume_off.png", "resume_card.png", "btn_init_on.png"})
			check(atlas.count(name) == 1, (std::string(name) + " in " + file).c_str());
	}
}

int main()
{
	parsesAtlasFrames();
	rejectsBrokenAtlas();
	detectsFamilies();
	cropsAndScales();
	ninePatchKeepsCorners();

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
