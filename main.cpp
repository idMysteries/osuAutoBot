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


#define _USE_MATH_DEFINES

#include <cmath>
#include <iostream>
#include "HitObject.h"
#include <Windows.h>
#include <fstream>
#include <thread>
#include <TlHelp32.h>
#include <psapi.h>
vector<TimingPoint> TimingPoints;
vector<HitObject> HitObjects;
float StackLeniency;
float CircleSize;
float SliderMultiplier;

float XMultiplier, YMultiplier;
float StackOffset;

HWND OsuWindow;
bool songStarted;
DWORD OsuProcessID;
HANDLE OsuProcessHandle;
LPVOID TimeAdress;
int SongTime;

LPVOID GetBaseAddress(HANDLE hProc)
{
	MODULEINFO miInfo;
	if (GetModuleInformation(hProc, nullptr, &miInfo, sizeof(miInfo)))
		return miInfo.EntryPoint;
	return nullptr;
}

DWORD GetMemorySize(HANDLE hProc)
{
	PROCESS_MEMORY_COUNTERS pmcInfo;
	if (GetProcessMemoryInfo(hProc, &pmcInfo, sizeof(pmcInfo)))
		return static_cast<DWORD>(pmcInfo.WorkingSetSize);
	return 0;
}

DWORD FindPattern(HANDLE ProcessHandle, BYTE Signature[], unsigned const int ByteCount) {
	const static unsigned int Mult = 4096;
	const static unsigned int StartAdress = int(GetBaseAddress(OsuProcessHandle));
	const static unsigned int EndAdress = StartAdress + GetMemorySize(OsuProcessHandle);
	static bool Hit;

	BYTE ByteInMemory[Mult];

	for (auto i = StartAdress; i < EndAdress; i += Mult - ByteCount) {
		ReadProcessMemory(ProcessHandle, LPCVOID(i), &ByteInMemory, Mult, nullptr);
		for (unsigned int a = 0; a < Mult; a++) {
			Hit = true;

			for (unsigned int j = 0; j < ByteCount && Hit; j++) {
				if (ByteInMemory[a + j] != Signature[j] && Signature[j] != 0x00) Hit = false;
			}
			if (Hit) {
				return i + a;
			}
		}
	}
	return 0;
}

void timeThread()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	while (true)
	{
		ReadProcessMemory(OsuProcessHandle, TimeAdress, &SongTime, 4, nullptr);
		this_thread::sleep_for(chrono::milliseconds(1));
	}
}

int OsuWindowX, OsuWindowY;

float sqrt2 = sqrtf(2.0f) / 2.0f;

float normalize_angle(const float &a)
{
	float res = a;
	if (res < float(-M_PI)) res += TWO_PI;
	else if (res > float(M_PI)) res -= TWO_PI;
	return res;
}

float GetDelta(float angle)
{
	float si = abs(sinf(angle));
	if (si > sqrt2) { si = abs(sinf(angle - float(M_PI_2))); }
	return (1.0f - sqrt2) * si;
}

void SpinerCircleMove(HitObject* spiner)
{
	vec2f center{ 
		spiner->getStartPosition().x * XMultiplier + OsuWindowX, 
		spiner->getStartPosition().y * YMultiplier + OsuWindowY 
	};
	auto angle = static_cast<float>((M_PI));
	float R = 70.0f;
	while (SongTime <= spiner->getEndTime() && songStarted)
	{
		//angle = normalize_angle(angle);
		float r = R * (sqrt2 + GetDelta(angle));
		int x = int(r * cos(angle) + center.x);
		int y = int(r * sin(angle) + center.y);
		SetCursorPos(x, y);
		angle -= 0.056f;
		this_thread::sleep_for(chrono::milliseconds(1));
	}
}

