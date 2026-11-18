#include "views/gamelist/GridGameListView.h"

#include "animations/LambdaAnimation.h"
#include "components/ClassicTileOverlay.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "resources/ResourceManager.h"
#include "scrapers/Scraper.h"
#include "SystemData.h"
#include "components/VideoVlcComponent.h"
#include "guis/GuiInfoPopup.h"
#include "guis/GuiMenu.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "FileSorts.h"
#include "Window.h"
#include <cstdlib>
#include <algorithm>
#include <stack>
#include <string>

GridGameListView::GridGameListView(Window* window, FileData* root) :
	ISimpleGameListView(window, root),
	mGrid(window), mMarquee(window),
	mImage(window),
	mVideo(nullptr),
	mVideoPlaying(false),
	mDescription(window),
	mDivider(window), mFavorite(window),
	mArrowLeft(window), mArrowRight(window),

	mLblRating(window), mLblReleaseDate(window), mLblDeveloper(window), mLblPublisher(window),
	mLblGenre(window), mLblPlayers(window), mLblLastPlayed(window), mLblPlayCount(window),

	mRating(window), mReleaseDate(window), mDeveloper(window), mPublisher(window),
	mGenre(window), mPlayers(window), mLastPlayed(window), mPlayCount(window),
	mName(window), mCode(window), mInfo(window), mIconBar(window), mFloatingCard(window), mDemo(window), mThumbnails(window), mLetters(window)
{
	const float padding = 0.01f;

	mVideo = new VideoVlcComponent(window, getTitlePath());

	mGrid.setPosition(mSize.x() * 0.1f, mSize.y() * 0.1f);
	mGrid.setDefaultZIndex(20);
	mGrid.setCursorChangedCallback([&](const CursorState& /*state*/) { updateInfoPanel(); });
	addChild(&mGrid);

	populateList(root->getChildrenListToDisplay());

	// metadata labels + values
	mLblRating.setText("Rating: ");
	addChild(&mLblRating);
	addChild(&mRating);
	mLblReleaseDate.setText("Released: ");
	addChild(&mLblReleaseDate);
	addChild(&mReleaseDate);
	mLblDeveloper.setText("Developer: ");
	addChild(&mLblDeveloper);
	addChild(&mDeveloper);
	mLblPublisher.setText("Publisher: ");
	addChild(&mLblPublisher);
	addChild(&mPublisher);
	mLblGenre.setText("Genre: ");
	addChild(&mLblGenre);
	addChild(&mGenre);
	mLblPlayers.setText("Players: ");
	addChild(&mLblPlayers);
	addChild(&mPlayers);
	mLblLastPlayed.setText("Last played: ");
	addChild(&mLblLastPlayed);
	mLastPlayed.setDisplayRelative(true);
	addChild(&mLastPlayed);
	mLblPlayCount.setText("Times played: ");
	addChild(&mLblPlayCount);
	addChild(&mPlayCount);

	mName.setPosition(mSize.x(), mSize.y());
	mName.setDefaultZIndex(40);
	mName.setColor(0xAAAAAAFF);
	mName.setFont(Font::get(FONT_SIZE_MEDIUM));
	mName.setHorizontalAlignment(ALIGN_CENTER);
	addChild(&mName);

	mCode.setFont(Font::get(FONT_SIZE_SMALL));
	mCode.setColor(0x7B7D7FFF);
	mCode.setHorizontalAlignment(ALIGN_RIGHT);
	mCode.setPosition(mSize.x() * 0.5f, mSize.y() * 0.93f);
	mCode.setSize(mSize.x() * 0.49f, 0.0f);
	mCode.setDefaultZIndex(40);
	addChild(&mCode);

	mInfo.setDefaultZIndex(40);
	mInfo.setVisible(false);
	addChild(&mInfo);

	mDescription.setPosition(mSize.x() * padding, mSize.y() * 0.65f);
	mDescription.setSize(mSize.x() * (0.50f - 2*padding), mSize.y() - mDescription.getPosition().y());
	mDescription.setDefaultZIndex(40);
	mDescription.setFont(Font::get(FONT_SIZE_SMALL));
	addChild(&mDescription);

	mDivider.setDefaultZIndex(40);
	addChild(&mDivider);

	mFavorite.setDefaultZIndex(41);
	mFavorite.setVisible(false);
	addChild(&mFavorite);

	mArrowLeft.setDefaultZIndex(30);
	mArrowRight.setDefaultZIndex(30);
	addChild(&mArrowLeft);
	addChild(&mArrowRight);

	// Image
	// Default to off the screen
	mImage.setOrigin(0.5f, 0.5f);
	mImage.setPosition(2.0f, 2.0f);
	mImage.setMaxSize(mSize.x(), mSize.y());
	mImage.setDefaultZIndex(30);
	mImage.setVisible(false);
	addChild(&mImage);

	// Video
	// Default to off the screen
	mVideo->setOrigin(0.5f, 0.5f);
	mVideo->setPosition(2.0f, 2.0f);
	mVideo->setSize(mSize.x(), mSize.y());
	mVideo->setDefaultZIndex(30);
	mVideo->setVisible(false);
	addChild(mVideo);

	// Marquee
	// Default to off the screen
	mMarquee.setOrigin(0.5f, 0.5f);
	mMarquee.setPosition(2.0f, 2.0f);
	mMarquee.setMaxSize(mSize.x(), mSize.y());
	mMarquee.setDefaultZIndex(35);
	mMarquee.setVisible(false);
	addChild(&mMarquee);
	mIconBar.setDefaultZIndex(70);
	mIconBar.setZIndex(70);
	addChild(&mIconBar);
	mFloatingCard.setDefaultZIndex(65);
	mFloatingCard.setZIndex(65);
	mThumbnails.setDefaultZIndex(30);
	mThumbnails.setZIndex(30);
	addChild(&mThumbnails);
	mLetters.setVisible(false);
	mLetters.setHorizontalAlignment(ALIGN_CENTER);
	addChild(&mLetters);
	mDefaultHeaderStatus = true;

	initMDLabels();
	initMDValues();
	mGrid.setTileOverlay(
		[this] { return std::make_shared<ClassicTileOverlay>(mWindow); },
		[this](GridTileOverlay& overlay, FileData* file)
		{
			static_cast<ClassicTileOverlay&>(overlay).setGame(suspendStates(file), file->getType() == GAME ? file->metadata.get("players") : std::string(), hasSaveData(file));
		});
	refreshFloating();
	updateInfoPanel();
}

