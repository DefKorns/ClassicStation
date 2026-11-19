#pragma once
#ifndef ES_CORE_COMPONENTS_GRID_TILE_COMPONENT_H
#define ES_CORE_COMPONENTS_GRID_TILE_COMPONENT_H

#include "NinePatchComponent.h"
#include "GridTileOverlay.h"
#include "ImageComponent.h"
#include "resources/TextureResource.h"

struct GridTileProperties
{
	Vector2f mSize;
	Vector2f mPadding;
	unsigned int mImageColor;
	std::string mBackgroundImage;
	Vector2f mBackgroundCornerSize;
	Vector2f mBackgroundBleed;
	unsigned int mBackgroundCenterColor;
	unsigned int mBackgroundEdgeColor;
	bool mImageOnTop = false;
	unsigned int mBackingColor = 0x101010FF;
	Vector2f mImageOffset = Vector2f::Zero();
};

class GridTileComponent : public GuiComponent
{
public:
	GridTileComponent(Window* window);

	void render(const Transform4x4f& parentTrans) override;
	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

	// Made this a static function because the ImageGridComponent need to know the default tile max size
	// to calculate the grid dimension before it instantiate the GridTileComponents
	static Vector2f getDefaultTileSize();
	Vector2f getSelectedTileSize() const;
	bool isSelected() const;
	static void setUnselectedBrightness(float brightness);
	static void setSmoothLoading(bool smooth);

	void reset();

	void setImage(const std::string& path);
	void setImage(const std::shared_ptr<TextureResource>& texture);
	void setSelected(bool selected, bool allowAnimation = true, Vector3f* pPosition = NULL, bool force=false);
	void setVisible(bool visible);

	void forceSize(Vector2f size, float selectedZoom);
	void setShiftX(float shift) { mShiftX = shift; }
	void setOverlay(std::shared_ptr<GridTileOverlay> overlay) { mOverlay = std::move(overlay); }
	GridTileOverlay* getOverlay() const { return mOverlay.get(); }

	Vector3f getBackgroundPosition();

	virtual void update(int deltaTime) override;

	std::shared_ptr<TextureResource> getTexture();

private:
	void resize();
	void applyCurrentProperties();
	void calcCurrentProperties();
	void setSelectedZoom(float percent);

	std::shared_ptr<ImageComponent> mImage;
	NinePatchComponent mBackground;
	std::shared_ptr<GridTileOverlay> mOverlay;
	unsigned int mShownImage = 0;
	unsigned int mShownCenter = 0;
	unsigned int mShownEdge = 0;
	bool mShownKnown = false;

	GridTileProperties mDefaultProperties;
	GridTileProperties mSelectedProperties;
	std::vector<std::shared_ptr<TextureResource>> mBackgroundTextures;
	GridTileProperties mCurrentProperties;

	float mShiftX = 0.0f;
	float mSelectedZoomPercent;
	bool mSelected;
	bool mVisible;

	Vector3f mAnimPosition;
};

#endif // ES_CORE_COMPONENTS_GRID_TILE_COMPONENT_H
