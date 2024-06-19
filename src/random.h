#pragma once

#include "Vec3.h"

inline float randomCap() {
	return rand() / (RAND_MAX + 1.0f);
}

Vec3 randomNormalSphere();
Vec3 randomNormalDisk();
