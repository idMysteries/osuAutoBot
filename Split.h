/*
osu!AutoBot
Copyright (C) 2016  Andrey Tokarev

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/
#pragma once
#include <vector>

inline std::vector<std::string> split_string(const std::string &str, const std::string &delim = " ", size_t pos = 0)
{
	std::vector<std::string> out;
	if (pos >= str.size())
		return out;

	size_t currentPos = 0;
	while (str.find(delim, pos + 1) != std::string::npos) {
		out.push_back(str.substr(currentPos, str.find(delim, pos + 1) - currentPos));
		pos = str.find(delim, pos + 1) + 1;
		currentPos = pos;
	}
	out.push_back(str.substr(pos));

	return out;
}