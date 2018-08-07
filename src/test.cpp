#include "test.h"

int numthreads = 8;
int maxNumbersDisplayed = 30;

// Comparator used in qsort()
int cmpfunc (const void * a, const void * b)
{
	return ( *(int*)a - *(int*)b );
}

// Used for SIMD
uint32_t get_time() {
	struct timeval T;
	gettimeofday(&T, NULL);
	return ((T.tv_sec * 1000000) + T.tv_usec)/1000;
}

// Serial quicksort
void quickSort(uint32_t* array, int left, int right) 
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
	if (left < j){ quickSort(array, left, j); }
	if (i < right){ quickSort(array, i, right); }

}


void printArray(int length, uint32_t* array) 
{
	if( length <= maxNumbersDisplayed ) 
	{
		for(int i = 0 ; i < length; i++ ) 
		{
			printf("%d ", array[i]);
		}
		printf("\n\n\n");
	}
}

bool compareArrays(int length, uint32_t* array1, uint32_t* array2)
{
	bool correctResult=true;
	int i = 0;
	while( (correctResult==true) && (i<length) )
	{
		if(array1[i]!=array2[i]) { correctResult=false; }
		i++;
	}
	return correctResult;
}



void singleTest (int length)
{

	// -------------------------------------------------------------------------------------- //
	//			                 Declarations and Allocation								  //
	// -------------------------------------------------------------------------------------- //

	int minMum = 1;
	int maxNum = length;

	double startTime, stopTime;
	double qsortTime, serialTime, ompTime, simdTime, ompSimdTime;

	uint32_t* arr1;			// Default
	uint32_t* arr2;  		// std::sort
	uint32_t* arr3;			// custom
	arr1 = (uint32_t*) malloc(length*sizeof(uint32_t));
	arr2 = (uint32_t*) malloc(length*sizeof(uint32_t));
	arr3 = (uint32_t*) malloc(length*sizeof(uint32_t));
	
	printf("Length:          %3.0E\n", (double)length);
	printf("Memory:          %i MBytes\n\n", (int)(3*length*sizeof(uint32_t)/(1024*1024))); // Mbytes



	// -------------------------------------------------------------------------------------- //
	//                     Initialization of random number array							  //
	// -------------------------------------------------------------------------------------- //

	int i;
	srand(5); // seed
	for (i=0; i<length; i++){
		arr1[i] = (uint32_t)(minMum+(rand()%maxNum));
		arr2[i] = arr1[i];
		arr3[i] = arr1[i];
	}
	
	printArray(length, arr1);



	// -------------------------------------------------------------------------------------- //
	//                               std::sort from stdlib.h					 			  //
	// -------------------------------------------------------------------------------------- //

	// Sort
	startTime = get_time();
	qsort(arr2, length, sizeof(int), cmpfunc);
	stopTime = get_time();

	printArray(length, arr2);

	// Calculate and print time
	qsortTime = (double)(stopTime-startTime)/CLOCKS_PER_SEC;
	printf("std::sort:       %f s\n", qsortTime);



	// -------------------------------------------------------------------------------------- //
	//                               	serial quicksort						 			  //
	// -------------------------------------------------------------------------------------- //

	// Sort
	startTime = get_time();
	quickSort(arr3, 0, length-1);
	stopTime = get_time();

	printArray(length, arr3);

	// Validate results
	if(!compareArrays(length, arr2, arr3))
	{
		printf("The result with 'custom serial QuickSort' is !!INCORRECT!!\n");
	}

	// Calculate and print time
	serialTime = (double)(stopTime-startTime)/CLOCKS_PER_SEC;
	printf("Serial:          %f s\t%f\n", serialTime, (1/(serialTime/qsortTime)));



	// -------------------------------------------------------------------------------------- //
	//                              		 omp quicksort						 			  //
	// -------------------------------------------------------------------------------------- //

	// Reset Array
	for (int i = 0; i<length;i++) {
		arr3[i] = arr1[i];
	}

	// Sort
	startTime = omp_get_wtime();
	quickSort_parallel(arr3, length, numthreads);
	stopTime = omp_get_wtime();

	printArray(length, arr3);

	// Validate results
	if(!compareArrays(length, arr2, arr3))
	{
		printf("The result with 'custom omp QuickSort' is ¡¡INCORRECT!!\n");
	}

	// Calculate and print time
	ompTime = (stopTime-startTime);
	printf("OMP:             %f s\t%f\n", ompTime, (1/(ompTime/qsortTime)));



	// -------------------------------------------------------------------------------------- //
	//                              		 simd quicksort						 			  //
	// -------------------------------------------------------------------------------------- //

	// Reset Array
	for (int i = 0; i<length;i++) {
		arr3[i] = arr1[i];
	}

	// Sort
	startTime = get_time();
	::qs::avx2::quicksort(arr3, 0, length-1);
	stopTime = get_time();

	printArray(length, arr3);

	// Validate results
	if(!compareArrays(length, arr2, arr3))
	{
		printf("The result with 'custom simd QuickSort' is ¡¡INCORRECT!!\n");
	}

	// Calculate and print time
	simdTime = (double)(stopTime-startTime)/CLOCKS_PER_SEC;
	printf("SIMD:            %f s\t%f\n", simdTime, (1/(simdTime/qsortTime)));



	// -------------------------------------------------------------------------------------- //
	//                              	 omp simd quicksort							 		  //
	// -------------------------------------------------------------------------------------- //

	// Reset Array
	for (int i = 0; i<length;i++) {
		arr3[i] = arr1[i];
	}

	// Sort
	startTime = omp_get_wtime();
	::qs::avx2::ompQuicksort(arr3, length, numthreads);
	stopTime = omp_get_wtime();

	printArray(length, arr3);

	// Validate results
	if(!compareArrays(length, arr2, arr3))
	{
		printf("The result with 'custom omp simd QuickSort' is ¡¡INCORRECT!!\n");
	}

	// Calculate and print time
	ompSimdTime = (stopTime-startTime);
	printf("OMP & SIMD:      %f s\t%f\n", ompSimdTime, (1/(ompSimdTime/qsortTime)));



	// -------------------------------------------------------------------------------------- //
	//                        	 	Outputs ans deallocation					 			  //
	// -------------------------------------------------------------------------------------- //

	printf("\n---------------------------------------------\n\n");

	free(arr1);
	free(arr2);
	free(arr3);
}



int main(){

	int lengths[] = {
		10000,
		100000,
		1000000,
		10000000,
		100000000
		};

	for (int i=0; i<(int)(sizeof(lengths) / sizeof(int)); i++)
	{
		singleTest(lengths[i]);
	}

	return 0;
}