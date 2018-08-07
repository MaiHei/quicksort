#include <x86intrin.h>
#include <cstdint>


// Needed to check for 0 in bytemasks for values < pivot.
#define _mm256_iszero(vec) (_mm256_testz_si256(vec, vec) != 0)


namespace qs {

    namespace avx2 {


        /*
         *  converts a bitmask to a bytemask
         *
         * Params:
         * uint8_t      bm          -->     Bitmask
         *
         * Returns: 
         * __m256i                  -->     Bytemask
         */
        __m256i FORCE_INLINE bitmask_to_bytemask_epi32(uint8_t bm) {

            const __m256i mask = _mm256_set1_epi32(bm);
            const __m256i bits = _mm256_setr_epi32(0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80);
            const __m256i tmp  = _mm256_and_si256(mask, bits);

            return _mm256_cmpeq_epi32(tmp, bits);
        }


        /*
         * Searchs for indices to swap in masks and creates integer vectors with these indices. 
         *
         * Params: 
         * uint8_t      a           -->     Left mask
         * uint8_t      b           -->     Right mask
         * uint8_t      rem_a       -->     Mask for left detected swaps
         * uint8_t      rem_b       -->     Mask for right detected swaps
         * __m256i      shuffle_a   -->     Integer vector with left indices
         * __m256i      shuffle_b   -->     Integer vector with right indices
         *
         */
        void FORCE_INLINE align_masks(uint8_t& a, uint8_t& b, uint8_t& rem_a, uint8_t& rem_b, __m256i& shuffle_a, __m256i& shuffle_b) {

            // Validate input
            assert(a != 0);
            assert(b != 0);

            uint8_t tmpA = a;
            uint8_t tmpB = b;

            // Needed to temporary store indices for swapping. 
            uint32_t __attribute__((__aligned__(32))) tmpshufa[8];
            uint32_t __attribute__((__aligned__(32))) tmpshufb[8];

            // Align masks
            while (tmpA != 0 && tmpB != 0) {

                // Get indices of leading ones.
                int idx_a = __builtin_ctz(tmpA);
                int idx_b = __builtin_ctz(tmpB);

                // Switch last 1 to 0
                tmpA = tmpA & (tmpA - 1);
                tmpB = tmpB & (tmpB - 1);

                // Store indices which are needed to swap. 
                tmpshufb[idx_a] = idx_b;
                tmpshufa[idx_b] = idx_a;
            }

            // Merge changes
            a = a ^ tmpA;
            b = b ^ tmpB;

            // Validate changes
            assert(a != 0);
            assert(b != 0);
            assert(_mm_popcnt_u64(a) == _mm_popcnt_u64(b));

            rem_a = tmpA;
            rem_b = tmpB;

            // Load to integer vector
            shuffle_a = _mm256_load_si256((__m256i*)tmpshufa);
            shuffle_b = _mm256_load_si256((__m256i*)tmpshufb);
        }


        /*
         * 
         *
         * Params:
         * __m256i      mask        -->     Integer vector containing bytemask
         * __m256i      a           -->     Integer vector containing values to swap
         * __m256i      b           -->     Integer vector containing values to swap
         *
         * Returns:
         * __m256i                  -->     Integer vector of swapped values
         *
         */
        __m256i FORCE_INLINE merge(const __m256i mask, const __m256i a, const __m256i b) {

            return _mm256_or_si256(
                    _mm256_and_si256(mask, a),
                    _mm256_andnot_si256(mask, b)
            );
        }


        /*
         * Swap values in given integer vector at position described in given mask.
         *
         * Params:
         * __m256i      a           -->     Left integer vector
         * __m256i      b           -->     Right integer vector
         * uint8_t      mask_a      -->     Left mask
         * uint8_t      mask_b      -->     Right mask
         * __m256i      shuffle_a   -->     Integer vector with left indices
         * __m256i      shuffle_b   -->     Integer vector with right indices
         *
         */
       void FORCE_INLINE swap_epi32(
            __m256i& a, __m256i& b,
            uint8_t mask_a, const __m256i shuffle_a,
            uint8_t mask_b, const __m256i shuffle_b) {

            // Copy values to swap in new integer vector
            const __m256i to_swap_b = _mm256_permutevar8x32_epi32(a, shuffle_a);
            const __m256i to_swap_a = _mm256_permutevar8x32_epi32(b, shuffle_b);

            // Generating masks
            const __m256i ma    = bitmask_to_bytemask_epi32(mask_a);
            const __m256i mb    = bitmask_to_bytemask_epi32(mask_b);

            // Swap
            a = merge(ma, to_swap_a, a);
            b = merge(mb, to_swap_b, b);
        }


