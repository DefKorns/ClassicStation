#include "MenuColors.h"
#include "guis/GuiMenu.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiCollectionSystemsOptions.h"
#include "guis/GuiDetectDevice.h"
#include "guis/GuiFramePicker.h"
#include <array>
#include "guis/GuiGeneralScreensaverOptions.h"
#include "guis/GuiMsgBox.h"
#include "views/ExtraButtons.h"
#include "guis/GuiScraperStart.h"
#include "guis/GuiVirtualKeyboard.h"
#include "guis/GuiSettings.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "DebugMode.h"
#include "Log.h"
#include "EmulationStation.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"
#include <SDL_events.h>
#include <algorithm>
#include <utility>
#include "platform.h"
#include "FileSorts.h"
#include "views/gamelist/IGameListView.h"
#include "guis/GuiInfoPopup.h"
#include "ConsoleTools.h"
#include "MusicPlayer.h"
#include "guis/GuiNetworkList.h"
#include "guis/GuiAbout.h"
#include "guis/GuiSystemInfo.h"
#include "guis/GuiToolRunner.h"
#include <cstdlib>
#include "utils/FileSystemUtil.h"

namespace
{
	const char* const CategoryTitles[] = { "DISPLAY", "SYSTEM", "NETWORK", "LIBRARY", "HELP" };
}

bool GuiMenu::sRebuildPending = false;

GuiMenu::GuiMenu(Window* window, int category, std::function<void(int)> onClose) : GuiComponent(window),
	mMenu(window, CategoryTitles[Display]), mVersion(window), mBar(window), mOnClose(std::move(onClose)),
	mFullUI(UIModeController::getInstance()->isUIModeFull())
{
	addChild(&mMenu);
	addVersionInfo();
	showCategory(mFullUI ? category : System);
}

void GuiMenu::useTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view)
{
	mTheme = theme;
	mView = view;
	if (mTheme)
		mBar.applyTheme(mTheme, mView);
}

GuiMenu::~GuiMenu()
{
	if (mOnClose)
		mOnClose(mCategory);
}

void GuiMenu::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
	if(!sRebuildPending)
		return;

	sRebuildPending = false;
	Window* window = mWindow;
	const int category = mCategory;
	std::function<void(int)> onClose = std::move(mOnClose);
	mOnClose = nullptr;
	const std::shared_ptr<ThemeData> theme = mTheme;
	const std::string view = mView;
	delete this;
	GuiMenu* menu = new GuiMenu(window, category, std::move(onClose));
	menu->useTheme(theme, view);
	window->pushGui(menu);
}

void GuiMenu::showCategory(int category)
{
	mCategory = category;
	mBar.setSelected(category);
	mMenu.clear();
	mMenu.setTitle(CategoryTitles[category], Font::get(FONT_SIZE_MENU_TITLE));

	switch (category)
	{
	case Display:
		addEntry("VIDEO MODE AND BEZEL", MenuColors::Text, true, [this] { openDisplaySettings(); });
		addEntry("UI SETTINGS", MenuColors::Text, true, [this] { openUISettings(); });
		addEntry("SCREENSAVER SETTINGS", MenuColors::Text, true, [this] { openScreensaverOptions(); });
		if (isFamicomPad())
			addToggle("EPILEPSY PROTECTION", "epilepsy state", "epilepsy toggle");
		break;
	case System:
		addEntry("SOUND SETTINGS", MenuColors::Text, true, [this] { openSoundSettings(); });
		if (mFullUI)
		{
			addEntry("CONTROLLER", MenuColors::Text, true, [this] { openController(); });
			addEntry("CLEAR CACHE", MenuColors::Text, false, [this] { showToolResult("cache"); });
			if (ConsoleTools::isOn("usb available"))
				addEntry("COPY RESOURCES TO USB", MenuColors::Text, false, [this] { copyResourcesToUsb(); });
			addEntry("OTHER SETTINGS", MenuColors::Text, true, [this] { openOtherSettings(); });
		}
		addEntry("QUIT", MenuColors::Text, true, [this] { openQuitMenu(); });
		break;
	case Network:
		addEntry("SCRAPER", MenuColors::Text, true, [this] { openScraperSettings(); });
		addEntry("DISPLAY IP ADDRESS", MenuColors::Text, false, [this] { showToolResult("ip"); });
		addEntry("RECONNECT", MenuColors::Text, false, [this] { showToolResult("reconnect"); });
		addEntry("CONNECT TO NETWORK", MenuColors::Text, true, [this] { openNetworkList(); });
		break;
	case Library:
		addEntry("GAME COLLECTION SETTINGS", MenuColors::Text, true, [this] { openCollectionSystemSettings(); });
		break;
	default:
		addEntry("SYSTEM INFORMATION", MenuColors::Text, false, [this] { openSystemInfo(); });
		addEntry("SAVE KERNEL LOG TO USB", MenuColors::Text, false, [this] { showToolResult("savelog"); });
		addEntry("ABOUT", MenuColors::Text, true, [this] { openAbout(); });
		if (DebugMode::isUnlocked() || DebugMode::isEnabled() || DebugMode::isJournalEnabled())
			addEntry("DEVELOPER", MenuColors::Text, true, [this] { openDeveloper(); });
		break;
	}

	setSize(mMenu.getSize());
	setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiMenu::render(const Transform4x4f& parentTrans)
{
	GuiComponent::render(parentTrans);
	if (mFullUI)
	{
		mBar.setFrameVisible(true);
		mBar.render(Transform4x4f::Identity());
	}
}

void GuiMenu::showToolResult(const std::string& arguments)
{
	mWindow->pushGui(new GuiToolRunner(mWindow, [arguments] { return ConsoleTools::run(arguments); }));
}

void GuiMenu::addToggle(const char* name, const std::string& stateArguments, const std::string& toggleArguments)
{
	auto toggle = std::make_shared<SwitchComponent>(mWindow, ConsoleTools::isOn(stateArguments));
	toggle->setChangedCallback([toggleArguments](bool) { ConsoleTools::run(toggleArguments); });

	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(mWindow, name, Font::get(FONT_SIZE_MENU), MenuColors::Text), true);
	row.addElement(toggle, false);
	mMenu.addRow(row);
}

bool GuiMenu::isFamicomPad()
{
	return ::isFamicomPad();
}

void GuiMenu::openController()
{
	auto s = new GuiSettings(mWindow, "CONTROLLER");

	const auto addToggleRow = [this, s](const char* label, const std::string& stateArguments, const std::string& toggleArguments) {
		auto toggle = std::make_shared<SwitchComponent>(mWindow, ConsoleTools::isOn(stateArguments));
		toggle->setChangedCallback([toggleArguments](bool) { ConsoleTools::run(toggleArguments); });
		s->addWithLabel(label, toggle);
	};

	ComponentListRow configureRow;
	configureRow.addElement(std::make_shared<TextComponent>(mWindow, "CONFIGURE INPUT", Font::get(FONT_SIZE_MENU), MenuColors::Text), true);
	configureRow.addElement(makeArrow(mWindow), false);
	configureRow.makeAcceptInputHandler([this] { openConfigInput(); });
	s->addRow(configureRow);

	addToggleRow("HOME COMBO", "clovercon home state", "clovercon home toggle");

	addToggleRow("AUTOFIRE", "clovercon state autofire", "clovercon toggle autofire");

	auto speed = std::make_shared< OptionListComponent<std::string> >(mWindow, "AUTOFIRE SPEED", false);
	const struct { const char* label; const char* interval; } speeds[] = { { "FAST", "4" }, { "NORMAL", "8" }, { "SLOW", "16" } };
	std::string current = "8";
	for (const auto& entry : speeds)
	{
		const bool selected = ConsoleTools::isOn(std::string("clovercon is autofire_interval ") + entry.interval);
		if (selected)
			current = entry.interval;
		speed->add(entry.label, entry.interval, selected);
	}
	s->addWithLabel("AUTOFIRE SPEED", speed);
	s->addSaveFunc([speed, current] {
		if (speed->getSelected() != current)
			ConsoleTools::run("clovercon set autofire_interval " + speed->getSelected());
	});

	addToggleRow("X/Y AS TURBO A/B", "clovercon state autofire_xy", "clovercon toggle autofire_xy");
	if (ConsoleTools::isOn("clovercon available fc_start"))
		addToggleRow("START ON 2ND CONTROLLER", "clovercon state fc_start", "clovercon toggle fc_start");

	mWindow->pushGui(s);
}

