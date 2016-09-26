
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
#include "vec2f.h"
#include "Split.h" 
#include "TimingPoint.h"
#include "segment.h"
#include <math.h>
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

using namespace std;

inline float distance(vec2f p0, vec2f p1) {
	return (p1 - p0).len();
}

float delta = 1.0f;
float TWO_PI = static_cast<float>(M_PI * 2.0f);
float HALF_PI = static_cast<float>(M_PI / 2.0f);

inline long binomialCoefficient(const int& n, int k) {
	if (k < 0 || k > n)
		return 0;
	if (k == 0 || k == n)
		return 1;
	k = MIN(k, n - k);  // take advantage of symmetry
	long c = 1;
	for (int i = 0; i < k; i++)
		c = c * (n - i) / (i + 1);
	return c;
}

inline float bernstein(const int& i, const int& n, const float& t) {
	return binomialCoefficient(n, i) * powf(t, i) * powf(1.0f - t, n - i);
}

inline vec2f bezier(const vector<vec2f>& pts, const float& t) {
	vec2f c;
	int n = pts.size() - 1;
	for (int i = 0; i <= n; i++) {
		float b = bernstein(i, n, t);
		c.x += pts[i].x * b;
		c.y += pts[i].y * b;
	}
	return c;
}

inline bool isIn(float a, float b, float c) {
	return (b > a && b < c) || (b < a && b > c);
}

inline float lerp(float a, float b, float t) {
	return a * (1.0f - t) + b * t;
}

inline vec2f intersect(vec2f a, vec2f ta, vec2f b, vec2f tb) {
	// xy = a + ta * t = b + tb * u
	// t =(b + tb*u -a)/ta
	//t(x) == t(y)
	//(b.x + tb.x*u -a.x)/ta.x = (b.y + tb.y*u -a.y)/ta.y
	// b.x*ta.y + tb.x*u*ta.y -a.x*ta.y = b.y*ta.x + tb.y*u*ta.x -a.y*ta.x
	// tb.x*u*ta.y - tb.y*u*ta.x= b.y*ta.x  -a.y*ta.x -b.x*ta.y +a.x*ta.y
	//u *(tb.x*ta.y - tb.y*ta.x) = (b.y-a.y)ta.x +(a.x-b.x)ta.y
	//u = ((b.y-a.y)ta.x +(a.x-b.x)ta.y) / (tb.x*ta.y - tb.y*ta.x);

	auto des = tb.x * ta.y - tb.y * ta.x;
	if (abs(des) < 0.00001f)
		cout << "Vectors are parallel." << endl;
	auto u = ((b.y - a.y) * ta.x + (a.x - b.x) * ta.y) / des;
	return b.cpy().add(tb.x * u, tb.y * u);
}

float CircleTAt(vec2f pt, vec2f centre)
{
	return atan2f(pt.y - centre.y, pt.x - centre.x);
}

void CircleThroughPoints(vec2f A, vec2f B, vec2f C, vec2f& centre,  float& radius, float& t_initial, float& t_final)
{
	float D = 2 * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));
	float AMagSq = A.LengthSquared();
	float BMagSq = B.LengthSquared();
	float CMagSq = C.LengthSquared();
	centre = { (AMagSq * (B.y - C.y) + BMagSq * (C.y - A.y) + CMagSq * (A.y - B.y)) / D,
		(AMagSq * (C.x - B.x) + BMagSq * (A.x - C.x) + CMagSq * (B.x - A.x)) / D };
	radius = (centre - A).len();

	t_initial = CircleTAt(A, centre);
	float t_mid = CircleTAt(B, centre);
	t_final = CircleTAt(C, centre);
	while (t_mid < t_initial) t_mid += 2.0f * M_PI;
	while (t_final < t_initial) t_final += 2.0f * M_PI;
	if (t_mid > t_final)
	{
		t_final -= 2.0f * M_PI;
	}
}

class HitObject
{

public:
	vec2f startPosition;
	int startTime;
	int endTime = 0;
	vector<vec2f> sliderPoints;
	vector<Segment> sliderSegments;
	vec2f PCenter;
	float PRadius;
	float startAng, endAng;

	float PixelLength;
	int sliderTime;
	char sliderType;
	int hitType;
	int stackId = 0;
	bool itSlider() const
	{
		return (hitType & 2) > 0;
	}

	bool itSpinner() const
	{
		return (hitType & 8) > 0;
	}

	int getStartTime() const
	{
		return startTime - 8;
	}

	int getEndTime() const
	{
		return endTime - 8;
	}

	int getStack() const
	{
		return stackId;
	}

	vec2f getEndPos()
	{
		float t = 1.0f;
		return getPointByT(t);
	}

	void setStack(int stack)
	{
		stackId = stack;
	}

	int getSliderTime() const
	{
		return sliderTime;
	}

	vec2f getStartPosition() const
	{
		return startPosition;
	}

