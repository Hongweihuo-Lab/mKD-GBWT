/* sdsl - succinct data structures library
    Copyright (C) 2011 Simon Gog

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/ .
*/
/*! \file rrr_helper.hpp
   \brief rrr_helper.hpp contains the sdsl::binomial class,
          a class which contains informations about the binomial coefficients
   \author Simon Gog, Matthias Petri, Stefan Arnold
*/
#ifndef SDSL_RRR_HELPER
#define SDSL_RRR_HELPER

#include <algorithm> // for next permutation
#include <iostream>
#include "bitmagic.hpp"
#include "uint128_t.hpp"
#include "uint256_t.hpp"

namespace sdsl
{

//! Trait class for the binomial coefficient class to handle different type of intergers
/*! This generic implementation works for 64-bit integers.
 */
template<uint16_t log_n>
struct binomial_coefficients_trait {
    typedef uint64_t number_type;
    static inline uint16_t l1BP(number_type x) {
        return bit_magic::l1BP(x);
    }

    //! Read a \f$len\f$-bit integer of type number_type from a bitvector.
    /*!
     *  \param bv 	A bit_vector of int_vector from which we extract the integer.
     *  \param pos  Position of the least significant bit of the integer which should be read.
     *  \param len  bit-width of the integer which should be read.
     *  \return The len-bit integer.
     */
    template<class bit_vector_type>
    static inline number_type get_int(const bit_vector_type& bv,
                                      typename bit_vector_type::size_type pos,
                                      uint16_t len) {
        return bv.get_int(pos, len);
    }

    //! Write a \f$len\f$-bit integer x of type number_type to a bitvector.
    /*!
     *	\param bv 	A bit_vecor or int_vector in which we write the integer.
     *  \param pos	Position of the least significant bit of the integer which should be written.
     *  \param x    The integer x which should be written.
     *  \param len  Bit-width of x.
     */
    template<class bit_vector_type>
    static void set_int(bit_vector_type& bv, typename bit_vector_type::size_type pos,
                        number_type x, uint16_t len) {
        bv.set_int(pos, x, len);
    }

    //! Count the number of set bits in x.
    /*!
     *	\param x The integer x.
     */
    static inline uint16_t popcount(number_type x) {
        return bit_magic::b1Cnt(x);
    }
};

//! Specialization of binomial_coefficients_trait for 128-bit integers.
template<>
struct binomial_coefficients_trait<7> {
    typedef uint128_t number_type;
    static inline uint16_t l1BP(number_type x) {
        if ((x >> 64)) {
            return bit_magic::l1BP(x >> 64) + 64;
        } else {
            return bit_magic::l1BP(x);
        }
    }

    template<class bit_vector_type>
    static inline number_type get_int(const bit_vector_type& bv,
                                      typename bit_vector_type::size_type pos,
                                      uint16_t len) {
        if (len <= 64) {
            return bv.get_int(pos, len);
        } else {
            return ((((number_type) bv.get_int(pos+64, len-64))<<64) + bv.get_int(pos, 64));
        }
    }

    template<class bit_vector_type>
    static void set_int(bit_vector_type& bv,
                        typename bit_vector_type::size_type pos,
                        number_type x, uint16_t len) {
        if (len <= 64) {
            bv.set_int(pos, x, len);
        } else {
            bv.set_int(pos, (uint64_t)x, 64); bv.set_int(pos+64, x>>64, len-64);
        }
    }

    static inline uint16_t popcount(number_type x) {
        return bit_magic::b1Cnt(x >> 64) + bit_magic::b1Cnt(x);
    }
};

//! Specialization of binomial_coefficients_trait for 256-bit integers.
template<>
struct binomial_coefficients_trait<8> {
    typedef uint256_t number_type;
    static inline uint16_t l1BP(number_type x) {
        return x.l1BP();
    }

    template<class bit_vector_type>
    static inline number_type get_int(const bit_vector_type& bv,
                                      typename bit_vector_type::size_type pos,
                                      uint16_t len) {
        if (len <= 64) {
            return number_type(bv.get_int(pos, len));
        } else if (len <= 128) {
            return number_type(bv.get_int(pos, 64), bv.get_int(pos+64, len-64));
        } else if (len <= 192) {
            return number_type(bv.get_int(pos, 64), bv.get_int(pos + 64, 64),
                               (uint128_t)bv.get_int(pos + 128, len-128));
        } else { // > 192
            return number_type(bv.get_int(pos, 64), bv.get_int(pos+64, 64),
                               (((uint128_t)bv.get_int(pos+192, len-192))<<64) | bv.get_int(pos+128, 64));
        }
    }

