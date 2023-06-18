/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref CustomerCapabilities class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CUSTOMER_CAPABILITIES_H
#define CUSTOMER_CAPABILITIES_H

#include <QString>

#include <cstdint>

class CustomersCapabilities;

/**
 * Trivial class used to hold information about what a customer can and can-not do.
 */
class CustomerCapabilities {
    friend class CustomersCapabilities;

    public:
        /**
         * Value used to represent a customer ID.
         */
        typedef std::uint32_t CustomerId;

        /**
         * Value used to represent the collection of bit flags.
         */
        typedef std::uint32_t Flags;

        /**
         * Value used to indicate an invalid customer ID.
         */
        static constexpr CustomerId invalidCustomerId = 0;

    public:
        CustomerCapabilities() {
            currentCustomerId            = invalidCustomerId;
            currentMaximumNumberMonitors = 0;
            currentPollingInterval       = 0;
            currentFlags                 = 0;
        }

        /**
         * Constructor.
         *
         * \param[in] customerId                       The ID used to identify this region.
         *
         * \param[in] maximumNumberMonitors            The maximum number of monitors this customer can use.
         *
         * \param[in] pollingInterval                  The polling interval required for this customer, in seconds.
         *
         * \param[in] expirationDays                   The latency expiration period in days.
         *
         * \param[in] customerActive                   If true, the the customer is active.  If false, the customer is
         *                                             currently inactive.
         *
         * \param[in] multiRegionChecking              If true, then checking should be performed across all available
         *                                             regions.  If false, then only a single region should be used.
         *
         * \param[in] supportsWordPress                If true, then customer WordPress functions should be
         *                                             supported.
         *
         * \param[in] supportsRestApi                  If true, then this customer is allowed to use our REST API.
         *
         * \param[in] supportsContentChecking          If true, then content checking should be supported.
         *
         * \param[in] supportsKeywordChecking          If true, then content should be scanned for keywords.
         *
         * \param[in] supportsPostMethod               If true, then POST method and associated functions should be
         *                                             supported.
         *
         * \param[in] supportsLatencyTracking          If true, the latency information should be collected.
         *
         * \param[in] supportsSslExpirationChecking    If true, then SSL expiration date/time should be checked.
         *
         * \param[in] supportsPingBasedPolling         if true, then ping based polling should be included.
         *
         * \param[in] supportsBlackListChecking        If true, then blacklisting should be monitored.
         *
         * \param[in] supportsDomainExpirationChecking If true, then the domain expiration date should be checked.
         *
         * \param[in] supportsMaintenanceMode          If true, then the customer can use maintenance (pause/resume)
         *                                             modes.
         *
         * \param[in] supportsRollups                  If true, then the customer can receive weekly rollups.
         *
         * \param[in] paused                           If true, then the customer is currently in a paused state.
         *                                             If false, then the customer is currently active.
         */
        CustomerCapabilities(
                CustomerId     customerId,
                unsigned short maximumNumberMonitors,
                unsigned short pollingInterval,
                unsigned       expirationDays,
                bool           customerActive,
                bool           multiRegionChecking,
                bool           supportsWordPress,
                bool           supportsRestApi,
                bool           supportsContentChecking,
                bool           supportsKeywordChecking,
                bool           supportsPostMethod,
                bool           supportsLatencyTracking,
                bool           supportsSslExpirationChecking,
                bool           supportsPingBasedPolling,
                bool           supportsBlackListChecking,
                bool           supportsDomainExpirationChecking,
                bool           supportsMaintenanceMode,
                bool           supportsRollups,
                bool           paused
            ):currentCustomerId(
                customerId
            ),currentMaximumNumberMonitors(
                maximumNumberMonitors
            ),currentPollingInterval(
                pollingInterval
            ),currentExpirationDays(
                expirationDays
            ) {
            currentFlags = (
                  (customerActive                   ? maskCustomerActive : 0)
                | (multiRegionChecking              ? maskMultiRegionChecking : 0)
                | (supportsWordPress                ? maskSupportsWordPress : 0)
                | (supportsRestApi                  ? maskSupportsRestApi : 0)
                | (supportsContentChecking          ? maskSupportsContentChecking : 0)
                | (supportsKeywordChecking          ? maskSupportsKeywordChecking : 0)
                | (supportsPostMethod               ? maskSupportsPostMethod : 0)
                | (supportsLatencyTracking          ? maskSupportsLatencyTracking : 0)
                | (supportsSslExpirationChecking    ? maskSupportsSslExpirationChecking : 0)
                | (supportsPingBasedPolling         ? maskSupportsPingBasedPolling : 0)
                | (supportsBlackListChecking        ? maskSupportsBlackListChecking : 0)
                | (supportsDomainExpirationChecking ? maskSupportsDomainExpirationChecking : 0)
                | (supportsMaintenanceMode          ? maskSupportsMaintenanceMode : 0)
                | (supportsRollups                  ? maskSupportsRollups : 0)
                | (paused                           ? maskPaused : 0)
            );
        }

