/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref LatencyPlotter class.
***********************************************************************************************************************/

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QPair>
#include <QList>
#include <QImage>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QtCharts>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QLinearGradient>
#include <QRectF>
#include <QPointF>
#include <QPainter>

#include <cmath>
#include <algorithm>

#include "short_latency_entry.h"
#include "latency_entry.h"
#include "aggregated_latency_entry.h"
#include "latency_interface_manager.h"
#include "plotter_base.h"
#include "latency_plotter.h"

LatencyPlotter::LatencyPlotter(
        LatencyInterfaceManager* latencyInterfaceManager,
        QObject*                 parent
    ):PlotterBase(
        parent
    ),currentLatencyInterfaceManager(
        latencyInterfaceManager
    ) {
    connect(this, &LatencyPlotter::issueRequestHistogramPlot, this, &LatencyPlotter::generateHistogramPlot);
    connect(this, &LatencyPlotter::issueRequestHistoryPlot, this, &LatencyPlotter::generateHistoryPlot);
}


LatencyPlotter::~LatencyPlotter() {
}


PlotMailbox& LatencyPlotter::requestHistoryPlot(
        LatencyPlotter::ThreadId     threadId,
        LatencyPlotter::CustomerId   customerId,
        LatencyPlotter::RegionId     regionId,
        LatencyPlotter::ServerId     serverId,
        LatencyPlotter::HostSchemeId hostSchemeId,
        LatencyPlotter::MonitorId    monitorId,
        unsigned long long           startTimestamp,
        unsigned long long           endTimestamp,
        const QString&               titleText,
        const QString&               xAxisTitle,
        const QString&               yAxisTitle,
        const QString&               dateFormatString,
        const QString&               titleFont,
        const QString&               axisTitleFont,
        const QString&               axisLabelFont,
        double                       maximumLatency,
        double                       minimumLatency,
        bool                         logScale,
        unsigned                     width,
        unsigned                     height
    ) {
    PlotMailbox& mb = mailbox(threadId);
    mb.forceEmpty();

    emit issueRequestHistoryPlot(
        threadId,
        customerId,
        regionId,
        serverId,
        hostSchemeId,
        monitorId,
        startTimestamp,
        endTimestamp,
        titleText,
        xAxisTitle,
        yAxisTitle,
        dateFormatString,
        titleFont,
        axisTitleFont,
        axisLabelFont,
        maximumLatency,
        minimumLatency,
        logScale,
        width,
        height
    );

    return mb;
}


PlotMailbox& LatencyPlotter::requestHistogramPlot(
        LatencyPlotter::ThreadId     threadId,
        LatencyPlotter::CustomerId   customerId,
        LatencyPlotter::RegionId     regionId,
        LatencyPlotter::ServerId     serverId,
        LatencyPlotter::HostSchemeId hostSchemeId,
        LatencyPlotter::MonitorId    monitorId,
        unsigned long long           startTimestamp,
        unsigned long long           endTimestamp,
        const QString&               titleText,
        const QString&               xAxisTitle,
        const QString&               yAxisTitle,
        const QString&               titleFont,
        const QString&               axisTitleFont,
        const QString&               axisLabelFont,
        double                       maximumLatency,
        double                       minimumLatency,
        unsigned                     width,
        unsigned                     height
    ) {
    PlotMailbox& mb = mailbox(threadId);
    mb.forceEmpty();

    emit issueRequestHistogramPlot(
        threadId,
        customerId,
        regionId,
        serverId,
        hostSchemeId,
        monitorId,
        startTimestamp,
        endTimestamp,
        titleText,
        xAxisTitle,
        yAxisTitle,
        titleFont,
        axisTitleFont,
        axisLabelFont,
        maximumLatency,
        minimumLatency,
        width,
        height
    );

    return mb;
}


PlotMailbox& LatencyPlotter::mailbox(unsigned threadId) {
    PlotMailbox* result;
    QMutexLocker locker(&mailboxMutex);

    unsigned currentNumberMailboxes = static_cast<unsigned>(mailboxes.size());
    if (currentNumberMailboxes <= threadId) {
        mailboxes.reserve(threadId + 1);
        unsigned numberRemaining = threadId - currentNumberMailboxes + 1;
        do {
            result = new PlotMailbox;
            result->moveToThread(thread());
            result->setParent(this);
            mailboxes.append(result);

            --numberRemaining;
        } while (numberRemaining > 0);
    } else {
        result = mailboxes[threadId];
    }

    return *result;
}


