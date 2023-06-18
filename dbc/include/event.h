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
* This header defines the \ref Event class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef EVENT_H
#define EVENT_H

#include <QString>
#include <QHash>
#include <QByteArray>

#include <cstdint>

#include "customer_capabilities.h"
#include "latency_entry.h"
#include "monitor.h"

class Events;

/**
 * Trivial class used to hold information about an event.
 */
class Event {
    friend class Events;

    public:
        /**
         * Value used to represent an event ID.
         */
        typedef std::uint32_t EventId;

        /**
         * Value used to represent a monitor ID.
         */
        typedef Monitor::MonitorId MonitorId;

        /**
         * Type used to represent a customer ID.
         */
        typedef CustomerCapabilities::CustomerId CustomerId;

        /**
         * Type used to represent a timestamp.
         */
        typedef LatencyEntry::ZoranTimeStamp ZoranTimeStamp;

        /**
         * Enumeration of supported event types.
         */
        enum class EventType : std::uint8_t {
            /**
             * Indicates an invalid event type.
             */
            INVALID = 0,

            /**
             * Indicates that the monitor is now working.
             */
            WORKING = 1,

            /**
             * Indicates no response from the requested endpoint.
             */
            NO_RESPONSE = 2,

            /**
             * Indicates that the site content changed state.
             */
            CONTENT_CHANGED = 3,

            /**
             * Indicates the keyword check for the endpoint failed.
             */
            KEYWORDS = 4,

            /**
             * Indicates an SSL certificate is due to expire.
             */
            SSL_CERTIFICATE_EXPIRING = 5,

            /**
             * Indicates an SSL certificate has been renewed.  This event is not used by the polling server.
             */
            SSL_CERTIFICATE_RENEWED = 6,

            /**
             * Indicates a customer triggered event.
             */
            CUSTOMER_1 = 7,

            /**
             * Indicates a customer triggered event.
             */
            CUSTOMER_2 = 8,

            /**
             * Indicates a customer triggered event.
             */
            CUSTOMER_3 = 9,

            /**
             * Indicates a customer triggered event.
             */
            CUSTOMER_4 = 10,

            /**
             * Indicates a customer triggered event.
             */
            CUSTOMER_5 = 11,

            /**
             * Indicates a customer triggered event.
             */
            CUSTOMER_6 = 12,

            /**
             * Indicates a customer triggered event.
             */
            CUSTOMER_7 = 13,

            /**
             * Indicates a customer triggered event.
             */
            CUSTOMER_8 = 14,

            /**
             * Indicates a customer triggered event.
             */
            CUSTOMER_9 = 15,

            /**
             * Indicates a customer triggered event.
             */
            CUSTOMER_10 = 16,

            /**
             * Indicates a transaction
             */
            TRANSACTION = 17,

            /**
             * Indicates a customer inquiry received.
             */
            INQUIRY = 18,

            /**
             * Indicates a support request.
             */
            SUPPORT_REQUEST = 19,

            /**
             * Indicates a storage limit has been reached.
             */
            STORAGE_LIMIT_REACHED = 20
        };

        /**
         * The monitor status code.
         */
        typedef Monitor::MonitorStatus MonitorStatus;

        /**
         * Value used to indicate an invalid event ID.
         */
        static constexpr EventId invalidEventId = 0;

        /**
         * Value used to indicate an invalid customerID.
         */
        static constexpr CustomerId invalidCustomerId = CustomerCapabilities::invalidCustomerId;

    private:
        /**
         * Constructor.
         *
         * \param[in] eventId    The ID used to identify this region.
         *
         * \param[in] monitorId  The monitor ID indicating the monitor that triggered the event.
         *
         * \param[in] customerId The customer ID of the customer tied to the event.
         *
         * \param[in] timestamp  The Zoran timestamp when the event occurred.
         *
         * \param[in] eventType  The type of event that was triggered.
         *
         * \param[in] message    A message explaining additional details about this event.
         *
         * \param[in] hash       The hash reported with this event.
         */
        inline Event(
                EventId           eventId,
                MonitorId         monitorId,
                CustomerId        customerId,
                ZoranTimeStamp    zoranTimestamp,
                EventType         eventType,
                const QString&    message,
                const QByteArray& hash
            ):currentEventId(
                eventId
            ),currentMonitorId(
                monitorId
            ),currentCustomerId(
                customerId
            ),currentZoranTimestamp(
                zoranTimestamp
            ),currentEventType(
                eventType
            ),currentMessage(
                message
            ),currentHash(
                hash
            ) {}