    template<class bit_vector_type>
    static void set_int(bit_vector_type& bv,
                        typename bit_vector_type::size_type pos,
                        number_type x,
                        uint16_t len) {
        if (len <= 64) {
            bv.set_int(pos, x, len);
        } else if (len <= 128) {
            bv.set_int(pos, x, 64); bv.set_int(pos+64, x>>64, len-64);
        } else if (len <= 192) {
            bv.set_int(pos, x, 64); bv.set_int(pos+64, x>>64, 64);
            bv.set_int(pos+128, x>>128, len-128);
        } else { // > 192
            bv.set_int(pos, x, 64); bv.set_int(pos+64, x>>64, 64);
            bv.set_int(pos+128, x>>128, 64); bv.set_int(pos+192, x>>192, len-192);
        }
    }

    static inline uint16_t popcount(number_type x) {
        return x.popcount();
    }
};

//! A class for the binomial coefficients up to n.
/*!
 * data.table[m][k] contains the number \f${m \choose k}\f$ for \f$ k\leq m\leq \leq n\f$.
 * data.space[m][k] returns the bits needed to encode a value between [0..data.table[m][k]], given m and k.
 * BINARY_SEARCH_THRESHOLD is equal to \f$ n/\lceil\log{n+1}\rceil \f$
 * \pre The template parameter n should be in the range [7..256].
 */
template<uint16_t n>
class binomial_coefficients
{
    public:
        static class impl
        {
            public:
                enum {MAX_LOG = (n>128 ? 8 : (n > 64 ? 7 : 6))};
                typedef binomial_coefficients_trait<MAX_LOG> trait;
                typedef typename trait::number_type number_type;
                static const uint16_t MAX_SIZE = (1 << MAX_LOG);
                number_type table[MAX_SIZE+1][MAX_SIZE+1];      // table for the binomial coefficients
                uint16_t space[MAX_SIZE+1][MAX_SIZE+1];    // for entry i,j \lceil \log( {i \choose j}+1 ) \rceil
                static const uint16_t BINARY_SEARCH_THRESHOLD = n/MAX_LOG;
                number_type L1Mask[MAX_SIZE+1]; // L1Mask[i] contains a word with the i least significant bits set to 1.
                // i.e. L1Mask[0] = 0, L1Mask[1] = 1,...
                number_type O1Mask[MAX_SIZE]; // O1Mask[i] contains a word with
                impl() {
                    for (uint16_t k=0; k <= MAX_SIZE; ++k) {
                        table[k][k] = 1;    // initialize diagonal
                        space[k][k] = 0;
                    }
                    for (uint16_t k=0; k <= MAX_SIZE; ++k) {
                        table[0][k] = 0;    // initialize first row
                        space[0][k] = 0;
                    }
                    for (uint16_t nn=0; nn <= MAX_SIZE; ++nn) {
                        table[nn][0] = 1;    // initialize first column
                        space[nn][0] = 0;
                    }
                    for (int nn=1; nn<=MAX_SIZE; ++nn) {
                        for (int k=1; k<=MAX_SIZE; ++k) {
                            table[nn][k] = table[nn-1][k-1] + table[nn-1][k];
                            space[nn][k] = (table[nn][k] == (number_type)1) ? 0 : trait::l1BP(table[nn][k]) + 1;
                        }
                    }
                    L1Mask[0] = 0;
                    number_type mask = 1;
                    O1Mask[0] = 1;
                    for (int i=1; i<=MAX_SIZE; ++i) {
                        L1Mask[i] = mask;
                        if (i < MAX_SIZE)
                            O1Mask[i] = O1Mask[i-1]<<1;
                        mask = (mask << 1);
                        mask |= (number_type)1;
                    }
                }
        } data;
};

template<uint16_t n>
typename binomial_coefficients<n>::impl binomial_coefficients<n>::data;

//! Class to encode and decode binomial coefficients on the fly.
/*!
 * The basic encoding and decoding process is described in
 * Gonzalo Navarro and Eliana Providel: Fast, Small, Simple Rank/Select on Bitmaps, SEA 2012
 *
 * Implemented optimizations in the decoding process:
 *   - Constant time handling for uniform blocks (only zeros or ones in the block)
 *   - Constant time handling for blocks contains only a single one bit.
 *   - Decode blocks with at most \f$ k<n\log(n) \f$ by a binary search for the ones.
 *   - For operations decode_popcount, decode_select, and decode_bit a block
 *     is only decoded as long as the query is not answered yet.
 */
template<uint16_t n>
class rrr_helper
{
    public:
        typedef binomial_coefficients<n> binomial; //!< The class containing the binomial coefficients
        typedef typename binomial::impl::number_type number_type; //!< The used number type, e.g. uint64_t, uint128_t,...
        typedef typename binomial::impl::trait trait; //!< The number trait