void GuiMenu::openNetworkList()
{
	Window* window = mWindow;
	window->pushGui(new GuiToolRunner(window, [] { return ConsoleTools::run("networks"); }, [window](const std::string& output) {
		window->pushGui(new GuiNetworkList(window, GuiNetworkList::parse(output)));
	}));
}

void GuiMenu::copyResourcesToUsb()
{
	Window* window = mWindow;
	window->pushGui(new GuiToolRunner(window, [] { return ConsoleTools::run("usb copy"); }, [window](const std::string& text) {
		window->pushGui(new GuiMsgBox(window, text, "OK", [] { quitES(QuitMode::RESTART); }));
	}));
}

void GuiMenu::openSystemInfo()
{
	Window* window = mWindow;
	window->pushGui(new GuiToolRunner(window, [] { return ConsoleTools::run("sysinfo"); }, [window](const std::string& text) {
		window->pushGui(new GuiSystemInfo(window, text));
	}));
}

void GuiMenu::openDeveloper()
{
	auto s = new GuiSettings(mWindow, "DEVELOPER");

	auto debugToggle = std::make_shared<SwitchComponent>(mWindow, DebugMode::isEnabled());
	debugToggle->setChangedCallback([](bool enabled) {
		DebugMode::setEnabled(enabled);
		DebugMode::setActive(enabled);
		Log::setReportingLevel(enabled ? LogDebug : LogInfo);
	});
	s->addWithLabel("DEBUG MODE", debugToggle);

	auto journalToggle = std::make_shared<SwitchComponent>(mWindow, DebugMode::isJournalEnabled());
	journalToggle->setChangedCallback([](bool enabled) { DebugMode::setJournalEnabled(enabled); });
	s->addWithLabel("VIDEO DEBUG LOG", journalToggle);

	if (ConsoleTools::isOn("overclock available"))
	{
		auto overclockToggle = std::make_shared<SwitchComponent>(mWindow, ConsoleTools::isOn("overclock state"));
		Window* window = mWindow;
		SwitchComponent* overclockSwitch = overclockToggle.get();
		overclockToggle->setChangedCallback([window, overclockSwitch](bool enabled) {
			if (!enabled)
			{
				ConsoleTools::run("overclock toggle");
				return;
			}
			auto confirm = new GuiMsgBox(window,
				"OVERCLOCK RAISES THE CPU TO ITS MAXIMUM FREQUENCY.\n\n"
				"USE AT YOUR OWN RISK. IT MAY NOT WORK ON EVERY CONSOLE AND CAN CAUSE CRASHES OR REBOOTS, "
				"ESPECIALLY WITH A WEAK POWER SUPPLY (5V/2A OR MORE RECOMMENDED) OR USB DEVICES ATTACHED.\n\n"
				"IT STAYS ON UNTIL YOU TURN IT OFF OR RESTART THE CONSOLE, AND TURNS OFF BY ITSELF AT 80 C.\n\n"
				"ENABLE OVERCLOCK?",
				"YES", [] { ConsoleTools::run("overclock toggle"); },
				"NO", [overclockSwitch] { overclockSwitch->setState(false); });
			confirm->selectButton(1);
			window->pushGui(confirm);
		});
		s->addWithLabel("OVERCLOCK", overclockToggle);

		auto autoToggle = std::make_shared<SwitchComponent>(mWindow, ConsoleTools::isOn("overclock auto state"));
		SwitchComponent* autoSwitch = autoToggle.get();
		autoToggle->setChangedCallback([window, autoSwitch](bool enabled) {
			if (!enabled)
			{
				ConsoleTools::run("overclock auto toggle");
				return;
			}
			auto confirm = new GuiMsgBox(window,
				"OVERCLOCK TURNS ON BY ITSELF ONLY WHILE YOU PLAY N64, PS1, ARCADE, DS, PSP OR GBA GAMES, "
				"AND GOES BACK TO NORMAL WHEN YOU LEAVE THE GAME.\n\n"
				"USE AT YOUR OWN RISK. IT MAY NOT WORK ON EVERY CONSOLE AND CAN CAUSE CRASHES OR REBOOTS, "
				"ESPECIALLY WITH A WEAK POWER SUPPLY (5V/2A OR MORE RECOMMENDED) OR USB DEVICES ATTACHED. "
				"IT TURNS OFF BY ITSELF AT 80 C.\n\n"
				"ENABLE OVERCLOCK IN HEAVY GAMES?",
				"YES", [] { ConsoleTools::run("overclock auto toggle"); },
				"NO", [autoSwitch] { autoSwitch->setState(false); });
			confirm->selectButton(1);
			window->pushGui(confirm);
		});
		s->addWithLabel("OVERCLOCK IN HEAVY GAMES", autoToggle);
	}

	if (ConsoleTools::isOn("swap available"))
	{
		Window* window = mWindow;
		auto swapToggle = std::make_shared<SwitchComponent>(mWindow, ConsoleTools::isOn("swap state"));
		SwitchComponent* swapSwitch = swapToggle.get();
		const auto runSwap = [window, swapSwitch](const std::string& action) {
			window->pushGui(new GuiToolRunner(window, [action] { return ConsoleTools::run("swap " + action); },
				[window, swapSwitch](const std::string& text) {
					swapSwitch->setState(ConsoleTools::isOn("swap state"));
					window->pushGui(new GuiMsgBox(window, text, "OK", nullptr));
				}));
		};
		swapToggle->setChangedCallback([window, swapSwitch, runSwap](bool enabled) {
			if (!enabled)
			{
				runSwap("disable");
				return;
			}
			auto confirm = new GuiMsgBox(window,
				"SWAP FILE USES PART OF THE SD CARD AS EXTRA MEMORY, WHICH CAN STOP HEAVY GAMES (N64, PSP) FROM CRASHING.\n\n"
				"IT TAKES " + ConsoleTools::run("swap size") + " MB ON THE SD CARD AND IS SLOWER THAN RAM. "
				"CREATING IT THE FIRST TIME TAKES A FEW MINUTES, DO NOT TURN OFF THE CONSOLE.\n\n"
				"USE AT YOUR OWN RISK: IT ADDS WEAR TO THE SD CARD. NO RESTART IS NEEDED AND IT STAYS ON AFTER RESTARTS.\n\n"
				"ENABLE SWAP FILE?",
				"YES", [runSwap] { runSwap("enable"); },
				"NO", [swapSwitch] { swapSwitch->setState(false); });
			confirm->selectButton(1);
			window->pushGui(confirm);
		});
		s->addWithLabel("SWAP FILE", swapToggle);

		auto swapSize = std::make_shared< OptionListComponent<std::string> >(mWindow, "SWAP SIZE", false);
		const std::string currentSize = ConsoleTools::run("swap size");
		for (const char* size : { "256", "512" })
			swapSize->add(std::string(size) + " MB", size, currentSize == size);
		OptionListComponent<std::string>* swapSizeList = swapSize.get();
		swapSize->setSelectedChangedCallback([window, swapSizeList] {
			ConsoleTools::run("swap setsize " + swapSizeList->getSelected());
			if (ConsoleTools::isOn("swap state"))
				window->pushGui(new GuiMsgBox(window,
					"THE NEW SIZE APPLIES WHEN YOU TURN SWAP FILE OFF AND ON AGAIN. THE FILE IS RECREATED, WHICH TAKES A FEW MINUTES.",
					"OK", nullptr));
		});
		s->addWithLabel("SWAP SIZE", swapSize);
	}

	s->addWithLabel("MEMORY", std::make_shared<TextComponent>(mWindow, ConsoleTools::run("swap info"), Font::get(FONT_SIZE_MENU), MenuColors::Dim, ALIGN_RIGHT));
	s->addWithLabel("TEMPERATURE", std::make_shared<TextComponent>(mWindow, ConsoleTools::run("temperature"), Font::get(FONT_SIZE_MENU), MenuColors::Dim, ALIGN_RIGHT));

	mWindow->pushGui(s);

	const std::string cutoffTemperature = ConsoleTools::run("overclock cutoff");
	if (!cutoffTemperature.empty())
		mWindow->pushGui(new GuiMsgBox(mWindow,
			"OVERCLOCK WAS TURNED OFF BY ITSELF BECAUSE THE CPU REACHED " + cutoffTemperature + " C.\n\n"
			"LET THE CONSOLE COOL DOWN BEFORE TURNING IT ON AGAIN.",
			"OK", nullptr));
}