GridGameListView::~GridGameListView()
{
	delete mVideo;
}

FileData* GridGameListView::getCursor()
{
	return mGrid.size() == 0 ? nullptr : mGrid.getSelected();
}

void GridGameListView::setCursor(FileData* file, bool refreshListCursorPos)
{
	if(!mGrid.setCursor(file) && (!file->isPlaceHolder()))
	{
		populateList(file->getParent()->getChildrenListToDisplay());
		mGrid.setCursor(file);
	}
}

std::string GridGameListView::getQuickSystemSelectRightButton()
{
	return "rightshoulder";
}

std::string GridGameListView::getQuickSystemSelectLeftButton()
{
	return "leftshoulder";
}

bool GridGameListView::input(InputConfig* config, Input input)
{
	if(mDemo.active() && input.value != 0)
	{
		mDemo.cancel();
		return true;
	}

	if(mHeaderFocus)
	{
		if(input.value != 0)
		{
			const int last = MenuIconBar::Count - 1;
			if(config->isMappedLike("left", input))
				mIconBar.setSelected(std::max(0, mIconBar.selected() - 1));
			else if(config->isMappedLike("right", input))
				mIconBar.setSelected(std::min(last, mIconBar.selected() + 1));
			else if(config->isMappedTo("a", input))
				openHeaderMenu();
			else if(config->isMappedTo("b", input) || config->isMappedLike("down", input))
				setHeaderFocus(false);
		}
		return true;
	}

	if(input.value != 0 && config->isMappedTo("b", input) && mFloating.valid && jumpToFloating())
		return true;

	if(input.value != 0 && config->isMappedTo("down", input) && openSuspendPoints())
		return true;

	if(input.value != 0 && config->isMappedLike("up", input))
	{
		if(UIModeController::getInstance()->isUIModeFull())
			setHeaderFocus(true);
		else
			mWindow->pushGui(new GuiMenu(mWindow));
		return true;
	}

	if(config->isMappedLike("left", input) || config->isMappedLike("right", input))
		return GuiComponent::input(config, input);

	return ISimpleGameListView::input(config, input);
}