void LatencyPlotter::generateHistoryPlot(
        unsigned           threadId,
        unsigned long      customerId,
        unsigned           regionId,
        unsigned           serverId,
        unsigned long      hostSchemeId,
        unsigned long      monitorId,
        unsigned long long startTimestamp,
        unsigned long long endTimestamp,
        const QString&     titleText,
        const QString&     xAxisTitle,
        const QString&     yAxisTitle,
        const QString&     dateFormatString,
        const QString&     titleFont,
        const QString&     axisTitleFont,
        const QString&     axisLabelFont,
        double             maximumLatency,
        double             minimumLatency,
        bool               logScale,
        unsigned           width,
        unsigned           height
    ) {
    typedef QList<AggregatedLatencyEntry>                 AggregatedEntryList;
    typedef QMap<unsigned long long, AggregatedEntryList> MergedEntries;

    fixTimestamp(startTimestamp, endTimestamp);

    LatencyInterfaceManager::LatencyEntryLists latencyData = currentLatencyInterfaceManager->getLatencyEntries(
        customerId,
        hostSchemeId,
        monitorId,
        regionId,
        serverId,
        startTimestamp,
        endTimestamp,
        databaseThreadId
    );

    LatencyInterfaceManager::LatencyEntryList           latencyEntryList           = latencyData.first;
    LatencyInterfaceManager::AggregatedLatencyEntryList aggregatedLatencyEntryList = latencyData.second;

    QtCharts::QLineSeries* recentSeries                = new QtCharts::QLineSeries();
    QtCharts::QLineSeries* aggregatedMinimumSeries     = new QtCharts::QLineSeries();
    QtCharts::QLineSeries* aggregatedMaximumSeries     = new QtCharts::QLineSeries();
    QtCharts::QLineSeries* aggregatedStdDevLowerSeries = new QtCharts::QLineSeries();
    QtCharts::QLineSeries* aggregatedStdDevUpperSeries = new QtCharts::QLineSeries();
    QtCharts::QAreaSeries* oneSigmaAreaSeries          = new QtCharts::QAreaSeries(
        aggregatedStdDevLowerSeries,
        aggregatedStdDevUpperSeries
    );

    QColor inesonicBlue(0x17, 0x6E, 0xDA);
//    QColor inesonicOrange(0xE0, 0x7E, 0x26);

    QPen recentSeriesPen(QBrush(inesonicBlue), 1.5);
    recentSeries->setPen(recentSeriesPen);

    QPen minMaxSeriesPen(QBrush(Qt::GlobalColor::red), 0.5);
    aggregatedMinimumSeries->setPen(minMaxSeriesPen);
    aggregatedMaximumSeries->setPen(minMaxSeriesPen);

    QColor inesonicBlueTranslucent(0x18, 0x6E, 0xDA, 32);
    QLinearGradient areaGradient(QPointF(0, 0), QPointF(0, 1));
    areaGradient.setColorAt(0.0, inesonicBlueTranslucent.lighter(120));
    areaGradient.setColorAt(1.0, inesonicBlueTranslucent.darker(120));

    QPen areaSeriesPen(QBrush(inesonicBlue), 0.25);
    oneSigmaAreaSeries->setPen(areaSeriesPen);
    oneSigmaAreaSeries->setBrush(areaGradient);

    unsigned           latencyEntryListSize     = static_cast<unsigned>(latencyEntryList.size());
    unsigned long long minimumTime              = std::numeric_limits<unsigned long long>::max();
    unsigned long long maximumTime              = 0;
    double             minimum                  = std::numeric_limits<double>::max();
    double             maximum                  = -minimum;
    bool               showDayOfWeek            = (dateFormatString == "dow");

    MergedEntries mergedEntries;
    unsigned      aggregatedLatencyEntryListSize = static_cast<unsigned>(aggregatedLatencyEntryList.size());
    if (aggregatedLatencyEntryListSize > 0) {

        unsigned long long   periodStartTimestamp = 0;
        unsigned long long   periodEndTimestamp   = 0;

        AggregatedEntryList* currentList = nullptr;
        for (unsigned i=0 ; i<aggregatedLatencyEntryListSize ; ++i) {
            const AggregatedLatencyEntry& entry          = aggregatedLatencyEntryList.at(i);
            unsigned long long            startTimestamp = entry.startTimestamp();
            unsigned long long            endTimestamp   = entry.endTimestamp();

            if (startTimestamp < periodStartTimestamp || endTimestamp > periodEndTimestamp || currentList == nullptr) {
                periodStartTimestamp = startTimestamp;
                periodEndTimestamp   = endTimestamp;

                MergedEntries::iterator it = mergedEntries.find(startTimestamp);
                if (it == mergedEntries.end()) {
                    it = mergedEntries.insert(startTimestamp, AggregatedEntryList());
                }

                currentList = &it.value();
            }

            currentList->append(entry);
        }

        minimumTime = mergedEntries.firstKey();
        maximumTime = mergedEntries.last().last().endTimestamp();
    }

    if (latencyEntryListSize > 0) {
        minimumTime = std::min(
            minimumTime,
            static_cast<unsigned long long>(latencyEntryList.first().unixTimestamp())
        );
        maximumTime = std::max(
            maximumTime,
            static_cast<unsigned long long>(latencyEntryList.last().unixTimestamp())
        );
    }

    unsigned long long weekStartTimestamp = 0;
    if (showDayOfWeek) {
        QDateTime startDateTime  = QDateTime::fromSecsSinceEpoch(minimumTime, Qt::TimeSpec::UTC);
        QDate     startDate      = startDateTime.date();
        QDate     dayStartOfWeek = startDate.addDays(1 - startDate.dayOfWeek());
        QDateTime weekStart      = QDateTime(dayStartOfWeek, QTime(0, 0), Qt::TimeSpec::UTC, 0);

        weekStartTimestamp = weekStart.toSecsSinceEpoch();
    }

    if (aggregatedLatencyEntryListSize > 0) {
        for (  MergedEntries::const_iterator timeSpanIterator    = mergedEntries.constBegin(),
                                             timeSpanEndIterator = mergedEntries.constEnd()
             ; timeSpanIterator != timeSpanEndIterator
             ; ++timeSpanIterator
            ) {
            const AggregatedEntryList& entryList                  = timeSpanIterator.value();
            unsigned long              numberEntries              = static_cast<unsigned long>(entryList.size());
            unsigned long long         aggregatedStartTime        = entryList.first().startTimestamp();
            unsigned long long         aggregatedEndTime          = entryList.last().endTimestamp();
            unsigned long              aggregatedNumberSamples    = 0;
            double                     aggregatedMinimumLatency   = std::numeric_limits<double>::max();
            double                     aggregatedMaximumLatency   = 0;
            double                     weightedSumMeanLatency     = 0;
            double                     weightedSumVarianceLatency = 0;

            for (unsigned long i=0 ; i<numberEntries ; ++i) {
                const AggregatedLatencyEntry& entry         = entryList.at(i);
                double                        avg           = entry.meanLatency() * 1.0E-6;
                double                        min           = entry.minimumLatency() * 1.0E-6;
                double                        max           = entry.maximumLatency() * 1.0E-6;
                double                        variance      = entry.varianceLatency() * 1.0E-12;
                unsigned long                 numberSamples = entry.numberSamples();

                if (min < aggregatedMinimumLatency) {
                    aggregatedMinimumLatency = min;
                }

                if (max > aggregatedMaximumLatency) {
                    aggregatedMaximumLatency = max;
                }

                weightedSumMeanLatency     += numberSamples * avg;
                weightedSumVarianceLatency += numberSamples * variance;
                aggregatedNumberSamples    += numberSamples;
            }

            double meanLatency = weightedSumMeanLatency / aggregatedNumberSamples;
            for (unsigned long i=0 ; i<numberEntries ; ++i) {
                const AggregatedLatencyEntry& entry = entryList.at(i);
                double                        d     = entry.meanLatency() * 1.0E-6 - meanLatency;
                weightedSumVarianceLatency += entry.numberSamples() * d * d;
            }

            double varianceLatency = weightedSumVarianceLatency / aggregatedNumberSamples;
            double stdDeviation    = std::sqrt(varianceLatency);
            double lower1Sigma     = std::max(0.0, meanLatency - stdDeviation);
            double upper1Sigma     = meanLatency + stdDeviation;

            if (showDayOfWeek) {
                double startDow = 1 + static_cast<double>(aggregatedStartTime - weekStartTimestamp) / secondsPerDay;
                if (startDow < 8.0) {
                    double endDow   = 1 + static_cast<double>(aggregatedEndTime - weekStartTimestamp) / secondsPerDay;
                    if (endDow >= 8.0) {
                        endDow = 7.9999999;
                    }

                    recentSeries->append(startDow, meanLatency);
                    aggregatedMinimumSeries->append(startDow, aggregatedMinimumLatency);
                    aggregatedMaximumSeries->append(startDow, aggregatedMaximumLatency);
                    aggregatedStdDevLowerSeries->append(startDow, lower1Sigma);
                    aggregatedStdDevUpperSeries->append(startDow, upper1Sigma);

                    recentSeries->append(endDow, meanLatency);
                    aggregatedMinimumSeries->append(endDow, aggregatedMinimumLatency);
                    aggregatedMaximumSeries->append(endDow, aggregatedMaximumLatency);
                    aggregatedStdDevLowerSeries->append(endDow, lower1Sigma);
                    aggregatedStdDevUpperSeries->append(endDow, upper1Sigma);
                }
            } else {
                unsigned long long startValue = aggregatedStartTime * 1000ULL;
                unsigned long long endValue   = aggregatedEndTime * 1000ULL;

                recentSeries->append(startValue, meanLatency);
                aggregatedMinimumSeries->append(startValue, aggregatedMinimumLatency);
                aggregatedMaximumSeries->append(startValue, aggregatedMaximumLatency);
                aggregatedStdDevLowerSeries->append(startValue, lower1Sigma);
                aggregatedStdDevUpperSeries->append(startValue, upper1Sigma);

                recentSeries->append(endValue, meanLatency);
                aggregatedMinimumSeries->append(endValue, aggregatedMinimumLatency);
                aggregatedMaximumSeries->append(endValue, aggregatedMaximumLatency);
                aggregatedStdDevLowerSeries->append(endValue, lower1Sigma);
                aggregatedStdDevUpperSeries->append(endValue, upper1Sigma);
            }

            if (aggregatedMinimumLatency < minimum) {
                minimum = aggregatedMinimumLatency;
            }

            if (aggregatedMaximumLatency > maximum) {
                maximum = aggregatedMaximumLatency;
            }
        }
    }

//    unsigned long long aggregatedEntriesEndTime = maximumTime;

    if (latencyEntryListSize > 0) {
        for (unsigned i=0 ; i<latencyEntryListSize ; ++i) {
            const LatencyEntry& entry = latencyEntryList.at(i);
            unsigned long long  unixTimestamp  = entry.unixTimestamp();
            double              latencySeconds = entry.latencySeconds();

            if (minimum > latencySeconds) {
                minimum = latencySeconds;
            }

            if (maximum < latencySeconds) {
                maximum = latencySeconds;
            }

            if (showDayOfWeek) {
                double dow = 1 + static_cast<double>(unixTimestamp - weekStartTimestamp) / secondsPerDay;
                if (dow < 8.0) {
                    recentSeries->append(dow, latencySeconds);
                }
            } else {
                recentSeries->append(unixTimestamp * 1000ULL, latencySeconds);
            }
        }
    }

    if (minimum == maximum) {
        minimum = 0.9 * minimum;
        maximum = 1.1 * maximum;
    } else if (minimum > maximum) {
        minimum = 0;
        maximum = 1.0;
    }

    QtCharts::QChart* chart = new QtCharts::QChart();
    chart->addSeries(recentSeries);
    chart->addSeries(aggregatedMinimumSeries);
    chart->addSeries(aggregatedMaximumSeries);
    chart->addSeries(oneSigmaAreaSeries);

    chart->legend()->hide();
    chart->setTitle(titleText);

    bool fontOk;
    QFont newTitleFont = toFont(titleFont, &fontOk);
    if (fontOk) {
        chart->setTitleFont(newTitleFont);
    }

    QtCharts::QAbstractAxis* axisX;
    if (showDayOfWeek) {
        QtCharts::QCategoryAxis* axisXValue = new QtCharts::QCategoryAxis();
        axisXValue->setTickCount(8);
        axisXValue->setLabelFormat("%1.0f");
        axisXValue->setRange(1.0, 8.0);
        axisXValue->append("Mon", 2.0);
        axisXValue->append("Tue", 3.0);
        axisXValue->append("Wed", 4.0);
        axisXValue->append("Thu", 5.0);
        axisXValue->append("Fri", 6.0);
        axisXValue->append("Sat", 7.0);
        axisXValue->append("Sun", 8.0);

        axisX = axisXValue;
    } else {
        QtCharts::QDateTimeAxis* axisXDateTime = new QtCharts::QDateTimeAxis;
        axisXDateTime->setTickCount(5);
        axisXDateTime->setFormat(dateFormatString); // "MMM dd - hh:mm");

        axisX = axisXDateTime;
    }

    axisX->setTitleText(xAxisTitle);
    chart->addAxis(axisX, Qt::AlignmentFlag::AlignBottom);

    recentSeries->attachAxis(axisX);
    aggregatedMinimumSeries->attachAxis(axisX);
    aggregatedMaximumSeries->attachAxis(axisX);
    oneSigmaAreaSeries->attachAxis(axisX);

    QtCharts::QAbstractAxis* axisY;
    if (logScale) {
        axisY = new QtCharts::QLogValueAxis;
    } else {
        axisY = new QtCharts::QValueAxis;
    }

    axisY->setTitleText(yAxisTitle);

    QFont newAxisTitleFont = toFont(axisTitleFont, &fontOk);
    if (fontOk) {
        axisX->setTitleFont(newAxisTitleFont);
        axisY->setTitleFont(newAxisTitleFont);
    }

    QFont newAxisLabelFont = toFont(axisLabelFont, &fontOk);
    if (fontOk) {
        axisX->setLabelsFont(newAxisLabelFont);
        axisY->setLabelsFont(newAxisLabelFont);
    }

    chart->addAxis(axisY, Qt::AlignmentFlag::AlignLeft);

    recentSeries->attachAxis(axisY);
    aggregatedMinimumSeries->attachAxis(axisY);
    aggregatedMaximumSeries->attachAxis(axisY);
    oneSigmaAreaSeries->attachAxis(axisY);

    if (!logScale) {
        if (minimumLatency < 0 && maximumLatency < 0) {
            unsigned recommendedTickCount = calculateNiceRange(minimum, maximum);

            static_cast<QtCharts::QValueAxis*>(axisY)->setRange(minimum, maximum);
            static_cast<QtCharts::QValueAxis*>(axisY)->setTickCount(recommendedTickCount + 1);
            static_cast<QtCharts::QValueAxis*>(axisY)->setMinorTickCount(1);
        } else {
            static_cast<QtCharts::QValueAxis*>(axisY)->setMin(minimumLatency >= 0 ? minimumLatency : minimum);
            static_cast<QtCharts::QValueAxis*>(axisY)->setMax(maximumLatency >= 0 ? maximumLatency : maximum);
            static_cast<QtCharts::QValueAxis*>(axisY)->setMinorTickCount(1);
            static_cast<QtCharts::QValueAxis*>(axisY)->applyNiceNumbers();
        }
    } else {
        calculateNiceLogRange(minimum, maximum);

        static_cast<QtCharts::QLogValueAxis*>(axisY)->setMin(minimumLatency >= 0 ? minimumLatency : minimum);
        static_cast<QtCharts::QLogValueAxis*>(axisY)->setMax(maximumLatency >= 0 ? maximumLatency : maximum);
        static_cast<QtCharts::QLogValueAxis*>(axisY)->setGridLineColor(QColor(0xC0, 0xC0, 0xC0));
        static_cast<QtCharts::QLogValueAxis*>(axisY)->setMinorTickCount(9);
    }

    QGraphicsScene scene;
    scene.addItem(chart);

    chart->setGeometry(0, 0, width, height);
    scene.setSceneRect(0, 0, width, height);

//    if (aggregatedLatencyEntryListSize > 0 && latencyEntryListSize > 0 && !showDayOfWeek) {
//        QRectF plotActiveArea = chart->plotArea();

//        unsigned long long leftX  = static_cast<QtCharts::QDateTimeAxis*>(axisX)->min().toMSecsSinceEpoch();
//        unsigned long long rightX = static_cast<QtCharts::QDateTimeAxis*>(axisX)->max().toMSecsSinceEpoch();
//        unsigned long long lineX  = aggregatedEntriesEndTime * 1000ULL;

//        float              sceneR        = static_cast<float>(lineX - leftX)/static_cast<float>(rightX - leftX);
//        float              sceneX        = plotActiveArea.width() * sceneR + plotActiveArea.left();
//        QGraphicsLineItem* separatorLine = new QGraphicsLineItem(
//            sceneX, plotActiveArea.top(),
//            sceneX, plotActiveArea.bottom()
//        );

//        QPen separatorLinePen(QBrush(inesonicOrange), 2);
//        separatorLine->setPen(separatorLinePen);

//        scene.addItem(separatorLine);
//    }

    QImage result(width, height, QImage::Format::Format_RGB888);
    result.fill(Qt::GlobalColor::white);

    QPainter painter(&result);

    scene.render(&painter);
    painter.end();

    PlotMailbox& mb = mailbox(threadId);
    mb.sendImage(result);
}