        /*
         *  SIMD Partition part for quicksort algorithm.
         *  With a 256 bit register this function compares 8 32 bit integers at once. 
         *
         *  Params:
         *  uint32_t*   array       -->     Array to sort
         *  uint32_t    pv          -->     Pivot element for comparison
         *  int         left        -->     Lower index
         *  int         right       -->     Higher index
         *
         */
        void FORCE_INLINE partition_epi32(uint32_t* array, uint32_t pv, int& left, int& right) {

            // the number of items in a register (256/32)
            const int N = 8; 

            __m256i L;
            __m256i R;
            uint8_t maskL = 0;
            uint8_t maskR = 0;

            // Load pivot into integer vector
            const __m256i pivot = _mm256_set1_epi32(pv);

            int origL = left;
            int origR = right;

            while (true) {

                // Check left side for values lower than pivot
                if (maskL == 0) {
                    while (true) {

                        // Check if distance between left and right index is lower than 3N-1 (Minimal distance for a calculation)
                        if (right - (left + N) + 1 < 2*N) {
                            goto end;
                        }

                        // Load left side of array into integer vector
                        L = _mm256_loadu_si256((__m256i*)(array + left));

                        // Compares pivot with loaded values from array (L).
                        // Returns mask with 1 for pivot > L and 0 for pivot < L.
                        const __m256i bytemask = _mm256_cmpgt_epi32(pivot, L);

                        // Check if bytemask contains values greater than pivot
                        if (_mm256_testc_ps((__m256)bytemask, (__m256)_mm256_set1_epi32(-1))) {
                            // No swap needed. Increment left by number of items in a register. 
                            left += N;
                        } else {
                            // Swap needed.
                            maskL = ~_mm256_movemask_ps((__m256)bytemask);
                            break;
                        }
                    }

                }

                // Check right side for values greater than pivot
                if (maskR == 0) {
                    while (true) {

                        // Check if distance between left and right index is lower than 3N-1 (Minimal distance for a calculation)
                        if ((right - N) - left + 1 < 2*N) {
                            goto end;
                        }

                        // Load right side of array into integer vector
                        R = _mm256_loadu_si256((__m256i*)(array + right - N + 1));
                        
                        // Compares pivot with loaded values from array (L).
                        // Returns mask with 1 for pivot > L and 0 for pivot < L.
                        const __m256i bytemask = _mm256_cmpgt_epi32(pivot, R);

                        // Check if bytemask contains values lower than pivot
                        if (_mm256_iszero(bytemask)) {
                            // No swap needed. Decrement right by number of items in a register. 
                            right -= N;
                        } else {
                            // Swap needed.
                            maskR = _mm256_movemask_ps((__m256)bytemask);
                            break;
                        }
                    }

                }

                // Check if loops worked correct.
                assert(left <= right);
                assert(maskL != 0);
                assert(maskR != 0);

                uint8_t mL;
                uint8_t mR;
                __m256i shuffleL;
                __m256i shuffleR;

                // Sync masks and swap values
                align_masks(maskL, maskR, mL, mR, shuffleL, shuffleR);
                swap_epi32(L, R, maskL, shuffleL, maskR, shuffleR);

                // Set masks to detected swaps from aligned_masks
                maskL = mL;
                maskR = mR;

                // Write swapped values back to array for left side
                if (maskL == 0) {
                    _mm256_storeu_si256((__m256i*)(array + left), L);
                    left += N;
                }

                // Write swapped values back to array for right side
                if (maskR == 0) {
                    _mm256_storeu_si256((__m256i*)(array + right - N + 1), R);
                    right -= N;
                }

            } // while

        // Called when while loop from above ends
        end:

            assert(!(maskL != 0 && maskR != 0));

            // Write all values back to array
            if (maskL != 0) {
                _mm256_storeu_si256((__m256i*)(array + left), L);
            } else if (maskR != 0) {
                _mm256_storeu_si256((__m256i*)(array + right - N + 1), R);
            }

            /* Check if all values are compared. 
             * If left < right there are still values left that are not compared.
             * This effect is caused by the stepwidth on every iteration by the above while loop.
             * The loop ends if the distance between left and right is lower than 2 * Stepwidth. 
             * So there can be (2*Stepwidth-1) values left uncompared.
             * These values need to be comared withoud SIMD. 
             */
            if (left < right) {
                int less    = 0;
                int greater = 0;
                const int all = right - left + 1;

                // Compare left values with pivot
                for (int i=left; i <= right; i++) {
                    less    += int(array[i] < pv);
                    greater += int(array[i] > pv);
                }

                if (all == less) {
                    // all elements in range [left, right] less than pivot
                    scalar_partition_epi32(array, pv, origL, left);
                } else if (all == greater) {
                    // all elements in range [left, right] greater than pivot
                    scalar_partition_epi32(array, pv, left, origR);
                } else {
                    scalar_partition_epi32(array, pv, left, right);
                }
            }
        }

    } // namespace avx2

} // namespace qs