const std::string GridGameListView::getImagePath(FileData* file)
{
	ImageSource src = mGrid.getImageSource();

	if (src == ImageSource::IMAGE)
		return file->getImagePath();
	else if (src == ImageSource::MARQUEE)
		return file->getMarqueePath();

	return file->getThumbnailPath();
}

void GridGameListView::populateList(const std::vector<FileData*>& files)
{
	mGrid.clear();
	mHeaderText.setText(mRoot->getSystem()->getFullName());
	if (files.size() > 0)
	{
		std::vector<std::string> thumbnails;
		for (auto it = files.cbegin(); it != files.cend(); it++)
		{
			const std::string image = getImagePath(*it);
			mGrid.add((*it)->getName(), image, *it);

			std::string thumbnail = (*it)->metadata.get("thumbnail");
			if (thumbnail.empty())
				thumbnail = image;
			if (image.find("/downloaded_images/") != std::string::npos && thumbnail.find("/downloaded_images/") == std::string::npos)
			{
				const std::string scraped = thumbnailPathFor(image);
				if (Utils::FileSystem::exists(scraped) || saveThumbnail(image, scraped, 30, 40))
					thumbnail = scraped;
			}
			if (thumbnail == image && image.size() > 4 && image.compare(image.size() - 4, 4, ".png") == 0)
			{
				const std::string small = image.substr(0, image.size() - 4) + "_small.png";
				if (Utils::FileSystem::exists(small))
					thumbnail = small;
			}
			if (thumbnail.empty() || !ResourceManager::getInstance()->fileExists(thumbnail))
				thumbnail = ":/cartridge.svg";
			thumbnails.push_back(thumbnail);
		}
		mThumbnails.setEntries(std::move(thumbnails));
	}
	else
	{
		addPlaceholder();
	}
	updateArrows();
}

void GridGameListView::updateArrows()
{
	const bool scrollable = mGrid.size() > static_cast<size_t>(mGrid.visibleCount());
	mArrowLeft.setVisible(scrollable);
	mArrowRight.setVisible(scrollable);
}

void GridGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);
	mIconBar.applyTheme(theme, getName());
	mFloatingCard.applyTheme(theme, getName());
	mThumbnails.applyTheme(theme, getName());
	mLetters.applyTheme(theme, getName(), "md_letters", ThemeFlags::ALL ^ ThemeFlags::TEXT);
	mLetters.setZIndex(45);

	using namespace ThemeFlags;

	mGrid.applyTheme(theme, getName(), "gamegrid", ALL);
	mName.applyTheme(theme, getName(), "md_name", ALL);
	mMarquee.applyTheme(theme, getName(), "md_marquee", POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
	mImage.applyTheme(theme, getName(), "md_image", POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
	mVideo->applyTheme(theme, getName(), "md_video", POSITION | ThemeFlags::SIZE | ThemeFlags::DELAY | Z_INDEX | ROTATION | VISIBLE);

	initMDLabels();
	std::vector<TextComponent*> labels = getMDLabels();
	assert(labels.size() == 8);
	const char* lblElements[8] = {
			"md_lbl_rating", "md_lbl_releasedate", "md_lbl_developer", "md_lbl_publisher",
			"md_lbl_genre", "md_lbl_players", "md_lbl_lastplayed", "md_lbl_playcount"
	};

	for(unsigned int i = 0; i < labels.size(); i++)
	{
		labels[i]->applyTheme(theme, getName(), lblElements[i], ALL);
	}


	initMDValues();
	std::vector<GuiComponent*> values = getMDValues();
	assert(values.size() == 8);
	const char* valElements[8] = {
			"md_rating", "md_releasedate", "md_developer", "md_publisher",
			"md_genre", "md_players", "md_lastplayed", "md_playcount"
	};

	for(unsigned int i = 0; i < values.size(); i++)
	{
		values[i]->applyTheme(theme, getName(), valElements[i], ALL ^ ThemeFlags::TEXT);
	}

	mCode.applyTheme(theme, getName(), "md_code", ALL ^ TEXT);

	const ThemeData::ThemeElement* info = theme->getElement(getName(), "md_info", "text");
	mInfo.setVisible(info != nullptr);
	if(info)
	{
		mInfo.applyTheme(theme, getName(), "md_info", ALL ^ TEXT);
		mInfoLine.load(*info);
		if(mGrid.size() > 0 && !mGrid.isScrolling())
			mInfo.setText(mInfoLine.build(*mGrid.getSelected()));
	}
	mDescription.applyTheme(theme, getName(), "md_description", ALL ^ (TEXT | ROTATION));
	mDivider.applyTheme(theme, getName(), "md_divider", ALL);
	mFavorite.applyTheme(theme, getName(), "md_favorite", ALL ^ VISIBLE);
	mDescription.clearIndicatorBox();
	if(const ThemeData::ThemeElement* bar = theme->getElement(getName(), "md_scrollbar", "image"))
		if(bar->has("pos") && bar->has("size"))
		{
			const Vector2f screen(static_cast<float>(Renderer::getScreenWidth()), static_cast<float>(Renderer::getScreenHeight()));
			mDescription.setIndicatorBox(bar->get<Vector2f>("pos") * screen, bar->get<Vector2f>("size") * screen);
		}
	mArrowLeft.applyTheme(theme, getName(), "md_arrow_left", ALL);
	mArrowRight.applyTheme(theme, getName(), "md_arrow_right", ALL);
	mDescriptionLayout = -1;
	mLayoutFonts.clear();

	// Repopulate list in case new theme is displaying a different image.  Preserve selection.
	FileData* file = mGrid.getSelected();
	populateList(mRoot->getChildrenListToDisplay());
	mGrid.setCursor(file);

	sortChildren();
}

void GridGameListView::initMDLabels()
{
	std::vector<TextComponent*> components = getMDLabels();

	const unsigned int colCount = 2;
	const unsigned int rowCount = (int)(components.size() / 2);

	Vector3f start(mSize.x() * 0.01f, mSize.y() * 0.625f, 0.0f);

	const float colSize = (mSize.x() * 0.48f) / colCount;
	const float rowPadding = 0.01f * mSize.y();

	for(unsigned int i = 0; i < components.size(); i++)
	{
		const unsigned int row = i % rowCount;
		Vector3f pos(0.0f, 0.0f, 0.0f);
		if(row == 0)
		{
			pos = start + Vector3f(colSize * (i / rowCount), 0, 0);
		}else{
			// work from the last component
			GuiComponent* lc = components[i-1];
			pos = lc->getPosition() + Vector3f(0, lc->getSize().y() + rowPadding, 0);
		}

		components[i]->setFont(Font::get(FONT_SIZE_SMALL));
		components[i]->setPosition(pos);
		components[i]->setDefaultZIndex(40);
	}
}

void GridGameListView::initMDValues()
{
	std::vector<TextComponent*> labels = getMDLabels();
	std::vector<GuiComponent*> values = getMDValues();

	std::shared_ptr<Font> defaultFont = Font::get(FONT_SIZE_SMALL);
	mRating.setSize(defaultFont->getHeight() * 5.0f, (float)defaultFont->getHeight());
	mReleaseDate.setFont(defaultFont);
	mDeveloper.setFont(defaultFont);
	mPublisher.setFont(defaultFont);
	mGenre.setFont(defaultFont);
	mPlayers.setFont(defaultFont);
	mLastPlayed.setFont(defaultFont);
	mPlayCount.setFont(defaultFont);

	float bottom = 0.0f;

	const float colSize = (mSize.x() * 0.48f) / 2;
	for(unsigned int i = 0; i < labels.size(); i++)
	{
		const float heightDiff = (labels[i]->getSize().y() - values[i]->getSize().y()) / 2;
		values[i]->setPosition(labels[i]->getPosition() + Vector3f(labels[i]->getSize().x(), heightDiff, 0));
		values[i]->setSize(colSize - labels[i]->getSize().x(), values[i]->getSize().y());
		values[i]->setDefaultZIndex(40);

		float testBot = values[i]->getPosition().y() + values[i]->getSize().y();
		if(testBot > bottom)
			bottom = testBot;
	}

	mDescription.setPosition(mDescription.getPosition().x(), bottom + mSize.y() * 0.01f);
	mDescription.setSize(mDescription.getSize().x(), mSize.y() - mDescription.getPosition().y());
}

void GridGameListView::keepLayoutFonts(const std::shared_ptr<ThemeData>& theme)
{
	if(!mLayoutFonts.empty())
		return;

	const float screenHeight = static_cast<float>(Renderer::getScreenHeight());
	for(const char* name : { "md_name", "md_developer", "md_genre" })
	{
		const ThemeData::ThemeElement* base = theme->getElement(getName(), name, "text");
		const ThemeData::ThemeElement* alternate = theme->getElement(getName(), std::string(name) + "_nodesc", "text");
		if(!base || !base->has("fontSize"))
			continue;

		const std::string path = base->has("fontPath") ? base->get<std::string>("fontPath") : Font::getDefaultPath();
		mLayoutFonts.push_back(Font::get(static_cast<int>(screenHeight * base->get<float>("fontSize")), path));
		if(alternate && alternate->has("fontSize"))
			mLayoutFonts.push_back(Font::get(static_cast<int>(screenHeight * alternate->get<float>("fontSize")),
				alternate->has("fontPath") ? alternate->get<std::string>("fontPath") : path));
	}
}

void GridGameListView::applyDescriptionLayout(bool hasDescription, bool favorite)
{
	const int state = (hasDescription ? 1 : 0) | (favorite ? 2 : 0);
	const std::shared_ptr<ThemeData>& theme = getTheme();
	if(state == mDescriptionLayout || !theme)
		return;
	mDescriptionLayout = state;
	keepLayoutFonts(theme);

	const std::pair<GuiComponent*, const char*> elements[] = {
		{ &mName, "md_name" }, { &mDeveloper, "md_developer" }, { &mGenre, "md_genre" },
		{ &mCode, "md_code" }, { &mInfo, "md_info" }, { &mFavorite, "md_favorite" } };
	for(const auto& element : elements)
	{
		element.first->applyTheme(theme, getName(), element.second, ThemeFlags::ALL ^ ThemeFlags::TEXT);
		for(const char* suffix : { "_nodesc", "_nofav", "_nodesc_nofav" })
		{
			const std::string variant(suffix);
			const bool applies = variant == "_nodesc" ? !hasDescription
				: variant == "_nofav" ? !favorite && hasDescription
				: !favorite && !hasDescription;
			const std::string alternate = std::string(element.second) + variant;
			if(applies && theme->getElement(getName(), alternate, ""))
				element.first->applyTheme(theme, getName(), alternate, ThemeFlags::ALL ^ ThemeFlags::TEXT);
		}
	}
}

void GridGameListView::updateInfoPanel()
{
	FileData* file = (mGrid.size() == 0 || mGrid.isScrolling()) ? NULL : mGrid.getSelected();
	if(!mGrid.isScrolling())
		updateHelpPrompts();
	syncFloatingCard();

	bool fadingOut;
	if(file == NULL)
	{
		mVideo->setVideo("");
		mVideo->setImage("");
		mVideoPlaying = false;

		//mDescription.setText("");
		fadingOut = true;
	}else{
		if (!mVideo->setVideo(file->getVideoPath()))
		{
			mVideo->setDefaultVideo();
		}
		mVideoPlaying = true;

		mVideo->setImageAsync(file->getThumbnailPath());
		mMarquee.setImageAsync(file->getMarqueePath());
		mImage.setImageAsync(file->getImagePath());

		mDescription.setText(file->metadata.get("desc"));
		const bool hasDescription = !file->metadata.get("desc").empty();
		mDivider.setVisible(hasDescription);
		const bool favorite = file->getType() == GAME && file->metadata.get("favorite") == "true";
		applyDescriptionLayout(hasDescription, favorite);
		mFavorite.setVisible(favorite);

		mRating.setValue(file->metadata.get("rating"));
		mReleaseDate.setValue(file->metadata.get("releasedate"));
		mDeveloper.setValue(file->metadata.get("developer"));
		mPublisher.setValue(file->metadata.get("publisher"));
		mGenre.setValue(file->metadata.get("genre"));
		mPlayers.setValue(file->metadata.get("players"));
		mName.setValue(file->metadata.get("name"));
		mCode.setText(file->getType() == GAME ? file->metadata.get("code") : std::string());
		mInfo.setText(file->getType() == GAME ? mInfoLine.build(*file) : std::string());

		if(file->getType() == GAME)
		{
			mLastPlayed.setValue(file->metadata.get("lastplayed"));
			mPlayCount.setValue(file->metadata.get("playcount"));
		}

		fadingOut = false;
	}

	std::vector<GuiComponent*> comps = getMDValues();
	comps.push_back(&mDescription);
	comps.push_back(&mDivider);
	comps.push_back(&mFavorite);
	comps.push_back(&mName);
	comps.push_back(&mCode);
	comps.push_back(&mInfo);
	comps.push_back(&mMarquee);
	comps.push_back(mVideo);
	comps.push_back(&mImage);
	std::vector<TextComponent*> labels = getMDLabels();
	comps.insert(comps.cend(), labels.cbegin(), labels.cend());

	for(auto it = comps.cbegin(); it != comps.cend(); it++)
	{
		GuiComponent* comp = *it;
		// an animation is playing
		//   then animate if reverse != fadingOut
		// an animation is not playing
		//   then animate if opacity != our target opacity
		if((comp->isAnimationPlaying(0) && comp->isAnimationReversed(0) != fadingOut) ||
		   (!comp->isAnimationPlaying(0) && comp->getOpacity() != (fadingOut ? 0 : 255)))
		{
			auto func = [comp](float t)
			{
				comp->setOpacity((unsigned char)(Math::lerp(0.0f, 1.0f, t)*255));
			};
			comp->setAnimation(new LambdaAnimation(func, 150), 0, nullptr, fadingOut);
		}
	}
}

void GridGameListView::addPlaceholder()
{
	// empty grid - add a placeholder
	mPlaceholder.reset(new FileData(PLACEHOLDER, "<No Entries Found>", this->mRoot->getSystem()->getSystemEnvData(), this->mRoot->getSystem()));
	mGrid.add(mPlaceholder->getName(), "", mPlaceholder.get());
}

void GridGameListView::launch(FileData* game)
{
	applyPendingResume();

	float screenWidth = (float) Renderer::getScreenWidth();
	float screenHeight = (float) Renderer::getScreenHeight();

	Vector3f target(screenWidth / 2.0f, screenHeight / 2.0f, 0);

	if(mMarquee.hasImage() &&
		(mMarquee.getPosition().x() < screenWidth && mMarquee.getPosition().x() > 0.0f &&
		 mMarquee.getPosition().y() < screenHeight && mMarquee.getPosition().y() > 0.0f))
	{
		target = Vector3f(mMarquee.getCenter().x(), mMarquee.getCenter().y(), 0);
	}
	else if(mImage.hasImage() &&
		(mImage.getPosition().x() < screenWidth && mImage.getPosition().x() > 2.0f &&
		 mImage.getPosition().y() < screenHeight && mImage.getPosition().y() > 2.0f))
	{
		target = Vector3f(mImage.getCenter().x(), mImage.getCenter().y(), 0);
	}
	else if(mVideo->getPosition().x() < screenWidth && mVideo->getPosition().x() > 0.0f &&
		 mVideo->getPosition().y() < screenHeight && mVideo->getPosition().y() > 0.0f)
	{
		target = Vector3f(mVideo->getCenter().x(), mVideo->getCenter().y(), 0);
	}

	ViewController::get()->launch(game, target);

}

void GridGameListView::remove(FileData *game, bool deleteFile, bool refreshView)
{
	if (deleteFile)
		Utils::FileSystem::removeFile(game->getPath());  // actually delete the file on the filesystem
	FileData* parent = game->getParent();
	if (getCursor() == game)                     // Select next element in list, or prev if none
	{
		std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
		auto gameIter = std::find(siblings.cbegin(), siblings.cend(), game);
		int gamePos = (int)std::distance(siblings.cbegin(), gameIter);
		if (gameIter != siblings.cend())
		{
			if ((gamePos + 1) < (int)siblings.size())
			{
				setCursor(siblings.at(gamePos + 1));
			} else if ((gamePos - 1) > 0) {
				setCursor(siblings.at(gamePos - 1));
			}
		}
	}
	mGrid.remove(game);
	if(mGrid.size() == 0)
	{
		addPlaceholder();
	}
	delete game;                                 // remove before repopulating (removes from parent)

	if(refreshView)
		onFileChanged(parent, FILE_REMOVED);     // update the view, with game removed
}

std::vector<TextComponent*> GridGameListView::getMDLabels()
{
	std::vector<TextComponent*> ret;
	ret.push_back(&mLblRating);
	ret.push_back(&mLblReleaseDate);
	ret.push_back(&mLblDeveloper);
	ret.push_back(&mLblPublisher);
	ret.push_back(&mLblGenre);
	ret.push_back(&mLblPlayers);
	ret.push_back(&mLblLastPlayed);
	ret.push_back(&mLblPlayCount);
	return ret;
}

std::vector<GuiComponent*> GridGameListView::getMDValues()
{
	std::vector<GuiComponent*> ret;
	ret.push_back(&mRating);
	ret.push_back(&mReleaseDate);
	ret.push_back(&mDeveloper);
	ret.push_back(&mPublisher);
	ret.push_back(&mGenre);
	ret.push_back(&mPlayers);
	ret.push_back(&mLastPlayed);
	ret.push_back(&mPlayCount);
	return ret;
}

std::vector<HelpPrompt> GridGameListView::getHelpPrompts()
{
	if(mHeaderFocus)
	{
		std::vector<HelpPrompt> headerPrompts;
		headerPrompts.push_back(HelpPrompt("left/right", "category"));
		headerPrompts.push_back(HelpPrompt("a", "open"));
		headerPrompts.push_back(HelpPrompt("b", "back"));
		return headerPrompts;
	}


	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt("up", "menu"));
	prompts.push_back(HelpPrompt("down", "suspend point list"));
	prompts.push_back(HelpPrompt("a", "select"));
	if(mFloating.valid && canJumpTo(findGame(mFloating.code)))
		prompts.push_back(HelpPrompt("b", "go to last saved game"));
	else
		prompts.push_back(HelpPrompt("b", mCursorStack.empty() ? "systems" : "back"));
	if(!hasExtraButtons())
		prompts.push_back(HelpPrompt("select", "options"));
	else if(!UIModeController::getInstance()->isUIModeKid())
		prompts.push_back(HelpPrompt("y", "favorite"));
	if(!(UIModeController::getInstance()->isUIModeKid() && Settings::getInstance()->getBool("DisableKidStartMenu")))
		prompts.push_back(HelpPrompt("start", "settings"));
	return prompts;
}

