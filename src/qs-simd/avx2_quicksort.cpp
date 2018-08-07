#include "common.h"
#include "avx2_partition.cpp"

namespace qs {

    namespace avx2 {

        // Entry point for SIMD implementation of quicksort.
        void quicksort(uint32_t* array, int left, int right) {

            int i = left;
            int j = right;

            /* Calculate pivot: 
            * The closer the pivot is to the median, the less has to be swapped.
            * The calculation of the median is too expensive, so we use the mean value of left right and center of array. 
            */ 
            const uint32_t pivot = (array[i] + array[(i + j) / 2] + array[j])/3;

            const int AVX2_REGISTER_SIZE = 8; // in 32-bit words


	        /* ------------------------- PARTITION PART ------------------------- */
            if (j - i >= 2 * AVX2_REGISTER_SIZE) {
                qs::avx2::partition_epi32(array, pivot, i, j);
            } else {
                scalar_partition_epi32(array, pivot, i, j);
            }


            /* ------------------------- RECURSION PART ------------------------- */
            if (left < j) {
                quicksort(array, left, j);
            }

            if (i < right) {
                quicksort(array, i, right);
            }
        }

        void ompQuicksortInternal(uint32_t* array, int left, int right, int cutoff) {
            
            int i = left;
            int j = right;

            /* Calculate pivot: 
            * The closer the pivot is to the median, the less has to be swapped.
            * The calculation of the median is too expensive, so we use the mean value of left right and center of array. 
            */ 
            const uint32_t pivot = (array[i] + array[(i + j) / 2] + array[j])/3;

            const int AVX2_REGISTER_SIZE = 8; // in 32-bit words


	        /* ------------------------- PARTITION PART ------------------------- */
            if (j - i >= 2 * AVX2_REGISTER_SIZE) {
                qs::avx2::partition_epi32(array, pivot, i, j);
            } else {
                scalar_partition_epi32(array, pivot, i, j);
            }


            /* ------------------------- RECURSION PART ------------------------- */
            // Cause managing threads is expensive we check for small blocks to get a balance between costs.
            if ( ((right-left)<cutoff) ){

                // Sequential
                if (left < j){ ompQuicksortInternal(array, left, j, cutoff); }
                if (i < right){ ompQuicksortInternal(array, i, right, cutoff); }

            } else {

                // Parallel
                #pragma omp task
                { ompQuicksortInternal(array, left, j, cutoff); }
                #pragma omp task
                { ompQuicksortInternal(array, i, right, cutoff); }

            }
        }
        
        // Entrypoint for SIMD and OMP implementation of quicksort
        void ompQuicksort(uint32_t* array, int lenArray, int numThreads) {

            int cutoff = 1000;

            #pragma omp parallel num_threads(numThreads)
            {	
                #pragma omp single nowait
                {
                    ompQuicksortInternal(array, 0, lenArray-1, cutoff);	
                }
            }	

        }

    } // namespace avx2

} // namespace qs