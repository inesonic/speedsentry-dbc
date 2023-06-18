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
* This header defines the \ref Resource class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef RESOURCE_H
#define RESOURCE_H

#include <QString>
#include <QByteArray>
#include <QList>

#include <cstdint>

#include "customer_capabilities.h"
#include "short_latency_entry.h"

class Monitors;
class PostSetting;

/**
 * Class used to hold information about a single resource.
 */
class Resource {
    friend class Resources;

    public:
        /**
         * Type used to represent a customer ID.
         */
        typedef CustomerCapabilities::CustomerId CustomerId;

        /**
         * Type used to indicate the type of value being stored.
         */
        typedef std::uint8_t ValueType;

        /**
         * Type used to store the value.
         */
        typedef float Value;

    private:
        /**
         * Constructor.
         *
         * \param[in] customerId
         *
         * \param[in] valueType
         *
         * \param[in] value
         *
         * \param[in] timestamp
         */
        constexpr Resource(
                CustomerId    customerId,
                ValueType     valueType,
                Value         value,
                std::uint64_t timestamp
            ):currentCustomerId(
                customerId
            ),currentValueType(
                valueType
            ),currentValue(
                value
            ),currentTimestamp(
                timestamp
            ) {}

    public:
        constexpr Resource():
            currentCustomerId(CustomerCapabilities::invalidCustomerId),
            currentValueType(0),
            currentValue(0),
            currentTimestamp(0) {}

        /**
         * Copy constructor
         *
         * \param[in] other The instance to be copied.
         */
        constexpr Resource(
                const Resource& other
            ):currentCustomerId(
                other.currentCustomerId
            ),currentValueType(
                other.currentValueType
            ),currentValue(
                other.currentValue
            ),currentTimestamp(
                other.currentTimestamp
            ) {}

        ~Resource() {}

        /**
         * Method that determines if this resource is valid.
         *
         * \return Returns true if the resource is valid.  Returns false if the resource is invalid.
         */
        inline bool isValid() const {
            return currentCustomerId != CustomerCapabilities::invalidCustomerId;
        }

        /**
         * Method that determines if this resource is invalid.
         *
         * \return Returns true if the resource is invalid.  Returns false if the resource is ialid.
         */
        inline bool isInvalid() const {
            return currentCustomerId == CustomerCapabilities::invalidCustomerId;
        }

        /**
         * Method you can use to obtain the customer ID.
         *
         * \return Returns the current customer ID.
         */
        inline CustomerId customerId() const {
            return currentCustomerId;
        }

        /**
         * Method you can use to set the current value you.
         *
         * \param[in] newValueType The new value type.
         */
        inline void setValueType(ValueType newValueType) {
            currentValueType = newValueType;
        }

        /**
         * Method you can use to obtain the current value type.
         *
         * \return Returns the value type.
         */
        inline ValueType valueType() const {
            return currentValueType;
        }

        /**
         * Method you can use to set the current value.
         *
         * \param[in] newValue The new value to assign to this instance.
         */
        inline void setValue(Value newValue) {
            currentValue = newValue;
        }

        /**
         * Method you can use to obtain the current value.
         *
         * \return Returns the value tracked by this entry.
         */
        inline Value value() const {
            return currentValue;
        }

        /**
         * Method you can use to set the current timestamp.
         *
         * \param[in] newTimestamp The new Unix timestamp.
         */
        inline void setUnixTimstamp(std::uint64_t newTimestamp) {
            currentTimestamp = newTimestamp;
        }

        /**
         * Method you can use to obtain the current Unix timestamp for this entry.
         *
         * \return Returns the current Unix timestamp for this entry.
         */
        inline std::uint64_t unixTimestamp() const {
            return currentTimestamp;
        }

        /**
         * Assignment operator.
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline Resource& operator=(const Resource& other) {
            currentCustomerId = other.currentCustomerId;
            currentValueType  = other.currentValueType;
            currentValue      = other.currentValue;
            currentTimestamp  = other.currentTimestamp;

            return *this;
        }

    private:
        /**
         * The customer who owns this monitor.
         */
        CustomerId currentCustomerId;

        /**
         * The value type.
         */
        ValueType currentValueType;

        /**
         * The current value.
         */
        Value currentValue;

        /**
         * The current timestamp.
         */
        std::uint64_t currentTimestamp;
};

#endif
