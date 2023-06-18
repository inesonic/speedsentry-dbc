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