        /**
         * Copy constructor.
         *
         * \param[in] other The instance to assign to this instance.
         */
        CustomerCapabilities(
                const CustomerCapabilities& other
            ):currentCustomerId(
                other.currentCustomerId
            ),currentMaximumNumberMonitors(
                other.currentMaximumNumberMonitors
            ),currentPollingInterval(
                other.currentPollingInterval
            ),currentExpirationDays(
                other.currentExpirationDays
            ),currentFlags(
                other.currentFlags
            ) {}

        /**
         * Method you can use to determine if this entry is valid.
         *
         * \return Returns true if this entry is valid.  Returns false if this entry is invalid.
         */
        inline bool isValid() const {
            return currentCustomerId != invalidCustomerId;
        }

        /**
         * Method you can use to determine if this entry is invalid.
         *
         * \return Returns true if this entry is invalid.  Returns false if this entry is valid.
         */
        inline bool isInvalid() const {
            return currentCustomerId == invalidCustomerId;
        }

        /**
         * Method that returns the customer ID.
         *
         * \return Returns the customer ID.
         */
        inline CustomerId customerId() const {
            return currentCustomerId;
        }

        /**
         * Method that returns the maximum number of monitors this customer can use.
         *
         * \return Returns the maximum number of monitors this customer is allowed to use.
         */
        inline unsigned short maximumNumberMonitors() const {
            return currentMaximumNumberMonitors;
        }

        /**
         * Method you can use to specify the maximum number of monitors this customer can use.
         *
         * \param[in] newMaximumNumberMonitors The new maximum number of monitors.
         */
        inline void setMaximumNumberMonitors(unsigned short newMaximumNumberMonitors) {
            currentMaximumNumberMonitors = newMaximumNumberMonitors;
        }

        /**
         * Method that returns the polling interval for this customer.
         *
         * \return Returns the polling interval for this customer.
         */
        inline unsigned short pollingInterval() const {
            return currentPollingInterval;
        }

        /**
         * Method you can use to specify the polling interval for this customer.
         *
         * \param[in] newPollingInterval The new polling interval.
         */
        inline void setPollingInterval(unsigned short newPollingInterval) {
            currentPollingInterval = newPollingInterval;
        }

        /**
         * Method that returns the latency data expiration period in days.
         *
         * \return Returns the maximum number of monitors this customer is allowed to use.
         */
        inline unsigned short expirationDays() const {
            return currentExpirationDays;
        }

        /**
         * Method you can use to specify the latency data expiration period in days.
         *
         * \param[in] newExpirationDays The new maximum number of monitors.
         */
        inline void setExpirationDays(unsigned short newExpirationDays) {
            currentExpirationDays = newExpirationDays;
        }

        /**
         * Method that indicates if this customer is active or inactive.
         *
         * \return Returns true if the customer is active.  Returns false if the customer is inactive.
         */
        inline bool customerActive() const {
            return currentFlags & maskCustomerActive ? true : false;
        }

        /**
         * Method you can use to specify whether the customer is now active.
         *
         * \param[in] nowActive If true, the customer is now active.  If false, the customer is now inactive.
         */
        inline void setCustomerActive(bool nowActive) {
            if (nowActive) {
                currentFlags |= maskCustomerActive;
            } else {
                currentFlags &= ~maskCustomerActive;
            }
        }

        /**
         * Method that indicates if this customer should be tracked from multiple regions.
         *
         * \return Returns true if the customer's site should be checked from multiple regions.  Otherwise returns
         *         false.
         */
        inline bool multiRegionChecking() const {
            return currentFlags & maskMultiRegionChecking ? true : false;
        }

