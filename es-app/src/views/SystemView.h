#pragma once
#ifndef ES_APP_VIEWS_SYSTEM_VIEW_H
#define ES_APP_VIEWS_SYSTEM_VIEW_H

#include "components/HeaderStatus.h"
#include "components/IList.h"
#include "components/ImageComponent.h"
#include "components/MenuIconBar.h"
#include "components/TextComponent.h"
#include "resources/Font.h"
#include "GuiComponent.h"
#include "Sound.h"
#include <memory>

class AnimatedImageComponent;
class SystemData;

enum CarouselType : unsigned int
{
	HORIZONTAL = 0,
	VERTICAL = 1,
	VERTICAL_WHEEL = 2,
	HORIZONTAL_WHEEL = 3
};

struct SystemViewData
{
	std::shared_ptr<GuiComponent> logo;
	std::shared_ptr<TextComponent> maker;
	std::shared_ptr<TextComponent> year;
	unsigned int makerColor = 0;
	unsigned int makerActiveColor = 0;
	unsigned int makerShownColor = 0;
	unsigned int yearColor = 0;
	unsigned int yearActiveColor = 0;
	unsigned int yearShownColor = 0;
	std::vector<GuiComponent*> backgroundExtras;
};

struct SystemViewCarousel
{
	CarouselType type;
	Vector2f pos;
	Vector2f size;
	Vector2f origin;
	float logoScale;
	float logoRotation;
	Vector2f logoRotationOrigin;
	Alignment logoAlignment;
	unsigned int color;
	unsigned int colorEnd;
	bool colorGradientHorizontal;
	int maxLogoCount; // number of logos shown on the carousel
	int scrollDuration;
	Vector2f logoSize;
	float zIndex;
	bool hasCards;
	Vector2f cardSize;
	Vector2f cardOffset;
};

class SystemView : public IList<SystemViewData, SystemData*>
{
public:
	SystemView(Window* window);

	virtual void onShow() override;
	virtual void onHide() override;

	void goToSystem(SystemData* system, bool animate);
	void refreshSystems();

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;

	void onThemeChanged(const std::shared_ptr<ThemeData>& theme);

	std::vector<HelpPrompt> getHelpPrompts() override;
	virtual HelpStyle getHelpStyle() override;

protected:
	void onScroll(int /*amt*/) override { if(!mScrollSound.empty()) Sound::get(mScrollSound)->play(); }
	void onCursorChanged(const CursorState& state) override;

private:
	void populate();
	void setHeaderFocus(bool focus);
	void openHeaderMenu();
	void getViewElements(const std::shared_ptr<ThemeData>& theme);
	void getDefaultElements(void);
	void getCarouselFromTheme(const ThemeData::ThemeElement* elem);

	void renderCarousel(const Transform4x4f& parentTrans);
	void renderExtras(const Transform4x4f& parentTrans, float lower, float upper);
	void renderInfoBar(const Transform4x4f& trans);
	void renderFade(const Transform4x4f& trans);


	SystemViewCarousel mCarousel;
	TextComponent mSystemInfo;
	ImageComponent mCard;
	ImageComponent mCardActive;
	ImageComponent mChip;
	ImageComponent mChipActive;
	HeaderStatus mHeaderStatus;
	MenuIconBar mIconBar;
	std::shared_ptr<bool> mAlive = std::make_shared<bool>(true);
	bool mHeaderFocus = false;
	int mScrollCursor = 0;
	std::string mScrollSound;

	// unit is list index
	float mCamOffset;
	float mExtrasCamOffset;
	float mExtrasFadeOpacity;

	bool mViewNeedsReload;
	bool mShowing;
};

#endif // ES_APP_VIEWS_SYSTEM_VIEW_H