void SpiralMove(HitObject* spiner)
{
	vec2f center = vec2f(spiner->getStartPosition().x * XMultiplier + OsuWindowX, spiner->getStartPosition().y * YMultiplier + OsuWindowY);
	float angle = static_cast<float>((M_PI));
	float R = 0.0f;
	while (SongTime <= spiner->getEndTime() && songStarted)
	{
		int x = int(R * cos(angle) + center.x);
		int y = int(R * sin(angle) + center.y);
		SetCursorPos(x, y);
		angle -= 0.056f;
		R += 0.2f;
		this_thread::sleep_for(chrono::milliseconds(1));
	}
}
vec2f CirclePoint(vec2f center, float R, float angle) {
	float x = cosf(angle) * R;
	float y = sinf(angle) * R;
	return vec2f(x, y) + center;
}
#include <limits>
#include <random>
void SliderMove(HitObject* slider)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	while (SongTime <= slider->getEndTime() && songStarted)
	{
		auto t = static_cast<float>(SongTime - slider->getStartTime()) / static_cast<float>(slider->getSliderTime());
		auto pos = slider->getPointByT(t);
		SetCursorPos(int((pos.x - slider->getStack() * StackOffset)* XMultiplier) + OsuWindowX, int((pos.y - slider->getStack() * StackOffset) * YMultiplier) + OsuWindowY);
		this_thread::sleep_for(chrono::milliseconds(1));
	}
}

vec2f CatmullRoll(float t, vec2f p1, vec2f p2, vec2f p3, vec2f p4)
{

	float t2 = t*t;
	float t3 = t*t*t;
	vec2f v;
	v.x = ((-t3 + 2 * t2 - t)*(p1.x) + (3 * t3 - 5 * t2 + 2)*(p2.x) + (-3 * t3 + 4 * t2 + t)* (p3.x) + (t3 - t2)*(p4.x)) / 2;
	v.y = ((-t3 + 2 * t2 - t)*(p1.y) + (3 * t3 - 5 * t2 + 2)*(p2.y) + (-3 * t3 + 4 * t2 + t)* (p3.y) + (t3 - t2)*(p4.y)) / 2;
	return v;
}

float vectorAngle(vec2f a, vec2f b) {
	return abs(atan2(a.x*b.y - b.x*a.y, a.x*b.x + a.y*b.y));
}


float ang = float(M_PI_2);

void DancingMoveTo(HitObject* object)
{
	POINT p;
	GetCursorPos(&p);
	auto p0 = vec2f(static_cast<float>(p.x), static_cast<float>(p.y));
	auto p1 = vec2f((object->getStartPosition().x - StackOffset * object->getStack()) * XMultiplier + OsuWindowX, (object->getStartPosition().y - StackOffset * object->getStack())  * YMultiplier + OsuWindowY);
	ang = -ang;
	auto p2 = rotate(vec2f(p1 - p0) / 2.0f, ang + vectorAngle(p0, p1)) + p0;
	//auto p3 = rotate(vec2f(p1 - p0) / 2.0f, ang - vectorAngle(p0, p1)) + p1;
	auto dt = static_cast<float>(object->getStartTime() - SongTime);
	while (SongTime < object->getStartTime() && songStarted)
	{
		auto t = (dt - float(object->getStartTime() - SongTime)) / dt;
		vec2f B = bezier(vector<vec2f>{
			p0, p2, /*p3,*/ p1
		}, t);
		SetCursorPos(static_cast<int>(B.x), static_cast<int>(B.y));
		this_thread::sleep_for(chrono::milliseconds(1));
	}
	SetCursorPos(int(p1.x), int(p1.y));
}

void LinearMoveTo(HitObject* object)
{
	//default_random_engine eng((random_device()()));
	//uniform_real_distribution<float> Arandom(0.0f, float(M_PI));
	//uniform_real_distribution<float> Rrandom(0.0f, StackOffset * 10.0f * XMultiplier * 0.95f);
	POINT p;
	GetCursorPos(&p);
	auto p0 = vec2f(static_cast<float>(p.x), static_cast<float>(p.y));
	auto p1 = vec2f((object->getStartPosition().x - StackOffset * object->getStack()) * XMultiplier + OsuWindowX, (object->getStartPosition().y - StackOffset * object->getStack())  * YMultiplier + OsuWindowY);
	auto dt = static_cast<float>(object->getStartTime() - SongTime);
	while (SongTime < object->getStartTime() && songStarted)
	{
		auto t = (dt - float(object->getStartTime() - SongTime)) / dt;
		vec2f B = p0 + t*(p1 - p0);
		SetCursorPos(static_cast<int>(B.x), static_cast<int>(B.y));
		this_thread::sleep_for(chrono::milliseconds(1));
	}
	SetCursorPos(int(p1.x), int(p1.y));
}

