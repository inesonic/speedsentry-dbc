/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref ActiveResources class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef ACTIVE_RESOURCES_H
#define ACTIVE_RESOURCES_H

#include <cstdint>

#include "resource.h"

class Regions;

/**
 * Trivial class used to hold data on which resources are active.
 */
class ActiveResources {
    public:
        /**
         * Type used to represent a value type.
         */
        typedef Resource::ValueType ValueType;

        /**
         * Value used to represent a customer ID.
         */
        typedef Resource::CustomerId CustomerId;

        /**
         * Value indicating the maximum number of available value types.
         */
        static constexpr unsigned long long numberValueTypes = 1 << (8 * sizeof(ValueType));

        /**
         * Value indicating an invalid value type.
         */
        static constexpr ValueType invalidValueType = static_cast<ValueType>(numberValueTypes - 1);

        ActiveResources();

        /**
         * Constructor
         *
         * \param[in] customerId The ID of the customer associated with the tracked value types.
         */
        ActiveResources(CustomerId customerId);

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        ActiveResources(const ActiveResources& other);

        ~ActiveResources();

        /**
         * Method you can use to obtain the customer ID of the customer tracked by these resources.
         *
         * \return Returns the associated customer ID.
         */
        inline CustomerId customerId() const {
            return currentCustomerId;
        }

        /**
         * Method you can use to determine if this active resource data is valid.
         *
         * \return Returns true if the active resource data is valid.  Returns false if the active resource data is
         *         invalid.
         */
        bool isValid() const;

        /**
         * Method you can use to determine if this active resource data is invalid.
         *
         * \return Returns true if the active resource data is invalid.  Returns false if the active resource data is
         *         valid.
         */
        inline bool isInvalid() const {
            return !isValid();
        }

        /**
         * Method you can use to set a resource as active or inactive.
         *
         * \param[in] valueType The value type of the resource to be set active.
         *
         * \param[in] nowActive If true, the value type will be marked as active.  If false, the value type will be
         *                      marked as inactive.
         */
        void setActive(ValueType valueType, bool nowActive = true);

        /**
         * Method you can use to set a resource as inactive or active.
         *
         * \param[in] valueType   The value type of the resource to be set active.
         *
         * \param[in] nowInactive If true, the value type will be marked as inactive.  If false, the value type will be
         *                        marked as active.
         */
        inline void setInactive(ValueType valueType, bool nowInactive = true) {
            setActive(valueType, !nowInactive);
        }

        /**
         * Method you can use to determine if a value type is active.
         *
         * \param[in] valueType The value type to be tested.
         *
         * \return Returns true if the value type is active.  Returns false if the value type is inactive.
         */
        bool isActive(ValueType valueType) const;

        /**
         * Method you can use to determine if a value type is inactive.
         *
         * \param[in] valueType The value type to be tested.
         *
         * \return Returns true if the value type is inactive.  Returns false if the value type is active.
         */
        inline bool isInactive(ValueType valueType) {
            return !isActive(valueType);
        }

        /**
         * Method you can use to find the next used value type.
         *
         * \param startingValueType The starting value type to be tested, inclusive.
         *
         * \return Returns the next used value type.  A value of \ref invalidValueType is returned if no valid value
         *         type is identified.
         */
        ValueType nextValidValueType(ValueType startingValueType = 0) const;

        /**
         * Assignment operator
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return returns a reference to this instance.
         */
        ActiveResources& operator=(const ActiveResources& other);

    private:
        /**
         * Type representing an active value type flag.
         */
        typedef std::uint64_t Flags;

        /**
         * The number of entries per flag.
         */
        static constexpr unsigned entriesPerFlag = 8 * sizeof(Flags);

        /**
         * The required number of flags.
         */
        static constexpr unsigned numberFlagElements = (numberValueTypes + entriesPerFlag - 1) / entriesPerFlag;

        /**
         * Function that calculates the location of the MSB of a 64-bit value.
         *
         *  param[in] value The value to determine the MSB location of.
         *
         * \return Returns the power of 2 indicating the location.  A value of -1 is returned for the value 0.
         */
        static int lsbLocation64(std::uint64_t value);

        /**
         * The customer ID of the customer associated with the resources.
         */
        CustomerId currentCustomerId;

        /**
         * Array of value type flags.
         */
        Flags activeFlags[numberFlagElements];
};

#endif
