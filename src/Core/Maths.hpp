#pragma once

template <typename T> inline T clamp(T value, T minVal, T maxVal) {
  return (value < minVal) ? minVal : (value > maxVal ? maxVal : value);
}

template <typename T> inline T lerp(T start, T end, float t) {
  return start + (end - start) * t;
}