void AutoThread()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	for (auto Hit : HitObjects)
	{
		DancingMoveTo(&Hit);
		//BezierMoveTo(&Hit);
		//keybd_event(0x67, 0, 0, 0);
		if (Hit.itSpinner())
		{
			SpinerCircleMove(&Hit);
		}
		if (Hit.itSlider())
		{
			SliderMove(&Hit);
		}
		//this_thread::sleep_for(chrono::milliseconds(10));
		//keybd_event(0x67, 0, KEYEVENTF_KEYUP, 0);
		if (!songStarted)
		{
			return;
		}
	}
}

void gameCheckerThread()
{
	SetConsoleTitle("");
	while (true)
	{
		RECT rect;
		POINT p; p.x = 0; p.y = 8;
		GetClientRect(OsuWindow, &rect);
		ClientToScreen(OsuWindow, &p);
		int x = min(rect.right, GetSystemMetrics(SM_CXSCREEN));
		int y = min(rect.bottom, GetSystemMetrics(SM_CYSCREEN));
		//cout << x << ' ' << y << endl;
		//cout << x << ' ' << y << endl;
		int swidth = x;
		int sheight = y;
		if (swidth * 3 > sheight * 4) {
			swidth = sheight * 4 / 3;
		}
		else {
			sheight = swidth * 3 / 4;
		}
		XMultiplier = swidth / 640.0f;
		YMultiplier = sheight / 480.0f;
		int xOffset = static_cast<int>(x - 512.0f * XMultiplier) / 2;
		int yOffset = static_cast<int>(y - 384.0f * YMultiplier) / 2;

		OsuWindowX = p.x + xOffset;
		OsuWindowY = p.y + yOffset;

		const static auto Length = 256;
		char TitleC[256];
		GetWindowText(OsuWindow, TitleC, Length);
		auto Title = string(TitleC);
		if (Title != "osu!" && Title != "")
		{
			if (!songStarted)
			{
				songStarted = true;
				thread Auto(AutoThread);
				Auto.detach();
			}
		}
		else
		{
			if (Title == "")
			{
				TerminateProcess(GetCurrentProcess(), 0);
			}
			songStarted = false;
		}
		Sleep(100);
	}
}
DWORD getProcessID()
{
	auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return 0;
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof pe;
	if (Process32First(hSnapshot, &pe))
	{
		do
		{
			if (string(pe.szExeFile) == "osu!.exe")
			{
				CloseHandle(hSnapshot);
				return pe.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &pe));
	}
	CloseHandle(hSnapshot);
	return 0;
}
void checkGame()
{
rescan:
	cout << "Search \"osu!\" window" << endl;
	OsuWindow = FindWindow(nullptr, TEXT("osu!"));
	if (OsuWindow == nullptr) {
		cout << "Please run osu!" << endl;
		while (OsuWindow == nullptr) {
			OsuWindow = FindWindow(nullptr, TEXT("osu!"));
			Sleep(100);
		}
	}
	cout << "Osu! founded" << endl;
	OsuProcessID = getProcessID();
	OsuProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, false, OsuProcessID);
	BYTE Pattern[] = { 0xA3, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x00, 0xA1, 0x00, 0x00, 0x00, 0x00, 0xA3 };
	const int PatternLength = sizeof Pattern / sizeof BYTE;
	DWORD ScanAdress = FindPattern(OsuProcessHandle, Pattern, PatternLength) + 1;
	if (ScanAdress == 1) {
		TimeAdress = nullptr;
		cout << "Error in \"Timer Find\"" << endl;
		CloseHandle(OsuProcessHandle);
		goto rescan;
	}
	int timerr;
	ReadProcessMemory(OsuProcessHandle, reinterpret_cast<LPCVOID>(ScanAdress), &timerr, 4, nullptr);
	TimeAdress = LPVOID(timerr + 0xC);
	cout << "Timer address: " << TimeAdress << endl;
	cout << "Start threads" << endl;
	thread game(gameCheckerThread);
	game.detach();
	thread Timer(timeThread);
	Timer.detach();
}