void GuiMenu::openAbout()
{
	mWindow->pushGui(new GuiAbout(mWindow));
}

void GuiMenu::openScraperSettings()
{
	auto s = new GuiSettings(mWindow, "SCRAPER");

	// scrape from
	auto scraper_list = std::make_shared< OptionListComponent< std::string > >(mWindow, "SCRAPE FROM", false);
	std::vector<std::string> scrapers = getScraperList();

	// Select either the first entry of the one read from the settings, just in case the scraper from settings has vanished.
	for(auto it = scrapers.cbegin(); it != scrapers.cend(); it++)
		scraper_list->add(*it, *it, *it == Settings::getInstance()->getString("Scraper"));

	s->addWithLabel("SCRAPE FROM", scraper_list);
	s->addSaveFunc([scraper_list] { Settings::getInstance()->setString("Scraper", scraper_list->getSelected()); });

	auto apiKeyLabel = std::make_shared<TextComponent>(mWindow, "THEGAMESDB API KEY", Font::get(FONT_SIZE_MENU), MenuColors::Text);
	const auto apiKeyText = [](const std::string& key) { return key.empty() ? "DEFAULT KEY" : "CUSTOM KEY"; };
	auto apiKeyValue = std::make_shared<TextComponent>(mWindow, apiKeyText(Settings::getInstance()->getString("TheGamesDBApiKey")), Font::get(FONT_SIZE_MENU), MenuColors::Dim, ALIGN_RIGHT);
	auto apiKeyArrow = makeArrow(mWindow);
	ComponentListRow apiKeyRow;
	apiKeyRow.addElement(apiKeyLabel, true);
	apiKeyRow.addElement(apiKeyValue, false);
	auto apiKeyGap = std::make_shared<GuiComponent>(mWindow);
	apiKeyGap->setSize(Font::get(FONT_SIZE_MENU)->getLetterHeight() * 1.2f, Font::get(FONT_SIZE_MENU)->getHeight());
	apiKeyRow.addElement(apiKeyGap, false);
	apiKeyRow.addElement(apiKeyArrow, false);
	Window* window = mWindow;
	apiKeyRow.makeAcceptInputHandler([window, scraper_list, apiKeyValue, apiKeyText] {
		if(scraper_list->getSelected() != "TheGamesDB")
			return;
		window->pushGui(new GuiVirtualKeyboard(window, "YOUR THEGAMESDB API KEY (EMPTY: DEFAULT KEY)", "", true,
			[apiKeyValue, apiKeyText](const std::string& key) {
				Settings::getInstance()->setString("TheGamesDBApiKey", key);
				Settings::getInstance()->saveFile();
				apiKeyValue->setText(apiKeyText(key));
			}));
	});
	s->addRow(apiKeyRow);

	const auto updateApiKeyRow = [scraper_list, apiKeyLabel, apiKeyValue, apiKeyArrow] {
		const unsigned char opacity = scraper_list->getSelected() == "TheGamesDB" ? 255 : 90;
		apiKeyLabel->setOpacity(opacity);
		apiKeyValue->setOpacity(opacity);
		apiKeyArrow->setOpacity(opacity);
	};
	scraper_list->setSelectedChangedCallback(updateApiKeyRow);
	updateApiKeyRow();

	// scrape ratings
	auto scrape_ratings = std::make_shared<SwitchComponent>(mWindow);
	scrape_ratings->setState(Settings::getInstance()->getBool("ScrapeRatings"));
	s->addWithLabel("SCRAPE RATINGS", scrape_ratings);
	s->addSaveFunc([scrape_ratings] { Settings::getInstance()->setBool("ScrapeRatings", scrape_ratings->getState()); });

	const struct { const char* label; const char* setting; } scrapeToggles[] = {
		{ "SCRAPE NAME", "ScrapeName" },
		{ "SCRAPE DESCRIPTION", "ScrapeDescription" },
		{ "SCRAPE COVER IMAGE", "ScrapeImage" },
		{ "SCRAPE LOGO", "ScrapeMarquee" },
		{ "SCRAPE DETAILS", "ScrapeDetails" },
		{ "OVERWRITE EXISTING DATA", "ScrapeOverwrite" }
	};
	for(const auto& toggle : scrapeToggles)
	{
		auto sw = std::make_shared<SwitchComponent>(mWindow);
		sw->setState(Settings::getInstance()->getBool(toggle.setting));
		s->addWithLabel(toggle.label, sw);
		s->addSaveFunc([sw, setting = std::string(toggle.setting)] { Settings::getInstance()->setBool(setting, sw->getState()); });
	}

	// scrape now
	ComponentListRow row;
	auto openScrapeNow = [this] { mWindow->pushGui(new GuiScraperStart(mWindow)); };
	std::function<void()> openAndSave = openScrapeNow;
	openAndSave = [s, openAndSave] { s->save(); openAndSave(); };
	row.makeAcceptInputHandler(openAndSave);

	auto scrape_now = std::make_shared<TextComponent>(mWindow, "SCRAPE NOW", Font::get(FONT_SIZE_MENU), MenuColors::Text);
	auto bracket = makeArrow(mWindow);
	row.addElement(scrape_now, true);
	row.addElement(bracket, false);
	s->addRow(row);

	mWindow->pushGui(s);
}

