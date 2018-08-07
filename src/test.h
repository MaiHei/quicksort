#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include <omp.h>
#include <string.h>
#include <errno.h>   // for errno
#include <limits.h>  // for INT_MAX
#include <nmmintrin.h>
#include <cassert>
#include <immintrin.h>
#include <stdint.h>
#include <sys/time.h>

#include "qs-simd/partition.cpp"
#include "parallel-quicksort.h"
#include "qs-simd/avx2_quicksort.cpp"