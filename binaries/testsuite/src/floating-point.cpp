#include "floating-point.hpp"
#include <algorithm>
#include <cmath>

float test_fadd(float a, float b) {
	return a + b;
}
float test_fsub(float a, float b) {
	return a - b;
}
float test_fmul(float a, float b) {
	return a * b;
}
float test_fdiv(float a, float b) {
	return a / b;
}
float test_fmax(float a, float b) {
	return std::max(a, b);
}
float test_fmin(float a, float b) {
	return std::min(a, b);
}
double test_ftod(float val) {
	return (double) val;
}
float  test_dtof(double val) {
	return (float) val;
}

float  test_fneg(float val) {
	return -val;
}
double test_dneg(double val) {
	return -val;
}

float test_fmadd(float a, float b, float c) {
	return a * b + c;
}
float test_fmsub(float a, float b, float c) {
	return a * b - c;
}
float test_fnmadd(float a, float b, float c) {
	return -(a * b) + c;
}
float test_fnmsub(float a, float b, float c) {
	return -(a * b) - c;
}

float test_fsqrt(float val) {
	return std::sqrt(val);
}

float test_sinf(float val) {
	return std::sin(val);
}
float test_cosf(float val) {
	return std::cos(val);
}
float test_tanf(float val) {
	return std::tan(val);
}