void GuiMenu::openSoundSettings()
{
	auto s = new GuiSettings(mWindow, "SOUND SETTINGS");

	// volume
	const bool hasMixer = VolumeControl::getInstance()->isAvailable();
	if (hasMixer)
	{
		auto volume = std::make_shared<SliderComponent>(mWindow, 0.f, 100.f, 1.f, "%");
		volume->setValue((float)VolumeControl::getInstance()->getVolume());
		s->addWithLabel("SYSTEM VOLUME", volume);
		s->addSaveFunc([volume] { VolumeControl::getInstance()->setVolume((int)Math::round(volume->getValue())); });
	}

	auto music = std::make_shared<SwitchComponent>(mWindow);
	music->setState(Settings::getInstance()->getBool("MusicEnabled"));
	s->addWithLabel("BACKGROUND MUSIC", music);
	s->addSaveFunc([music] {
		Settings::getInstance()->setBool("MusicEnabled", music->getState());
		if (music->getState())
			MusicPlayer::get().start();
		else
			MusicPlayer::get().stop();
	});

	auto musicVolume = std::make_shared<SliderComponent>(mWindow, 0.f, 100.f, 5.f, "%");
	musicVolume->setValue((float)Settings::getInstance()->getInt("MusicVolume"));
	s->addWithLabel("MUSIC VOLUME", musicVolume);
	s->addSaveFunc([musicVolume] { Settings::getInstance()->setInt("MusicVolume", (int)Math::round(musicVolume->getValue())); });

	if (UIModeController::getInstance()->isUIModeFull())
	{
#if defined(__linux__)
		if (hasMixer)
		{
		// audio card
		auto audio_card = std::make_shared< OptionListComponent<std::string> >(mWindow, "AUDIO CARD", false);
		std::vector<std::string> audio_cards;
		audio_cards.push_back("default");
		audio_cards.push_back("sysdefault");
		audio_cards.push_back("dmix");
		audio_cards.push_back("hw");
		audio_cards.push_back("plughw");
		audio_cards.push_back("null");
		if (Settings::getInstance()->getString("AudioCard") != "") {
			if(std::find(audio_cards.begin(), audio_cards.end(), Settings::getInstance()->getString("AudioCard")) == audio_cards.end()) {
				audio_cards.push_back(Settings::getInstance()->getString("AudioCard"));
			}
		}
		for(auto ac = audio_cards.cbegin(); ac != audio_cards.cend(); ac++)
			audio_card->add(*ac, *ac, Settings::getInstance()->getString("AudioCard") == *ac);
		s->addWithLabel("AUDIO CARD", audio_card);
		s->addSaveFunc([audio_card] {
			Settings::getInstance()->setString("AudioCard", audio_card->getSelected());
			VolumeControl::getInstance()->deinit();
			VolumeControl::getInstance()->init();
		});

		// volume control device
		auto vol_dev = std::make_shared< OptionListComponent<std::string> >(mWindow, "AUDIO DEVICE", false);
		std::vector<std::string> transitions;
		transitions.push_back("PCM");
		transitions.push_back("HDMI");
		transitions.push_back("Headphone");
		transitions.push_back("Speaker");
		transitions.push_back("Master");
		transitions.push_back("Digital");
		transitions.push_back("Analogue");
		if (Settings::getInstance()->getString("AudioDevice") != "") {
			if(std::find(transitions.begin(), transitions.end(), Settings::getInstance()->getString("AudioDevice")) == transitions.end()) {
				transitions.push_back(Settings::getInstance()->getString("AudioDevice"));
			}
		}
		for(auto it = transitions.cbegin(); it != transitions.cend(); it++)
			vol_dev->add(*it, *it, Settings::getInstance()->getString("AudioDevice") == *it);
		s->addWithLabel("AUDIO DEVICE", vol_dev);
		s->addSaveFunc([vol_dev] {
			Settings::getInstance()->setString("AudioDevice", vol_dev->getSelected());
			VolumeControl::getInstance()->deinit();
			VolumeControl::getInstance()->init();
		});
		}
#endif

		// disable sounds
		auto sounds_enabled = std::make_shared<SwitchComponent>(mWindow);
		sounds_enabled->setState(Settings::getInstance()->getBool("EnableSounds"));
		s->addWithLabel("ENABLE NAVIGATION SOUNDS", sounds_enabled);
		s->addSaveFunc([sounds_enabled] {
			if (sounds_enabled->getState()
				&& !Settings::getInstance()->getBool("EnableSounds")
				&& PowerSaver::getMode() == PowerSaver::INSTANT)
			{
				Settings::getInstance()->setString("PowerSaverMode", "default");
				PowerSaver::init();
			}
			Settings::getInstance()->setBool("EnableSounds", sounds_enabled->getState());
		});

	}

	mWindow->pushGui(s);

}

void GuiMenu::openThemeOptions(GuiComponent* parent)
{
	std::vector<ThemeSubset> subsets;
	for(SystemData* system : SystemData::sSystemVector)
		if(!system->getTheme()->getSubsets().empty())
		{
			subsets = system->getTheme()->getSubsets();
			break;
		}

	if(subsets.empty())
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, "THE CURRENT THEME HAS NO OPTIONS.", "OK", nullptr));
		return;
	}

	auto s = new GuiSettings(mWindow, "THEME OPTIONS");
	typedef OptionListComponent<std::string> SubsetList;
	auto lists = std::make_shared<std::vector<std::shared_ptr<SubsetList>>>();

	for(const ThemeSubset& subset : subsets)
	{
		const std::string label = Utils::String::toUpper(subset.displayName);
		auto list = std::make_shared<SubsetList>(mWindow, label, false);
		for(const ThemeSubsetOption& option : subset.options)
			list->add(Utils::String::toUpper(option.displayName), option.name, option.name == subset.selected);
		s->addWithLabel(label, list);
		lists->push_back(list);
	}

	Window* window = mWindow;
	s->addSaveFunc([subsets, lists, window, parent]
	{
		bool changed = false;
		for(std::size_t i = 0; i < subsets.size(); ++i)
		{
			const std::string selected = (*lists)[i]->getSelected();
			if(selected == subsets[i].selected)
				continue;
			Settings::getInstance()->setString(subsets[i].settingKey(), selected);
			changed = true;
		}

		if(!changed)
			return;

		Settings::getInstance()->saveFile();
		window->renderLoadingScreen("Loading...");
		CollectionSystemManager::get()->updateSystemsList();
		ViewController::get()->reloadAll(true);
		GuiMenu::requestRebuild();
		delete parent;
	});

	mWindow->pushGui(s);
}

namespace
{
	std::string scanlineName(int level)
	{
		static const std::array<const char*, 4> names = { "ORIGINAL", "SOFT", "MEDIUM", "STRONG" };
		return names[static_cast<size_t>(level - 1)];
	}
}

