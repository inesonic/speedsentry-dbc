/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref CacheBase class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CACHE_BASE_H
#define CACHE_BASE_H

#include <cstdint>

/**
 * Class used to manage a cache of customer secrets.  All protected functions are designed to be fully thread safe.
 */
class CacheBase {
    public:
        /**
         * Type used to represent an index into the cache table.
         */
        typedef unsigned long Index;

        /**
         * Value used to represent an invalid cache index.
         */
        static constexpr unsigned long invalidIndex = static_cast<unsigned long>(-1);

        CacheBase();

        ~CacheBase();

    protected:
        /**
         * Basis for the Fowler-Noll-Vo hash algorithm,
         */
        static constexpr std::uint64_t fnv1aOffsetBasis = 14695981039346656037ULL;

        /**
         * Method that implements the Fowler-Noll-Vo hash for an 8-bit value.
         *
         * \param[in] value The value to calculate the hash for.
         *
         * \param[in] hash  The running hash or basis.
         *
         * \return Returns a hash of the value.
         */
        static std::uint64_t fnv1a8(std::uint8_t value, std::uint64_t hash = fnv1aOffsetBasis);

        /**
         * Method that implements the Fowler-Noll-Vo hash for a 16-bit value.
         *
         * \param[in] value The value to calculate the hash for.
         *
         * \param[in] hash  The running hash or basis.
         *
         * \return Returns a hash of the value.
         */
        static std::uint64_t fnv1a16(std::uint16_t value, std::uint64_t hash = fnv1aOffsetBasis);

        /**
         * Method that implements the Fowler-Noll-Vo hash for a 32-bit value.
         *
         * \param[in] value The value to calculate the hash for.
         *
         * \param[in] hash  The running hash or basis.
         *
         * \return Returns a hash of the value.
         */
        static std::uint64_t fnv1a32(std::uint32_t value, std::uint64_t hash = fnv1aOffsetBasis);

        /**
         * Method that implements the Fowler-Noll-Vo hash for a 64-bit value.
         *
         * \param[in] value The value to calculate the hash for.
         *
         * \param[in] hash  The running hash or basis.
         *
         * \return Returns a hash of the value.
         */
        static std::uint64_t fnv1a64(std::uint64_t value, std::uint64_t hash = fnv1aOffsetBasis);

        /**
         * Method that calculates an optimal cache table size based on a requested size.  The function locates the next
         * prime equal to or larger than the specified value.  This function will also include a reservation margin
         * in order to keep the cache search length low.
         *
         * Note that ths function is time consuming so it should not be used on a regular basis.
         *
         * \param[in] requestedSize The value to use as a basis.
         *
         * \return Returns the recommended cache table size.
         */
        static Index calculateCacheTableSize(Index requestedSize);

        /**
         * Method that calculates a random index for eviction.  The algorithm uses the XORSHRO256++ algorithm with
         * random initial seeds.
         *
         * \param[in] cacheTableSize The size of the cache table, in entries.
         */
        Index randomIndex(Index cacheTableSize);

    private:
        /**
         * The Fowler-Noll-Vo prime.
         */
        static constexpr std::uint64_t fnv1Prime = 1099611628211ULL;

        /**
         * The cache reservation margin to guarantee a statistically high hit rate.
         */
        static constexpr float cacheReservationMargin = 1.25F;

        /**
         * Method that determines if one number divides another.
         *
         * \param[in[ dividend The dividend.
         *
         * \param[in] divisor  The divisor.
         *
         * \return Returns true if the divisor divides into the dividend.
         */
        static inline bool divides(Index dividend, Index divisor) {
            return (dividend % divisor) == 0;
        }

        /**
         * Method that determines if a value is prime.
         *
         * \param[in] value The value to be tested.
         *
         * \return Returns true if the value is prime.  Returns false if the value is not prime.
         */
        static bool isPrime(Index value);

        /**
         * Method that finds the next prime equal to or greater than a specified value.
         *
         * \param[in] value The value to find the next higher prime from.
         *
         * \return Returns the next higher prime value.
         */
        static Index calculateNextPrime(Index value);

        /**
         * Seed used to generate random values.
         */
        std::uint64_t seed[4];
};

#endif
