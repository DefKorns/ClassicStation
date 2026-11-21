/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include "ConsoleTools.h"

#include <arpa/inet.h>
#include <array>
#include <cstdio>
#include <ifaddrs.h>
#include <memory>
#include <net/if.h>
#include <netinet/in.h>

namespace
{
	constexpr const char* Script = "/var/lib/hakchi/emulationstation/es-tools.sh";

	std::string shellQuote(const std::string& value)
	{
		std::string quoted = "'";
		for(const char c : value)
		{
			if(c == '\'')
				quoted += "'\\''";
			else
				quoted += c;
		}
		return quoted + "'";
	}
}

namespace ConsoleTools
{
	std::string run(const std::string& arguments, const std::map<std::string, std::string>& environment)
	{
		std::string command;
		for(const auto& entry : environment)
			command += entry.first + "=" + shellQuote(entry.second) + " ";
		command += std::string("sh ") + Script + " " + arguments + " 2>&1";
		const std::unique_ptr<FILE, int (*)(FILE*)> pipe(popen(command.c_str(), "r"), pclose);
		if(!pipe)
			return "";

		std::string output;
		std::array<char, 256> buffer;
		while(std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
			output += buffer.data();

		while(!output.empty() && (output.back() == '\n' || output.back() == '\r' || output.back() == ' '))
			output.pop_back();
		return output;
	}

	bool isOn(const std::string& arguments)
	{
		return run(arguments) == "on";
	}

	std::string localAddress()
	{
		struct ifaddrs* all = nullptr;
		if(getifaddrs(&all) != 0)
			return "";

		std::string address;
		for(const struct ifaddrs* entry = all; entry != nullptr; entry = entry->ifa_next)
		{
			if(entry->ifa_addr == nullptr || entry->ifa_addr->sa_family != AF_INET || (entry->ifa_flags & IFF_LOOPBACK) != 0)
				continue;
			char text[INET_ADDRSTRLEN];
			const auto* socket = reinterpret_cast<const struct sockaddr_in*>(entry->ifa_addr);
			if(inet_ntop(AF_INET, &socket->sin_addr, text, sizeof text) != nullptr)
			{
				address = text;
				if(std::string(entry->ifa_name) == "wlan0")
					break;
			}
		}
		freeifaddrs(all);
		return address;
	}
}