        //! Returns the space usage in bits of the binary representation of the number \f${n \choose k}\f$
        static inline uint16_t space_for_bt(uint16_t i) {
            return binomial::data.space[n][i];
        }

        template<class bit_vector_type>
        static inline number_type decode_btnr(const bit_vector_type& bv,
                                              typename bit_vector_type::size_type btnrp, uint16_t btnrlen) {
            return trait::get_int(bv, btnrp, btnrlen);
        }

        template<class bit_vector_type>
        static void set_bt(bit_vector_type& bv, typename bit_vector_type::size_type pos,
                           number_type bt, uint16_t space_for_bt) {
            trait::set_int(bv, pos, bt, space_for_bt);
        }

        template<class bit_vector_type>
        static inline uint16_t get_bt(const bit_vector_type& bv, typename bit_vector_type::size_type pos,
                                      uint16_t block_size) {
            return trait::popcount(trait::get_int(bv, pos, block_size));
        }

        static inline number_type bin_to_nr(number_type bin) {
            if (bin == (number_type)0 or bin == binomial::data.L1Mask[n]) {  // handle special case
                return 0;
            }
            number_type nr = 0;
            uint16_t  k  = trait::popcount(bin);
            uint16_t  nn = n; // size of the block
            while (bin != (number_type)0) {
                if (1ULL & bin) {
                    nr += binomial::data.table[nn-1][k];
                    --k; // go to the case (n-1, k-1)
                }// else go to the case (n-1, k)
                bin = (bin >> 1);
                --nn;
            }
            return nr;
        }

        //! Decode the bit at position \f$ off \f$ of the block encoded by the pair (k, nr).
        static inline bool decode_bit(uint16_t k, number_type nr, uint16_t off) {
            if (k == n) {  // if n==k, then the encoded block consists only of ones
                return 1;
            } else if (k == 0) { // if k==0 then the encoded block consists only of zeros
                return 0;
            } else if (k == 1) { // if k==1 then the encoded block contains exactly on set bit at
                return (n-nr-1) == off; // position n-nr-1
            }
            uint16_t nn = n;
            // if k < n \log n, it is better to do a binary search for each of the on bits
            if (k < binomial::data.BINARY_SEARCH_THRESHOLD) {
                while (k > 1) {
                    uint16_t nn_lb = k, nn_rb = nn+1; // invariant nr >= binomial::data.table[nn_lb-1][k]
                    while (nn_lb < nn_rb) {
                        uint16_t nn_mid = (nn_lb + nn_rb) / 2;
                        if (nr >= binomial::data.table[nn_mid-1][k]) {
                            nn_lb = nn_mid+1;
                        } else {
                            nn_rb = nn_mid;
                        }
                    }
                    nn = nn_lb-1;
                    if (n-nn >= off) {
                        return (n-nn) == off;
                    }
                    nr -= binomial::data.table[nn-1][k];
                    --k;
                    --nn;
                }
            } else { // else do a linear decoding
                int i = 0;
                while (k > 1) {
                    if (i > off) {
                        return 0;
                    }
                    if (nr >= binomial::data.table[nn-1][k]) {
                        nr -= binomial::data.table[nn-1][k];
                        --k;
                        if (i == off)
                            return 1;
                    }
                    --nn;
                    ++i;
                }
            }
            return (n-nr-1) == off;
        }

