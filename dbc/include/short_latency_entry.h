/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref ShortLatencyEntry class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef SHORT_LATENCY_ENTRY_H
#define SHORT_LATENCY_ENTRY_H

#include <cstdint>

/**
 * Class used to track a single short latency entry.
 */
class ShortLatencyEntry {
    public:
        /**
         * Type used to represent a timestamp.
         */
        typedef std::uint32_t ZoranTimeStamp;

        /**
         * Type used to represent a stored latency value.  Value is in microseconds.
         */
        typedef std::uint32_t LatencyMicroseconds;

        /**
         * Value indicating the maximum allowed latency for the system.  Values above this indicate a bad reading.
         */
        static constexpr LatencyMicroseconds maximumAllowedLatencyMicroseconds = 60000000;

        /**
         * Value indicating the correction between the start of the Zoran epoch and the Unix epoch.
         *
         * The value translates to seconds between 00:00:00 January 1, 2021 and 00:00:00 January 1, 1970.
         */
        static constexpr std::uint64_t startOfZoranEpoch = 1609484400;

        constexpr ShortLatencyEntry():
            currentTimestamp(0),
            currentLatencyMicroseconds(0) {}

        /**
         * Constructor
         *
         * \param[in] zoranTimestamp      The timestamp relative to the start of the Zoran epoch.
         *
         * \param[in] latencyMicroseconds The latency measurement, in microseconds.
         */
        constexpr ShortLatencyEntry(
                ZoranTimeStamp      zoranTimestamp,
                LatencyMicroseconds latencyMicroseconds
            ):currentTimestamp(
                zoranTimestamp
            ),currentLatencyMicroseconds(
                latencyMicroseconds
            ) {}

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        constexpr ShortLatencyEntry(
                const ShortLatencyEntry& other
            ):currentTimestamp(
                other.currentTimestamp
            ),currentLatencyMicroseconds(
                other.currentLatencyMicroseconds
            ) {}

        ~ShortLatencyEntry() = default;

        /**
         * Method you can use to obtain the current Zoran timestamp.
         *
         * \return Returns the current timestamp in Zoran time.
         */
        inline ZoranTimeStamp zoranTimestamp() const {
            return currentTimestamp;
        }

        /**
         * Method you can use to obtain the current Unix timestamp for this entry.
         *
         * \return Returns the current Unix timestamp for this entry.
         */
        inline std::uint64_t unixTimestamp() const {
            return currentTimestamp + startOfZoranEpoch;
        }

        /**
         * Method you can use to obtain the current latency value, in microseconds.
         *
         * \return Returns the current latency value, in microseconds.
         */
        inline LatencyMicroseconds latencyMicroseconds() const {
            return currentLatencyMicroseconds;
        }

        /**
         * Method you can use to obtain the current latency value, in seconds.
         *
         * \return Returns the current latency value, in seconds.
         */
        inline double latencySeconds() const {
            return currentLatencyMicroseconds / 1000000.0;
        }

        /**
         * Assignment operator.
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline ShortLatencyEntry& operator=(const ShortLatencyEntry& other) {
            currentTimestamp = other.currentTimestamp;
            currentLatencyMicroseconds = other.currentLatencyMicroseconds;

            return *this;
        }

        /**
         * Method you can use to convert Zoran time to Unix time.
         *
         * \param[in] zoranTimestamp The Zoran timestamp to be converted.
         *
         * \return Returns the resulting Unix timestamp.
         */
        static inline std::uint64_t toUnixTimestamp(ZoranTimeStamp zoranTimestamp) {
            return zoranTimestamp + startOfZoranEpoch;
        }

        /**
         * Method you can use to convert Unix time to Zoran time.
         *
         * \param[in] unixTimestamp The Unix timestamp to be converted.
         *
         * \return Returns the resulting Zoran timestamp.
         */
        static inline ZoranTimeStamp toZoranTimestamp(std::uint64_t unixTimestamp) {
            return static_cast<ZoranTimeStamp>(unixTimestamp - startOfZoranEpoch);
        }

    private:
        /**
         * The timestamp value being tracked.
         */
        ZoranTimeStamp currentTimestamp;

        /**
         * The current latency value being tracked, in microseconds.
         */
        LatencyMicroseconds currentLatencyMicroseconds;
};

#endif
