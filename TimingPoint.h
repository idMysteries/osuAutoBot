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