        //! Decode the first off bits bits of the block encoded by the pair (k, nr) and return the set bits.
        static inline uint16_t decode_popcount(uint16_t k, number_type nr, uint16_t off) {
            if (k == n) {  // if n==k, then the encoded block consists only of ones
                return off;   // i.e. the answer is off
            } else if (k == 0) { // if k==0, then the encoded block consists only on zeros
                return 0;    // i.e. the result is zero
            } else if (k == 1) { // if k==1 then the encoded block contains exactly on set bit at
                return (n-nr-1) < off; // position n-nr-1, and popcount is 1 if off > (n-nr-1).
            }
            uint16_t result = 0;
            uint16_t nn = n;
            // if k < n \log n, it is better to do a binary search for each of the on bits
            if (k < binomial::data.BINARY_SEARCH_THRESHOLD) {
                while (k > 1) {
                    uint16_t nn_lb = k, nn_rb = nn+1; // invariant nr >= binomial::data.table[nn_lb-1][k]
                    while (nn_lb < nn_rb) {
                        uint16_t nn_mid = (nn_lb + nn_rb) / 2;
                        if (nr >= binomial::data.table[nn_mid-1][k]) {
                            nn_lb = nn_mid+1;
                        } else {
                            nn_rb = nn_mid;
                        }
                    }
                    nn = nn_lb-1;
                    if (n-nn >= off) {
                        return result;
                    }
                    ++result;
                    nr -= binomial::data.table[nn-1][k];
                    --k;
                    --nn;
                }
            } else {
                int i = 0;
                while (k > 1) {
                    if (i >= off) {
                        return result;
                    }
                    if (nr >= binomial::data.table[nn-1][k]) {
                        nr -= binomial::data.table[nn-1][k];
                        --k;
                        ++result;
                    }
                    --nn;
                    ++i;
                }
            }
            return result + ((n-nr-1) < off);
        }

        /*! \pre k >= sel, sel>0
         */
        static inline uint16_t decode_select(uint16_t k, number_type& nr, uint16_t sel) {
            if (k == n) {  // if n==k, then the encoded block consists only of ones
                return sel-1;
            } else if (k == 1 and sel == 1) {
                return n-nr-1;
            }
            uint16_t nn = n;
            // if k < n \log n, it is better to do a binary search for each of the on bits
            if (sel < binomial::data.BINARY_SEARCH_THRESHOLD) {
                while (sel > 0) {
                    uint16_t nn_lb = k, nn_rb = nn+1; // invariant nr >= iii.m_coefficients[nn_lb-1]
                    while (nn_lb < nn_rb) {
                        uint16_t nn_mid = (nn_lb + nn_rb) / 2;
                        if (nr >= binomial::data.table[nn_mid-1][k]) {
                            nn_lb = nn_mid+1;
                        } else {
                            nn_rb = nn_mid;
                        }
                    }
                    nn = nn_lb-1;
                    nr -= binomial::data.table[nn-1][k];
                    --sel;
                    --nn;
                    --k;
                }
                return n-nn-1;
            } else {
                int i = 0;
                while (sel > 0) {   // TODO: this condition only work if the precondition holds
                    if (nr >= binomial::data.table[nn-1][k]) {
                        nr -= binomial::data.table[nn-1][k];
                        --sel;
                        --k;
                    }
                    --nn;
                    ++i;
                }
                return i-1;
            }
        }

        /*! \pre k >= sel, sel>0
         */
        template<uint8_t pattern, uint8_t len>
        static inline uint16_t decode_select_bitpattern(uint16_t k, number_type& nr, uint16_t sel) {
            int i = 0;
            uint8_t decoded_pattern = 0;
            uint8_t decoded_len     = 0;
            uint16_t nn = n;
            while (sel > 0) {   // TODO: this condition only work if the precondition holds
                decoded_pattern = decoded_pattern<<1;
                ++decoded_len;
                if (nr >= binomial::data.table[nn-1][k]) {
                    nr -= binomial::data.table[nn-1][k];
                    // a one is decoded
                    decoded_pattern |= 1; // add to the pattern
                    --k;
                }
                --nn;
                ++i;
                if (decoded_len == len) {  // if decoded pattern length equals len of the searched pattern
                    if (decoded_pattern == pattern) {  // and pattern equals the searched pattern
                        --sel;
                    }
                    decoded_pattern = 0; decoded_len = 0; // reset pattern
                }
            }
            return i-len; // return the starting position of $sel$th occurence of the pattern
        }

};

} // end namespace
#endif
