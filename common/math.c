#include "typedefs.h"
#include "FPToolkit.c"

typedef struct { int x; int y; } int2;
int2 add_int2(int2 a, int2 b) { return (int2){a.x + b.x, a.y + b.y}; }

typedef struct { int x; int y; int z; } int3;
int3 add_int3(int3 a, int3 b) { return (int3){a.x + b.x, a.y + b.y, a.z + b.z}; }

double2 add_double2(double2 a, double2 b) { return (double2){a.x + b.x, a.y + b.y}; }

double3 add_double3(double3 a, double3 b) { return (double3){a.x + b.x, a.y + b.y, a.z + b.z}; }
double3 sub_double3(double3 a, double3 b) { return (double3){a.x - b.x, a.y - b.y, a.z - b.z}; }
double3 scale_double3(double3 a, double s) { return (double3){a.x * s, a.y * s, a.z * s}; }
double len_double3(double3 a) { return sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }
double lensq_double3(double3 a) { return a.x * a.x + a.y * a.y + a.z * a.z; }
double3 normalize_double3(double3 a) { return scale_double3(a, 1.0 / len_double3(a)); }

double lerp(double a, double b, double t) {
	return a + t * (b - a);
}
double2 lerp_double2(double2 a, double2 b, double t) {
	return (double2){
		lerp(a.x, b.x, t),
		lerp(a.y, b.y, t),
	};
}
double3 lerp_double3(double3 a, double3 b, double t) {
	return (double3){
		lerp(a.x, b.x, t),
		lerp(a.y, b.y, t),
		lerp(a.z, b.z, t),
	};
}

// https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
double hue2rgb(double3 pqt) {
	double p = pqt.x, q = pqt.y, t = pqt.z;
	if (t < 0.0) { t += 1; }
	if (t > 1.0) { t -= 1; }
	if (t < 1.0/6) { return p + (q - p) * 6 * t; }
	if (t < 1.0/2) { return q; }
	if (t < 2.0/3) { return p + (q - p) * (2.0/3 - t) * 6; }
	return p;
}
double3 hsl2rgb(double3 hsl) {
	double h = hsl.x, s = hsl.y, l = hsl.z;
	if (s == 0) { return (double3) {l, l, l}; }
	double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
	double p = 2 * l - q;
	return (double3){
		hue2rgb((double3){p, q, h + 1.0/3}),
		hue2rgb((double3){p, q, h}),
		hue2rgb((double3){p, q, h - 1.0/3}),
	};
}

double rand_double() {
	return drand48();
}
double rand_double_range(double min, double max) {
	return min + (max - min) * drand48();
}
double3 rand_point_on_sphere() {
	double3 p;
	double len;
	do {
		p = (double3) {
			drand48() * 2 - 1,
			drand48() * 2 - 1,
			drand48() * 2 - 1,
		};
		len = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
	} while (len > 1 || len == 0);
	return scale_double3(p, 1/len); // normalize
}
double3 rand_point_on_cube() {
	double3 p;
	double r = rand_double();
	double a = rand_double() < 0.5 ? -0.5 : 0.5;
	double u = rand_double_range(-0.5, 0.5);
	double v = rand_double_range(-0.5, 0.5);
	if (r < 1.0/3) {
		return (double3) {a, u, v};
	} else if (r < 2.0/3) {
		return (double3) {u, a, v};
	} else {
		return (double3) {u, v, a};
	}
}
