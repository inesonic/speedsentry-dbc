/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref LatencyAggregator::Private class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef LATENCY_AGGREGATOR_PRIVATE_H
#define LATENCY_AGGREGATOR_PRIVATE_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QMutex>
#include <QSqlDatabase>

#include <cstdint>

#include "short_latency_entry.h"
#include "aggregated_latency_entry.h"
#include "latency_aggregator.h"

class QTimer;
class DatabaseManager;

/**
 * Private implementation of the \ref LatencyAggregator class.
 */
class LatencyAggregator::Private:public QThread {
    Q_OBJECT

    public:
        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager tracking customer data.
         *
         * \param[in] parent          Pointer to the parent object.
         */
        Private(DatabaseManager* databaseManager, QObject* parent = nullptr);

        ~Private() override;

        /**
         * Method you can use to obtain the current input table name.
         *
         * \return Returns the current input table name.
         */
        const QString& inputTableName() const;

        /**
         * Method you can use to obtain the current output table name.
         *
         * \return Returns the current output table name.
         */
        const QString& outputTableName() const;

        /**
         * Method you can use to determine the maximum age for input table entries, in seconds.
         *
         * \return Returns the maximum age for the input table entries, in seconds.
         */
        unsigned long inputTableMaximumAge() const;

        /**
         * Method you can use to determine the resample period for this aggregator.
         *
         * \return Returns the resample period for this aggregator, in seconds.
         */
        unsigned long resamplePeriod() const;

        /**
         * Method you can use to determine if the input table has already been aggregated.
         *
         * \return Returns true if the input table has already been aggregated.
         */
        bool inputAlreadyAggregated() const;

    public slots:
        /**
         * Method you can use to set the input and output table and table type.
         *
         * \param[in] inputTableName         The name of the input table.
         *
         * \param[in] outputTableName        The name of the output table.
         *
         * \param[in] inputTableMaximumAge   The maximum age of input entries, in seconds.
         *
         * \param[in] resamplePeriod         The period to run this aggregator, in seconds.
         *
         * \param[in] expungePeriod          The maximum age for any entry before being expunged.
         *
         * \param[in] inputAlreadyAggregated If true, then the input table will contain additional entries for mean and
         *                                   variance.
         */
        void setParameters(
            const QString& inputTableName,
            const QString& outputTableName,
            unsigned long  inputTableMaximumAge,
            unsigned long  resamplePeriod,
            unsigned long  expungePeriod,
            bool           inputAggregated
        );

        /**
         * Slot you can trigger to delete latency entries for a user.
         *
         * \param[in] customerIds The customer IDs of the users to have entries deleted for.
         *
         * \param[in] threadId    The ID of the thread to be used to delete these entries.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteByCustomerId(const CustomersCapabilities::CustomerIdSet& customerIds, unsigned threadId);

    protected:
        /**
         * Method that performs the aggregation in the background.
         */
        void run() override;

    private:
        /**
         * Class used to track our weights and mean values.
         */
        class WeightsAndMeans {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] meanValue     The mean value in this population.
                 *
                 * \param[in] numberSamples The number of samples in this population.
                 */
                inline WeightsAndMeans(
                        double        meanValue,
                        unsigned long numberSamples = 1
                    ):currentMeanValue(
                           meanValue
                    ),currentNumberSamples(
                        numberSamples
                    ) {}

                /**
                 * Copy constructor
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline WeightsAndMeans(
                        const WeightsAndMeans& other
                    ):currentMeanValue(
                        other.currentMeanValue
                    ),currentNumberSamples(
                        other.currentNumberSamples
                    ) {}

                ~WeightsAndMeans() = default;

                /**
                 * Method that returns the mean value.
                 *
                 * \return Returns the mean value.
                 */
                inline double meanValue() const {
                    return currentMeanValue;
                }

                /**
                 * Method that returns the number of samples or population weight.
                 *
                 * \return Returns the number of samples.
                 */
                inline unsigned long numberSamples() const {
                    return currentNumberSamples;
                }

                /**
                 * Assignment operator
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline WeightsAndMeans& operator=(const WeightsAndMeans& other) {
                    currentMeanValue     = other.currentMeanValue;
                    currentNumberSamples = other.currentNumberSamples;

                    return *this;
                }

            private:
                /**
                 * The current mean value for this population.
                 */
                double currentMeanValue;

