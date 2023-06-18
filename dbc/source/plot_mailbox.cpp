/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref PlotMailbox class.
***********************************************************************************************************************/

#include <QObject>
#include <QImage>
#include <QSemaphore>

#include "plot_mailbox.h"

PlotMailbox::PlotMailbox(QObject* parent):QObject(parent) {
    currentStatus = Status::NOT_AVAILABLE;
}


PlotMailbox::~PlotMailbox() {}


void PlotMailbox::forceEmpty() {
    if (currentStatus != Status::NOT_AVAILABLE) {
        currentImageData = QImage();
        currentStatus = Status::NOT_AVAILABLE;

        imageSemaphore.tryAcquire();
    }
}


QImage PlotMailbox::waitForImage() {
    imageSemaphore.acquire();
    currentStatus = Status::NOT_AVAILABLE;

    QImage result;
    currentImageData.swap(result);

    return result;
}


PlotMailbox::Status PlotMailbox::status() const {
    return currentStatus;
}


void PlotMailbox::sendImage(const QImage& imageData) {
    currentImageData = imageData;
    currentStatus    = Status::AVAILABLE;

    // The code below is safe since we're the only thread that will ever release the resource.
    if (imageSemaphore.available() == 0) {
        imageSemaphore.release(1);
    }

    emit imageAvailable();
}


void PlotMailbox::sendFailedStatus() {
    currentImageData = QImage();
    currentStatus    = Status::FAILED;

    imageSemaphore.release(1);

    emit failed();
}