void GuiMenu::openDisplaySettings()
{
	auto s = new GuiSettings(mWindow, "VIDEO MODE AND BEZEL");

	auto mode = std::make_shared< OptionListComponent<std::string> >(mWindow, "VIDEO MODE", false);
	const std::string currentMode = Settings::getInstance()->getString("DisplayMode");
	mode->add("4:3", "keep-aspect-ratio", currentMode == "keep-aspect-ratio");
	mode->add("PIXEL PERFECT", "pixel-perfect", currentMode == "pixel-perfect");
	mode->add("CRT FILTER", "crt-filter", currentMode == "crt-filter");
	s->addWithLabel("VIDEO MODE", mode);
	s->addSaveFunc([mode] { Settings::getInstance()->setString("DisplayMode", mode->getSelected()); });

	auto frames = Utils::FileSystem::getDirContent("/usr/share/backgrounds");
	frames.sort();
	std::vector<std::string> frameNames{ "" };
	for (const auto& dir : frames)
		if (Utils::FileSystem::isDirectory(dir))
			frameNames.push_back(Utils::FileSystem::getFileName(dir));

	Window* window = mWindow;
	if (frameNames.size() > 1)
	{
		auto bezel = std::make_shared< OptionListComponent<std::string> >(mWindow, "BEZEL", false);
		const std::string currentBezel = Settings::getInstance()->getString("DisplayFrame");
		for (const auto& name : frameNames)
			bezel->add(name.empty() ? std::string("NONE") : Utils::String::toUpper(name), name, name == currentBezel);
		s->addWithLabel("BEZEL", bezel);
		bezel->setOpenHandler([window, mode, bezel, frameNames]
		{
			const std::string suffix = mode->getSelected() == "pixel-perfect" ? "pixel_perfect" : "4_3";
			window->pushGui(new GuiFramePicker(window, frameNames, bezel->getSelected(), suffix, [bezel](const std::string& picked)
			{
				bezel->select(picked);
			}));
		});
		s->addSaveFunc([bezel] { Settings::getInstance()->setString("DisplayFrame", bezel->getSelected()); });
	}

	const bool famicom = isFamicomPad();

	auto sharp = std::make_shared< OptionListComponent<int> >(mWindow, "CRT SMOOTHING", false);
	const int currentSharp = Settings::getInstance()->getInt("DisplayCrtSharp");
	sharp->add("STOCK", 0, currentSharp == 0);
	sharp->add("SOFT", 1, currentSharp == 1);
	sharp->add("SHARP", 2, currentSharp == 2);
	sharp->add("PIXEL", 3, currentSharp == 3);
	auto square = std::make_shared< OptionListComponent<int> >(mWindow, "CRT SIZE", false);
	const int currentSquare = Settings::getInstance()->getInt("DisplayCrtSquare");
	square->add("4:3", 0, currentSquare == 0);
	square->add("PIXEL PERFECT", 1, currentSquare == 1);
	if (famicom)
	{
		s->addWithLabel("CRT SMOOTHING", sharp);
		s->addWithLabel("CRT SIZE", square);
		s->addSaveFunc([square] { Settings::getInstance()->setInt("DisplayCrtSquare", square->getSelected()); });
		s->addSaveFunc([sharp] { Settings::getInstance()->setInt("DisplayCrtSharp", sharp->getSelected()); });
	}

	std::string currentFilter = Settings::getInstance()->getString("DisplayFilter");
	if (currentFilter.empty())
	{
		if (currentMode == "composite")
			currentFilter = "composite";
		else if (Settings::getInstance()->getInt("DisplaySmoothness") == 1)
			currentFilter = "nearest";
		else if (Settings::getInstance()->getInt("DisplaySmoothness") == 3 || Settings::getInstance()->getBool("DisplayCrtSmooth"))
			currentFilter = "linear";
		else
			currentFilter = famicom ? "nearest" : "sharp";
	}
	if (famicom && currentFilter != "nearest")
		currentFilter = "linear";
	auto filter = std::make_shared< OptionListComponent<std::string> >(mWindow, "FILTER", false);
	if (famicom)
	{
		filter->add("OFF", "nearest", currentFilter == "nearest");
		filter->add("ON", "linear", currentFilter == "linear");
	}
	else
	{
		filter->add("NEAREST", "nearest", currentFilter == "nearest");
		filter->add("LINEAR", "linear", currentFilter == "linear");
		filter->add("HORIZONTAL LINEAR", "hlinear", currentFilter == "hlinear");
		filter->add("SHARP", "sharp", currentFilter == "sharp");
		filter->add("COMPOSITE", "composite", currentFilter == "composite");
		filter->add("PHOSPHOR", "phosphor", currentFilter == "phosphor");
	}
	s->addWithLabel("FILTER", filter);
	if (!famicom)
	{
		filter->setOpenHandler([window, filter, mode]
		{
			const bool pixelPerfect = mode->getSelected() == "pixel-perfect";
			const std::string series = pixelPerfect ? "pp" : "43";
			struct Entry { const char* value; const char* label; };
			const std::array<Entry, 6> entries = {{ { "nearest", "NEAREST" }, { "linear", "LINEAR" }, { "hlinear", "HORIZONTAL LINEAR" }, { "sharp", "SHARP" }, { "composite", "COMPOSITE" }, { "phosphor", "PHOSPHOR" } }};
			std::vector<GuiFramePicker::Item> items;
			std::vector<std::string> values;
			size_t current = 0;
			for (const auto& entry : entries)
			{
				if (pixelPerfect && std::string(entry.value) == "sharp")
					continue;
				if (entry.value == filter->getSelected() || (pixelPerfect && filter->getSelected() == "sharp" && std::string(entry.value) == "nearest"))
					current = items.size();
				items.push_back({ entry.label, ":/filters/filter_" + series + "_" + entry.value + ".png" });
				values.push_back(entry.value);
			}
			window->pushGui(new GuiFramePicker(window, items, current, 480, 270,
				[filter, values](size_t index) { filter->select(values[index]); }));
		});
	}

	int currentLevel = Settings::getInstance()->getInt("DisplayScanlineLevel");
	if (currentLevel < 1 && Settings::getInstance()->getBool("DisplayCrtScanlines"))
		currentLevel = 1;
	auto scanlines = std::make_shared< OptionListComponent<int> >(mWindow, "SCANLINES", false);
	scanlines->add("OFF", 0, currentLevel == 0);
	for (int level = 1; level <= 4; ++level)
		scanlines->add(scanlineName(level), level, currentLevel == level);
	s->addWithLabel("SCANLINES", scanlines);
	if (!famicom)
	{
		scanlines->setOpenHandler([window, scanlines]
		{
			std::vector<GuiFramePicker::Item> items;
			for (int level = 0; level <= 4; ++level)
				items.push_back({ level == 0 ? std::string("OFF") : scanlineName(level),
					":/scanlines/scanlines_" + std::to_string(level) + ".png" });
			window->pushGui(new GuiFramePicker(window, items, static_cast<size_t>(scanlines->getSelected()), 480, 270,
				[scanlines](size_t index) { scanlines->select(static_cast<int>(index)); }));
		});
	}

	s->addSaveFunc([filter, scanlines]
	{
		const std::string chosen = filter->getSelected();
		const int level = scanlines->getSelected();
		Settings::getInstance()->setString("DisplayFilter", chosen);
		Settings::getInstance()->setInt("DisplaySmoothness", chosen == "nearest" ? 1 : (chosen == "linear" ? 3 : 0));
		Settings::getInstance()->setBool("DisplayCrtSmooth", chosen == "linear" || chosen == "hlinear");
		Settings::getInstance()->setInt("DisplayScanlineLevel", level);
		Settings::getInstance()->setBool("DisplayCrtScanlines", level > 0);
	});

	const auto updateEffects = [mode, filter, scanlines, sharp, square, famicom]
	{
		const bool crt = mode->getSelected() == "crt-filter";
		const std::string chosen = filter->getSelected();
		const bool scanlinesFixed = crt || chosen == "composite" || chosen == "phosphor";
		filter->setEnabled(!crt);
		filter->setOpacity(crt ? 90 : 255);
		sharp->setEnabled(crt);
		sharp->setOpacity(crt ? 255 : 90);
		square->setEnabled(crt);
		square->setOpacity(crt ? 255 : 90);
		const bool scanlinesLocked = scanlinesFixed && !(crt && famicom);
		if (crt && famicom && scanlines->getSelected() == 0)
			scanlines->select(1);
		scanlines->setEnabled(!scanlinesLocked);
		scanlines->setOpacity(scanlinesLocked ? 90 : 255);
	};
	mode->setSelectedChangedCallback(updateEffects);
	auto previousFilter = std::make_shared<std::string>(currentFilter);
	filter->setSelectedChangedCallback([filter, mode, previousFilter, updateEffects]
	{
		std::string chosen = filter->getSelected();
		if (chosen == "sharp" && mode->getSelected() == "pixel-perfect")
		{
			filter->select(*previousFilter == "hlinear" ? "composite" : "hlinear");
			return;
		}
		*previousFilter = chosen;
		updateEffects();
	});
	updateEffects();

	mWindow->pushGui(s);
}