    public:
        inline Event() {
            currentEventId        = invalidEventId;
            currentMonitorId      = 0;
            currentCustomerId     = invalidCustomerId;
            currentZoranTimestamp = 0;
            currentEventType      = EventType::INVALID;
        }

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline Event(
                const Event& other
            ):currentEventId(
                other.currentEventId
            ),currentMonitorId(
                other.currentMonitorId
            ),currentCustomerId(
                other.currentCustomerId
            ),currentZoranTimestamp(
                other.currentZoranTimestamp
            ),currentEventType(
                other.currentEventType
            ),currentMessage(
                other.currentMessage
            ),currentHash(
                other.currentHash
            ) {}

        /**
         * Move constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline Event(
                Event&& other
            ):currentEventId(
                other.currentEventId
            ),currentMonitorId(
                other.currentMonitorId
            ),currentCustomerId(
                other.currentCustomerId
            ),currentZoranTimestamp(
                other.currentZoranTimestamp
            ),currentEventType(
                other.currentEventType
            ),currentMessage(
                other.currentMessage
            ),currentHash(
                other.currentHash
            ) {}

        ~Event() = default;

        /**
         * Method you can use to determine if this event instance is valid.
         *
         * \return Returns True if the event instance is valid.  Returns false if the event instance is invalid.
         */
        inline bool isValid() const {
            return currentEventId != invalidEventId;
        }

        /**
         * Method you can use to determine if this region instance is invalid.
         *
         * \return Returns True if the region instance is invalid.  Returns false if the region instance is valid.
         */
        inline bool isInvalid() const {
            return !isValid();
        }

        /**
         * Method you can use to get the event ID for this event.
         *
         * \return Returns the event ID tied to this event.
         */
        inline EventId eventId() const {
            return currentEventId;
        }

        /**
         * Method you can use to get the monitor ID tied to this event.
         *
         * \return Returns the monitor ID for this event.
         */
        inline MonitorId monitorId() const {
            return currentMonitorId;
        }

        /**
         * Method you can use to change the monitor ID tied to this event.
         *
         * \param[in] newMonitorId The new monitor ID for this event.
         */
        inline void setMonitorId(MonitorId newMonitorId) {
            currentMonitorId = newMonitorId;
        }

        /**
         * Method you can use to obtain the customer ID of the customer tied to this event.
         *
         * \return Returns the customer ID of the owner customer.
         */
        inline CustomerId customerId() const {
            return currentCustomerId;
        }

        /**
         * Method you can use to change the customer ID of the customer tied to this event.
         *
         * \param[in] newCustomerId The new customer ID.
         */
        inline void setCustomerId(CustomerId newCustomerId) {
            currentCustomerId = newCustomerId;
        }


        /**
         * Method you can use to obtain the current Zoran timestamp.
         *
         * \return Returns the current timestamp in Zoran time.
         */
        inline ZoranTimeStamp zoranTimestamp() const {
            return currentZoranTimestamp;
        }

        /**
         * Method you can use to obtain the current Unix timestamp for this entry.
         *
         * \return Returns the current Unix timestamp for this entry.
         */
        inline std::uint64_t unixTimestamp() const {
            return currentZoranTimestamp + LatencyEntry::startOfZoranEpoch;
        }

        /**
         * Method you can use to set the Zoran timestamp for this entry.
         *
         * \param[in] newZoranTimestamp The Zoran timestamp for this entry.
         */
        inline void setZoranTimestamp(ZoranTimeStamp newZoranTimestamp) {
            currentZoranTimestamp = newZoranTimestamp;
        }

        /**
         * Method you can use to set the Unix timestamp for this entry.
         *
         * \param[in] newUnixTimestamp The Unix timestamp for this entry.
         */
        inline void setUnixTimestamp(unsigned long long newUnixTimestamp) {
            currentZoranTimestamp = static_cast<ZoranTimeStamp>(newUnixTimestamp - LatencyEntry::startOfZoranEpoch);
        }

        /**
         * Method you can use to obtain the event type for this event.
         *
         * \return Returns the event type.
         */
        inline EventType eventType() const {
            return currentEventType;
        }

