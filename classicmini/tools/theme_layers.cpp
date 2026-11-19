/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "theme_layers.h"

#include "lodepng.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

namespace
{
	using namespace theme_layers;

	std::string readFile(const std::string& path)
	{
		std::ifstream stream(path, std::ios::binary);
		if(!stream)
			throw std::runtime_error("cannot read " + path);
		std::ostringstream content;
		content << stream.rdbuf();
		return content.str();
	}

	void writeFile(const std::string& path, const std::string& content)
	{
		std::ofstream stream(path, std::ios::binary);
		stream << content;
		if(!stream)
			throw std::runtime_error("cannot write " + path);
	}

	Image loadPng(const std::string& path)
	{
		std::vector<unsigned char> pixels;
		unsigned width = 0;
		unsigned height = 0;
		const unsigned error = lodepng::decode(pixels, width, height, path);
		if(error)
			throw std::runtime_error(path + ": " + lodepng_error_text(error));

		Image image(static_cast<int>(width), static_cast<int>(height));
		image.pixels() = std::move(pixels);
		return image;
	}

	void savePng(const std::string& path, const Image& image)
	{
		const unsigned error = lodepng::encode(path, image.pixels(), static_cast<unsigned>(image.width()), static_cast<unsigned>(image.height()));
		if(error)
			throw std::runtime_error(path + ": " + lodepng_error_text(error));
	}

	void makeDirectory(const std::string& path)
	{
		if(::mkdir(path.c_str(), 0755) != 0 && errno != EEXIST)
			throw std::runtime_error("cannot create " + path);
	}

	bool parseSize(const std::string& text, int& width, int& height)
	{
		const std::size_t separator = text.find('x');
		if(separator == std::string::npos)
			return false;
		width = std::atoi(text.substr(0, separator).c_str());
		height = std::atoi(text.substr(separator + 1).c_str());
		return width > 0 && height > 0;
	}

	int usage()
	{
		std::cerr << "usage: theme_layers info <atlas.json>\n"
		             "       theme_layers demo <characters.png> <out_dir> [--scale N]\n"
		             "       theme_layers scale <in.png> <out.png> <factor>\n"
		             "       theme_layers layers <atlas.json> <atlas.png> <out_dir> [--icon-scale N] [--frame-scale N] [--row WxH] [--wide-width N] [--title WxH] [--selector WxH] [--logo-align left] [--background RRGGBB] [--wall PNG]\n";
		return 2;
	}

	int info(const std::string& jsonPath)
	{
		const Atlas atlas = parseAtlas(readFile(jsonPath));
		std::cout << familyName(detectFamily(atlas)) << " " << atlas.size() << "\n";
		return detectFamily(atlas) == Family::Unknown ? 1 : 0;
	}

	int upscale(int argc, char* argv[])
	{
		if(argc != 5)
			return usage();
		const int factor = std::atoi(argv[4]);
		if(factor < 1)
			return usage();
		savePng(argv[3], scale(loadPng(argv[2]), factor));
		return 0;
	}

	int demo(int argc, char* argv[])
	{
		if(argc != 4 && argc != 6)
			return usage();
		int factor = 4;
		if(argc == 6)
		{
			if(std::string(argv[4]) != "--scale")
				return usage();
			factor = std::atoi(argv[5]);
		}
		if(factor < 1)
			return usage();
		const std::string outDir = argv[3];
		makeDirectory(outDir);
		for(const Layer& layer : buildDemoSprites(loadPng(argv[2]), factor))
			savePng(outDir + "/" + layer.name + ".png", layer.image);
		return 0;
	}

	int layers(int argc, char* argv[])
	{
		if(argc < 5)
			return usage();

		Options options;
		std::string consoleLogoPath;
		bool logoLeft = false;
		for(int i = 5; i < argc; ++i)
		{
			const std::string flag = argv[i];
			if(i + 1 >= argc)
				return usage();
			const std::string value = argv[++i];
			if(flag == "--icon-scale")
				options.iconScale = std::atoi(value.c_str());
			else if(flag == "--frame-scale")
				options.frameScale = std::atoi(value.c_str());
			else if(flag == "--row" && parseSize(value, options.rowWidth, options.rowHeight))
				continue;
			else if(flag == "--title" && parseSize(value, options.titleWidth, options.titleHeight))
				continue;
			else if(flag == "--wide-width")
				options.wideWidth = std::atoi(value.c_str());
			else if(flag == "--wall")
				options.wall = loadPng(value);
			else if(flag == "--logo-align")
				logoLeft = value == "left";
			else if(flag == "--console-logo")
				consoleLogoPath = value;
			else if(flag == "--background")
				options.background = static_cast<int>(std::strtol(value.c_str(), nullptr, 16));
			else if(flag == "--selector" && parseSize(value, options.selectorWidth, options.selectorHeight))
				continue;
			else
				return usage();
		}

		const Atlas atlas = parseAtlas(readFile(argv[2]));
		if(detectFamily(atlas) == Family::Unknown)
		{
			std::cerr << "not a known atlas: " << argv[2] << "\n";
			return 1;
		}

		const Image sheet = loadPng(argv[3]);
		const std::string outDir = argv[4];
		makeDirectory(outDir);
		const Family family = detectFamily(atlas);
		for(const Layer& layer : buildLayers(sheet, atlas, family, options))
			savePng(outDir + "/" + layer.name + ".png", layer.image);

		makeDirectory(outDir + "/ui");
		makeDirectory(outDir + "/ui/help");
		for(const UiIcon& icon : buildUiIcons(sheet, atlas))
			savePng(outDir + "/ui/" + icon.path, icon.image);

		const Image bar = sprite(sheet, atlas, "menubar_L");
		const Rect logo = bakedLogoArea(bar, family);
		if(logo.w > 0)
		{
			if(family == Family::A && !consoleLogoPath.empty())
				writeFile(consoleLogoPath, logoSvg(bar, logo));
			const double scaleX = options.barScale;
			const double offsetX = (bar.width() * scaleX - options.screenWidth) / 2.0;
			const double left = logo.x * scaleX - offsetX;
			const double top = 630.0 + logo.y * scaleX;
			const double width = logo.w * scaleX;
			const double height = logo.h * scaleX;
			char text[512];
			std::snprintf(text, sizeof text,
				"<theme>\n\t<formatVersion>3</formatVersion>\n\n\t<variables>\n\t\t<barLogoPos>%.6f %.6f</barLogoPos>\n\t\t<barLogoMax>%.6f %.6f</barLogoMax>\n\t</variables>\n</theme>\n",
				(logoLeft ? left : left + width / 2.0) / options.screenWidth, (top + height / 2.0) / 720.0, width / options.screenWidth, height / 720.0);
			writeFile(outDir + "/logo.xml", text);
		}

		return 0;
	}
}

int main(int argc, char* argv[])
{
	if(argc < 2)
		return usage();

	try
	{
		const std::string command = argv[1];
		if(command == "info" && argc == 3)
			return info(argv[2]);
		if(command == "demo")
			return demo(argc, argv);
		if(command == "scale")
			return upscale(argc, argv);
		if(command == "layers")
			return layers(argc, argv);
		return usage();
	}
	catch(const std::exception& error)
	{
		std::cerr << "theme_layers: " << error.what() << "\n";
		return 1;
	}
}