void GuiMenu::openUISettings()
{
	auto s = new GuiSettings(mWindow, "UI SETTINGS");

	//UI mode
	auto UImodeSelection = std::make_shared< OptionListComponent<std::string> >(mWindow, "UI MODE", false);
	std::vector<std::string> UImodes = UIModeController::getInstance()->getUIModes();
	for (auto it = UImodes.cbegin(); it != UImodes.cend(); it++)
		UImodeSelection->add(*it, *it, Settings::getInstance()->getString("UIMode") == *it);
	s->addWithLabel("UI MODE", UImodeSelection);
	Window* window = mWindow;
	s->addSaveFunc([ UImodeSelection, window]
	{
		std::string selectedMode = UImodeSelection->getSelected();
		if (selectedMode != "Full")
		{
			std::string msg = "You are changing the UI to a restricted mode:\n" + selectedMode + "\n";
			msg += "This will hide most menu-options to prevent changes to the system.\n";
			msg += "Only the games marked as kid games will be shown.\n";
			msg += "To unlock and return to the full UI, enter this code: \n";
			msg += "\"" + UIModeController::getInstance()->getFormattedPassKeyStr() + "\"\n\n";
			msg += "Do you want to proceed?";
			window->pushGui(new GuiMsgBox(window, msg,
				"YES", [selectedMode] {
					LOG(LogDebug) << "Setting UI mode to " << selectedMode;
					Settings::getInstance()->setString("UIMode", selectedMode);
					Settings::getInstance()->saveFile();
			}, "NO",nullptr));
		}
	});

	// quick system select (left/right in game list view)
	auto quick_sys_select = std::make_shared<SwitchComponent>(mWindow);
	quick_sys_select->setState(Settings::getInstance()->getBool("QuickSystemSelect"));
	s->addWithLabel("QUICK SYSTEM SELECT", quick_sys_select);
	s->addSaveFunc([quick_sys_select] { Settings::getInstance()->setBool("QuickSystemSelect", quick_sys_select->getState()); });

	// carousel transition option
	auto move_carousel = std::make_shared<SwitchComponent>(mWindow);
	move_carousel->setState(Settings::getInstance()->getBool("MoveCarousel"));
	s->addWithLabel("CAROUSEL TRANSITIONS", move_carousel);
	s->addSaveFunc([move_carousel] {
		if (move_carousel->getState()
			&& !Settings::getInstance()->getBool("MoveCarousel")
			&& PowerSaver::getMode() == PowerSaver::INSTANT)
		{
			Settings::getInstance()->setString("PowerSaverMode", "default");
			PowerSaver::init();
		}
		Settings::getInstance()->setBool("MoveCarousel", move_carousel->getState());
	});

	// transition style
	auto transition_style = std::make_shared< OptionListComponent<std::string> >(mWindow, "TRANSITION STYLE", false);
	std::vector<std::string> transitions;
	transitions.push_back("fade");
	transitions.push_back("slide");
	transitions.push_back("instant");
	for(auto it = transitions.cbegin(); it != transitions.cend(); it++)
		transition_style->add(*it, *it, Settings::getInstance()->getString("TransitionStyle") == *it);
	s->addWithLabel("TRANSITION STYLE", transition_style);
	s->addSaveFunc([transition_style] {
		if (Settings::getInstance()->getString("TransitionStyle") == "instant"
			&& transition_style->getSelected() != "instant"
			&& PowerSaver::getMode() == PowerSaver::INSTANT)
		{
			Settings::getInstance()->setString("PowerSaverMode", "default");
			PowerSaver::init();
		}
		Settings::getInstance()->setString("TransitionStyle", transition_style->getSelected());
	});

	// theme set
	auto themeSets = ThemeData::getThemeSets();

	if(!themeSets.empty())
	{
		std::map<std::string, ThemeSet>::const_iterator selectedSet = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
		if(selectedSet == themeSets.cend())
			selectedSet = themeSets.cbegin();

		auto theme_set = std::make_shared< OptionListComponent<std::string> >(mWindow, "THEME SET", false);
		for(auto it = themeSets.cbegin(); it != themeSets.cend(); it++)
			theme_set->add(it->first, it->first, it == selectedSet);
		s->addWithLabel("THEME SET", theme_set);

		Window* window = mWindow;
		s->addSaveFunc([window, theme_set]
		{
			bool needReload = false;
			std::string oldTheme = Settings::getInstance()->getString("ThemeSet");
			if(oldTheme != theme_set->getSelected())
				needReload = true;

			Settings::getInstance()->setString("ThemeSet", theme_set->getSelected());

			if(needReload)
			{
				Scripting::fireEvent("theme-changed", theme_set->getSelected(), oldTheme);
				CollectionSystemManager::get()->updateSystemsList();
				ViewController::get()->reloadAll(true); // TODO - replace this with some sort of signal-based implementation
				if(Settings::getInstance()->getString("GamelistViewStyle") == "grid" && !ViewController::themeHasGridView())
				{
					Settings::getInstance()->setString("GamelistViewStyle", "automatic");
					ViewController::get()->reloadAll(true);
				}
			}
		});
	}

	// the theme sets the scheme itself when it defines es_scheme, so the option would do nothing
	bool themeSetsScheme = false;
	for(SystemData* system : SystemData::sSystemVector)
		if(!system->getTheme()->getVariable("es_scheme").empty())
		{
			themeSetsScheme = true;
			break;
		}

	if(!themeSetsScheme)
	{
		// color scheme
		auto color_scheme = std::make_shared< OptionListComponent<std::string> >(mWindow, "COLOR SCHEME", false);
		const bool lightScheme = Settings::getInstance()->getString("ColorScheme") == "light";
		color_scheme->add("DARK", "dark", !lightScheme);
		color_scheme->add("LIGHT", "light", lightScheme);
		s->addWithLabel("COLOR SCHEME", color_scheme);
		s->addSaveFunc([color_scheme, window]
		{
			if(color_scheme->getSelected() == Settings::getInstance()->getString("ColorScheme"))
				return;

			window->renderLoadingScreen("Loading...");
			Settings::getInstance()->setString("ColorScheme", color_scheme->getSelected());
			MenuColors::apply(MenuColors::schemeFromName(color_scheme->getSelected()));
			ViewController::get()->reloadAll(true);
			GuiMenu::requestRebuild();
		});
	}

	// theme options (subsets declared by the theme)
	const bool themeHasOptions = std::any_of(SystemData::sSystemVector.cbegin(), SystemData::sSystemVector.cend(),
		[](SystemData* system) { return !system->getTheme()->getSubsets().empty(); });
	if (themeHasOptions)
	{
		ComponentListRow themeOptionsRow;
		themeOptionsRow.addElement(std::make_shared<TextComponent>(mWindow, "THEME OPTIONS", Font::get(FONT_SIZE_MENU), MenuColors::Text), true);
		themeOptionsRow.addElement(makeArrow(mWindow), false);
		themeOptionsRow.makeAcceptInputHandler([this, s] { openThemeOptions(s); });
		s->addRow(themeOptionsRow);
	}

	// GameList view style
	auto gamelist_style = std::make_shared< OptionListComponent<std::string> >(mWindow, "GAMELIST VIEW STYLE", false);
	std::vector<std::string> styles;
	styles.push_back("automatic");
	styles.push_back("basic");
	styles.push_back("detailed");
	if (ViewController::themeHasGridView())
		styles.push_back("grid");

	for (auto it = styles.cbegin(); it != styles.cend(); it++)
		gamelist_style->add(*it, *it, Settings::getInstance()->getString("GamelistViewStyle") == *it);
	s->addWithLabel("GAMELIST VIEW STYLE", gamelist_style);
	s->addSaveFunc([gamelist_style] {
		bool needReload = false;
		if (Settings::getInstance()->getString("GamelistViewStyle") != gamelist_style->getSelected())
			needReload = true;
		Settings::getInstance()->setString("GamelistViewStyle", gamelist_style->getSelected());
		if (needReload)
			ViewController::get()->reloadAll();
	});

	// Optionally ignore leading articles when sorting game titles
	auto ignore_articles = std::make_shared<SwitchComponent>(mWindow);
	ignore_articles->setState(Settings::getInstance()->getBool("IgnoreLeadingArticles"));
	s->addWithLabel("IGNORE ARTICLES (NAME SORT ONLY)", ignore_articles);
	s->addSaveFunc([ignore_articles, window] {
		bool articles_are_ignored = Settings::getInstance()->getBool("IgnoreLeadingArticles");
		Settings::getInstance()->setBool("IgnoreLeadingArticles", ignore_articles->getState());
		if (ignore_articles->getState() != articles_are_ignored)
		{
			//For each system...
			for (auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++)
			{
				//Apply sort recursively
				FileData* root = (*it)->getRootFolder();
				root->sort(getSortTypeFromString(root->getSortName()));

				//Notify that the root folder was sorted
				ViewController::get()->getGameListView((*it))->onFileChanged(root, FILE_SORTED);
			}

			//Display popup to inform user
			GuiInfoPopup* popup = new GuiInfoPopup(window, "Files sorted", 4000);
			window->setInfoPopup(popup);
		}
	});

	// lb/rb uses full screen size paging instead of -10/+10 steps
	auto use_fullscreen_paging = std::make_shared<SwitchComponent>(mWindow);
	use_fullscreen_paging->setState(Settings::getInstance()->getBool("UseFullscreenPaging"));
	s->addWithLabel("USE FULL SCREEN PAGING FOR LB/RB", use_fullscreen_paging);
	s->addSaveFunc([use_fullscreen_paging] {
		Settings::getInstance()->setBool("UseFullscreenPaging", use_fullscreen_paging->getState());
	});

	// Optionally start in selected system
	auto systemfocus_list = std::make_shared< OptionListComponent<std::string> >(mWindow, "START ON SYSTEM", false);
	systemfocus_list->add("NONE", "", Settings::getInstance()->getString("StartupSystem") == "");
	for (auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++)
	{
		if ("retropie" != (*it)->getName())
		{
			systemfocus_list->add((*it)->getName(), (*it)->getName(), Settings::getInstance()->getString("StartupSystem") == (*it)->getName());
		}
	}
	s->addWithLabel("START ON SYSTEM", systemfocus_list);
	s->addSaveFunc([systemfocus_list] {
		Settings::getInstance()->setString("StartupSystem", systemfocus_list->getSelected());
	});

	// show help
	auto show_help = std::make_shared<SwitchComponent>(mWindow);
	show_help->setState(Settings::getInstance()->getBool("ShowHelpPrompts"));
	s->addWithLabel("ON-SCREEN HELP", show_help);
	s->addSaveFunc([show_help] { Settings::getInstance()->setBool("ShowHelpPrompts", show_help->getState()); });

	auto menu_animations = std::make_shared<SwitchComponent>(mWindow);
	menu_animations->setState(Settings::getInstance()->getBool("MenuAnimations"));
	s->addWithLabel("HEADER ANIMATIONS", menu_animations);
	auto background_animation = std::make_shared<SwitchComponent>(mWindow);
	background_animation->setState(Settings::getInstance()->getBool("BackgroundAnimation"));
	s->addWithLabel("BACKGROUND ANIMATION", background_animation);
	s->addSaveFunc([background_animation] { Settings::getInstance()->setBool("BackgroundAnimation", background_animation->getState()); });
	auto suspend_animations = std::make_shared<SwitchComponent>(mWindow);
	suspend_animations->setState(Settings::getInstance()->getBool("SuspendAnimations"));
	s->addWithLabel("SUSPEND POINT ANIMATIONS", suspend_animations);
	s->addSaveFunc([suspend_animations] { Settings::getInstance()->setBool("SuspendAnimations", suspend_animations->getState()); });
	auto demo_mode = std::make_shared<SwitchComponent>(mWindow);
	demo_mode->setState(Settings::getInstance()->getBool("DemoMode"));
	s->addWithLabel("DEMO MODE", demo_mode);
	s->addSaveFunc([demo_mode] { Settings::getInstance()->setBool("DemoMode", demo_mode->getState()); });
	s->addSaveFunc([menu_animations] { Settings::getInstance()->setBool("MenuAnimations", menu_animations->getState()); });

	// enable filters (ForceDisableFilters)
	auto enable_filter = std::make_shared<SwitchComponent>(mWindow);
	enable_filter->setState(!Settings::getInstance()->getBool("ForceDisableFilters"));
	s->addWithLabel("ENABLE FILTERS", enable_filter);
	s->addSaveFunc([enable_filter] {
		bool filter_is_enabled = !Settings::getInstance()->getBool("ForceDisableFilters");
		Settings::getInstance()->setBool("ForceDisableFilters", !enable_filter->getState());
		if (enable_filter->getState() != filter_is_enabled) ViewController::get()->ReloadAndGoToStart();
	});

	// hide start menu in Kid Mode
	auto disable_start = std::make_shared<SwitchComponent>(mWindow);
	disable_start->setState(Settings::getInstance()->getBool("DisableKidStartMenu"));
	s->addWithLabel("DISABLE START MENU IN KID MODE", disable_start);
	s->addSaveFunc([disable_start] { Settings::getInstance()->setBool("DisableKidStartMenu", disable_start->getState()); });

	mWindow->pushGui(s);

}

