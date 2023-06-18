/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref LatencyAggregator class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef LATENCY_AGGREGATOR_H
#define LATENCY_AGGREGATOR_H

#include <QObject>
#include <QString>

#include <customers_capabilities.h>

class QTimer;
class QSqlQuery;
class DatabaseManager;

/**
 * Class that aggregates latency information to reduce the number of database entries.
 *
 * This class is given the name of an input and an output table.  Input tables should be of the form:
 *
 *     CREATE TABLE latency_seconds (
 *         monitor_id  INTEGER  UNSIGNED NOT NULL,
 *         server_id   SMALLINT UNSIGNED NOT NULL,
 *         timestamp   INTEGER  UNSIGNED NOT NULL,
 *         latency     INTEGER  UNSIGNED NOT NULL,
 *         KEY latency_seconds_constraint_1 (monitor_id),
 *         CONSTRAINT latency_seconds_constraint_1 FOREIGN KEY (monitor_id)
 *             REFERENCES monitor (monitor_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION
 *         KEY latency_seconds_constraint_2 (server_id),
 *         CONSTRAINT latency_seconds_constraint_2 FOREIGN KEY (server_id)
 *             REFERENCES servers (server_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1;
 *
 * or:
 *
 *     CREATE TABLE latency_aggregated (
 *         monitor_id              INTEGER  UNSIGNED NOT NULL,
 *         server_id               SMALLINT UNSIGNED NOT NULL,
 *         timestamp               INTEGER  UNSIGNED NOT NULL,
 *         latency                 INTEGER  UNSIGNED NOT NULL,
 *         start_timestamp         INTEGER  UNSIGNED NOT NULL,
 *         end_timestamp           INTEGER  UNSIGNED NOT NULL,
 *         mean_latency            DOUBLE            NOT NULL,
 *         variance_latency        DOUBLE            NOT NULL,
 *         minimum_latency         INTEGER  UNSIGNED NOT NULL,
 *         maximum_latency         INTEGER  UNSIGNED NOT NULL,
 *         number_samples          INTEGER  UNSIGNED NOT NULL,
 *         KEY latency_aggregated_constraint_1 (monitor_id),
 *         CONSTRAINT latency_aggregated_constraint_1 FOREIGN KEY (monitor_id)
 *             REFERENCES monitor (monitor_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION
 *         KEY latency_aggregated_constraint_2 (server_id),
 *         CONSTRAINT latency_aggregated_constraint_2 FOREIGN KEY (server_id)
 *             REFERENCES servers (server_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1;
 *
 * The resulting table will always be of the second form.
 */
class LatencyAggregator:public QObject {
    Q_OBJECT

    public:
        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager tracking customer data.
         *
         * \param[in] parent          Pointer to the parent object.
         */
        LatencyAggregator(DatabaseManager* databaseManager, QObject* parent = nullptr);

        ~LatencyAggregator() override;

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

    private:
        /**
         * Method that is triggered to start the aggregation function.
         */
        void startAggregation();

    private:
        /**
         * Timer used to trigger the underlying aggregator at period intervals.
         */
        QTimer* aggregationTimer;

        /**
         * The underlying implementation.
         */
        class Private;

        /**
         * The underlying implementation.
         */
        Private* impl;
};

#endif