                /**
                 * The current number of samples or weights.
                 */
                unsigned long currentNumberSamples;
        };

        /**
         * Method that obtains a list of aggregated entries.
         *
         * \param[out]    success         A boolean value that will hold true on success.
         *
         * \param[in,out] database        The database instance to be used.
         *
         * \param[in]     timeThreshold   The time threshold for this aggregation.
         *
         * \param[in]     resamplePeriod  The period for resampling operations.
         *
         * \param[in]     inputTableName  The name of the input table to be processed.
         *
         * \param[in]     inputAggregated If true, the input table has already been aggregated.  If false, the input
         *                                table holds raw data.
         *
         * \return Returns a list of aggregated values.
         */
        QList<AggregatedLatencyEntry> generateAggregatedEntries(
            bool&              success,
            QSqlDatabase&      database,
            unsigned long long timeThreshold,
            unsigned long      resamplePeriod,
            const QString&     inputTableName,
            bool               inputAggregated
        );

        /**
         * Method that writes aggregated entries to the database.
         *
         * \param[in,out] database          The database instance to be used.
         *
         * \param[in]     aggregatedEntries A list of aggregated entries to be written.
         *
         * \param[in]     outputTableName   The name of the table to write the aggregated entries to.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool writeAggregatedEntries(
            QSqlDatabase&                        database,
            const QList<AggregatedLatencyEntry>& aggregatedEntries,
            const QString&                       outputTableName
        );

        /**
         * Method that deletes old entries from the database.
         *
         * \param[in,out] database        The database instance to be used.
         *
         * \param[in]     timeThreshold   The time threshold for this aggregation.
         *
         * \param[in]     inputTableName  The name of the input table to be processed.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteOldEntries(QSqlDatabase& database, unsigned long long timeThreshold, const QString& inputTableName);

        /**
         * Method that calculates and checks field index values from a query.
         *
         * \param[in]  query                The query to obtain the field data from.
         *
         * \param[in]  inputIsAggregated    If true, then the input table is assumed to be an aggregation from other
         *                                  tables and will include an extended number of fields.  If false, the input
         *                                  table is expected to only have the monitor ID, server ID, timestamp, and
         *                                  latency columns.
         *
         * \param[out] monitorIdField       Returns the field index of the monitor ID field.
         *
         * \param[out] serveridField        Returns the field index of the server ID field.
         *
         * \param[out] timestampField       Returns the field index of the timestamp field.
         *
         * \param[out] latencyField         Returns the field index of the latency field.
         *
         * \param[out] startTimestampField  Returns the field index of the start timestamp field.
         *
         * \param[out] endTimestampField    Returns the field index of the end timestamp field.
         *
         * \param[out] meanLatencyField     Returns the field index of the mean latency field.
         *
         * \param[out] varianceLatencyField Returns the field index of the variance field.
         *
         * \param[out] minimumLatencyField  Returns the field index of the minimum latency field.
         *
         * \param[out] maximumLatencyField  Returns the field index of the maximum latency field.
         *
         * \param[out] numberSamplesField   Returns the field index of the number samples field.
         *
         * \return Returns true on success.  Returns false on error.
         */
        static bool getFieldIndexes(
            const QSqlQuery& query,
            bool             inputIsAggregated,
            int&             monitorIdField,
            int&             serverIdField,
            int&             timestampField,
            int&             latencyField,
            int&             startTimestampField,
            int&             endTimestampField,
            int&             meanLatencyField,
            int&             varianceLatencyField,
            int&             minimumLatencyField,
            int&             maximumLatencyField,
            int&             numberSamplesField
        );

        /**
         * Method that gets next next value.
         *
         * \param[in]  query                The query to obtain the field data from.
         *
         * \param[in]  inputIsAggregated    If true, then the input table is assumed to be an aggregation from other
         *                                  tables and will include an extended number of fields.  If false, the input
         *                                  table is expected to only have the monitor ID, server ID, timestamp, and
         *                                  latency columns.
         *
         * \param[out] monitorIdField       Returns the field index of the monitor ID field.
         *
         * \param[out] serveridField        Returns the field index of the server ID field.
         *
         * \param[out] timestampField       Returns the field index of the timestamp field.
         *
         * \param[out] latencyField         Returns the field index of the latency field.
         *
         * \param[out] startTimestampField  Returns the field index of the start timestamp field.
         *
         * \param[out] endTimestampField    Returns the field index of the end timestamp field.
         *
         * \param[out] meanLatencyField     Returns the field index of the mean latency field.
         *
         * \param[out] varianceLatencyField Returns the field index of the variance field.
         *
         * \param[out] minimumLatencyField  Returns the field index of the minimum latency field.
         *
         * \param[out] maximumLatencyField  Returns the field index of the maximum latency field.
         *
         * \param[out] numberSamplesField   Returns the field index of the number samples field.
         *
         * \param[out] monitorId            Returns the monitor ID field value.
         *
         * \param[out] serverid             Returns the server ID field value.
         *
         * \param[out] timestamp            Returns the timestamp field value.
         *
         * \param[out] latency              Returns the latency field value.
         *
         * \param[out] startTimestamp       Returns the start timestamp field value.
         *
         * \param[out] endTimestamp         Returns the end timestamp field value.
         *
         * \param[out] meanLatency          Returns the mean latency field value.
         *
         * \param[out] varianceLatency      Returns the variance field value.
         *
         * \param[out] minimumLatency       Returns the minimum latency field value.
         *
         * \param[out] maximumLatency       Returns the maximum latency field value.
         *
         * \param[out] numberSamples        Returns the number samples field value.
         *
         * \return Returns true on success.  Returns false on error.
         */
        static bool getRecord(
            const QSqlQuery&                   query,
            bool                               inputIsAggregated,
            int                                monitorIdField,
            int                                serverIdField,
            int                                timestampField,
            int                                latencyField,
            int                                startTimestampField,
            int                                endTimestampField,
            int                                meanLatencyField,
            int                                varianceLatencyField,
            int                                minimumLatencyField,
            int                                maximumLatencyField,
            int                                numberSamplesField,
            Monitor::MonitorId&                monitorId,
            Server::ServerId&                  serverId,
            unsigned long long&                timestamp,
            LatencyEntry::LatencyMicroseconds& latency,
            unsigned long long&                startTimestamp,
            unsigned long long&                endTimestamp,
            double&                            meanLatency,
            double&                            varianceLatency,
            double&                            minimumLatency,
            double&                            maximumLatency,
            unsigned long&                     numberSamples
        );

        /**
         * Method that processes the collected result data to return a new aggregation entry.
         *
         * \param[in]     monitorId              The monitor ID of the monitor that collected this population.
         *
         * \param[in]     serverId               The server ID of the server that collected this population.
         *
         * \param[in] weightedSumMeanLatency     The sum of the mean values from each input times the number of samples
         *                                       from each input, i.e.: \f$ \sum_{i} n _ i \mu _ i \f$.
         *
         * \param[in] weightedSumVarianceLatency The sum of the variances from each input times the number of samples
         *                                       in each input, i.e.: \f$ \sum_{i} n _ i \sigma ^ 2 _ i \f$.
         *
         * \param[in] rawValues                  A list of raw values.  This list is used to select a random entry from
         *                                       the sample population.
         *
         * \param[in] weightsAndMeans            A list of pairs of populations and mean values.  Used to calculate our
         *                                       resulting variance.
         *
         * \param[in] periodStartTimestamp       The earliest measured timestamp in the input population.
         *
         * \param[in] periodEndTimestamp         The latest measured timestamp in the input population.
         *
         * \param[in] minimumLatency             The minimum latency measured across the input population.
         *
         * \param[in] maximumLatency             The maximum latency measured across the input population.
         *
         * \param[in] numberSamples              The number of raw samples that represent this population.
         *
         * \return Returns a \ref AggregatedLatencyEntry instance generated from the data above.
         */
        AggregatedLatencyEntry generateEntry(
            AggregatedLatencyEntry::MonitorId           monitorId,
            AggregatedLatencyEntry::ServerId            serverId,
            double                                      weightedSumMeanLatency,
            double                                      weightedSumVarianceLatency,
            const QList<ShortLatencyEntry>&             rawValues,
            const QList<WeightsAndMeans>&               weightsAndMeans,
            unsigned long long                          periodStartTimestamp,
            unsigned long long                          periodEndTimestamp,
            AggregatedLatencyEntry::LatencyMicroseconds minimumLatency,
            AggregatedLatencyEntry::LatencyMicroseconds maximumLatency,
            unsigned long                               numberSamples
        );

        /**
         * Method that calculates a random value  The algorithm uses the XORSHRO256++ algorithm with a random initial
         * seeds.
         */
        std::uint32_t prng();

        /**
         * Mutex used to control access to variables across threads.
         */
        QMutex accessMutex;

        /**
         * The database manager to use to access the underlying database.
         */
        DatabaseManager* currentDatabaseManager;

        /**
         * The input table name.
         */
        QString currentInputTableName;

        /**
         * The output table name.
         */
        QString currentOutputTableName;

        /**
         * The maximum input table entry age, in seconds.
         */
        unsigned long currentInputTableMaximumAge;

        /**
         * The resample period, in seconds.
         */
        unsigned long currentResamplePeriod;

        /**
         * The maximum age to allow entries to exist before deleting them.
         */
        unsigned long currentExpungePeriod;

        /**
         * Holds true if the input table has already been aggregated.
         */
        bool currentInputAggregated;

        /**
         * Seed used to generate random values.
         */
        std::uint64_t seed[4];

        /**
         * Value used to generate fewer random values.
         */
        std::uint32_t nextPrngValue;

        /**
         * Flag indicating that the next value should be used.
         */
        bool useNextPrngValue;
};

#endif
