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
* This header defines the \ref PlotMailbox class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef PLOT_MAILBOX_H
#define PLOT_MAILBOX_H

#include <QObject>
#include <QImage>
#include <QSemaphore>

/**
 * Class to manage inter-thread plot mailboxes.
 */
class PlotMailbox:public QObject {
    Q_OBJECT

    public:
        /**
         * Enumeration of mailbox status codes.
         */
        enum class Status {
            /**
             * Indicates the image is not yet available.
             */
            NOT_AVAILABLE,

            /**
             * Indicates we have an image.
             */
            AVAILABLE,

            /**
             * Indicates an error status code.
             */
            FAILED
        };

        /**
         * Private constructor.
         *
         * \param[in] parent Pointer to the parent instance.
         */
        PlotMailbox(QObject* parent = nullptr);

        ~PlotMailbox() override;

        /**
         * Method that empties the mailbox.  Only call this method if you expect the mailbox to be empty.
         */
        void forceEmpty();

        /**
         * Method you can call to wait for a generated image.
         *
         * \return Returns the newly generated image.  Returns an empty image on error.
         */
        QImage waitForImage();

        /**
         * Method you can use to determine if an image is available.
         *
         * \return Returns the image status.
         */
        Status status() const;

        /**
         * Method that is used to update the mailbox image.
         *
         * \param[in] imageData The image data to be reported.
         */
        void sendImage(const QImage& imageData);

        /**
         * Method that is used to report an error.
         */
        void sendFailedStatus();

    signals:
        /**
         * Signal that is emitted when an image becomes available.  Be sure to always call \ref waitForImage to release
         * the image.
         */
        void imageAvailable();

        /**
         * Signal that is emitted if an error occurs.  Be sure to always call \ref waitForImage to release
         * the current status.
         */
        void failed();

    private:
        /**
         * Semaphore used to block until an image is available.
         */
        QSemaphore imageSemaphore;

        /**
         * The current image data.
         */
        QImage currentImageData;

        /**
         * The current status.
         */
        Status currentStatus;
};

#endif
