/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref LatencyPlotter class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef LATENCY_PLOTTER_H
#define LATENCY_PLOTTER_H

#include <QObject>
#include <QMutex>
#include <QVector>
#include <QFont>

#include <cstdint>

#include "customer_capabilities.h"
#include "monitor.h"
#include "server.h"
#include "short_latency_entry.h"
#include "latency_entry.h"
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

class LatencyInterfaceManager;
class Monitors;
class Servers;

/**
 * Class used to generate plots from customer latency information.
 */
class LatencyPlotter:public PlotterBase {
    Q_OBJECT

    public:
        /**
         * Type used to represent a region ID.
         */
        typedef Region::RegionId RegionId;

        /**
         * Type used to represent a server ID.
         */
        typedef Server::ServerId ServerId;

        /**
         * Type used to represent a host/scheme ID.
         */
        typedef HostScheme::HostSchemeId HostSchemeId;

        /**
         * Value used to represent a host/scheme ID.
         */
        typedef Monitor::MonitorId MonitorId;

        /**
         * Constructor
         *
         * \param[in] latencyInterfaceManager The latency interface manager used to get plot data.
         *
         * \param[in] parent                  Pointer to the parent object.
         */
        LatencyPlotter(LatencyInterfaceManager* latencyInterfaceManager, QObject* parent = nullptr);

        ~LatencyPlotter() override;

        /**
         * Method that generates a plot showing latency over time.  This function is fully reentrant and thread safe.
         * Use the \ref waitForImage method to obtain the generated image.
         *
         * \param[in] threadId        The zero based ID of the thread requesting this plot.
         *
         * \param[in] customerId      The customer ID of the customer tied to the plot.  An invalid customer ID
         *                            indicates all customers.
         *
         * \param[in] regionId        The region ID of the region to limit the plot to.  An invalid region ID indicates
         *                            all regions.
         *
         * \param[in] serverId        The server ID of the server to limit the plot to.  An invalid server ID indicates
         *                            all servers.
         *
         * \param[in] hostSchemeId    The host/scheme ID of the host scheme to limit the plot to.  An invalid
         *                            host/scheme ID indicates all host schemes.
         *
         * \param[in] monitorId       The monitor ID of the monitor to limit the plot to.  An invalid monitor ID
         *                            indicates all monitors.
         *
         * \param[in] startTimestamp  The Unix timestamp to limit the plot to.  A start timestamp of 0 indicates no
         *                            start timestamp.
         *
         * \param[in] endTimestamp    The Unix timestamp to limit the plot to.  An end timestamp of 0 indicates no
         *                            start timestamp.
         *
         * \param[in] titleText       The chart title text.
         *
         * \param[in] xAxisTitle      The title to apply to the X axis.
         *
         * \param[in] yAxisTitle      The title to apply to the Y axis.
         *
         * \param[in] dateFormat      The date format string.
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
         * \param[in] maximumLatency  The maximum latency to show.  A negative value indicates that the value should be
         *                            determined by the provided data.
         *
         * \param[in] minimumLatency  The minimum latency to show.  A negative value indicates that the value should be
         *                            determined by the provided data.
         *
         * \param[in] latencyLogScale If true, then latency will be shown using a log scale.
         *
         * \param[in] width           The plot width, in pixels.
         *
         * \param[in] height          The plot height, in pixels.
         *
         * \return Returns a reference to the mailbox to receive the image.
         */
        PlotMailbox& requestHistoryPlot(
            ThreadId           threadId,
            CustomerId         customerId,
            RegionId           regionId,
            ServerId           serverId,
            HostSchemeId       hostSchemeId,
            MonitorId          monitorId,
            unsigned long long startTimestamp,
            unsigned long long endTimestamp,
            const QString&     titleText,
            const QString&     xAxisTitle,
            const QString&     yAxisTitle,
            const QString&     dateFormatString,
            const QString&     titleFont,
            const QString&     axisTitleFont,
            const QString&     axisLabelFont,
            double             maximumLatency = -1,
            double             minimumLatency = -1,
            bool               logScale = false,
            unsigned           width = defaultWidth,
            unsigned           height = defaultHeight
        );

