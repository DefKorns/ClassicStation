/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#pragma once
#ifndef ES_CORE_I18N_H
#define ES_CORE_I18N_H

#include <string>

namespace I18n
{
	void load(const std::string& language);
	void loadFromText(const std::string& text);
	const std::string& language();
	std::string tr(const std::string& text);
}

#endif // ES_CORE_I18N_H