	vec2f getPointByT(float& t){
		auto floor = floorf(t);
		t = static_cast<int>(floor) % 2 == 0 ? t - floor : floor + 1.0f - t;
		if (sliderType == 'P'){
			auto dist = PixelLength * t;
			auto currDist = 0.0f;
			auto oldpoint = startPosition;
			auto ct = 0.0f;
			while (currDist < dist)
			{
				auto ang = endAng * ct + startAng * (1.f - ct);
				vec2f p{ PCenter.x + PRadius * cosf(ang), PCenter.y + PRadius * sinf(ang) };
				currDist += distance(p, oldpoint);
				if (currDist > dist) { return oldpoint; }
				oldpoint = p;
				ct += 1.0f / (PixelLength * 0.5f);
			}
			return oldpoint;
		}
		auto dist = PixelLength * t;
		auto currDist = 0.0f;
		auto oldpoint = startPosition;
		for (int i = 0; i < sliderSegments.size(); i++)
		{
			auto seg = sliderSegments[i];
			if (i == sliderSegments.size() - 1)
			{
				auto ct = 0.0f;
				while (currDist < PixelLength)
				{
					auto p{ bezier(seg.points, ct) };
					currDist += distance(p, oldpoint);
					if (currDist > dist) { return oldpoint; }
					oldpoint = p;
					ct += 1.0f / (seg.points.size() * 50 - 1);
				}
			}
			for (auto ct = 0.0f; ct < 1.0f + (1.0f / (seg.points.size() * 50 - 1)); ct += 1.0f / (seg.points.size() * 50 - 1))
			{
				auto p{ bezier(seg.points, ct) };
				currDist += distance(p, oldpoint);
				if (currDist > dist) { return oldpoint; }
				oldpoint = p;
			}
		}
		return oldpoint;
	}

	HitObject(string hitstring, vector<TimingPoint> *timingPoints, float MapSliderMultiplier) {
		endTime = 0;
		auto tokens = split_string(hitstring, ",");
		startPosition = vec2f(stof(tokens.at(0)), stof(tokens.at(1)));
		startTime = atoi(tokens.at(2).c_str());
		hitType = atoi(tokens.at(3).c_str());
		if (itSlider())
		{
			auto RepeatCount = atoi(tokens.at(6).c_str());
			PixelLength = stof(tokens.at(7));
			float beatLengthBase = timingPoints->at(0).getBPM();
			float BPM = timingPoints->at(0).getBPM();

			for (auto point : *timingPoints) {
				if (point.getTime() <= startTime) {
					if (point.getBPM() >= 0.0f) {
						beatLengthBase = point.getBPM();
					}
					BPM = point.getBPM();
					//break;
				}
			}

			if (BPM < 0.0f) {auto newMulti = BPM / -100.0f; BPM = beatLengthBase * newMulti;}

			sliderTime = static_cast<int>(BPM * (PixelLength / MapSliderMultiplier) / 100.0f);
			endTime = sliderTime * RepeatCount + startTime;

			auto SliderTokens = split_string(tokens.at(5), "|");

			sliderPoints.push_back(startPosition);
			for (auto i = 1; i < static_cast<int>(SliderTokens.size()); i++) {
				auto p = split_string(SliderTokens.at(i), ":");
				auto point = vec2f(stof(p.at(0)), stof(p.at(1)));
				sliderPoints.push_back(point);
			}
			if (sliderPoints[sliderPoints.size() - 1] == sliderPoints[sliderPoints.size() - 2]) { // EndPoint == red dot
				sliderPoints.resize(sliderPoints.size() - 1);
			}
			sliderType = SliderTokens[0].c_str()[0];
			if (sliderType == 'L'|| sliderType == 'C') {
				for (int i = 1; i < static_cast<int>(sliderPoints.size()); i++) {
					auto p0 = sliderPoints[i - 1];
					auto p1 = sliderPoints[i];
					vector<vec2f> segmentPoints = {p0, p1};
					Segment seg;
					seg.points = segmentPoints;
					sliderSegments.push_back(seg);
				}
			}
			else if (sliderType == 'B') {
			bezier:
				vector<vector<vec2f>> curveList;
				vector<vec2f> curve;
				for (auto point : sliderPoints) {
					if (curve.size() > 1) {
						if (point == curve[curve.size() - 1]) {
							curveList.push_back(curve);
							curve.clear();
						}
					}
					curve.push_back(point);
				}

				curveList.push_back(curve);

				curve.clear();
				for (auto plot : curveList) {
					Segment seg;
					seg.points = plot;
					sliderSegments.push_back(seg);
				}
			}
			else if (sliderType == 'P') {
				if (sliderPoints.size() != 3){
					sliderType = 'B';
					goto bezier;
				}
				vec2f a = sliderPoints[0];
				vec2f b = sliderPoints[1];
				vec2f c = sliderPoints[2];

				CircleThroughPoints(a, b, c, PCenter, PRadius, startAng, endAng);
			}
			else {
				sliderType = 'B';
				goto bezier;
			}
		}
		else if (itSpinner()) {
			endTime = atoi(tokens.at(5).c_str());
		}
	}

	~HitObject() {
	
	}
};
