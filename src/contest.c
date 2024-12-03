#include "contest.h"

inline int min(int x, int y) { return y ^ ((x ^ y) & -(x < y)); }
inline int max(int x, int y) { return x ^ ((x ^ y) & -(x < y)); }