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
	vec2f midPoint(vec2f o) const
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
