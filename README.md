# Quicksort Speed-up Test
## Description
This repository contains a speed-up test for quicksort in different ways:

- Serial implementation
- Parallel implementation with OpenMP
- Parallel implementation with SIMD
- Parallel implementation with OpenMP and SIMD

All sorted arrays will be validated against an array sorted by sort-method from stdlib. Also all measured times will be logged to console in an absolute time in seconds and a relative time against the sort-method from stdlib.

## Set up
Caused by a quick and dirty solution for my windows machine without make, this repository contains a gulp file to build this project. To run this gulp task type in console `npm i` and after this installation run `gulp`.
For these script npm and node are required. If not installed, you can simply run the following command to build this project:
`g++ -fopenmp -std=c++11 -mbmi2 -Wall -Wpedantic -Wextra -mavx2 -DHAVE_AVX2_INSTRUCTIONS -march=haswell -O3 -DNDEBUG src/test.cpp -o build/test`

## Sources
This project is a mix of some existing implementations of quicksort.
SIMD-Implementation: [simd-sort by WojciechMula](https://github.com/WojciechMula/simd-sort)
OpenMP-Implementation: [quicksort-parallel by eduardlopez](https://github.com/eduardlopez/quicksort-parallel)
