#pragma once

#ifdef EMSCRIPTEN
#include<emscripten/emscripten.h>
#define GLFW_INCLUDE_ES3

#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>

#include <math.h>
#include <stdio.h>

#include <functional>

#define DEBUG_GROUPS

// these two are pretty useful, when debugging in RenderDoc or Nsight for instance.
inline void dpush(const char* str) {
#ifdef DEBUG_GROUPS
#ifndef EMSCRIPTEN
	glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, str);
#endif
#endif
}
inline void dpop() {
#ifdef DEBUG_GROUPS
#ifndef EMSCRIPTEN
	glad_glPopDebugGroup();
#endif
#endif
}

class vec2 {
public:
	float x, y;

	vec2(float x, float y) { this->x = x; this->y = y; }

	vec2() { this->x = this->y = 0; }
};

//
//
// Begin vec3.
//
//

inline float clamp(float v, float min, float max) { if (v < min) { return min; } else if (v > max) { return max; } else { return v; } }

class vec3 {
public:
	float x, y, z;

	vec3(float x, float y, float z) { this->x = x; this->y = y; this->z = z; }

	vec3() { this->x = this->y = this->z = 0; }

	vec3& operator+=(const vec3& b) { (*this) = (*this) + b; return (*this); }

	friend vec3 operator-(const vec3& a, const vec3& b) { return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
	friend vec3 operator+(const vec3& a, const vec3& b) { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
	friend vec3 operator*(const float s, const vec3& a) { return vec3(s * a.x, s * a.y, s * a.z); }
	friend vec3 operator*(const vec3& a, const float s) { return s * a; }

	static float length(const vec3& a) { return (float)sqrt(vec3::dot(a, a)); }

	// dot product.
	static float dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

	static float distance(const vec3& a, const vec3& b) { return length(a - b); }
	static vec3 normalize(const vec3& a) { return (1.0f / vec3::length(a)) * a; }

	static vec3 mix(const vec3& x, const vec3& y, const float a) { return x * (1.0f - a) + y * a; }


	// cross product.
	static vec3 cross(const vec3& a, const vec3& b) { return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }

	//
	// Rotate the vector 'v' around the 'axis' for 'theta' degrees.
	//
	static vec3 rotate(const vec3& v, const float theta, const vec3& axis) {
		vec3 k = vec3::normalize(axis); // normalize for good measure.
		return v * (float)cos(theta) + vec3::cross(k, v) * (float)sin(theta) + (k * vec3::dot(k, v)) * (1.0f - (float)cos(theta));
	}

	static vec3 clamp(const vec3& v, const float min, const float max) {
		return vec3(
			::clamp(v.x, min, max), ::clamp(v.y, min, max), ::clamp(v.z, min, max)
			//0.0f, 0.0f, 0.0f
		);
	}

};

class vec4 {
public:
	float x, y, z, w;

	vec4(float x, float y, float z, float w) { this->x = x; this->y = y; this->z = z; this->w = w; }

	vec4() { this->x = this->y = this->z = 0; this->w = 0; }

	vec4& operator+=(const vec4& b) { (*this) = (*this) + b; return (*this); }

	friend vec4 operator-(const vec4& a, const vec4& b) { return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
	friend vec4 operator+(const vec4& a, const vec4& b) { return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
	friend vec4 operator*(const float s, const vec4& a) { return vec4(s * a.x, s * a.y, s * a.z, s * a.w); }
	friend vec4 operator*(const vec4& a, const float s) { return s * a; }
	/*
	static float length(const vec3& a) { return sqrt(vec3::dot(a, a)); }

	// dot product.
	static float dot(const vec3& a, const vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

	static float distance(const vec3& a, const vec3& b) { return length(a - b); }
	static vec3 normalize(const vec3& a) { return (1.0f / vec3::length(a)) * a; }
	*/
	//
	// Rotate the vector 'v' around the 'axis' for 'theta' degrees.
	// This is basically Rodrigues' rotation formula.
	//
};

//
//
// Begin mat4
//
//

class mat4 {
public:
	float m[4][4];

	mat4() {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				m[i][j] = (i == j) ? 1.0f : 0.0f;
			}
		}
	}

	mat4 inverse() {

		float inv[16], det;

		float m00 = m[0][0];
		float m01 = m[1][0];
		float m02 = m[2][0];
		float m03 = m[3][0];

		float m10 = m[0][1];
		float m11 = m[1][1];
		float m12 = m[2][1];
		float m13 = m[3][1];

		float m20 = m[0][2];
		float m21 = m[1][2];
		float m22 = m[2][2];
		float m23 = m[3][2];

		float m30 = m[0][3];
		float m31 = m[1][3];
		float m32 = m[2][3];
		float m33 = m[3][3];


		inv[0] = m11 * m22 * m33 -
			m11 * m23 * m32 -
			m21 * m12 * m33 +
			m21 * m13 * m32 +
			m31 * m12 * m23 -
			m31 * m13 * m22;

		inv[4] = -m10 * m22 * m33 +
			m10 * m23 * m32 +
			m20 * m12 * m33 -
			m20 * m13 * m32 -
			m30 * m12 * m23 +
			m30 * m13 * m22;

		inv[8] = m10 * m21 * m33 -
			m10 * m23 * m31 -
			m20 * m11 * m33 +
			m20 * m13 * m31 +
			m30 * m11 * m23 -
			m30 * m13 * m21;

		inv[12] = -m10 * m21 * m32 +
			m10 * m22 * m31 +
			m20 * m11 * m32 -
			m20 * m12 * m31 -
			m30 * m11 * m22 +
			m30 * m12 * m21;

		inv[1] = -m01 * m22 * m33 +
			m01 * m23 * m32 +
			m21 * m02 * m33 -
			m21 * m03 * m32 -
			m31 * m02 * m23 +
			m31 * m03 * m22;

		inv[5] = m00 * m22 * m33 -
			m00 * m23 * m32 -
			m20 * m02 * m33 +
			m20 * m03 * m32 +
			m30 * m02 * m23 -
			m30 * m03 * m22;

		inv[9] = -m00 * m21 * m33 +
			m00 * m23 * m31 +
			m20 * m01 * m33 -
			m20 * m03 * m31 -
			m30 * m01 * m23 +
			m30 * m03 * m21;

		inv[13] = m00 * m21 * m32 -
			m00 * m22 * m31 -
			m20 * m01 * m32 +
			m20 * m02 * m31 +
			m30 * m01 * m22 -
			m30 * m02 * m21;

		inv[2] = m01 * m12 * m33 -
			m01 * m13 * m32 -
			m11 * m02 * m33 +
			m11 * m03 * m32 +
			m31 * m02 * m13 -
			m31 * m03 * m12;

		inv[6] = -m00 * m12 * m33 +
			m00 * m13 * m32 +
			m10 * m02 * m33 -
			m10 * m03 * m32 -
			m30 * m02 * m13 +
			m30 * m03 * m12;

		inv[10] = m00 * m11 * m33 -
			m00 * m13 * m31 -
			m10 * m01 * m33 +
			m10 * m03 * m31 +
			m30 * m01 * m13 -
			m30 * m03 * m11;

		inv[14] = -m00 * m11 * m32 +
			m00 * m12 * m31 +
			m10 * m01 * m32 -
			m10 * m02 * m31 -
			m30 * m01 * m12 +
			m30 * m02 * m11;

		inv[3] = -m01 * m12 * m23 +
			m01 * m13 * m22 +
			m11 * m02 * m23 -
			m11 * m03 * m22 -
			m21 * m02 * m13 +
			m21 * m03 * m12;

		inv[7] = m00 * m12 * m23 -
			m00 * m13 * m22 -
			m10 * m02 * m23 +
			m10 * m03 * m22 +
			m20 * m02 * m13 -
			m20 * m03 * m12;

		inv[11] = -m00 * m11 * m23 +
			m00 * m13 * m21 +
			m10 * m01 * m23 -
			m10 * m03 * m21 -
			m20 * m01 * m13 +
			m20 * m03 * m11;

		inv[15] = m00 * m11 * m22 -
			m00 * m12 * m21 -
			m10 * m01 * m22 +
			m10 * m02 * m21 +
			m20 * m01 * m12 -
			m20 * m02 * m11;

		det = m00 * inv[0] + m01 * inv[4] + m02 * inv[8] + m03 * inv[12];

		if (det == 0)
			printf("Determinant() = 0, so the matrix can not be inversed");

		det = 1.0f / det;

		mat4 res;

		res.m[0][0] = inv[0] * det;
		res.m[1][0] = inv[1] * det;
		res.m[2][0] = inv[2] * det;
		res.m[3][0] = inv[3] * det;

		res.m[0][1] = inv[4] * det;
		res.m[1][1] = inv[5] * det;
		res.m[2][1] = inv[6] * det;
		res.m[3][1] = inv[7] * det;

		res.m[0][2] = inv[8] * det;
		res.m[1][2] = inv[9] * det;
		res.m[2][2] = inv[10] * det;
		res.m[3][2] = inv[11] * det;

		res.m[0][3] = inv[12] * det;
		res.m[1][3] = inv[13] * det;
		res.m[2][3] = inv[14] * det;
		res.m[3][3] = inv[15] * det;

		return res;
	}

	// return perspective projection matrix.
	static mat4 perspective(float fovy, float aspect, float zNear, float zFar) {

		mat4 m;

		float ymax = zNear * (float)tan(fovy);
		float xmax = ymax * aspect;

		float left = -xmax;
		float right = +xmax;

		float bottom = -ymax;
		float top = +ymax;

		float f1, f2, f3, f4;
		f1 = 2.0f * zNear;
		f2 = right - left;
		f3 = top - bottom;
		f4 = zFar - zNear;

		m.m[0][0] = f1 / f2;
		m.m[1][1] = f1 / f3;
		m.m[2][2] = (-zFar - zNear) / f4;

		m.m[2][0] = (right + left) / f2;
		m.m[2][1] = (top + bottom) / f3;

		m.m[2][3] = -1;
		m.m[3][2] = (-zFar * f1) / f4;

		m.m[3][3] = 0.0f;
		return m;
	}

	static mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up)
	{
		mat4 m;
		// compute the basis vectors.
		vec3 forward = vec3::normalize(center - eye); // forward vector.
		vec3 left = vec3::normalize(vec3::cross(forward, up)); // left vector.
		vec3 u = vec3::cross(left, forward); // up vector.

		m.m[0][0] = left.x;
		m.m[1][0] = left.y;
		m.m[2][0] = left.z;
		m.m[0][1] = u.x;
		m.m[1][1] = u.y;
		m.m[2][1] = u.z;
		m.m[0][2] = -forward.x;
		m.m[1][2] = -forward.y;
		m.m[2][2] = -forward.z;
		m.m[3][0] = -vec3::dot(left, eye);
		m.m[3][1] = -vec3::dot(u, eye);
		m.m[3][2] = vec3::dot(forward, eye);

		return m;
	}

	static mat4 orthographic(
		float left, float right,

		float bottom, float top,
		float znear, float zfar
	) {

		mat4 m;

		m.m[0][0] = 2.0f / (right - left);
		m.m[1][1] = 2.0f / (top - bottom);
		m.m[2][2] = 2.0f / (zfar - znear);

		m.m[3][0] = -(right + left) / (right - left);

		m.m[3][1] = -(top + bottom) / (top - bottom);
		m.m[3][2] = -(zfar + znear) / (zfar - znear);

		m.m[3][3] = 1.0f;

		return m;
	}

	friend mat4 operator*(const mat4& a, const mat4& b) {
		mat4 m;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				m.m[i][j] = 0.0f;
				for (int k = 0; k < 4; k++) {
					m.m[i][j] += a.m[i][k] * b.m[k][j];
				}
			}
		}
		return m;
	}

	friend vec4 operator*(const vec4& v, const mat4& m) {
		float res[4];
		res[0] = 0.0f;
		res[1] = 0.0f;
		res[2] = 0.0f;
		res[3] = 0.0f;

		res[0] += m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w;

		res[1] += m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w;

		res[2] += m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w;

		res[3] += m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w;

		return vec4(res[0], res[1], res[2], res[3]);
	}
};

//
//
// begin Camera
//
//
class Camera {
public:

	vec3 viewDir;
	vec3 right;
	vec3 up;
	vec3 position;

	double prevMouseX = 0.0;
	double prevMouseY = 0.0;

	double curMouseX = 0.0;
	double curMouseY = 0.0;

public:
	mat4 GetViewMatrix() {
		return mat4::lookAt(position, position + viewDir, up);

	}

	Camera(const vec3& position_, const vec3& viewDir_) : position(position_), viewDir(viewDir_) {
		viewDir = vec3::normalize(viewDir);
		right = vec3::normalize(vec3::cross(viewDir, vec3(0.0f, 0.0f, 1.0f)));
		up = vec3(0, 0.0f, 1);
	}

	void Update(const float delta);

	vec3 GetPosition() const {
		return position;
	}
};

constexpr int WINDOW_WIDTH = (1920*3)/4;
constexpr int WINDOW_HEIGHT = (1080*3)/4;

extern int fbWidth;
extern int fbHeight;

extern Camera camera;

void initGlfw(const std::function<void()>& fn);
void startRenderLoop(const std::function<void()>& fn);
