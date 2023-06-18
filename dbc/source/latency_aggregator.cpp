/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref LatencyAggregator class.
***********************************************************************************************************************/

#include <QObject>
#include <QDateTime>
#include <QTimer>

#include "database_manager.h"
#include "customers_capabilities.h"
#include "latency_aggregator.h"
#include "latency_aggregator_private.h"

LatencyAggregator::LatencyAggregator(
        DatabaseManager* databaseManager,
        QObject*         parent
    ):QObject(
        parent
    ),impl(
        new Private(
            databaseManager,
            this
        )
    ) {
    aggregationTimer = new QTimer(this);
    aggregationTimer->setSingleShot(false);

    connect(aggregationTimer, &QTimer::timeout, this, &LatencyAggregator::startAggregation);
}


LatencyAggregator::~LatencyAggregator() {}


const QString& LatencyAggregator::inputTableName() const {
    return impl->inputTableName();
}


const QString& LatencyAggregator::outputTableName() const {
    return impl->outputTableName();
}


unsigned long LatencyAggregator::inputTableMaximumAge() const {
    return impl->inputTableMaximumAge();
}


unsigned long LatencyAggregator::resamplePeriod() const {
    return impl->resamplePeriod();
}


bool LatencyAggregator::inputAlreadyAggregated() const {
    return impl->inputAlreadyAggregated();
}


void LatencyAggregator::setParameters(
        const QString& inputTableName,
        const QString& outputTableName,
        unsigned long  inputTableMaximumAge,
        unsigned long  resamplePeriod,
        unsigned long  expungePeriod,
        bool           inputAggregated
    ) {
    impl->setParameters(
        inputTableName,
        outputTableName,
        inputTableMaximumAge,
        resamplePeriod,
        expungePeriod,
        inputAggregated
    );

    unsigned long long currentTime           = QDateTime::currentSecsSinceEpoch();
    unsigned long      secondsToNextInterval = (resamplePeriod - (currentTime % resamplePeriod)) % resamplePeriod;
    aggregationTimer->start(secondsToNextInterval * 1000ULL);
}


bool LatencyAggregator::deleteByCustomerId(const CustomersCapabilities::CustomerIdSet& customerId, unsigned threadId) {
    return impl->deleteByCustomerId(customerId, threadId);
}


void LatencyAggregator::startAggregation() {
    impl->start();
}
