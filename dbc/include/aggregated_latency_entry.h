/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref AggregatedLatencyEntry class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef AGGREGATED_LATENCY_ENTRY_H
#define AGGREGATED_LATENCY_ENTRY_H

#include <cstdint>

#include "latency_entry.h"

/**
 * Class used to track a single aggregated latency entry.
 */
class AggregatedLatencyEntry:public LatencyEntry {
    public:
        constexpr AggregatedLatencyEntry():
            currentStartZoranTimestamp(0),
            currentEndZoranTimestamp(0),
            currentMeanLatency(0),
            currentVarianceLatency(0),
            currentMinimumLatency(0),
            currentMaximumLatency(0),
            currentNumberSamples(0) {}

        /**
         * Constructor
         *
         * \param[in] monitorId           The ID of the monitor tied to this entry.
         *
         * \param[in] serverId            The ID of the region server where this measurement was taken.
         *
         * \param[in] zoranTimestamp      The timestamp relative to the start of the Zoran epoch.
         *
         * \param[in] latencyMillisconds  The latency measurement, in microseconds.
         *
         * \param[in] startZoranTimestamp The timestamp of the first entry in this aggregation.
         *
         * \param[in] endZoranTimestamp   The timestamp of the last entry in this aggregation.
         *
         * \param[in] meanLatency         The average latency in this aggregation.
         *
         * \param[in] varianceLatency     The variance of the latency in this aggregation.
         *
         * \param[in] minimumLatency      The minimum latency in this aggregation.
         *
         * \param[in] maximumLatency      The maximum latency in this aggregation.
         *
         * \param[in] numberSamples       The number of samples represented by this aggregation.
         */
        constexpr AggregatedLatencyEntry(
                MonitorId           monitorId,
                ServerId            serverId,
                ZoranTimeStamp      zoranTimestamp,
                LatencyMicroseconds latencyMicroseconds,
                ZoranTimeStamp      startZoranTimestamp,
                ZoranTimeStamp      endZoranTimestamp,
                double              meanLatency,
                double              varianceLatency,
                LatencyMicroseconds minimumLatency,
                LatencyMicroseconds maximumLatency,
                unsigned long       numberSamples
            ):LatencyEntry(
                monitorId,
                serverId,
                zoranTimestamp,
                latencyMicroseconds
            ),currentStartZoranTimestamp(
                startZoranTimestamp
            ),currentEndZoranTimestamp(
                endZoranTimestamp
            ),currentMeanLatency(
                meanLatency
            ),currentVarianceLatency(
                varianceLatency
            ),currentMinimumLatency(
                minimumLatency
            ),currentMaximumLatency(
                maximumLatency
            ),currentNumberSamples(
                numberSamples
            ) {}

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        constexpr AggregatedLatencyEntry(
                const AggregatedLatencyEntry& other
            ):LatencyEntry(
                other
            ),currentStartZoranTimestamp(
                other.currentStartZoranTimestamp
            ),currentEndZoranTimestamp(
                other.currentEndZoranTimestamp
            ),currentMeanLatency(
                other.currentMeanLatency
            ),currentVarianceLatency(
                other.currentVarianceLatency
            ),currentMinimumLatency(
                other.currentMinimumLatency
            ),currentMaximumLatency(
                other.currentMaximumLatency
            ),currentNumberSamples(
                other.currentNumberSamples
            ) {}

        ~AggregatedLatencyEntry() = default;

        /**
         * Method that returns the start time for this aggregation, Zoran time.
         *
         * \return Returns the timestamp for the start of this aggregation.
         */
        inline ZoranTimeStamp startZoranTimestamp() const {
            return currentStartZoranTimestamp;
        }

        /**
         * Method that returns the start time for this aggregation, Unix time.
         *
         * \return Returns the timestamp for the start of this aggregation.
         */
        inline unsigned long long startTimestamp() const {
            return currentStartZoranTimestamp + startOfZoranEpoch;
        }

        /**
         * Method that returns the end time for this aggregation, Zoran time.
         *
         * \return Returns the timestamp for the end of this aggregation.
         */
        inline ZoranTimeStamp endZoranTimestamp() const {
            return currentEndZoranTimestamp;
        }

        /**
         * Method that returns the end time for this aggregation, Unix time.
         *
         * \return Returns the timestamp for the end of this aggregation.
         */
        inline unsigned long long endTimestamp() const {
            return currentEndZoranTimestamp + startOfZoranEpoch;
        }

        /**
         * Method that returns the mean latency value.
         *
         * \return Returns the mean latency value.
         */
        inline double meanLatency() const {
            return currentMeanLatency;
        }

        /**
         * Method that returns the variance latency value.
         *
         * \return Returns the variance latency value.
         */
        inline double varianceLatency() const {
            return currentVarianceLatency;
        }

        /**
         * Method that returns the minimum measured latency.
         *
         * \return Returns the minimum measured latency.
         */
        inline LatencyMicroseconds minimumLatency() const {
            return currentMinimumLatency;
        }

        /**
         * Method that returns the maximum measured latency.
         *
         * \return Returns the maximum measured latency.
         */
        LatencyMicroseconds maximumLatency() const {
            return currentMaximumLatency;
        }

        /**
         * Method that returns the number of samples represented by this aggregation.
         *
         * \return Returns the number of samples represented by this aggregation.
         */
        unsigned long numberSamples() const {
            return currentNumberSamples;
        }

        /**
         * Assignment operator
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        AggregatedLatencyEntry& operator=(const AggregatedLatencyEntry& other) {
            LatencyEntry::operator=(other);

            currentStartZoranTimestamp = other.currentStartZoranTimestamp;
            currentEndZoranTimestamp   = other.currentEndZoranTimestamp;
            currentMeanLatency         = other.currentMeanLatency;
            currentVarianceLatency     = other.currentVarianceLatency;
            currentMinimumLatency      = other.currentMinimumLatency;
            currentMaximumLatency      = other.currentMaximumLatency;
            currentNumberSamples       = other.currentNumberSamples;

            return *this;
        }

    private:
        /**
         * The time of the earliest measurement in this aggregation.
         */
        ZoranTimeStamp currentStartZoranTimestamp;

        /**
         * The time of the last measurement in this aggregation.
         */
        ZoranTimeStamp currentEndZoranTimestamp;

        /**
         * the average latency for the points in this aggregation.
         */
        double currentMeanLatency;

        /**
         * The variance for the points in this aggregation.
         */
        double currentVarianceLatency;

        /**
         * The minimum latency found in this aggregation.
         */
        LatencyMicroseconds currentMinimumLatency;

        /**
         * The maximimum latency found in this aggregation.
         */
        LatencyMicroseconds currentMaximumLatency;

        /**
         * The number of samples in this aggregation.
         */
        unsigned long currentNumberSamples;
};

#endif