void GridGameListView::update(int deltaTime)
{
	ImageComponent::kickScrolling(mScrollCursor, mGrid.getCursorIndex(), static_cast<int>(mGrid.size()));
	mScrollCursor = mGrid.getCursorIndex();
	mThumbnails.setPosition(mGrid.getVisualCursor());
	const bool fast = mGrid.size() > 0 && mGrid.isFastScrolling();
	mLetters.setVisible(fast);
	if(fast)
	{
		mLetters.setText(Utils::String::toUpper(mGrid.getSelectedName().substr(0, 2)));
		mName.setVisible(false);
		mCode.setVisible(false);
		mFavorite.setVisible(false);
		mLettersShown = true;
	}
	else if(mLettersShown)
	{
		mLettersShown = false;
		mName.setVisible(true);
		mCode.setVisible(true);
		updateInfoPanel();
	}
	mDemo.update(deltaTime, mGrid.size() > 0, mGrid.getPosition().y() + 0.075f * static_cast<float>(Renderer::getScreenHeight()));
	mFloatingCard.update(deltaTime);
	ISimpleGameListView::update(deltaTime);
	mVideo->update(deltaTime);
}

void GridGameListView::onShow()
{
	GuiComponent::onShow();
	refreshFloating();
	updateInfoPanel();
}

