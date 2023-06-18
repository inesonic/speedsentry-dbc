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
* This header defines the \ref PlotterBase class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef PLOTTER_BASE_H
#define PLOTTER_BASE_H

#include <QObject>
#include <QMutex>
#include <QVector>
#include <QFont>

#include <cstdint>

#include "customer_capabilities.h"

namespace QtCharts {
    class QChart;
    class QLineSeries;
    class QAreaSeries;
    class QDateTimeAxis;
    class QValueAxis;
    class QLogValueAxis;
};

class QGraphicsScene;

/**
 * Class used to generate plots from customer latency information.
 */
class PlotterBase:public QObject {
    Q_OBJECT

    public:
        /**
         * The default image width, in pixels.
         */
        static constexpr unsigned defaultWidth = 1024;

        /**
         * The default image height, in pixels.
         */
        static constexpr unsigned defaultHeight = 768;

        /**
         * Type used to hold a thread ID.
         */
        typedef unsigned ThreadId;

        /**
         * Type used to represent a customer ID.
         */
        typedef CustomerCapabilities::CustomerId CustomerId;

        /**
         * Constructor
         *
         * \param[in] parent Pointer to the parent object.
         */
        PlotterBase(QObject* parent = nullptr);

        ~PlotterBase() override;

    protected:
        /**
         * The plotter database thread ID.
         */
        static constexpr unsigned databaseThreadId = static_cast<unsigned>(-10);

        /**
         * Value used to perform nice scaling.  The value relates roughly to the denominator of the fraction of a plot
         * axis that will be unused, worst case, due to rounding.
         */
        static constexpr double distanceThreshold = 8.0;

        /**
         * Value indicating the number of seconds per day.
         */
        static constexpr double secondsPerDay = 24 * 60 * 60;

        /**
         * Method that fixes the supplied timestamp values.
         *
         * \param[in,out] startTimestamp The starting timestamp.
         *
         * \param[in,out] endTimestamp   The ending timestamp.
         */
        static void fixTimestamp(unsigned long long& startTimestamp, unsigned long long& endTimestamp);

        /**
         * Method that finds a nice value close to but less (or greater than) than a provided value.
         *
         * \param[in,out] minimum The minimum value to use.
         *
         * \param[in,out] maximum The maximum value to use.
         *
         * \return Returns a recommended number of steps for this range.
         */
        static unsigned calculateNiceRange(double& minimum, double& maximum);

        /**
         * Method that finds a nice value close to but less (or greater than) than a provided value.
         *
         * \param[in,out] minimum The minimum value to use.
         *
         * \param[in,out] maximum The maximum value to use.
         */
        static void calculateNiceLogRange(double& minimum, double& maximum);

        /**
         * Method that converts a string description of a font to a string.
         *
         * \param[in]  description The font description.
         *
         * \param[out] ok          Optional pointer to a boolean value that holds true on exit if successful.
         *
         * \return Returns a font matching the description.
         */
        static QFont toFont(const QString& description, bool* ok = nullptr);
};

#endif
