/*
 *  Calculates the partition part in a scalar (serial) way. 
 *
 *  Params:
 *  uint32_t*   array       -->     Array to sort
 *  uint32_t    pv          -->     Pivot element for comparison
 *  int         left        -->     Lower index
 *  int         right       -->     Higher index
 *
 */
void scalar_partition_epi32(uint32_t* array, const uint32_t pivot, int& left, int& right) {

    // While left <= right
    while (left <= right) {

		// Serach on left side for a value > pivot
        while (array[left] < pivot) {
            left += 1;
        }

		// Search on right side for a value < pivot
        while (array[right] > pivot) {
            right -= 1;
        }

		// Swap values
        if (left <= right) {
            const uint32_t t = array[left];
            array[left]      = array[right];
            array[right]     = t;

            left  += 1;
            right -= 1;
        }
    }
    
}
