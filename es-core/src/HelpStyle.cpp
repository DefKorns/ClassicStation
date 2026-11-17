#include "HelpStyle.h"

#include "resources/Font.h"

HelpStyle::HelpStyle()
{
	position = Vector2f(Renderer::getScreenWidth() * 0.012f, Renderer::getScreenHeight() * 0.9515f);
	origin = Vector2f(0.0f, 0.0f);
	iconColor = 0x777777FF;
	textColor = 0x777777FF;

	if(FONT_SIZE_SMALL != 0)
		font = Font::get(FONT_SIZE_SMALL);
	else
		font = nullptr;
}

void HelpStyle::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element)
{
	auto elem = theme->getElement(view, element, "helpsystem");
	if(!elem)
		return;

	if(elem->has("pos"))
		position = elem->get<Vector2f>("pos") * Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	if(elem->has("origin"))
		origin = elem->get<Vector2f>("origin");

	if(elem->has("textColor"))
		textColor = elem->get<unsigned int>("textColor");

	if(elem->has("iconColor"))
		iconColor = elem->get<unsigned int>("iconColor");

	if(elem->has("iconHeight"))
		iconHeight = elem->get<float>("iconHeight") * (float)Renderer::getScreenHeight();

	if(elem->has("iconTextSpacing"))
		iconTextSpacing = elem->get<float>("iconTextSpacing");

	if(elem->has("entrySpacing"))
		entrySpacing = elem->get<float>("entrySpacing");

	if(elem->has("fontPath") || elem->has("fontSize"))
		font = Font::getFromTheme(elem, ThemeFlags::ALL, font);
}
