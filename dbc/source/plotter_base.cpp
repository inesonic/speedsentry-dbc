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
* This header implements the \ref PlotterBase class.
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

PlotterBase::PlotterBase(QObject* parent):QObject(parent) {}


PlotterBase::~PlotterBase() {}


void PlotterBase::fixTimestamp(unsigned long long& startTimestamp, unsigned long long& endTimestamp) {
    if (endTimestamp == 0) {
        endTimestamp = static_cast<unsigned long long>(-1);
    }

    if (endTimestamp == startTimestamp) {
        endTimestamp = startTimestamp + 24 * 2600;
    } else if (endTimestamp < startTimestamp) {
        std::swap(endTimestamp, startTimestamp);
    }
}


unsigned PlotterBase::calculateNiceRange(double& minimum, double& maximum) {
    unsigned numberSteps;

    if (minimum == maximum) {
        minimum     = minimum - 0.5;
        maximum     = maximum + 0.5;
        numberSteps = 1;
    } else {
        double span             = std::abs(maximum - minimum);
        double magnitudeMinimum = std::abs(minimum);
        double magnitudeMaximum = std::abs(maximum);

        double distance;
        if (10.0 * magnitudeMinimum < magnitudeMaximum) {
            distance = magnitudeMaximum;
        } else if (span < magnitudeMinimum) {
            distance = span;
        } else {
            distance = std::max(magnitudeMinimum, magnitudeMaximum);
        }

        double powerOf10  = std::floor(std::log10(distance)) - 1;
        double rounding1  = std::pow(10, powerOf10);
        double rounding2  = 2.0 * rounding1;
        double rounding5  = 5.0 * rounding1;
        double rounding10 = 10.0 * rounding1;
        double score1     = std::abs(distanceThreshold - (span / rounding1));
        double score2     = std::abs(distanceThreshold - (span / rounding2));
        double score5     = std::abs(distanceThreshold - (span / rounding5));
        double score10    = std::abs(distanceThreshold - (span / rounding10));

        double rounding;
        if (score1 < score2 && score1 < score5 && score1 < score10) {
            rounding = rounding1;
        } else if (score2 < score1 && score2 < score5 && score2 < score10) {
            rounding = rounding2;
        } else if (score5 < score1 && score5 < score2 && score5 < score10) {
            rounding = rounding5;
        } else {
            rounding = rounding10;
        }

        minimum     = rounding * std::floor(minimum / rounding);
        maximum     = rounding * std::ceil(maximum / rounding);
        numberSteps = (maximum - minimum) / rounding;
    }

    return numberSteps;
}


void PlotterBase::calculateNiceLogRange(double& minimum, double& maximum) {
    if (minimum <= 0) {
        minimum = 1.0E-6;
    } else {
        double powerOf10 = std::floor(std::log10(minimum));
        minimum = std::pow(10, powerOf10);
    }

    if (maximum <= 0) {
        maximum = 1.0E-6;
    } else {
        double powerOf10 = std::ceil(std::log10(maximum));
        maximum = std::pow(10, powerOf10);
    }
}


QFont PlotterBase::toFont(const QString& description, bool* ok) {
    QFont       result;
    bool        success = true;

    QStringList fields  = description.split(QChar(','));
    if (fields.size() >= 2 && fields.size() <= 3) {
        QString family = fields[0].toLower().trimmed();

        unsigned pointSize = 12;
        if (fields.size() >= 2) {
            const QString& pointSizeString = fields.at(1);
            pointSize = pointSizeString.toUInt(&success);
            if (pointSize < 6 || pointSize > 32) {
                success = false;
            }
        }

        QFont::Weight weight = QFont::Weight::Normal;
        if (success && fields.size() == 3) {
            QString weightString = fields.at(2).trimmed().toLower();
            if (weightString == "normal") {
                weight = QFont::Weight::Normal;
            } else if (weightString == "light") {
                weight = QFont::Weight::Light;
            } else if (weightString == "bold") {
                weight = QFont::Weight::Bold;
            } else {
                success = false;
            }
        }

        if (success) {
            result = QFont(family, pointSize, weight);
        }
    } else {
        success = false;
    }

    if (ok != nullptr) {
        *ok = success;
    }

    return result;
}
