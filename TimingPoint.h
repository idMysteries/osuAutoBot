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
#include "Split.h"
#include <string>

class TimingPoint
{
	int TimingTime;
	float BPM;
public:
	explicit TimingPoint(std::string TimingString) { //6590,461.538461538462,4,2,1,6,1,0
		auto tokens = split_string(TimingString, ",");
		TimingTime = stoi(tokens.at(0));
		BPM = stof(tokens.at(1));
	}

	float getBPM() const
	{
		return BPM;
	}

	int getTime() const
	{
		return TimingTime;
	}

	~TimingPoint() {

	}
};