void GuiMenu::openOtherSettings()
{
	auto s = new GuiSettings(mWindow, "OTHER SETTINGS");

	// maximum vram
	auto max_vram = std::make_shared<SliderComponent>(mWindow, 0.f, 1000.f, 10.f, "Mb");
	max_vram->setValue((float)(Settings::getInstance()->getInt("MaxVRAM")));
	s->addWithLabel("VRAM LIMIT", max_vram);
	s->addSaveFunc([max_vram] { Settings::getInstance()->setInt("MaxVRAM", (int)Math::round(max_vram->getValue())); });

	// power saver
	auto power_saver = std::make_shared< OptionListComponent<std::string> >(mWindow, "POWER SAVER MODES", false);
	std::vector<std::string> modes;
	modes.push_back("disabled");
	modes.push_back("default");
	modes.push_back("enhanced");
	modes.push_back("instant");
	for (auto it = modes.cbegin(); it != modes.cend(); it++)
		power_saver->add(*it, *it, Settings::getInstance()->getString("PowerSaverMode") == *it);
	s->addWithLabel("POWER SAVER MODES", power_saver);
	s->addSaveFunc([this, power_saver] {
		if (Settings::getInstance()->getString("PowerSaverMode") != "instant" && power_saver->getSelected() == "instant") {
			Settings::getInstance()->setString("TransitionStyle", "instant");
			Settings::getInstance()->setBool("MoveCarousel", false);
			Settings::getInstance()->setBool("EnableSounds", false);
		}
		Settings::getInstance()->setString("PowerSaverMode", power_saver->getSelected());
		PowerSaver::init();
	});

	// gamelists
	auto gamelistsSaveMode = std::make_shared< OptionListComponent<std::string> >(mWindow, "SAVE METADATA", false);
	std::vector<std::string> saveModes;
	saveModes.push_back("on exit");
	saveModes.push_back("always");
	saveModes.push_back("never");

	for(auto it = saveModes.cbegin(); it != saveModes.cend(); it++)
		gamelistsSaveMode->add(*it, *it, Settings::getInstance()->getString("SaveGamelistsMode") == *it);
	s->addWithLabel("SAVE METADATA", gamelistsSaveMode);
	s->addSaveFunc([gamelistsSaveMode] {
		Settings::getInstance()->setString("SaveGamelistsMode", gamelistsSaveMode->getSelected());
	});

	auto async_file_io = std::make_shared<SwitchComponent>(mWindow);
	async_file_io->setState(Settings::getInstance()->getBool("AsyncFileIO"));
	s->addWithLabel("ASYNC FILE IO", async_file_io);
	s->addSaveFunc([async_file_io] { Settings::getInstance()->setBool("AsyncFileIO", async_file_io->getState()); });


	// framerate
	auto framerate = std::make_shared<SwitchComponent>(mWindow);
	framerate->setState(Settings::getInstance()->getBool("DrawFramerate"));
	s->addWithLabel("SHOW FRAMERATE", framerate);
	s->addSaveFunc([framerate] { Settings::getInstance()->setBool("DrawFramerate", framerate->getState()); });

	mWindow->pushGui(s);

}

