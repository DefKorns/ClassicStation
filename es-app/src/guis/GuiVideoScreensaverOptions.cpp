#include "MenuColors.h"
#include "guis/GuiVideoScreensaverOptions.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "Settings.h"

GuiVideoScreensaverOptions::GuiVideoScreensaverOptions(Window* window, const char* title) : GuiScreensaverOptions(window, title)
{
	// timeout to swap videos
	auto swap = std::make_shared<SliderComponent>(mWindow, 10.f, 1000.f, 1.f, "s");
	swap->setValue((float)(Settings::getInstance()->getInt("ScreenSaverSwapVideoTimeout") / (1000)));
	addWithLabel("SWAP VIDEO AFTER (SECS)", swap);
	addSaveFunc([swap] {
		int playNextTimeout = (int)Math::round(swap->getValue()) * (1000);
		Settings::getInstance()->setInt("ScreenSaverSwapVideoTimeout", playNextTimeout);
		PowerSaver::updateTimeouts();
	});

	auto stretch_screensaver = std::make_shared<SwitchComponent>(mWindow);
	stretch_screensaver->setState(Settings::getInstance()->getBool("StretchVideoOnScreenSaver"));
	addWithLabel("STRETCH VIDEO ON SCREENSAVER", stretch_screensaver);
	addSaveFunc([stretch_screensaver] { Settings::getInstance()->setBool("StretchVideoOnScreenSaver", stretch_screensaver->getState()); });


	// Render Video Game Name as subtitles
	auto ss_info = std::make_shared< OptionListComponent<std::string> >(mWindow, "SHOW GAME INFO", false);
	std::vector<std::string> info_type;
	info_type.push_back("always");
	info_type.push_back("start & end");
	info_type.push_back("never");
	for(auto it = info_type.cbegin(); it != info_type.cend(); it++)
		ss_info->add(*it, *it, Settings::getInstance()->getString("ScreenSaverGameInfo") == *it);
	addWithLabel("SHOW GAME INFO ON SCREENSAVER", ss_info);
	addSaveFunc([ss_info, this] { Settings::getInstance()->setString("ScreenSaverGameInfo", ss_info->getSelected()); });

	auto ss_video_mute = std::make_shared<SwitchComponent>(mWindow);
	ss_video_mute->setState(Settings::getInstance()->getBool("ScreenSaverVideoMute"));
	addWithLabel("MUTE SCREENSAVER AUDIO", ss_video_mute);
	addSaveFunc([ss_video_mute] { Settings::getInstance()->setBool("ScreenSaverVideoMute", ss_video_mute->getState()); });


	auto ss_vlc_resolution = std::make_shared< OptionListComponent<std::string> >(mWindow, "VIDEO RESOLUTION", false);
	std::vector<std::string> vlc_res;
	vlc_res.push_back("original"); // renders at original video resolution, stretched to fit screen
	vlc_res.push_back("low"); // 25% of screen resolution
	vlc_res.push_back("medium"); // 50% of screen resolution
	vlc_res.push_back("high"); // 75% of screen resolution
	vlc_res.push_back("max"); // full screen resolution
	for(auto it = vlc_res.cbegin(); it != vlc_res.cend(); it++)
		ss_vlc_resolution->add(*it, *it, Settings::getInstance()->getString("VlcScreenSaverResolution") == *it);
	addWithLabel("VLC: SCREENSAVER VIDEO RESOLUTION", ss_vlc_resolution);
	addSaveFunc([ss_vlc_resolution, this] { Settings::getInstance()->setString("VlcScreenSaverResolution", ss_vlc_resolution->getSelected()); });

}

GuiVideoScreensaverOptions::~GuiVideoScreensaverOptions()
{
}

void GuiVideoScreensaverOptions::save()
{
	GuiScreensaverOptions::save();

}