void GridGameListView::onFocusLost() {
	mDescription.restart();
}

void GridGameListView::setHeaderFocus(bool focus)
{
	mHeaderFocus = focus && mIconBar.shown();
	mIconBar.setFrameVisible(focus);
	updateHelpPrompts();
}

void GridGameListView::openHeaderMenu()
{
	mIconBar.setFrameVisible(false);
	const std::weak_ptr<bool> alive = mAlive;
	GuiMenu* menu = new GuiMenu(mWindow, mIconBar.selected(), [this, alive](int category) {
		if(alive.expired())
			return;
		mIconBar.setVisible(true);
		mIconBar.setSelected(category);
		setHeaderFocus(true);
	});
	menu->useTheme(getTheme(), getName());
	mIconBar.setVisible(false);
	mWindow->pushGui(menu);
}

void GridGameListView::onFloatingChanged()
{
	syncFloatingCard();
}

void GridGameListView::renderOverHelp(const Transform4x4f& parentTrans)
{
	mFloatingCard.render(parentTrans * getTransform());
	mDemo.render(parentTrans * getTransform());
}

void GridGameListView::syncFloatingCard()
{
	mFloatingCard.setShown(mFloating.valid);
	if(!mFloating.valid)
		return;
	mFloatingCard.setScreenshot(mFloating.thumbnail);
	mFloatingCard.setPlaytime(mFloating.playtime);
	mFloatingCard.setHighlighted(mGrid.size() > 0 && isFloating(mGrid.getSelected()));
}
