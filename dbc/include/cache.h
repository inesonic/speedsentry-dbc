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
* This header defines the \ref Cache template class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CACHE_H
#define CACHE_H

#include <cstdint>

#include "cache_base.h"
#include "customer_secret.h"

class QTimer;
class DatabaseManager;

/**
 * Template class you can use to create a cache with random eviction.
 *
 * The parameter T is the datatype being cached.  The type T must be default constructable and assignable.  The
 * parameter ID should be a C++ pod type with a predictable byte representation.
 *
 * You should create a derived class and overload the pure-virtual method \ref idFromValue.
 */
template<typename T, typename ID> class Cache:public CacheBase {
    public:
        /**
         * Constructor
         *
         * \param[in] maximumCacheDepth The maximum allowed cache depth.
         */
        Cache(Index maximumCacheDepth) {
            cacheTable = nullptr;
            resizeCache(maximumCacheDepth);
        }

        virtual ~Cache() {
            delete[] cacheTable;
        }

        /**
         * Method that resizes the cache.  This method will evict all cache entries.
         *
         * \param[in] newCacheSize The new cache size.
         */
        void resizeCache(unsigned long newCacheSize) {
            if (cacheTable != nullptr) {
                delete[] cacheTable;
            }

            currentNumberCachedEntries = 0;
            currentMaximumCacheDepth   = newCacheSize;
            currentCacheTableSize      = calculateCacheTableSize(newCacheSize);
            cacheTable                 = new Entry[currentCacheTableSize]();
        }

        /**
         * Method you can use to get a cache entry from the cache by ID.
         *
         * \param[in] entryId The ID of the desired entry.
         *
         * \return Returns a pointer to the cached entry.  Returns a null pointer if the requested entry
         *         is not in the cache.
         */
        const T* getCacheEntry(ID entryId) const {
            Index index = locateEntry(entryId);
            return index != invalidIndex ? cacheTable[index].pointer() : nullptr;
        }

        /**
         * Method you can use to get a cache entry from the cache by ID.  Note that this method should be used with
         * care.
         *
         * \param[in] entryId The ID of the desired entry.
         *
         * \return Returns a pointer to the cached entry.  Returns a null pointer if the requested entry
         *         is not in the cache.
         */
        T* getCacheEntry(ID entryId) {
            Index index = locateEntry(entryId);
            return index != invalidIndex ? cacheTable[index].pointer() : nullptr;
        }

        /**
         * Method you can use to evict a cache entry from the cache.
         *
         * \param[in] entryId The ID of the entry to be evicted.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool evictCacheEntry(ID entryId) {
            bool success = false;

            Index hashedIndex     = initialHashIndex(entryId);
            Index maximumDistance = cacheTable[hashedIndex].maximumDistance();

            if (maximumDistance > 0) {
                Entry* entry         = cacheTable + hashedIndex;
                bool   entryOccupied = entry->isOccupied();
                ID     thisEntryId   = entryOccupied ? idFromValue(entry->reference()) : 0;

                if (maximumDistance == 1 && entryOccupied && thisEntryId == entryId) {
                    *entry = Entry();

                    --currentNumberCachedEntries;
                    success = true;
                } else {
                    Index newMaximumDistance = 0;
                    Index currentDistance    = 0;
                    Index index              = hashedIndex;

                    while (maximumDistance > currentDistance && (!entryOccupied || thisEntryId != entryId)) {
                        ++currentDistance;

                        if (entryOccupied && initialHashIndex(thisEntryId) == hashedIndex) {
                            newMaximumDistance = currentDistance;
                        }

                        index = nextIndex(index);

                        entry = cacheTable + index;
                        entryOccupied = entry->isOccupied();
                        if (entryOccupied) {
                            thisEntryId = idFromValue(entry->reference());
                        }
                    }

                    if (thisEntryId == entryId) {
                        entry->clearValue();
                        entry->markUnoccupied();

                        if (currentDistance == maximumDistance) {
                            cacheTable[hashedIndex].setMaximumDistance(newMaximumDistance);
                        }

                        --currentNumberCachedEntries;
                        success = true;
                    }
                }
            }

            return success;
        }

        /**
         * Method you can use to add a new cache entry to the cache.  If needed, another entry will be evicted to
         * make room for this entry.
         *
         * \param[in] value The value to be added.
         */
        void addToCache(const T& value) {
            ID entryId = idFromValue(value);

            Index hashedIndex = initialHashIndex(entryId);
            Index index       = locateEntry(entryId, hashedIndex);

            if (index != invalidIndex) {
                cacheTable[index].setValue(value);
            } else {
                if (currentNumberCachedEntries >= currentMaximumCacheDepth) {
                    performRandomEviction();
                }

                // Note that the algorithm below will hang if the cache ever becomes completely full.  We rely on the
                // fact that we perform evictions above to avoid having to add extra code to detect the cache full
                // table condition.
                //
                // Note that keeping the cache relatively unpopulated causes us to statistically find most entries on
                // the first try.  If needed, this can be modeled reasonably accurately via a binomial distribution.

                Entry* hashedEntry     = cacheTable + hashedIndex;
                Entry* entry           = hashedEntry;
                Index  maximumDistance = hashedEntry->maximumDistance();
                Index  currentDistance = 1;

                index = hashedIndex;
                while (entry->isOccupied()) {
                    ++currentDistance;
                    index = nextIndex(index);
                    entry = cacheTable + index;
                }

                entry->setValue(value);
                entry->markOccupied();
                if (currentDistance > maximumDistance) {
                    hashedEntry->setMaximumDistance(currentDistance);
                }

                ++currentNumberCachedEntries;
            }
        }

    protected:
        /**
         * Method that obtains the ID used to access a specific value.
         *
         * \param[in] value The value to calculate the ID for.
         *
         * \return Returns the ID to associate with this value.
         */
        virtual ID idFromValue(const T& value) const = 0;

    private:
        /**
         * Class that holds a single cache entry.
         */
        class Entry {
            public:
                Entry() {
                    currentValue           = T();
                    currentMaximumDistance = 0;
                    currentIsOccupied      = false;
                }

                /**
                 * Constructor
                 *
                 * \param[in] value The value to be cached.
                 */
                inline Entry(
                        const T& value
                    ):currentValue(
                        value
                    ),currentMaximumDistance(
                        0
                    ),currentIsOccupied(
                        false
                    ) {}

                /**
                 * Constructor
                 *
                 * \param[in] value           The value to be cached.
                 *
                 * \param[in] maximumDistance The maximum distance to specify for this entry.
                 */
                inline Entry(
                        const T& value,
                        Index    maximumDistance
                    ):currentValue(
                        value
                    ),currentMaximumDistance(
                        maximumDistance
                    ),currentIsOccupied(
                        true
                    ) {}

                /**
                 * Constructor
                 *
                 * \param[in] value           The value to be cached.
                 *
                 * \param[in] maximumDistance The maximum distance to specify for this entry.
                 *
                 * \param[in] isOccupied      Flag indicating if this entry should be marked as occupied.
                 */
                inline Entry(
                        const T& value,
                        Index    maximumDistance,
                        bool     isOccupied
                    ):currentValue(
                        value
                    ),currentMaximumDistance(
                        maximumDistance
                    ),currentIsOccupied(
                        isOccupied
                    ) {}

                /**
                 * Copy constructor
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline Entry(
                        const Entry& other
                    ):currentValue(
                        other.currentValue
                    ),currentMaximumDistance(
                        other.currentMaximumDistance
                    ),currentIsOccupied(
                        other.currentIsOccupied
                    ) {}

                /**
                 * Move constructor
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline Entry(
                        Entry&& other
                    ):currentValue(
                        other.currentValue
                    ),currentMaximumDistance(
                        other.currentMaximumDistance
                    ),currentIsOccupied(
                        other.currentIsOccupied
                    ) {}

                ~Entry() {}

                /**
                 * Method you can use to determine if this location is occupied.
                 *
                 * \return Returns true if the location is occupied.  Returns false if the location is not occupied.
                 */
                inline bool isOccupied() const {
                    return currentIsOccupied;
                }

                /**
                 * Method you can use to determine if this location is unoccupied.
                 *
                 * \return Returns true if the location is unoccupied.  Returns false if the location is occupied.
                 */
                inline bool isUnoccupied() const {
                    return !currentIsOccupied;
                }

                /**
                 * Method you can use to mark this location as occupied.
                 *
                 * \param[in] nowOccupied If true, the location will be marked as occupied.  If false, the location
                 *                        will be marked as unoccupied.
                 */
                inline void markOccupied(bool nowOccupied = true) {
                    currentIsOccupied = nowOccupied;
                }

                /**
                 * Method you can use to mark this location as unoccupied.
                 *
                 * \param[in] nowUnoccupied If true, the location will be marked as unoccupied.  If false, the location
                 *                          will be marked as occupied.
                 */
                inline void markUnoccupied(bool nowUnoccupied = true) {
                    currentIsOccupied = !nowUnoccupied;
                }

                /**
                 * Method that clears the value for this entry.
                 */
                inline void clearValue() {
                    currentValue = T();
                }

                /**
                 * Method you can use to obtain the value at this location.
                 *
                 * \return Returns a constant reference to the value.
                 */
                inline const T& reference() const {
                    return currentValue;
                }

                /**
                 * Method you can use to obtain the value at this location.
                 *
                 * \return Returns a constant reference to the value.
                 */
                inline T& reference() {
                    return currentValue;
                }

                /**
                 * Method you can use to obtain the value at this location.
                 *
                 * \return Returns a constant reference to the value.
                 */
                inline const T* pointer() const {
                    return &currentValue;
                }

                /**
                 * Method you can use to obtain the value at this location.
                 *
                 * \return Returns a constant reference to the value.
                 */
                inline T* pointer() {
                    return &currentValue;
                }

                /**
                 * Method you can use to set the value at this location.
                 *
                 * \param[in] newValue The new value for the location.
                 */
                inline void setValue(const T& newValue) {
                    currentValue = newValue;
                }

                /**
                 * Method you can use to obtain the current maximum distance for this hash location.
                 *
                 * \return Returns the maximum distance.
                 */
                inline Index maximumDistance() const {
                    return currentMaximumDistance;
                }

                /**
                 * Method you can use to update the maximum distance for this hash location.
                 *
                 * \param[in] newMaximumDistance The new maximum distance value.
                 */
                inline void setMaximumDistance(Index newMaximumDistance) {
                    currentMaximumDistance = newMaximumDistance;
                }

                /**
                 * Assignment operator
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline Entry& operator=(const Entry& other) {
                    currentValue           = other.currentValue;
                    currentMaximumDistance = other.currentMaximumCacheDepth;
                    currentIsOccupied      = other.currentIsOccupied;

                    return *this;
                }

                /**
                 * Assignment operator
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline Entry& operator=(Entry&& other) {
                    currentValue           = other.currentValue;
                    currentMaximumDistance = other.currentMaximumDistance;
                    currentIsOccupied      = other.currentIsOccupied;

                    return *this;
                }

            private:
                /**
                 * The current value.
                 */
                T currentValue;

                /**
                 * The current maximum distance to the last cache entry associated with this location.  A value is one
                 * based.  A value of 0 indicates no cache entries tied to this hash value.
                 */
                Index currentMaximumDistance;

                /**
                 * Flag holding true if this location is occupied.
                 */
                bool currentIsOccupied;
        };

        /**
         * Method that finds the entry in the cache.
         *
         * \param[in] entryId          The ID of the entry to be evicted.
         *
         * \param[in] initialHashIndex The initial hash index to be used.
         *
         * \return Returns the index into the cache where the entry resides.  A value of \ref invalidIndex is returned
         *         if the entry can not be found.
         */
        Index locateEntry(ID entryId, Index initialHashIndex) const {
            Index index             = initialHashIndex;
            Index remainingDistance = cacheTable[index].maximumDistance();

            while (remainingDistance > 0                                      &&
                   (cacheTable[index].isUnoccupied()                     ||
                   idFromValue(cacheTable[index].reference()) != entryId    )    ) {
                --remainingDistance;
                index = nextIndex(index);
            }

            return remainingDistance == 0 ? invalidIndex : index;
        }

        /**
         * Method that finds the entry in the cache.  This version will calculate an initial hash index.
         *
         * \param[in] entryId          The ID of the entry to be evicted.
         *
         * \param[in] initialHashIndex The initial hash index to be used.
         *
         * \return Returns the index into the cache where the entry resides.  A value of \ref invalidIndex is returned
         *         if the entry can not be found.
         */
        Index locateEntry(ID entryId) const {
            return locateEntry(entryId, initialHashIndex(entryId));
        }

        /**
         * Method that calculates the next index position.
         *
         * \param[in] currentIndex The current index value.
         *
         * \return Returns the next index value.
         */
        inline Index nextIndex(Index currentIndex) const {
            ++currentIndex;
            return currentIndex >= currentCacheTableSize ? 0 : currentIndex;
        }

        /**
         * Method that calculates the previous index position.
         *
         * \param[in] currentIndex The current index value.
         *
         * \return Returns the previous index value.
         */
        inline Index previousIndex(Index currentIndex) const {
            return (currentIndex == 0 ? currentCacheTableSize : currentIndex) - 1;
        }

        /**
         * Method that calculates an initial hash index from an entry ID.
         *
         * \param[in] entryId          The ID of the entry to be evicted.
         *
         * \return Returns an initial hash index.
         */
        inline Index initialHashIndex(ID entryId) const {
            std::uint64_t hashValue;
            if (sizeof(ID) == 1) {
                hashValue = fnv1a8(static_cast<std::uint8_t>(entryId));
            } else if (sizeof(ID) == 2) {
                hashValue = fnv1a16(static_cast<std::uint16_t>(entryId));
            } else if (sizeof(ID) == 4) {
                hashValue = fnv1a32(static_cast<std::uint32_t>(entryId));
            } else if (sizeof(ID) == 8) {
                hashValue = fnv1a64(static_cast<std::uint64_t>(entryId));
            } else {
                hashValue = fnv1aOffsetBasis;
                const std::uint8_t* d = reinterpret_cast<const std::uint8_t*>(&entryId);
                for (unsigned i=0 ; i<sizeof(ID) ; ++i) {
                    hashValue = fnv1a8(d[i], hashValue);
                }
            }

            return static_cast<Index>(hashValue % currentCacheTableSize);
        }

        /**
         * Method that calculates the distnace between two entries.
         *
         * \param[in] startingIndex The starting index.
         *
         * \param[in] endingIndex   The ending index.
         *
         * \return Returns the distance.
         */
        inline Index distance(Index startingIndex, Index endingIndex) {
            return   startingIndex <= endingIndex
                   ? endingIndex - startingIndex + 1
                   : endingIndex + currentCacheTableSize - startingIndex + 1;
        }

        /**
         * Method that performs a random eviction from the cache.
         */
        void performRandomEviction() {
            Index  index;
            Entry* entry;
            do {
                index = randomIndex(currentCacheTableSize);
                entry = cacheTable + index;
            } while (entry->isUnoccupied());

            ID entryId = idFromValue(entry->reference());
            evictCacheEntry(entryId);
        }

        /**
         * The maximum allowed cache depth.
         */
        Index currentMaximumCacheDepth;

        /**
         * The size of the cache table.
         */
        Index currentCacheTableSize;

        /**
         * A count of the current number of cached entries.
         */
        Index currentNumberCachedEntries;

        /**
         * The cache implemented as a circular hash table.
         */
        Entry* cacheTable;
};

#endif
