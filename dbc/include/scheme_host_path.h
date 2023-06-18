/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref SchemeHostPath class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef SCHEME_HOST_PATH_H
#define SCHEME_HOST_PATH_H

#include <QString>
#include <QHash>

#include <cstdint>

#include "host_scheme.h"

/**
 * Trivial class that tracks a combination host/scheme and path.
 */
class SchemeHostPath {
    public:
        /**
         * Type used to represent a host/scheme ID.
         */
        typedef HostScheme::HostSchemeId HostSchemeId;

        /**
         * Constructor
         *
         * \param[in] hostSchemeId The host/scheme tracked by this class.
         *
         * \param[in] path         The path tracked by this class.
         */
        inline SchemeHostPath(
                HostSchemeId   hostSchemeId = HostScheme::invalidHostSchemeId,
                const QString& path = QString()
            ):currentHostSchemeId(
                hostSchemeId
            ),currentPath(
                path
            ) {}

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline SchemeHostPath(
                const SchemeHostPath& other
            ):currentHostSchemeId(
                other.currentHostSchemeId
            ),currentPath(
                other.currentPath
            ) {}

        /**
         * Move constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline SchemeHostPath(
                SchemeHostPath&& other
            ):currentHostSchemeId(
                other.currentHostSchemeId
            ),currentPath(
                other.currentPath
            ) {}

        ~SchemeHostPath() = default;

        /**
         * Method you can use to get the current host/scheme ID.
         *
         * \return Returns the host/scheme ID tied to this instance.
         */
        HostSchemeId hostSchemeId() const {
            return currentHostSchemeId;
        }

        /**
         * Method you can use to get the path tied to this instance.
         *
         * \return Returns the path tied to this instance.
         */
        const QString& path() const {
            return currentPath;
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if the instances are equal.  Returns false if the instances are not equal.
         */
        inline bool operator==(const SchemeHostPath& other) const {
            return currentHostSchemeId == other.currentHostSchemeId && currentPath == other.currentPath;
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if the instances are not equal.  Returns false if the instances are equal.
         */
        inline bool operator!=(const SchemeHostPath& other) const {
            return currentHostSchemeId != other.currentHostSchemeId || currentPath != other.currentPath;
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if this instance should precede the other instance.  Returns false if the instances
         *         are equivalent or this instance should follow the other instance.
         */
        inline bool operator<(const SchemeHostPath& other) const {
            return (
                   currentHostSchemeId < other.currentHostSchemeId
                || (   currentHostSchemeId == other.currentHostSchemeId
                    && currentPath < other.currentPath
                   )
            );
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if this instance should follow the other instance.  Returns false if the instances
         *         are equivalent or this instance should precede the other instance.
         */
        inline bool operator>(const SchemeHostPath& other) const {
            return (
                   currentHostSchemeId > other.currentHostSchemeId
                || (   currentHostSchemeId == other.currentHostSchemeId
                    && currentPath > other.currentPath
                   )
            );
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if this instance should precede the other instance or is equivalent to the other
         *         instance.  Returns false if this instance should follow the other instance.
         */
        inline bool operator<=(const SchemeHostPath& other) const {
            return !operator>(other);
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if this instance should follow the other instance or is equivalent to the other
         *         instance.  Returns false if this instance should precede the other instance.
         */
        inline bool operator>=(const SchemeHostPath& other) const {
            return !operator<(other);
        }

        /**
         * Assignment operator
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline SchemeHostPath& operator=(const SchemeHostPath& other) {
            currentHostSchemeId = other.currentHostSchemeId;
            currentPath         = other.currentPath;

            return *this;
        }

        /**
         * Assignment operator (move semantics)
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline SchemeHostPath& operator=(SchemeHostPath&& other) {
            currentHostSchemeId = other.currentHostSchemeId;
            currentPath         = other.currentPath;

            return *this;
        }

    private:
        /**
         * The host/scheme indicating the server this monitor is checking.
         */
        HostSchemeId currentHostSchemeId;

        /**
         * The current path under the host.  Value is as entered by the customer.
         */
        QString currentPath;
};

/**
 * Hash function for the \ref SchemeHostPath class.
 *
 * \param[in] value The value to be hashed.
 *
 * \param[in] seed  An optional seed to apply to the hash.
 *
 * \return Returns a numerical hash of the \ref SchemeHostPath instance.
 */
inline unsigned qHash(const SchemeHostPath& value, unsigned seed = 0) {
    return qHash(value.hostSchemeId(), seed) ^ qHash(value.path(), seed);
}

#endif