void LatencyPlotter::generateHistogramPlot(
        unsigned           threadId,
        unsigned long      customerId,
        unsigned           regionId,
        unsigned           serverId,
        unsigned long      hostSchemeId,
        unsigned long      monitorId,
        unsigned long long startTimestamp,
        unsigned long long endTimestamp,
        const QString&     titleText,
        const QString&     xAxisTitle,
        const QString&     yAxisTitle,
        const QString&     titleFont,
        const QString&     axisTitleFont,
        const QString&     axisLabelFont,
        double             maximumLatency,
        double             minimumLatency,
        unsigned           width,
        unsigned           height
    ) {
    fixTimestamp(startTimestamp, endTimestamp);

    LatencyInterfaceManager::LatencyEntryLists latencyData = currentLatencyInterfaceManager->getLatencyEntries(
        customerId,
        hostSchemeId,
        monitorId,
        regionId,
        serverId,
        startTimestamp,
        endTimestamp,
        databaseThreadId
    );

    LatencyInterfaceManager::LatencyEntryList           latencyEntryList           = latencyData.first;
    LatencyInterfaceManager::AggregatedLatencyEntryList aggregatedLatencyEntryList = latencyData.second;

    unsigned long latencyEntryListSize           = static_cast<unsigned long>(latencyEntryList.size());
    unsigned long aggregatedLatencyEntryListSize = static_cast<unsigned long>(aggregatedLatencyEntryList.size());
    unsigned long totalEntries                   = latencyEntryListSize + aggregatedLatencyEntryListSize;

    double minimum = minimumLatency;
    double maximum = maximumLatency;

    QVector<double> latencyValues;
    latencyValues.reserve(totalEntries);

    double min = std::numeric_limits<double>::max();
    double max = -minimum;

    for (unsigned long i=0 ; i<latencyEntryListSize ; ++i) {
        const LatencyEntry& entry          = latencyEntryList.at(i);
        double              latencySeconds = entry.latencySeconds();

        latencyValues.append(latencySeconds);

        if (latencySeconds < min) {
            min = latencySeconds;
        }

        if (latencySeconds > max) {
            max = latencySeconds;
        }
    }

    for (unsigned long i=0 ; i<aggregatedLatencyEntryListSize ; ++i) {
        const AggregatedLatencyEntry& entry          = aggregatedLatencyEntryList.at(i);
        double                        latencySeconds = entry.latencySeconds();

        latencyValues.append(latencySeconds);

        if (latencySeconds < min) {
            min = latencySeconds;
        }

        if (latencySeconds > max) {
            max = latencySeconds;
        }
    }

    if (min == max) {
        min = 0.9 * min;
        max = 1.1 * max;
    } else if (min > max) {
        min = 0;
        max = 1.0;
    }

    if (minimum < 0) {
        minimum = min;
    }

    if (maximum < 0) {
        maximum = max;
    }

    unsigned recommendedNumberSteps = calculateNiceRange(minimum, maximum);

    unsigned numberBuckets = static_cast<unsigned>(std::min(100UL, totalEntries / 500));
    if (numberBuckets < recommendedNumberSteps) {
        numberBuckets = recommendedNumberSteps;
        while (numberBuckets < 10) {
            numberBuckets <<= 1;
        }
    } else {
        if (numberBuckets > 100) {
            numberBuckets = 100;
        }

        unsigned d = numberBuckets / recommendedNumberSteps;
        numberBuckets = d * recommendedNumberSteps;
    }

    unsigned minorTicksPerMajorTick = (numberBuckets / recommendedNumberSteps) - 1;

    double bucketWidth = (maximum - minimum) / numberBuckets;
    QVector<unsigned long> counts(numberBuckets, 0UL);

    double sumValues       = 0;
    double sumSquareValues = 0;
    for (unsigned long i=0 ; i<totalEntries ; ++i) {
        double v = latencyValues.at(i);
        if (v >= minimum && v <= maximum) {
            sumValues       += v;
            sumSquareValues += v * v;

            unsigned bucketIndex = static_cast<unsigned>((v - minimum) / bucketWidth);
            if (bucketIndex >= numberBuckets) {
                ++counts.last();
            } else {
                ++counts[bucketIndex];
            }
        }
    }

    double average = sumValues / totalEntries;
    double stdDev  = std::sqrt(sumSquareValues / totalEntries - average * average);

    double maximumCount = 0;
    QtCharts::QBarSet* barSet = new QtCharts::QBarSet(QString());
    for (unsigned bucketIndex=0 ; bucketIndex<numberBuckets ; ++bucketIndex) {
        unsigned count = static_cast<unsigned>(counts.at(bucketIndex));
        barSet->append(count);
        if (count > maximumCount) {
            maximumCount = count;
        }
    }

    double   minimumCount     = 0;
    unsigned numberCountTicks = calculateNiceRange(minimumCount, maximumCount);

    QtCharts::QBarSeries* barSeries = new QtCharts::QBarSeries;
    barSeries->append(barSet);

    QColor inesonicBlue(0x17, 0x6E, 0xDA);

    QBrush barSetBrush(inesonicBlue);
    QPen   barSetPen(QBrush(inesonicBlue.darker(150)), 1);

    barSet->setBrush(barSetBrush);
    barSet->setPen(barSetPen);

    QtCharts::QChart* chart = new QtCharts::QChart();
    chart->addSeries(barSeries);

    chart->legend()->hide();
    chart->setTitle(titleText);

    bool fontOk;
    QFont newTitleFont = toFont(titleFont, &fontOk);
    if (fontOk) {
        chart->setTitleFont(newTitleFont);
    }

    QtCharts::QValueAxis* axisX = new QtCharts::QValueAxis;
    axisX->setRange(minimum, maximum);
    axisX->setTickCount(recommendedNumberSteps + 1);
    axisX->setMinorTickCount(minorTicksPerMajorTick);
    axisX->setMinorGridLineColor(QColor(0xF4, 0xF4, 0xF4));
    axisX->setTickAnchor(minimum);
    axisX->setTitleText(xAxisTitle);

    if (maximum > 10) {
        axisX->setLabelFormat("%.0f");
    } else if (maximum >= 1) {
        axisX->setLabelFormat("%.1f");
    } else if (maximum >= 0.1) {
        axisX->setLabelFormat("%.2f");
    } else if (maximum >= 0.01) {
        axisX->setLabelFormat("%.3f");
    }

    QtCharts::QValueAxis* axisY = new QtCharts::QValueAxis;
    axisY->setRange(minimumCount, maximumCount);
    axisY->setTickCount(numberCountTicks + 1);
    axisY->setTickAnchor(minimumCount);
    axisY->setTitleText(yAxisTitle);

    QFont newAxisTitleFont = toFont(axisTitleFont, &fontOk);
    if (fontOk) {
        axisX->setTitleFont(newAxisTitleFont);
        axisY->setTitleFont(newAxisTitleFont);
    }

    QFont newAxisLabelFont = toFont(axisLabelFont, &fontOk);
    if (fontOk) {
        axisX->setLabelsFont(newAxisLabelFont);
        axisY->setLabelsFont(newAxisLabelFont);
    }

    chart->addAxis(axisX, Qt::AlignmentFlag::AlignBottom);
    chart->addAxis(axisY, Qt::AlignmentFlag::AlignLeft);

    barSeries->attachAxis(axisY);

    QGraphicsScene scene;
    scene.addItem(chart);

    chart->setGeometry(0, 0, width, height);
    scene.setSceneRect(0, 0, width, height);

    QImage result(width, height, QImage::Format::Format_RGB888);
    result.fill(Qt::GlobalColor::white);

    QPainter painter(&result);

    scene.render(&painter);
    painter.end();

    PlotMailbox& mb = mailbox(threadId);
    mb.sendImage(result);
}