float mapDifficultyRange(float difficulty, float min, float mid, float max)
{
	if (difficulty > 5.0f)
		return mid + (max - mid)*(difficulty - 5.0f) / 5.0f;
	if (difficulty < 5.0f)
		return mid - (mid - min)*(5.0f - difficulty) / 5.0f;
	return mid;
}
float OD;
void ParseSong(string path)
{
	ifstream t(path);
	bool General = false;
	bool Difficulty = false;
	bool Timing = false;
	bool Hits = false;
	while (t)
	{
		string str;
		getline(t, str);
		if (str.find("[General]") != string::npos)
		{
			General = true;
		}
		else if (General)
		{
			if (str.find("StackLeniency") != string::npos)
			{
				StackLeniency = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find(':') == string::npos)
			{
				General = false;
			}
		}
		else if (str.find("[Difficulty]") != string::npos)
		{
			Difficulty = true;
		}
		else if (Difficulty)
		{
			if (str.find("OverallDifficulty:") != string::npos) {
				OD = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find("CircleSize") != string::npos)
			{
				CircleSize = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find("SliderMultiplier") != string::npos)
			{
				SliderMultiplier = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find(':') == string::npos)
			{
				Difficulty = false;
			}
		}
		else if (str.find("[TimingPoints]") != string::npos)
		{
			Timing = true;
		}
		else if (Timing)
		{
			if (str.find(',') == string::npos)
			{
				Timing = false;
			}
			else
			{
				TimingPoint TP = TimingPoint(str);
				TimingPoints.push_back(TP);
			}
		}
		else if (str.find("[HitObjects]") != string::npos)
		{
			Hits = true;
		}
		else if (Hits)
		{
			if (str.find(',') == string::npos)
			{
				Hits = false;
			}
			else
			{
				HitObject HO = HitObject(str, &TimingPoints, SliderMultiplier);
				HitObjects.push_back(HO);
			}
		}
	}
	t.close();
	float PreEmpt = mapDifficultyRange(OD, 1800.0f, 1200.0f, 450.0f);
	StackOffset = (512.0f / 16.0f) * (1.0f - 0.7f * (CircleSize - 5.0f) / 5.0f) / 10.0f;
	cout << StackOffset << endl;
	//for (int i = HitObjects.size() - 1; i > 0; i--) {
	//	HitObject* hitObjectI = &HitObjects[i];
	//	if (hitObjectI->getStack() != 0 || hitObjectI->itSpinner()) {
	//		continue;
	//	}

	//	for (int n = i - 1; n >= 0; n--) {
	//		HitObject* hitObjectN = &HitObjects[n];
	//		if (hitObjectN->itSpinner()) {
	//			continue;
	//		}
	//		// check if in range stack calculation
	//		float timeI = hitObjectI->startTime - PreEmpt * StackLeniency;
	//		float timeN = float(hitObjectN->itSlider() ? HitObjects[n].endTime : hitObjectN->startTime);
	//		if (timeI > timeN)
	//			break;
	//		
	//		if (hitObjectN->itSlider()) {
	//			vec2f p1 = HitObjects[i].getStartPosition();
	//			vec2f p2 = HitObjects[n].getEndPos();
	//			float distance = ::distance(p1, p2);

	//			// check if hit object part of this stack
	//			if (StackLeniency > 0.0f) {
	//				if (distance < circleRadius) {
	//					int offset = hitObjectI->getStack() - hitObjectN->getStack() + 1;
	//					for (int j = n + 1; j <= i; j++) {
	//						HitObject* hitObjectJ = &HitObjects[j];
	//						p1 = hitObjectJ->getStartPosition();
	//						distance = ::distance(p1, p2);
	//						//cout << offset;
	//						// hit object below slider end
	//						if (distance < circleRadius)
	//							hitObjectJ->setStack(hitObjectJ->getStack() - offset);
	//					}
	//					break;  // slider end always start of the stack: reset calculation
	//				}
	//			}
	//			else
	//			{
	//				break;
	//			}
	//		}
	//		auto distance = ::distance(
	//			hitObjectI->getStartPosition(),
	//			hitObjectN->getStartPosition()
	//			);
	//		if (distance < StackLeniency) {
	//			hitObjectN->setStack(hitObjectI->getStack() + 1);
	//			hitObjectI = hitObjectN;
	//		}
	//	}
	//}
	const int STACK_LENIENCE = 3;
	for (int i = HitObjects.size() - 1; i > 0; i--)
	{
		int n = i;
		/* We should check every note which has not yet got a stack.
		* Consider the case we have two interwound stacks and this will make sense.
		*
		* o <-1      o <-2
		*  o <-3      o <-4
		*
		* We first process starting from 4 and handle 2,
		* then we come backwards on the i loop iteration until we reach 3 and handle 1.
		* 2 and 1 will be ignored in the i loop because they already have a stack value.
		*/

		HitObject *objectI = &HitObjects[i];

		if (objectI->stackId != 0 || objectI->itSpinner()) continue;

		/* If this object is a hitcircle, then we enter this "special" case.
		* It either ends with a stack of hitcircles only, or a stack of hitcircles that are underneath a slider.
		* Any other case is handled by the "is Slider" code below this.
		*/
		if (objectI->endTime == 0)
		{
			while (--n >= 0)
			{
				HitObject* objectN = &HitObjects[n];

				if (objectN->itSpinner()) continue;

				//HitObjectSpannable spanN = objectN as HitObjectSpannable;
				float timeI = objectI->startTime - PreEmpt * StackLeniency;
				float timeN = float(objectN->itSlider() ? objectN->endTime : objectN->startTime);
				if (timeI > timeN)
					break;

				/* This is a special case where hticircles are moved DOWN and RIGHT (negative stacking) if they are under the *last* slider in a stacked pattern.
				*    o==o <- slider is at original location
				*        o <- hitCircle has stack of -1
				*         o <- hitCircle has stack of -2
				*/
				if (objectN->endTime != 0 && distance(objectN->getEndPos(), objectI->startPosition) < STACK_LENIENCE)
				{
					int offset = objectI->stackId - objectN->stackId + 1;
					for (int j = n + 1; j <= i; j++)
					{
						//For each object which was declared under this slider, we will offset it to appear *below* the slider end (rather than above).
						if (distance(objectN->getEndPos(), HitObjects[j].startPosition) < STACK_LENIENCE)
							HitObjects[j].stackId -= offset;
					}

					//We have hit a slider.  We should restart calculation using this as the new base.
					//Breaking here will mean that the slider still has StackCount of 0, so will be handled in the i-outer-loop.
					break;
				}

				if (distance(objectN->startPosition, objectI->startPosition) < STACK_LENIENCE)
				{
					//Keep processing as if there are no sliders.  If we come across a slider, this gets cancelled out.
					//NOTE: Sliders with start positions stacking are a special case that is also handled here.

					objectN->stackId = objectI->stackId + 1;
					objectI = objectN;
				}
			}
		}
		else if (objectI->itSlider())
		{
			/* We have hit the first slider in a possible stack.
			* From this point on, we ALWAYS stack positive regardless.
			*/
			while (--n >= 0)
			{
				HitObject* objectN = &HitObjects[n];

				if (objectN->itSpinner()) continue;

				//HitObjectSpannable spanN = objectN as HitObjectSpannable;

				if (objectI->startTime - (PreEmpt * StackLeniency) > objectN->startTime)
					break;

				if (distance((objectN->endTime != 0 ? objectN->getEndPos() : objectN->startPosition), objectI->startPosition) < STACK_LENIENCE)
				{
					objectN->stackId = objectI->stackId + 1;
					objectI = objectN;
				}
			}
		}
	}
}

void OpenSong()
{
	OPENFILENAME ofn;       // common dialog box structure
	char szFile[260];       // buffer for file name
	HWND hwnd = nullptr;              // owner window
	ZeroMemory(&ofn, sizeof ofn);
	ofn.lStructSize = sizeof ofn;
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "osu! map\0*.osu\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box.

	if(GetOpenFileName(&ofn)){
		ParseSong(ofn.lpstrFile);
	}
}


int main()
{
	//TimingPoint timingTest = TimingPoint("6590,461.538461538462,4,2,1,6,1,0");
	//AllocConsole();
	//SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	OpenSong();
	checkGame();
	getchar();
	return 0;
}