        /**
         * Method you can use to specify whether the customer's sites should be checked from multiple regions.
         *
         * \param[in] nowAllowed If true, the customer sites should be checked from multiple regions.  If false, the
         *                       customer sites should be checked from a single region.
         */
        inline void setMultiRegionChecking(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskMultiRegionChecking;
            } else {
                currentFlags &= ~maskMultiRegionChecking;
            }
        }

        /**
         * Method that indicates if this customer can use the WordPress API.
         *
         * \return Returns true if the customer can use the WordPress API.  Otherwise returns false.
         */
        inline bool supportsWordPress() const {
            return currentFlags & maskSupportsWordPress ? true : false;
        }

        /**
         * Method you can use to determine if the customer can use the WordPress API.
         *
         * \param[in] nowAllowed If true, the customer can now use the Wordpress API.  If false, the customer can-not
         *                       use the WordPress API.
         */
        inline void setSupportsWordPress(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsWordPress;
            } else {
                currentFlags &= ~maskSupportsWordPress;
            }
        }

        /**
         * Method that indicates if the customer can use the REST API.
         *
         * \return Returns true if the customer can use the REST API.  Returns false if the customer can-not use the
         *         REST API.
         */
        inline bool supportsRestApi() const {
            return currentFlags & maskSupportsRestApi ? true : false;
        }

        /**
         * Method you can use to determine if the customer can use the REST API.
         *
         * \param[in] nowAllowed If true, the customer can now use the REST API.  If false, the customer can-not use
         *                       the REST API.
         */
        inline void setSupportsRestApi(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsRestApi;
            } else {
                currentFlags &= ~maskSupportsRestApi;
            }
        }

        /**
         * Method you can use to determine if the customer can use the exact match content checking functions.
         *
         * \return Returns true if the customer can use the exact match content checking functions.
         */
        inline bool supportsContentChecking() const {
            return currentFlags & maskSupportsContentChecking ? true : false;
        }

        /**
         * Method you can use to indicate whether the customer can use the exact match content checking functions.
         *
         * \param[in] nowAllowed If true, the customer can now use the exact match content checking functions.  If
         *                       false, the customer can-not use the exact match content checking functions.
         */
        inline void setSupportsContentChecking(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsContentChecking;
            } else {
                currentFlags &= ~maskSupportsContentChecking;
            }
        }

        /**
         * Method you can use to determine if the customer can use keyword checking functions.
         *
         * \return Returns true if the customer can use the keyword checking functions.
         */
        inline bool supportsKeywordChecking() const {
            return currentFlags & maskSupportsKeywordChecking ? true : false;
        }

        /**
         * Method you can use to indicate whether the customer is allowed to use the keyword checking functions.
         *
         * \param[in] nowAllowed If true, the customer can now use the keyword checking functions.  If false, the
         *                       customer can-not use the keyword checking functions.
         */
        inline void setSupportsKeywordChecking(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsKeywordChecking;
            } else {
                currentFlags &= ~maskSupportsKeywordChecking;
            }
        }

        /**
         * Method you can use to determine if the customer can use the POST method and associated controls.
         *
         * \return Returns true if the customer can use the POST method and associated controls.
         */
        inline bool supportsPostMethod() const {
            return currentFlags & maskSupportsPostMethod ? true : false;
        }

        /**
         * Method you can use to specify whether the customer can use the POST method and associated controls.
         *
         * \param[in] nowAllowed If true, the customer can now use the POST method and associated controls.  If false,
         *                       the customer can-not use the POST method.
         */
        inline void setSupportsPostMethod(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsPostMethod;
            } else {
                currentFlags &= ~maskSupportsPostMethod;
            }
        }

        /**
         * Method you can use to determine if we should collect latency data for this customer.
         *
         * \return Returns true if latency data is collected for this customer.
         */
        inline bool supportsLatencyTracking() const {
            return currentFlags & maskSupportsLatencyTracking ? true : false;
        }

        /**
         * Method you can use to indicate whether the customer should have latency data collected.
         *
         * \param[in] nowAllowed If true, the latency data should be collected for the customer.  If false, then
         *                       latency data should not be collected for the customer.
         */
        inline void setSupportsLatencyTracking(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsLatencyTracking;
            } else {
                currentFlags &= ~maskSupportsLatencyTracking;
            }
        }

        /**
         * Method you can use to determine if SSL expiration should be tracked.
         *
         * \return Returns true if SSL expiration for each host should be tracked.
         */
        inline bool supportsSslExpirationChecking() const {
            return currentFlags & maskSupportsSslExpirationChecking ? true : false;
        }

        /**
         * Method you can use to indicate whether the customer can track SSL expiration date/time information.
         *
         * \param[in] nowAllowed If true, the SSL expiration data should be tracked.  If false, then SSL expiration
         *                       data should not be tracked.
         */
        inline void setSupportsSslExpirationChecking(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsSslExpirationChecking;
            } else {
                currentFlags &= ~maskSupportsSslExpirationChecking;
            }
        }

        /**
         * Method that determines if ping based polling is supported for this customer.
         *
         * \return Returns true if ping based polling is supported for this customer.  Returns false if ping based
         *         polling is not supported for this customer.
         */
        inline bool supportsPingBasedPolling() const {
            return currentFlags & maskSupportsPingBasedPolling ? true : false;
        }

        /**
         * Method you can use specify whether ping based polling is supported for this customer.
         *
         * \param[in] nowAllowed If true, the ping based polling from one region should be used for this customer.  If
         *                       false, then ping based polling should not be used.
         */
        inline void setSupportsPingBasedPolling(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsPingBasedPolling;
            } else {
                currentFlags &= ~maskSupportsPingBasedPolling;
            }
        }

        /**
         * Method that determines if the customer's site should be checked for black listing.
         *
         * \return Returns true if the customer's site should be checked for blacklisting.
         */
        inline bool supportsBlackListChecking() const {
            return currentFlags & maskSupportsBlackListChecking ? true : false;
        }

        /**
         * Method you can use to specify whether blacklist checking should be enabled for this customer.
         *
         * \param[in] nowAllowed If true, the blacklist checking should be supported for this customer.  If false, the
         *                       blacklist checking should not be supported for this customer.
         */
        inline void setSupportsBlackListChecking(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsBlackListChecking;
            } else {
                currentFlags &= ~maskSupportsBlackListChecking;
            }
        }

        /**
         * Method that determines if the domain expiration date/time should be checked for this customer.
         *
         * \return Returns true if domain expiration date/time data should be checked for this customer.
         */
        inline bool supportsDomainExpirationChecking() const {
            return currentFlags & maskSupportsDomainExpirationChecking ? true : false;
        }

        /**
         * Method you can use to specify whether domain expiration checking should be enabled for this customer.
         *
         * \param[in] nowAllowed If true, the domain expiration checking should be performed.  If false, then domain
         *                       expiration checking should not be performed.
         */
        inline void setSupportsDomainExpirationChecking(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsDomainExpirationChecking;
            } else {
                currentFlags &= ~maskSupportsDomainExpirationChecking;
            }
        }

        /**
         * Method that determines if maintenance mode should be allowed for this customer.
         *
         * \return Returns true if maintenance mode can be used by this customer.  Returns false if maintenance mode
         *         can-not be used by this customer.
         */
        inline bool supportsMaintenanceMode() const {
            return currentFlags & maskSupportsMaintenanceMode ? true : false;
        }

        /**
         * Method you can use to specify maintenance mode should be allowed for this customer.
         *
         * \param[in] nowAllowed If true, maintenance mode can be used.  If false, maintenance mode can-not be used.
         */
        inline void setSupportsMaintenanceMode(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsMaintenanceMode;
            } else {
                currentFlags &= ~maskSupportsMaintenanceMode;
            }
        }

        /**
         * Method that determines if rollups should be generated for this customer.
         *
         * \return Returns true if rollups can be generated for this customer.  Returns false if rollups should not be
         *         generated for this customer.
         */
        inline bool supportsRollups() const {
            return currentFlags & maskSupportsRollups ? true : false;
        }

        /**
         * Method you can use to specify rollups should be generated for this customer.
         *
         * \param[in] nowAllowed If true, rollups should be generated.  If false, then roll-ups should not be
         *                       generated.
         */
        inline void setSupportsRollups(bool nowAllowed) {
            if (nowAllowed) {
                currentFlags |= maskSupportsRollups;
            } else {
                currentFlags &= ~maskSupportsRollups;
            }
        }

        /**
         * Method that determines if the customer is currently paused.
         *
         * \return Returns true if the customer is paused.  Returns false if the customer is active.
         */
        inline bool paused() const {
            return currentFlags & maskPaused ? true : false;
        }

        /**
         * Method you can use to specify whether the customer is currently paused or active.
         *
         * \param[in] nowPaused If true, then the customer show be paused.  If false, then the customer should be
         *                      active.
         */
        inline void setPaused(bool nowPaused) {
            if (nowPaused) {
                currentFlags |= maskPaused;
            } else {
                currentFlags &= ~maskPaused;
            }
        }

        /**
         * Assignment operator.
         *
         * \param[in] other The value to assign to this value.
         *
         * \return Returns a reference to this object.
         */
        CustomerCapabilities& operator=(const CustomerCapabilities& other) {
            currentCustomerId            = other.currentCustomerId;
            currentMaximumNumberMonitors = other.currentMaximumNumberMonitors;
            currentPollingInterval       = other.currentPollingInterval;
            currentExpirationDays        = other.currentExpirationDays;
            currentFlags                 = other.currentFlags;

            return *this;
        }

    private:
        /**
         * Constructor.
         *
         * \param[in] customerId            The ID used to identify this region.
         *
         * \param[in] maximumNumberMonitors The maximum number of monitors this customer can use.
         *
         * \param[in] pollingInterval       The polling interval required for this customer, in seconds.
         *
         * \param[in] expirationDays        The latency expiration period in days.
         *
         * \param[in] flags                 Value representing all the collected boolean values.
         */
        CustomerCapabilities(
                CustomerId     customerId,
                unsigned short maximumNumberMonitors,
                unsigned short pollingInterval,
                unsigned       expirationDays,
                Flags          flags
            ):currentCustomerId(
                customerId
            ),currentMaximumNumberMonitors(
                maximumNumberMonitors
            ),currentPollingInterval(
                pollingInterval
            ),currentExpirationDays(
                expirationDays
            ),currentFlags(
                flags
            ) {}

        /**
         * Method used by the CustomersCapabilities class to get the raw flag values.
         *
         * \return Returns the current raw flags value.
         */
        Flags flags() const {
            return currentFlags;
        }

        /**
         * Bit mask for the customer active flag.
         */
        static constexpr Flags maskCustomerActive = 1 << 0;

        /**
         * Bit mask for the multi-region checking flag.
         */
        static constexpr Flags maskMultiRegionChecking = 1 << 1;

        /**
         * Bit mask for the supports wordpress flag.
         */
        static constexpr Flags maskSupportsWordPress = 1 << 2;

        /**
         * Bit mask for the supports REST API flag.
         */
        static constexpr Flags maskSupportsRestApi = 1 << 3;

        /**
         * Bit mask for the supports content checking flag.
         */
        static constexpr Flags maskSupportsContentChecking = 1 << 4;

        /**
         * Bit mask for the supports keyword checking flag.
         */
        static constexpr Flags maskSupportsKeywordChecking = 1 << 5;

        /**
         * Bit mask for the supports POST method flag.
         */
        static constexpr Flags maskSupportsPostMethod = 1 << 6;

        /**
         * Bit mask for the supports latency tracking flag.
         */
        static constexpr Flags maskSupportsLatencyTracking = 1 << 7;

        /**
         * Bit mask for the supports SSL expiration checking flag.
         */
        static constexpr Flags maskSupportsSslExpirationChecking = 1 << 8;

        /**
         * Bit mask for the supports ping based polling flag.
         */
        static constexpr Flags maskSupportsPingBasedPolling = 1 << 9;

        /**
         * Bit mask for the supports blacklist checking flag.
         */
        static constexpr Flags maskSupportsBlackListChecking = 1 << 10;

        /**
         * Bit mask for the supports domain expiration checking flag.
         */
        static constexpr Flags maskSupportsDomainExpirationChecking = 1 << 11;

        /**
         * Bit mask for the supports maintenance mode flag.
         */
        static constexpr Flags maskSupportsMaintenanceMode = 1 << 12;

        /**
         * Bit mask for the supports rollups flag.
         */
        static constexpr Flags maskSupportsRollups = 1 << 13;

        /**
         * Bit mask for the paused status flag.
         */
        static constexpr Flags maskPaused = 1 << 15;

        /**
         * The customer ID.
         */
        CustomerId currentCustomerId;

        /**
         * The current number of supported monitors.
         */
        unsigned short currentMaximumNumberMonitors;

        /**
         * The polling interval for this customer, in seconds.
         */
        unsigned short currentPollingInterval;

        /**
         * The current expiration period for customer data, in days.
         */
        unsigned currentExpirationDays;

        /**
         * The current flags.
         */
        Flags currentFlags;
};

#endif
