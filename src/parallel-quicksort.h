/* C implementation QuickSort */
#include <omp.h>

void quickSort_parallel(uint32_t* array, int lenArray, int numThreads);
void quickSort_parallel_internal(uint32_t* array, int left, int right, int cutoff);

void quickSort_parallel(uint32_t* array, int lenArray, int numThreads){

	int cutoff = 1000;

	#pragma omp parallel num_threads(numThreads)
	{	
		#pragma omp single nowait
		{
			quickSort_parallel_internal(array, 0, lenArray-1, cutoff);	
		}
	}	

}

void quickSort_parallel_internal(uint32_t* array, int left, int right, int cutoff) 
{
	
	int i = left, j = right;


	/* Calculate pivot: 
	 * The closer the pivot is to the median, the less has to be swapped.
	 * The calculation of the median is too expensive, so we use the mean value of left right and center of array. 
	 */ 
	uint32_t pivot = (array[i] + array[(i + j) / 2] + array[j])/3;
	

	/* ------------------------- PARTITION PART ------------------------- */
	scalar_partition_epi32(array, pivot, i, j);


	/* ------------------------- RECURSION PART ------------------------- */
	// Cause managing threads is expensive we check for small blocks to get a balance between costs.
	if ( ((right-left)<cutoff) ){

		// Sequential
		if (left < j){ quickSort_parallel_internal(array, left, j, cutoff); }
		if (i < right){ quickSort_parallel_internal(array, i, right, cutoff); }

	} else {

		// Parallel
		#pragma omp task
		{ quickSort_parallel_internal(array, left, j, cutoff); }
		#pragma omp task
		{ quickSort_parallel_internal(array, i, right, cutoff); }
	}
}
