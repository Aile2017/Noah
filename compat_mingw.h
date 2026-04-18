// MinGW/Clang C++ compatibility: restore min/max macros suppressed by __cplusplus guard
#pragma once
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
