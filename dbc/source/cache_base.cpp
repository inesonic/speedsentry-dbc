/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 - 2023 Inesonic, LLC.
*
* GNU Public License, Version 3:
*   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
*   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
*   version.
*   
*   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
*   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
*   details.
*   
*   You should have received a copy of the GNU General Public License along with this program.  If not, see
*   <https://www.gnu.org/licenses/>.
********************************************************************************************************************//**
* \file
*
* This header implements the \ref CacheBase class.
***********************************************************************************************************************/

#include <QRandomGenerator>

#include <cstdint>
#include <cmath>

#include "cache_base.h"

CacheBase::CacheBase() {
    for (unsigned i=0 ; i<(sizeof(seed)/sizeof(std::uint64_t)) ; ++i) {
        seed[i] = QRandomGenerator::global()->generate64();
    }
}


CacheBase::~CacheBase() {}


std::uint64_t CacheBase::fnv1a8(std::uint8_t value, std::uint64_t hash) {
    return (hash ^ value) * fnv1Prime;
}


std::uint64_t CacheBase::fnv1a16(std::uint16_t value, std::uint64_t hash) {
    unsigned char* bytes = reinterpret_cast<unsigned char*>(&value);

    hash = (hash ^ bytes[0]) * fnv1Prime;
    hash = (hash ^ bytes[1]) * fnv1Prime;

    return hash;
}


std::uint64_t CacheBase::fnv1a32(std::uint32_t value, std::uint64_t hash) {
    unsigned char* bytes = reinterpret_cast<unsigned char*>(&value);

    hash = (hash ^ bytes[0]) * fnv1Prime;
    hash = (hash ^ bytes[1]) * fnv1Prime;
    hash = (hash ^ bytes[2]) * fnv1Prime;
    hash = (hash ^ bytes[3]) * fnv1Prime;

    return hash;
}


std::uint64_t CacheBase::fnv1a64(std::uint64_t value, std::uint64_t hash) {
    unsigned char* bytes = reinterpret_cast<unsigned char*>(&value);

    hash = (hash ^ bytes[0]) * fnv1Prime;
    hash = (hash ^ bytes[1]) * fnv1Prime;
    hash = (hash ^ bytes[2]) * fnv1Prime;
    hash = (hash ^ bytes[3]) * fnv1Prime;
    hash = (hash ^ bytes[4]) * fnv1Prime;
    hash = (hash ^ bytes[5]) * fnv1Prime;
    hash = (hash ^ bytes[6]) * fnv1Prime;
    hash = (hash ^ bytes[7]) * fnv1Prime;

    return hash;
}


CacheBase::Index CacheBase::calculateCacheTableSize(Index requestedSize) {
    return calculateNextPrime(requestedSize * cacheReservationMargin);
}


bool CacheBase::isPrime(CacheBase::Index value) {
    bool prime;

    if (value == 2 || value == 3) {
        prime = true;
    } else if (value < 5) {
        prime = false;
    } else {
        unsigned long endingIndex = static_cast<unsigned long>(std::sqrt(value)) + 1;
        unsigned long i           = 6;

        prime = true;
        while (prime && i <= endingIndex) {
            prime = !divides(value, i - 1) && !divides(value, i + 1);
            i += 6;
        }
    }

    return prime;
}


CacheBase::Index CacheBase::calculateNextPrime(Index value) {
    Index nextPrime;
    if (value < 5) {
        if (value < 2) {
            nextPrime = 2;
        } else if (value < 3) {
            nextPrime = 3;
        } else {
            nextPrime = 5;
        }
    } else {
        nextPrime = 0;

        Index i  = 6UL * ((value + 2UL) / 6UL);
        Index v1 = i - 1;
        if (v1 > value && isPrime(v1)) {
            nextPrime = v1;
        } else {
            Index v2 = i + 1;
            if (v2 > value && isPrime(v2)) {
                nextPrime = v2;
            } else {
                do {
                    i += 6;
                    v1 = i - 1;
                    if (isPrime(v1)) {
                        nextPrime = v1;
                    } else {
                        v2 = i + 1;
                        if (isPrime(v2)) {
                            nextPrime = v2;
                        }
                    }
                } while (nextPrime == 0);
            }
        }
    }

    return nextPrime;
}


CacheBase::Index CacheBase::randomIndex(CacheBase::Index cacheTableSize) {
    std::uint64_t s0 = seed[0];
    std::uint64_t s1 = seed[1];
    std::uint64_t s2 = seed[2];
    std::uint64_t s3 = seed[3];

    std::uint64_t t      = s1 << 17;
    std::uint64_t result = s0 + s3;

    s2 ^= s0;
    s3 ^= s1;
    s1 ^= s2;
    s0 ^= s3;

    s2 ^= t;
    s3 = (s3 << 45) | (s3 >> 19);

    seed[0] = s0;
    seed[1] = s1;
    seed[2] = s2;
    seed[3] = s3;

    return result % cacheTableSize;
}