        /**
         * Method that generates a histogram of latency.  This function is fully reentrant and thread safe.  Use the
         * \ref waitForImage method to obtain the generated image.
         *
         * \param[in] threadId        The zero based ID of the thread requesting this plot.
         *
         * \param[in] customerId      The customer ID of the customer tied to the plot.  An invalid customer ID
         *                            indicates all customers.
         *
         * \param[in] regionId        The region ID of the region to limit the plot to.  An invalid region ID indicates
         *                            all regions.
         *
         * \param[in] serverId        The server ID of the server to limit the plot to.  An invalid server ID indicates
         *                            all servers.
         *
         * \param[in] hostSchemeId    The host/scheme ID of the host scheme to limit the plot to.  An invalid
         *                            host/scheme ID indicates all host schemes.
         *
         * \param[in] monitorId       The monitor ID of the monitor to limit the plot to.  An invalid monitor ID
         *                            indicates all monitors.
         *
         * \param[in] startTimestamp  The Unix timestamp to limit the plot to.  A start timestamp of 0 indicates no
         *                            start timestamp.
         *
         * \param[in] endTimestamp    The Unix timestamp to limit the plot to.  An end timestamp of 0 indicates no
         *                            start timestamp.
         *
         * \param[in] titleText       The chart title text.
         *
         * \param[in] xAxisTitle      The title to apply to the X axis.
         *
         * \param[in] yAxisTitle      The title to apply to the Y axis.
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
         * \param[in] maximumLatency  The maximum latency to show.  A negative value indicates that the value should be
         *                            determined by the provided data.
         *
         * \param[in] minimumLatency  The minimum latency to show.  A negative value indicates that the value should be
         *                            determined by the provided data.
         *
         * \param[in] width           The plot width, in pixels.
         *
         * \param[in] height          The plot height, in pixels.
         *
         * \return Returns a reference to the mailbox to receive the image.
         */
        PlotMailbox& requestHistogramPlot(
            ThreadId           threadId,
            CustomerId         customerId,
            RegionId           regionId,
            ServerId           serverId,
            HostSchemeId       hostSchemeId,
            MonitorId          monitorId,
            unsigned long long startTimestamp,
            unsigned long long endTimestamp,
            const QString&     titleText,
            const QString&     xAxisTitle,
            const QString&     yAxisTitle,
            const QString&     titleFont,
            const QString&     axisTitleFont,
            const QString&     axisLabelFont,
            double             maximumLatency = -1,
            double             minimumLatency = -1,
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
         * Signal that is emitted to request a plot showing latency over time.
         *
         * \param[out] threadId        The zero based ID of the thread requesting this plot.
         *
         * \param[out] customerId      The customer ID of the customer tied to the plot.  An invalid customer ID
         *                             indicates all customers.
         *
         * \param[out] regionId        The region ID of the region to limit the plot to.  An invalid region ID indicates
         *                             all regions.
         *
         * \param[out] serverId        The server ID of the server to limit the plot to.  An invalid server ID indicates
         *                             all servers.
         *
         * \param[out] hostSchemeId    The host/scheme ID of the host scheme to limit the plot to.  An invalid
         *                             host/scheme ID indicates all host schemes.
         *
         * \param[out] monitorId       The monitor ID of the monitor to limit the plot to.  An invalid monitor ID
         *                             indicates all monitors.
         *
         * \param[out] startTimestamp  The Unix timestamp to limit the plot to.  A start timestamp of 0 indicates no
         *                             start timestamp.
         *
         * \param[out] endTimestamp    The Unix timestamp to limit the plot to.  An end timestamp of 0 indicates no
         *                             start timestamp.
         *
         * \param[out] titleText       The chart title text.
         *
         * \param[out] xAxisTitle      The title to apply to the X axis.
         *
         * \param[out] yAxisTitle      The title to apply to the Y axis.
         *
         * \param[out] dateFormat      The date format string.
         *
         * \param[in] titleFont        The font to use for the title.  An invalid font will cause the default font to be
         *                             used.
         *
         * \param[in] axisTitleFont    The font to use for the axis titles.  An invalid font will cause the default font
         *                             to be used.
         *
         * \param[in] axisLabelFont    The font to use for the axis labels.  An invalid font will cause the default font
         *                             to be used.
         *
         * \param[out] maximumLatency  The maximum latency to show.  A negative value indicates that the value should be
         *                             determined by the provided data.
         *
         * \param[out] minimumLatency  The minimum latency to show.  A negative value indicates that the value should be
         *                             determined by the provided data.
         *
         * \param[out] latencyLogScale If true, then latency will be shown using a log scale.
         *
         * \param[out] width           The plot width, in pixels.
         *
         * \param[out] height          The plot height, in pixels.
         *
         * \return Returns a reference to the mailbox to receive the image.
         */
        void issueRequestHistoryPlot(
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
            double             maximumLatency = -1,
            double             minimumLatency = -1,
            bool               logScale = false,
            unsigned           width = defaultWidth,
            unsigned           height = defaultHeight
        );

        /**
         * Signal that is emitted to request a histogram of latency.
         *
         * \param[out] threadId        The zero based ID of the thread requesting this plot.
         *
         * \param[out] customerId      The customer ID of the customer tied to the plot.  An invalid customer ID
         *                             indicates all customers.
         *
         * \param[out] regionId        The region ID of the region to limit the plot to.  An invalid region ID indicates
         *                             all regions.
         *
         * \param[out] serverId        The server ID of the server to limit the plot to.  An invalid server ID indicates
         *                             all servers.
         *
         * \param[out] hostSchemeId    The host/scheme ID of the host scheme to limit the plot to.  An invalid
         *                             host/scheme ID indicates all host schemes.
         *
         * \param[out] monitorId       The monitor ID of the monitor to limit the plot to.  An invalid monitor ID
         *                             indicates all monitors.
         *
         * \param[out] startTimestamp  The Unix timestamp to limit the plot to.  A start timestamp of 0 indicates no
         *                             start timestamp.
         *
         * \param[out] endTimestamp    The Unix timestamp to limit the plot to.  An end timestamp of 0 indicates no
         *                             start timestamp.
         *
         * \param[out] titleText       The chart title text.
         *
         * \param[out] xAxisTitle      The title to apply to the X axis.
         *
         * \param[out] yAxisTitle      The title to apply to the Y axis.
         *
         * \param[in] titleFont        The font to use for the title.  An invalid font will cause the default font to be
         *                             used.
         *
         * \param[in] axisTitleFont    The font to use for the axis titles.  An invalid font will cause the default font
         *                             to be used.
         *
         * \param[in] axisLabelFont    The font to use for the axis labels.  An invalid font will cause the default font
         *                             to be used.
         *
         * \param[out] maximumLatency  The maximum latency to show.  A negative value indicates that the value should be
         *                             determined by the provided data.
         *
         * \param[out] minimumLatency  The minimum latency to show.  A negative value indicates that the value should be
         *                             determined by the provided data.
         *
         * \param[out] width           The plot width, in pixels.
         *
         * \param[out] height          The plot height, in pixels.
         *
         * \return Returns a reference to the mailbox to receive the image.
         */
        void issueRequestHistogramPlot(
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
            double             maximumLatency = -1,
            double             minimumLatency = -1,
            unsigned           width = defaultWidth,
            unsigned           height = defaultHeight
        );

    private slots:
        /**
         * Slot that is triggered to generate a plot showing latency over time.
         *
         * \param[in] threadId        The zero based ID of the thread requesting this plot.
         *
         * \param[in] customerId      The customer ID of the customer tied to the plot.  An invalid customer ID
         *                            indicates all customers.
         *
         * \param[in] regionId        The region ID of the region to limit the plot to.  An invalid region ID indicates
         *                            all regions.
         *
         * \param[in] serverId        The server ID of the server to limit the plot to.  An invalid server ID indicates
         *                            all servers.
         *
         * \param[in] hostSchemeId    The host/scheme ID of the host scheme to limit the plot to.  An invalid
         *                            host/scheme ID indicates all host schemes.
         *
         * \param[in] monitorId       The monitor ID of the monitor to limit the plot to.  An invalid monitor ID
         *                            indicates all monitors.
         *
         * \param[in] startTimestamp  The Unix timestamp to limit the plot to.  A start timestamp of 0 indicates no
         *                            start timestamp.
         *
         * \param[in] endTimestamp    The Unix timestamp to limit the plot to.  An end timestamp of 0 indicates no
         *                            start timestamp.
         *
         * \param[in] titleText       The chart title text.
         *
         * \param[in] xAxisTitle      The title to apply to the X axis.
         *
         * \param[in] yAxisTitle      The title to apply to the Y axis.
         *
         * \param[in] dateFormat      The date format string.
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
         * \param[in] maximumLatency  The maximum latency to show.  A negative value indicates that the value should be
         *                            determined by the provided data.
         *
         * \param[in] minimumLatency  The minimum latency to show.  A negative value indicates that the value should be
         *                            determined by the provided data.
         *
         * \param[in] latencyLogScale If true, then latency will be shown using a log scale.
         *
         * \param[in] width           The plot width, in pixels.
         *
         * \param[in] height          The plot height, in pixels.
         *
         * \return Returns a reference to the mailbox to receive the image.
         */
        void generateHistoryPlot(
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
            double             maximumLatency = -1,
            double             minimumLatency = -1,
            bool               logScale = false,
            unsigned           width = defaultWidth,
            unsigned           height = defaultHeight
        );

        /**
         * Slot that is triggered to generates a histogram of latency.
         *
         * \param[in] threadId        The zero based ID of the thread requesting this plot.
         *
         * \param[in] customerId      The customer ID of the customer tied to the plot.  An invalid customer ID
         *                            indicates all customers.
         *
         * \param[in] regionId        The region ID of the region to limit the plot to.  An invalid region ID indicates
         *                            all regions.
         *
         * \param[in] serverId        The server ID of the server to limit the plot to.  An invalid server ID indicates
         *                            all servers.
         *
         * \param[in] hostSchemeId    The host/scheme ID of the host scheme to limit the plot to.  An invalid
         *                            host/scheme ID indicates all host schemes.
         *
         * \param[in] monitorId       The monitor ID of the monitor to limit the plot to.  An invalid monitor ID
         *                            indicates all monitors.
         *
         * \param[in] startTimestamp  The Unix timestamp to limit the plot to.  A start timestamp of 0 indicates no
         *                            start timestamp.
         *
         * \param[in] endTimestamp    The Unix timestamp to limit the plot to.  An end timestamp of 0 indicates no
         *                            start timestamp.
         *
         * \param[in] titleText       The chart title text.
         *
         * \param[in] xAxisTitle      The title to apply to the X axis.
         *
         * \param[in] yAxisTitle      The title to apply to the Y axis.
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
         * \param[in] maximumLatency  The maximum latency to show.  A negative value indicates that the value should be
         *                            determined by the provided data.
         *
         * \param[in] minimumLatency  The minimum latency to show.  A negative value indicates that the value should be
         *                            determined by the provided data.
         *
         * \param[in] width           The plot width, in pixels.
         *
         * \param[in] height          The plot height, in pixels.
         *
         * \return Returns a reference to the mailbox to receive the image.
         */
        void generateHistogramPlot(
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
            double             maximumLatency = -1,
            double             minimumLatency = -1,
            unsigned           width = defaultWidth,
            unsigned           height = defaultHeight
        );

    private:
        /**
         * The latency interface manager used to fetch latency data.
         */
        LatencyInterfaceManager* currentLatencyInterfaceManager;

        /**
         * Mutex used to guard our list of mailboxes.
         */
        QMutex mailboxMutex;

        /**
         * Our list of mailboxes.
         */
        QVector<PlotMailbox*> mailboxes;

        /**
         * The graphics/scene instance used by this class.
         */
        QGraphicsScene* scene;

        /**
         * The chart instance used by this class.
         */
        QtCharts::QChart* chart;

        /**
         * The line series used for recent data.
         */
        QtCharts::QLineSeries* recentSeries;

        /**
         * The line series used for the averaged aggregated data.
         */
        QtCharts::QLineSeries* aggregatedLatencySeries;

        /**
         * The line series used for aggregated minimum values;
         */
        QtCharts::QLineSeries* aggregatedMinimumSeries;

        /**
         * The line series used for aggregated maximum values.
         */
        QtCharts::QLineSeries* aggregatedMaximumSeries;

        /**
         * The line series used for the lower 1 standard deviation values.
         */
        QtCharts::QLineSeries* aggregatedStdDevLowerSeries;

        /**
         * The line series used for the upper 1 standard deviation values.
         */
        QtCharts::QLineSeries* aggregatedStdDevUpperSeries;

        /**
         * The series used to shade 1 standard deviation.
         */
        QtCharts::QAreaSeries* aggregatedStdDevSeries;

        /**
         * The horizontal time axis.
         */
        QtCharts::QDateTimeAxis* horizontalTimeAxis;

        /**
         * The horizontal latency axis.
         */
        QtCharts::QValueAxis* horizontalLatencyAxis;

        /**
         * The vertical linear axis.
         */
        QtCharts::QValueAxis* verticalLinearAxis;

        /**
         * The vertical logarithmic axis.
         */
        QtCharts::QLogValueAxis* verticalLogAxis;
};

#endif
