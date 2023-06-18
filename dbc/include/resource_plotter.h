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
* This header defines the \ref ResourcePlotter class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef RESOURCE_PLOTTER_H
#define RESOURCE_PLOTTER_H

#include <QObject>
#include <QMutex>
#include <QVector>
#include <QFont>

#include <cstdint>

#include "customer_capabilities.h"
#include "monitor.h"
#include "server.h"
#include "resource.h"
#include "customer_capabilities.h"
#include "monitor.h"
#include "region.h"
#include "server.h"
#include "host_scheme.h"
#include "plot_mailbox.h"
#include "plotter_base.h"

namespace QtCharts {
    class QChart;
    class QLineSeries;
    class QAreaSeries;
    class QDateTimeAxis;
    class QValueAxis;
    class QLogValueAxis;
};

class QGraphicsScene;

class Resource;
class Resources;
class PlotMailbox;

/**
 * Class used to generate plots from customer resource information.
 */
class ResourcePlotter:public PlotterBase {
    Q_OBJECT

    public:
        /**
         * Type used to indicate the type of value being stored.
         */
        typedef Resource::ValueType ValueType;

        /**
         * Type used to store the value.
         */
        typedef Resource::Value Value;

        /**
         * Constructor
         *
         * \param[in] resources The resource database API.
         *
         * \param[in] parent    Pointer to the parent object.
         */
        ResourcePlotter(Resources* resources, QObject* parent = nullptr);

        ~ResourcePlotter() override;

        /**
         * Method that generates a plot showing resource over time.  This function is fully reentrant and thread safe.
         * Use the \ref waitForImage method to obtain the generated image.
         *
         * \param[in] threadId       The zero based ID of the thread requesting this plot.
         *
         * \param[in] customerId     The customer ID of the customer tied to the plot.
         *
         * \param[in] valueType      The value type for the data to be plotted.
         *
         * \param[in] startTimestamp The Unix timestamp to limit the plot to.  A start timestamp of 0 indicates no
         *                           start timestamp.
         *
         * \param[in] endTimestamp   The Unix timestamp to limit the plot to.  An end timestamp of 0 indicates no
         *                           start timestamp.
         *
         * \param[in] scaleFactor    A vertical scale factor to apply to presented values.
         *
         * \param[in] titleText      The chart title text.
         *
         * \param[in] xAxisTitle     The title to apply to the X axis.
         *
         * \param[in] yAxisTitle     The title to apply to the Y axis.
         *
         * \param[in] dateFormat     The date format string.
         *
         * \param[in] titleFont      The font to use for the title.  An invalid font will cause the default font to be
         *                           used.
         *
         * \param[in] axisTitleFont  The font to use for the axis titles.  An invalid font will cause the default font
         *                           to be used.
         *
         * \param[in] axisLabelFont  The font to use for the axis labels.  An invalid font will cause the default font
         *                           to be used.
         *
         * \param[in] width          The plot width, in pixels.
         *
         * \param[in] height         The plot height, in pixels.
         *
         * \return Returns a reference to the mailbox to receive the image.
         */
        PlotMailbox& requestPlot(
            ThreadId           threadId,
            CustomerId         customerId,
            ValueType          valueType,
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
            unsigned           width = defaultWidth,
            unsigned           height = defaultHeight
        );

        /**
         * Method you can use to obtain a mailbox for a specific thread ID.
         *
         * \param[in] threadId The thread ID of the thread we need the mailbox for.
         *
         * \return Returns a reference to the desired mailbox.
         */
        PlotMailbox& mailbox(unsigned threadId);

    signals:
        /**
         * Signal that is emitted to request a plot showing resource over time.
         *
         * \param[out] threadId       The zero based ID of the thread requesting this plot.
         *
         * \param[out] customerId     The customer ID of the customer tied to the plot.
         *
         * \param[out] valueType      The value type for the data to be plotted.
         *
         * \param[out] startTimestamp The Unix timestamp to limit the plot to.  A start timestamp of 0 indicates no
         *                            start timestamp.
         *
         * \param[out] endTimestamp   The Unix timestamp to limit the plot to.  An end timestamp of 0 indicates no
         *                            start timestamp.
         *
         * \param[in] scaleFactor     A vertical scale factor to apply to presented values.
         *
         * \param[out] titleText      The chart title text.
         *
         * \param[out] xAxisTitle     The title to apply to the X axis.
         *
         * \param[out] yAxisTitle     The title to apply to the Y axis.
         *
         * \param[out] dateFormat     The date format string.
         *
         * \param[in] titleFont       The font to use for the title.  An invalid font will cause the default font to be
         *                            used.
         *
         * \param[in] axisTitleFont   The font to use for the axis titles.  An invalid font will cause the default font
         *                            to be used.
         *
         * \param[in] axisLabelFont   The font to use for the axis labels.  An invalid font will cause the default font
         *                            to be used.
         *
         * \param[out] width          The plot width, in pixels.
         *
         * \param[out] height         The plot height, in pixels.
         *
         * \return Returns a reference to the mailbox to receive the image.
         */
        void issueRequestPlot(
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
            unsigned           width = defaultWidth,
            unsigned           height = defaultHeight
        );

    private slots:
        /**
         * Slot that is triggered to generate a plot showing resource over time.
         *
         * \param[in] threadId       The zero based ID of the thread requesting this plot.
         *
         * \param[in] customerId     The customer ID of the customer tied to the plot.  An invalid customer ID
         *                           indicates all customers.
         *
         * \param[in] valueType      The value type for the data to be plotted.
         *
         * \param[in] startTimestamp The Unix timestamp to limit the plot to.  A start timestamp of 0 indicates no
         *                           start timestamp.
         *
         * \param[in] endTimestamp   The Unix timestamp to limit the plot to.  An end timestamp of 0 indicates no
         *                           start timestamp.
         *
         * \param[in] scaleFactor    A vertical scale factor to apply to presented values.
         *
         * \param[in] titleText      The chart title text.
         *
         * \param[in] xAxisTitle     The title to apply to the X axis.
         *
         * \param[in] yAxisTitle     The title to apply to the Y axis.
         *
         * \param[in] dateFormat     The date format string.
         *
         * \param[in] titleFont      The font to use for the title.  An invalid font will cause the default font to be
         *                           used.
         *
         * \param[in] axisTitleFont  The font to use for the axis titles.  An invalid font will cause the default font
         *                           to be used.
         *
         * \param[in] axisLabelFont  The font to use for the axis labels.  An invalid font will cause the default font
         *                           to be used.
         *
         * \param[in] width          The plot width, in pixels.
         *
         * \param[in] height         The plot height, in pixels.
         *
         * \return Returns a reference to the mailbox to receive the image.
         */
        void generatePlot(
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
            unsigned           width = defaultWidth,
            unsigned           height = defaultHeight
        );

    private:
        /**
         * The resource interface manager used to fetch resource data.
         */
        Resources* currentResources;

        /**
         * Mutex used to guard our list of mailboxes.
         */
        QMutex mailboxMutex;

        /**
         * Our list of mailboxes.
         */
        QVector<PlotMailbox*> mailboxes;
};

#endif