        /**
         * Method you can use to change the event type for this entry.
         *
         * \param[in] newEventType The new event type.
         */
        inline void setEventType(EventType newEventType) {
            currentEventType = newEventType;
        }

        /**
         * Method you can use to obtain the event message text.
         *
         * \return Returns the event message text.
         */
        inline const QString& message() const {
            return currentMessage;
        }

        /**
         * Method you can use to set the event message text.
         *
         * \param[in] newMessage The new event message.
         */
        inline void setMessge(const QString& newMessage) {
            currentMessage = newMessage;
        }

        /**
         * Method you can use to obtain the event hash.
         *
         * \return Returns the current event hash.
         */
        inline const QByteArray& hash() const {
            return currentHash;
        }

        /**
         * Method you can use to set the event hash.
         *
         * \param[in] newHash The new message hash.
         */
        inline void setHash(const QByteArray& newHash) {
            currentHash = newHash;
        }

        /**
         * Assignment operator
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline Event& operator=(const Event& other) {
            currentEventId        = other.currentEventId;
            currentMonitorId      = other.currentMonitorId;
            currentCustomerId     = other.currentCustomerId;
            currentZoranTimestamp = other.currentZoranTimestamp;
            currentEventType      = other.currentEventType;
            currentMessage        = other.currentMessage;
            currentHash           = other.currentHash;

            return *this;
        }

        /**
         * Assignment operator (move semantics)
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline Event& operator=(Event&& other) {
            currentEventId        = other.currentEventId;
            currentMonitorId      = other.currentMonitorId;
            currentCustomerId     = other.currentCustomerId;
            currentZoranTimestamp = other.currentZoranTimestamp;
            currentEventType      = other.currentEventType;
            currentMessage        = other.currentMessage;
            currentHash           = other.currentHash;

            return *this;
        }

        /**
         * Method you can use to convert an event type to a string.
         *
         * \param[in] value The value to be converted.
         *
         * \return Returns a string representation of the value.
         */
        static QString toString(EventType value);

        /**
         * Method you can use to convert a string to an event type.
         *
         * \param[in] str The string to be converted.
         *
         * \param[in] ok  An optional pointer to a boolean value that will contain true on success or false on error.
         *
         * \return Returns the generated event type value.
         */
        static EventType toEventType(const QString& str, bool* ok = nullptr);

        /**
         * Method you can use to convert a string to an event type.
         *
         * \param[in] typeIndex A 1-based type index indicating the event.
         *
         * \return Returns the generated event type value.  A value of EventType::INVALID is returned if the index is
         *         invalid.
         */
        static EventType toCustomerEventType(unsigned typeIndex);

        /**
         * Method you can use to determine if an event is a customer event.
         *
         * \param[in] eventType The event type to be checked.
         *
         * \return Returns true if the event is a customer event.  Returns false for Inesonic events.
         */
        static bool isCustomerEvent(EventType eventType);

        /**
         * Method you can use to determine if an event is not a customer event.
         *
         * \param[in] eventType The event type to be checked.
         *
         * \return Returns true if the event is an Inesonic event.  Returns false if the event is a customer event.
         */
        static inline bool isNotCustomerEvent(EventType eventType) {
            return !isCustomerEvent(eventType);
        }

    private:
        /**
         * The ID used to uniquely identify this event.
         */
        EventId currentEventId;

        /**
         * The monitor ID of the monitor tied to this event.
         */
        MonitorId currentMonitorId;

        /**
         * The customer ID of the customer tied to this event.
         */
        CustomerId currentCustomerId;

        /**
         * The event timestamp (Zoran time)
         */
        ZoranTimeStamp currentZoranTimestamp;

        /**
         * The type of event that occurred.
         */
        EventType currentEventType;

        /**
         * The event message.
         */
        QString currentMessage;

        /**
         * The current message hash.
         */
        QByteArray currentHash;
};

/**
 * Function that calculates a hash for the EventType value.
 *
 * \param[in] value The value to calculate the hash for.
 *
 * \param[in] seed  An optional seed to be applied.
 *
 * \return Returns a hash of the calculated value.
 */
inline unsigned qHash(Event::EventType value, unsigned seed = 0) {
    return qHash(static_cast<unsigned>(value), seed);
}

#endif