void GuiMenu::openConfigInput()
{
	Window* window = mWindow;
	window->pushGui(new GuiMsgBox(window, "ARE YOU SURE YOU WANT TO CONFIGURE INPUT?", "YES",
		[window] {
		window->pushGui(new GuiDetectDevice(window, false, nullptr));
	}, "NO", nullptr)
	);

}

void GuiMenu::openQuitMenu()
{
	auto s = new GuiSettings(mWindow, "QUIT");

	Window* window = mWindow;

	// command line switch
	bool confirm_quit = Settings::getInstance()->getBool("ConfirmQuit");

	ComponentListRow row;
	if (UIModeController::getInstance()->isUIModeFull())
	{
		auto static restart_es_fx = []() {
			Scripting::fireEvent("quit");
			if (quitES(QuitMode::RESTART)) {
				LOG(LogWarning) << "Restart terminated with non-zero result!";
			}
		};

		if (confirm_quit) {
			row.makeAcceptInputHandler([window] {
				window->pushGui(new GuiMsgBox(window, "REALLY RESTART?", "YES", restart_es_fx, "NO", nullptr));
			});
		} else {
			row.makeAcceptInputHandler(restart_es_fx);
		}
		row.addElement(std::make_shared<TextComponent>(window, "RESTART CLASSICSTATION", Font::get(FONT_SIZE_MENU), MenuColors::Text), true);
		s->addRow(row);

		if(Settings::getInstance()->getBool("ShowExit"))
		{
			auto static quit_es_fx = [] {
				Scripting::fireEvent("quit");
				quitES();
			};

			row.elements.clear();
			if (confirm_quit) {
				row.makeAcceptInputHandler([window] {
					window->pushGui(new GuiMsgBox(window, "REALLY QUIT?", "YES", quit_es_fx, "NO", nullptr));
				});
			} else {
				row.makeAcceptInputHandler(quit_es_fx);
			}
			row.addElement(std::make_shared<TextComponent>(window, "QUIT CLASSICSTATION", Font::get(FONT_SIZE_MENU), MenuColors::Text), true);
			s->addRow(row);
		}
	}

	auto static reboot_sys_fx = [] {
		Scripting::fireEvent("quit", "reboot");
		Scripting::fireEvent("reboot");
		if (quitES(QuitMode::REBOOT)) {
			LOG(LogWarning) << "Restart terminated with non-zero result!";
		}
	};

	row.elements.clear();
	if (confirm_quit) {
		row.makeAcceptInputHandler([window] {
			window->pushGui(new GuiMsgBox(window, "REALLY RESTART?", "YES", {reboot_sys_fx}, "NO", nullptr));
		});
	} else {
		row.makeAcceptInputHandler(reboot_sys_fx);
	}
	row.addElement(std::make_shared<TextComponent>(window, "RESTART SYSTEM", Font::get(FONT_SIZE_MENU), MenuColors::Text), true);
	s->addRow(row);

	auto static shutdown_sys_fx = [] {
		Scripting::fireEvent("quit", "shutdown");
		Scripting::fireEvent("shutdown");
		if (quitES(QuitMode::SHUTDOWN)) {
			LOG(LogWarning) << "Shutdown terminated with non-zero result!";
		}
	};

	row.elements.clear();
	if (confirm_quit) {
		row.makeAcceptInputHandler([window] {
			window->pushGui(new GuiMsgBox(window, "REALLY SHUTDOWN?", "YES", shutdown_sys_fx, "NO", nullptr));
		});
	} else {
		row.makeAcceptInputHandler(shutdown_sys_fx);
	}
	row.addElement(std::make_shared<TextComponent>(window, "SHUTDOWN SYSTEM", Font::get(FONT_SIZE_MENU), MenuColors::Text), true);
	s->addRow(row);
	mWindow->pushGui(s);
}

void GuiMenu::addVersionInfo()
{
	std::string  buildDate = (Settings::getInstance()->getBool("Debug") ? std::string( "   (" + Utils::String::toUpper(PROGRAM_BUILT_STRING) + ")") : (""));

	mVersion.setFont(Font::get(FONT_SIZE_SMALL));
	mVersion.setColor(MenuColors::Dim);
	mVersion.setText("CLASSICSTATION V" + Utils::String::toUpper(PROGRAM_VERSION_STRING) + buildDate);
	mVersion.setHorizontalAlignment(ALIGN_CENTER);
	addChild(&mVersion);
}

void GuiMenu::openScreensaverOptions() {
	mWindow->pushGui(new GuiGeneralScreensaverOptions(mWindow, "SCREENSAVER SETTINGS"));
}

void GuiMenu::openCollectionSystemSettings() {
	mWindow->pushGui(new GuiCollectionSystemsOptions(mWindow));
}

void GuiMenu::onSizeChanged()
{
	mVersion.setSize(mSize.x(), 0);
	mVersion.setPosition(0, mSize.y() - mVersion.getSize().y());
}

void GuiMenu::addEntry(const char* name, unsigned int color, bool add_arrow, const std::function<void()>& func)
{
	std::shared_ptr<Font> font = Font::get(FONT_SIZE_MENU);

	// populate the list
	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(mWindow, name, font, color), true);

	if(add_arrow)
	{
		std::shared_ptr<ImageComponent> bracket = makeArrow(mWindow);
		row.addElement(bracket, false);
	}

	row.makeAcceptInputHandler(func);

	mMenu.addRow(row);
}

bool GuiMenu::input(InputConfig* config, Input input)
{
	if(GuiComponent::input(config, input))
		return true;

	if(mFullUI && input.value != 0)
	{
		if(config->isMappedLike("left", input))
		{
			showCategory((mCategory + CategoryCount - 1) % CategoryCount);
			return true;
		}
		if(config->isMappedLike("right", input))
		{
			showCategory((mCategory + 1) % CategoryCount);
			return true;
		}
	}

	if((config->isMappedTo("b", input) || config->isMappedTo("start", input)) && input.value != 0)
	{
		delete this;
		return true;
	}

	return false;
}

HelpStyle GuiMenu::getHelpStyle()
{
	HelpStyle style = HelpStyle();
	style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
	style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system", "menu_help");
	return style;
}

std::vector<HelpPrompt> GuiMenu::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	if(mFullUI)
		prompts.push_back(HelpPrompt("left/right", "category"));
	prompts.push_back(HelpPrompt("up/down", "choose"));
	prompts.push_back(HelpPrompt("a", "select"));
	prompts.push_back(HelpPrompt("start", "close"));
	return prompts;
}
