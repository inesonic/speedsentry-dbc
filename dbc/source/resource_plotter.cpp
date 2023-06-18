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
* This header implements the \ref ResourcePlotter class.
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

#include "resource.h"
#include "resources.h"
#include "plot_mailbox.h"
#include "resource_plotter.h"

ResourcePlotter::ResourcePlotter(
        Resources* resources,
        QObject*   parent
    ):PlotterBase(
        parent
    ),currentResources(
        resources
    ) {
    connect(this, &ResourcePlotter::issueRequestPlot, this, &ResourcePlotter::generatePlot);
}


ResourcePlotter::~ResourcePlotter() {}


PlotMailbox& ResourcePlotter::requestPlot(
        ResourcePlotter::ThreadId   threadId,
        ResourcePlotter::CustomerId customerId,
        ResourcePlotter::ValueType  valueType,
        unsigned long long          startTimestamp,
        unsigned long long          endTimestamp,
        float                       scaleFactor,
        const QString&              titleText,
        const QString&              xAxisTitle,
        const QString&              yAxisTitle,
        const QString&              dateFormatString,
        const QString&              titleFont,
        const QString&              axisTitleFont,
        const QString&              axisLabelFont,
        unsigned                    width,
        unsigned                    height
    ) {
    PlotMailbox& mb = mailbox(threadId);
    mb.forceEmpty();

    emit issueRequestPlot(
        threadId,
        customerId,
        valueType,
        startTimestamp,
        endTimestamp,
        scaleFactor,
        titleText,
        xAxisTitle,
        yAxisTitle,
        dateFormatString,
        titleFont,
        axisTitleFont,
        axisLabelFont,
        width,
        height
    );

    return mb;
}


PlotMailbox& ResourcePlotter::mailbox(unsigned threadId) {
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


void ResourcePlotter::generatePlot(
        unsigned           threadId,
        unsigned long      customerId,
        unsigned           valueType,
        unsigned long long startTimestamp,
        unsigned long long endTimestamp,
        float              scaleFactor,
        const QString&     titleText,
        const QString&     xAxisTitle,
        const QString&     yAxisTitle,
        const QString&     dateFormatString,
        const QString&     titleFont,
        const QString&     axisTitleFont,
        const QString&     axisLabelFont,
        unsigned           width,
        unsigned           height
    ) {
    typedef Resources::ResourceList ResourceList;

    fixTimestamp(startTimestamp, endTimestamp);

    ResourceList resources = currentResources->getResources(
        customerId,
        valueType,
        startTimestamp,
        endTimestamp,
        databaseThreadId
    );

    QtCharts::QLineSeries* series = new QtCharts::QLineSeries();

    QColor inesonicBlue(0x17, 0x6E, 0xDA);
//    QColor inesonicOrange(0xE0, 0x7E, 0x26);

    QPen seriesPen(QBrush(inesonicBlue), 1.5);
    series->setPen(seriesPen);

    unsigned resourcesListSize = static_cast<unsigned>(resources.size());
    bool     showDayOfWeek     = (dateFormatString == "dow");

    unsigned long long minimumTime  = 0;
    unsigned long long maximumTime  = 1;
    double             minimumValue = std::numeric_limits<float>::max();
    double             maximumValue = -minimumValue;
    if (!resources.isEmpty()) {
        if (startTimestamp == 0) {
            minimumTime = resources.first().unixTimestamp();
        }

        if (endTimestamp == 0) {
            maximumTime = resources.last().unixTimestamp();
        }

        unsigned long long weekStartTimestamp = 0;
        if (showDayOfWeek) {
            QDateTime startDateTime  = QDateTime::fromSecsSinceEpoch(minimumTime, Qt::TimeSpec::UTC);
            QDate     startDate      = startDateTime.date();
            QDate     dayStartOfWeek = startDate.addDays(1 - startDate.dayOfWeek());
            QDateTime weekStart      = QDateTime(dayStartOfWeek, QTime(0, 0), Qt::TimeSpec::UTC, 0);

            weekStartTimestamp = weekStart.toSecsSinceEpoch();

            for (unsigned i=0 ; i<resourcesListSize ; ++i) {
                const Resource& resource = resources.at(i);
                double dow = 1 + static_cast<double>(resource.unixTimestamp() - weekStartTimestamp) / secondsPerDay;
                double v   = resource.value() * scaleFactor;
                series->append(dow, v);

                if (v < minimumValue) {
                    minimumValue = v;
                }

                if (v > maximumValue) {
                    maximumValue = v;
                }
            }
        } else {
            for (unsigned i=0 ; i<resourcesListSize ; ++i) {
                const Resource& resource = resources.at(i);
                double          v        = resource.value() * scaleFactor;
                series->append(resource.unixTimestamp() * 1000, v);

                if (v < minimumValue) {
                    minimumValue = v;
                }

                if (v > maximumValue) {
                    maximumValue = v;
                }
            }
        }

        if (minimumValue == maximumValue) {
            minimumValue = 0.9 * minimumValue;
            maximumValue = 1.1 * maximumValue;
        } else if (minimumValue > maximumValue) {
            minimumValue = 0;
            maximumValue = 1.0;
        }
    } else {
        minimumValue = 0;
        maximumValue = 1;
    }

    QtCharts::QChart* chart = new QtCharts::QChart();
    chart->addSeries(series);

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

    series->attachAxis(axisX);

    QtCharts::QValueAxis* axisY = new QtCharts::QValueAxis;
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
    series->attachAxis(axisY);

    unsigned recommendedTickCount = calculateNiceRange(minimumValue, maximumValue);
    static_cast<QtCharts::QValueAxis*>(axisY)->setRange(minimumValue, maximumValue);
    static_cast<QtCharts::QValueAxis*>(axisY)->setTickCount(recommendedTickCount + 1);
    static_cast<QtCharts::QValueAxis*>(axisY)->setMinorTickCount(1);

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


