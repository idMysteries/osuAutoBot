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
class vec2f {
public:
	float x, y;
	vec2f(float x, float y) : x(x), y(y) {}
	vec2f() : x(0.0f), y(0.0f) {}
	vec2f nor() {
		auto nx = -y, ny = x;
		x = nx;
		y = ny;
		return *this;
	}
	vec2f midPoint(const vec2f& o) const
	{
		return vec2f((x + o.x) / 2.0f, (y + o.y) / 2.0f);
	}
	vec2f sub(vec2f o) {
		x -= o.x;
		y -= o.y;
		return *this;
	}
	vec2f cpy() const
	{ return vec2f(x, y); }

	vec2f add(float nx, float ny) {
		x += nx;
		y += ny;
		return *this;
	}
	float len() const
	{ return sqrtf(x * x + y * y); }
	float LengthSquared() const
	{ return x*x + y*y;	}

	vec2f operator+=(const vec2f& alt) {
		x += alt.x;
		y += alt.y;
		return *this;
	}
};

inline vec2f rotate(vec2f point, float angle) {
	vec2f rotated_point;
	rotated_point.x = point.x * cos(angle) - point.y * sin(angle);
	rotated_point.y = point.x * sin(angle) + point.y * cos(angle);
	return rotated_point;
}

inline vec2f operator + (vec2f a, vec2f b)
	{
		return vec2f(a.x + b.x, a.y + b.y);
	}
inline vec2f operator - (vec2f a, vec2f b) {
	return vec2f(a.x - b.x, a.y - b.y);
}

inline vec2f operator * (float s, vec2f a) {
	return vec2f(s * a.x, s * a.y);
}
inline vec2f operator * (vec2f a, float s) {
	return vec2f(s * a.x, s * a.y);
}

inline vec2f operator / (vec2f a, float s) {
	return vec2f(a.x / s, a.y / s);
}
inline bool operator == (vec2f p1, vec2f p2) {
	return p1.x == p2.x && p1.y == p2.y;
}
