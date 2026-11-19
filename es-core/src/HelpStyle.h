#pragma once
#ifndef ES_CORE_HELP_STYLE_H
#define ES_CORE_HELP_STYLE_H

#include "math/Vector2f.h"
#include <memory>
#include <string>

class Font;
class ThemeData;

struct HelpStyle
{
	Vector2f position;
	Vector2f origin;
	unsigned int iconColor;
	unsigned int textColor;
	std::shared_ptr<Font> font;
	float iconHeight = 0.0f;
	float iconTextSpacing = 8.0f;
	float entrySpacing = 16.0f;

	HelpStyle(); // default values
	void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element = "help");
};

#endif // ES_CORE_HELP_STYLE_H
