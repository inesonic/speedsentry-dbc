/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
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
