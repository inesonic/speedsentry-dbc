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
* This header implements the \ref MonitorUpdater class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QTimer>
#include <QByteArray>
#include <QMap>
#include <QSet>
#include <QHash>
#include <QDateTime>

#include <cstdint>
#include <limits>
#include <algorithm>

#include "customer_capabilities.h"
#include "host_scheme.h"
#include "monitor.h"
#include "scheme_host.h"
#include "scheme_host_path.h"
#include "host_schemes.h"
#include "monitors.h"
#include "server_administrator.h"
#include "monitor_updater.h"

MonitorUpdater::MonitorUpdater(
        HostSchemes*         hostSchemes,
        Monitors*            monitors,
        ServerAdministrator* serverAdministrator
    ):currentHostSchemes(
        hostSchemes
    ),currentMonitors(
        monitors
    ),currentServerAdministrator(
        serverAdministrator
    ) {
    updateTimer = new QTimer(this);
    updateTimer->setSingleShot(true);

    connect(
        this,
        &MonitorUpdater::schedulePollingServerUpdate,
        this,
        &MonitorUpdater::performSchedulePollingServerUpdate
    );

    connect(updateTimer, &QTimer::timeout, this, &MonitorUpdater::performUpdates);
}


MonitorUpdater::~MonitorUpdater() {}


bool MonitorUpdater::modifyHostScheme(const HostScheme& hostScheme, unsigned threadId) {
    bool success = false;
    if (hostScheme.isValid()) {
        success = currentHostSchemes->modifyHostScheme(hostScheme, threadId);
        if (success) {
            HostScheme::CustomerId customerId = hostScheme.customerId();
            emit schedulePollingServerUpdate(customerId, false);
        }
    }

    return success;
}


bool MonitorUpdater::deleteMonitor(const Monitor& monitor, unsigned int threadId) {
    bool success = false;
    if (monitor.isValid()) {
        HostSchemeId hostSchemeId = monitor.hostSchemeId();
        Monitors::MonitorList monitorsUnderHostScheme = currentMonitors->getMonitorsUnderHostScheme(
            hostSchemeId,
            threadId
        );

        if (monitorsUnderHostScheme.size() < 2) {
            HostScheme hostScheme = currentHostSchemes->getHostScheme(hostSchemeId, threadId);
            success = currentHostSchemes->deleteHostScheme(hostScheme, threadId);
        } else {
            success = currentMonitors->deleteMonitor(monitor, threadId);
        }

        if (success) {
            Monitor::CustomerId customerId = monitor.customerId();
            emit schedulePollingServerUpdate(customerId, false);
        }
    }

    return success;
}


bool MonitorUpdater::deleteHostScheme(const HostScheme& hostScheme, unsigned int threadId) {
    bool success = false;
    if (hostScheme.isValid()) {
        success = currentHostSchemes->deleteHostScheme(hostScheme, threadId);
        if (success) {
            HostScheme::CustomerId customerId = hostScheme.customerId();
            emit schedulePollingServerUpdate(customerId, false);
        }
    }

    return success;
}


bool MonitorUpdater::deleteCustomer(Monitor::CustomerId customerId, unsigned int threadId) {
    bool success = currentHostSchemes->deleteCustomerHostSchemes(customerId, threadId);
    if (success) {
        emit schedulePollingServerUpdate(customerId, true);
    }

    return success;
}


MonitorUpdater::Errors MonitorUpdater::update(
        const CustomerCapabilities&     customerCapabilities,
        MonitorUpdater::MonitorEntries& newMonitorEntries,
        unsigned                        threadId
    ) {
    Errors errors;

    CustomerId customerId = customerCapabilities.customerId();
    if (!newMonitorEntries.isEmpty()) {
        SortedEntries sortedEntries = sortAndCheckEntries(errors, customerCapabilities, newMonitorEntries);
        if (errors.isEmpty()) {
            HostSchemes::HostSchemeHash existingHostSchemes = currentHostSchemes->getHostSchemes(
                customerId,
                threadId
            );
            Monitors::MonitorsBySchemeHostPath existingMonitors = currentMonitors->getMonitorsBySchemeHostPath(
                customerId,
                threadId
            );

            HostSchemesBySchemeHost hashedExistingHostSchemes = hashHostSchemesBySchemeHost(existingHostSchemes);

            QSet<const HostScheme*> usedHostSchemes;
            QList<HostScheme*>      newHostSchemes;
            QList<Monitor*>         newMonitors;

            const HostScheme* previousHostScheme = nullptr;
            for (  SortedEntries::const_iterator entryIterator    = sortedEntries.constBegin(),
                                                 entryEndIterator = sortedEntries.constEnd()
                 ; entryIterator != entryEndIterator
                 ; ++entryIterator
                ) {
                const Entry* entry = entryIterator.value();
                QUrl         uri   = entry->uri();
                QString      slug  = urlToSlug(uri);

                const HostScheme* hostScheme;
                if (uri.isRelative()) { // This check is enough since we've already validated entries above.
                    hostScheme = previousHostScheme;
                } else {
                    SchemeHost schemeHost(uri);
                    hostScheme = hashedExistingHostSchemes.value(schemeHost, nullptr);
                }

                if (hostScheme == nullptr) {
                    HostScheme* newHostScheme = new HostScheme(
                        currentHostSchemes->createHostScheme(customerId, uri, threadId)
                    );

                    hostScheme = newHostScheme;
                    newHostSchemes.append(newHostScheme);

                    if (newHostScheme->isValid()) {
                        Monitor* monitor = createMonitor(
                            customerId,
                            hostScheme->hostSchemeId(),
                            slug,
                            *entry,
                            threadId
                        );

                        if (!monitor->isValid()) {
                            errors.append(Error(entry->userOrdering(), QString("failed to create monitor entry")));
                        }

                        newMonitors.append(monitor);
                    } else {
                        errors.append(Error(entry->userOrdering(), QString("failed to create monitor entry")));
                    }
                } else {
                    SchemeHostPath shp(hostScheme->hostSchemeId(), slug);
                    Monitors::MonitorsBySchemeHostPath::iterator existingIterator = existingMonitors.find(shp);
                    if (existingIterator != existingMonitors.end()) {
                        Monitor* monitor = &(existingIterator.value());

                        bool updateNeeded = updateMonitor(monitor, *entry);
                        if (updateNeeded) {
                            bool success = currentMonitors->modifyMonitor(*monitor, threadId);
                            if (!success) {
                                errors.append(
                                    Error(entry->userOrdering(), QString("failed to update monitor settings"))
                                );
                            }
                        }

                        existingMonitors.erase(existingIterator);
                    } else {
                        Monitor* monitor = createMonitor(
                            customerId,
                            hostScheme->hostSchemeId(),
                            slug,
                            *entry,
                            threadId
                        );
                        newMonitors.append(monitor);

                        if (!monitor->isValid()) {
                            errors.append(Error(entry->userOrdering(), QString("failed to create monitor entry")));
                        }
                    }
                }

                usedHostSchemes.insert(hostScheme);
                previousHostScheme = hostScheme;
            }

            deleteUnusedMonitors(errors, existingMonitors, threadId);
            deleteUnusedHostSchemes(errors, existingHostSchemes, usedHostSchemes, threadId);
            deleteContents(newMonitors);
            deleteContents(newHostSchemes);
        }
    } else {
        bool success = deleteCustomer(customerId, threadId);
        if (!success) {
            errors.append(Error(0, QString("could not delete existing monitors")));
        }
    }

    if (errors.isEmpty()) {
        bool deactivate = !customerCapabilities.customerActive();
        emit schedulePollingServerUpdate(customerId, deactivate);
    }

    return errors;
}


void MonitorUpdater::performSchedulePollingServerUpdate(unsigned long customerId, bool deactivate) {
    // This runs in the same thread as our timer so life should be simple.

    unsigned long long currentTimestamp    = QDateTime::currentSecsSinceEpoch();
    unsigned long long updateTimestamp     = currentTimestamp + updateDelaySeconds;
    unsigned long long lastUpdateTimestamp = updateTimestampByCustomerId.value(customerId, 0);

    unsigned long long firstTimestamp =   pendingUpdatesByTimestamp.isEmpty()
                                        ? 1
                                        : pendingUpdatesByTimestamp.firstKey();

    if (lastUpdateTimestamp != 0) {
        PendingUpdatesByTimestamp::iterator pit = pendingUpdatesByTimestamp.find(lastUpdateTimestamp);
        Q_ASSERT(pit != pendingUpdatesByTimestamp.end());

        PendingUpdates& customersAtTime = pit.value();
        if (customersAtTime.size() > 1) {
            customersAtTime.remove(customerId);
        } else {
            pendingUpdatesByTimestamp.erase(pit);
        }
    }

    pendingUpdatesByTimestamp[updateTimestamp].insert(customerId, deactivate);
    updateTimestampByCustomerId.insert(customerId, updateTimestamp);

    unsigned long long newFirstTimestamp = pendingUpdatesByTimestamp.firstKey();
    if (!updateTimer->isActive() || firstTimestamp != newFirstTimestamp) {
        unsigned newDelay = newFirstTimestamp > currentTimestamp ? newFirstTimestamp - currentTimestamp : 0;
        updateTimer->start(1000UL * newDelay);
    }
}


void MonitorUpdater::performUpdates() {
    if (!pendingUpdatesByTimestamp.isEmpty()) {
        unsigned long long                  currentTimestamp;
        PendingUpdatesByTimestamp::iterator pit = pendingUpdatesByTimestamp.begin();
        do {
            const PendingUpdates pendingUpdates = pit.value();
            for (  PendingUpdates::const_iterator cit = pendingUpdates.constBegin(), end = pendingUpdates.constEnd()
                 ; cit != end
                 ; ++cit
                ) {
                Monitor::CustomerId customerId = cit.key();
                bool                deactivate = cit.value();

                if (deactivate) {
                    currentServerAdministrator->deactivateCustomer(customerId, timerThreadId);
                } else {
                    currentServerAdministrator->activateCustomer(customerId, timerThreadId);
                }

                updateTimestampByCustomerId.remove(customerId);
            }

            pit = pendingUpdatesByTimestamp.erase(pit);
            currentTimestamp = QDateTime::currentSecsSinceEpoch();
        } while (pit != pendingUpdatesByTimestamp.end() && pit.key() <= currentTimestamp);

        if (!pendingUpdatesByTimestamp.isEmpty()) {
            unsigned long long newFirstTimestamp = pendingUpdatesByTimestamp.firstKey();
            unsigned           newDelay          =   newFirstTimestamp > currentTimestamp
                                                   ? newFirstTimestamp - currentTimestamp
                                                   : 0;

            updateTimer->start(1000UL * newDelay);
        }
    }
}


MonitorUpdater::SortedEntries MonitorUpdater::sortAndCheckEntries(
        MonitorUpdater::Errors&         errors,
        const CustomerCapabilities&     customerCapabilities,
        MonitorUpdater::MonitorEntries& newMonitorEntries
    ) {
    SortedEntries  sortedEntries;

    unsigned       numberNewEntries    = static_cast<unsigned>(newMonitorEntries.size());
    UserOrdering   lowestUserOrdering  = std::numeric_limits<UserOrdering>::max();
    UserOrdering   highestUserOrdering = std::numeric_limits<UserOrdering>::lowest();

    for (unsigned entryIndex=0 ; entryIndex<numberNewEntries ; ++entryIndex) {
        Entry&       entry      = newMonitorEntries[entryIndex];
        UserOrdering entryOrder = entry.userOrdering();

        if (!sortedEntries.contains(entryOrder)) {
            QUrl uri = entry.uri();
            if (uri.isRelative() == !uri.host().isEmpty()) {
                errors.append(Error(entryOrder, QString("must include both host and scheme or just path")));
            } else if (!uri.userInfo().isEmpty()) {
                errors.append(Error(entryOrder, QString("user authentication is not supported")));
            } else if (uri.hasFragment()) {
                errors.append(Error(entryOrder, QString("fragments are not supported")));
            } else {
                Method method = entry.method();
                if (method == Method::POST && !customerCapabilities.supportsPostMethod()) {
                    errors.append(Error(entryOrder, QString("POST method not supported")));
                } else {
                    ContentCheckMode contentCheckMode = entry.contentCheckMode();
                    if ((contentCheckMode == ContentCheckMode::CONTENT_MATCH       ||
                         contentCheckMode == ContentCheckMode::SMART_CONTENT_MATCH    ) &&
                        !customerCapabilities.supportsContentChecking()                    ) {
                        errors.append(Error(entryOrder, QString("Content match checking not supported")));
                    } else if ((contentCheckMode == ContentCheckMode::ANY_KEYWORDS ||
                                contentCheckMode == ContentCheckMode::ALL_KEYWORDS    ) &&
                               !customerCapabilities.supportsKeywordChecking()             ) {
                        errors.append(Error(entryOrder, QString("Keyword checking not supported")));
                    } else {
                        sortedEntries.insert(entryOrder, &entry);
                        highestUserOrdering = std::max(highestUserOrdering, entryOrder);
                    }
                }
            }
        } else {
            errors.append(Error(entryOrder, QString("duplicate user ordering value")));
        }
    }

    const Entry* firstEntry = sortedEntries.first();
    QUrl         firstHostScheme(firstEntry->uri());
    if (firstHostScheme.isRelative() || firstHostScheme.host().isEmpty()) {
        errors.append(Error(lowestUserOrdering, QString("first entry must include scheme and host")));
    }

    SortedEntries result;
    UserOrdering  newUserOrdering = 0;
    for (UserOrdering oldUserOrdering=0 ; oldUserOrdering<=highestUserOrdering ; ++oldUserOrdering) {
        SortedEntries::iterator it = sortedEntries.find(oldUserOrdering);
        if (it != sortedEntries.end()) {
            Entry* entry = it.value();
            entry->setUserOrdering(newUserOrdering);
            ++newUserOrdering;

            result.insert(newUserOrdering, entry);
        }
    }

    return result;
}


MonitorUpdater::HostSchemesBySchemeHost MonitorUpdater::hashHostSchemesBySchemeHost(
        const HostSchemes::HostSchemeHash& hostSchemes
    ) {
    HostSchemesBySchemeHost result;

    for (  HostSchemes::HostSchemeHash::const_iterator it = hostSchemes.constBegin(), end = hostSchemes.constEnd()
         ; it!=end
         ; ++it
        ) {
        const HostScheme* hostScheme = &it.value();
        result.insert(SchemeHost(hostScheme->url()), hostScheme);
    }

    return result;
}


QString MonitorUpdater::urlToSlug(const QUrl& url) {
    QString result = url.path();
    if (url.hasQuery()) {
        if (!result.endsWith('/')) {
            result += QString("/?%1").arg(url.query());
        } else {
            result += QString("?%1").arg(url.query());
        }
    }

    return result;
}


Monitor* MonitorUpdater::createMonitor(
        MonitorUpdater::CustomerId   customerId,
        MonitorUpdater::HostSchemeId hostSchemeId,
        const QString&               slug,
        const MonitorUpdater::Entry& entry,
        unsigned int                 threadId
    ) {
    return new Monitor(
        currentMonitors->createMonitor(
            customerId,
            hostSchemeId,
            entry.userOrdering(),
            slug,
            entry.method(),
            entry.contentCheckMode(),
            entry.keywords(),
            entry.contentType(),
            entry.userAgent(),
            entry.postContent(),
            threadId
        )
    );
}


bool MonitorUpdater::updateMonitor(Monitor* monitor, const Entry& monitorEntry) {
    bool monitorChanged = false;

    if (monitorEntry.userOrdering() != monitor->userOrdering()) {
        monitor->setUserOrdering(monitorEntry.userOrdering());
        monitorChanged = true;
    }

    QString slug = urlToSlug(monitorEntry.uri());
    if (slug != monitor->path()) {
        monitor->setPath(slug);
        monitorChanged = true;
    }

    if (monitorEntry.method() != monitor->method()) {
        monitor->setMethod(monitorEntry.method());
        monitorChanged = true;
    }

    if (monitorEntry.contentCheckMode() != monitor->contentCheckMode()) {
        monitor->setContentCheckMode(monitorEntry.contentCheckMode());
        monitorChanged = true;
    }

    if (monitorEntry.keywords() != monitor->keywords()) {
        monitor->setKeywords(monitorEntry.keywords());
        monitorChanged = true;
    }

    if (monitorEntry.contentType() != monitor->contentType()) {
        monitor->setContentType(monitorEntry.contentType());
        monitorChanged = true;
    }

    if (monitorEntry.userAgent() != monitor->userAgent()) {
        monitor->setUserAgent(monitorEntry.userAgent());
        monitorChanged = true;
    }

    if (monitorEntry.postContent() != monitor->postContent()) {
        monitor->setPostContent(monitorEntry.postContent());
        monitorChanged = true;
    }

    return monitorChanged;
}


void MonitorUpdater::deleteUnusedMonitors(
        MonitorUpdater::Errors&                   errors,
        const Monitors::MonitorsBySchemeHostPath& existingMonitors,
        unsigned                                  threadId
    ) {
    for (  Monitors::MonitorsBySchemeHostPath::const_iterator
               monitorIterator    = existingMonitors.constBegin(),
               monitorEndIterator = existingMonitors.constEnd()
         ; monitorIterator != monitorEndIterator
         ; ++monitorIterator
        ) {
        const Monitor* monitor = &monitorIterator.value();
        bool success = currentMonitors->deleteMonitor(*monitor, threadId);
        if (!success) {
            errors.append(Error(monitor->userOrdering(), QString("failed to delete unused monitor.")));
        }
    }
}


void MonitorUpdater::deleteUnusedHostSchemes(
        MonitorUpdater::Errors&            errors,
        const HostSchemes::HostSchemeHash& existingHostSchemes,
        const QSet<const HostScheme*>&     usedHostSchemes,
        unsigned                           threadId
    ) {
    for (  HostSchemes::HostSchemeHash::const_iterator
               hostSchemeIterator    = existingHostSchemes.constBegin(),
               hostSchemeEndIterator = existingHostSchemes.constEnd()
         ; hostSchemeIterator != hostSchemeEndIterator
         ; ++hostSchemeIterator
        ) {
        const HostScheme* hostScheme = &hostSchemeIterator.value();
        if (!usedHostSchemes.contains(hostScheme)) {
            bool success = currentHostSchemes->deleteHostScheme(*hostScheme, threadId);
            if (!success) {
                errors.append(
                    Error(
                        0xFFFF,
                        QString("failed to delete host/scheme %1").arg(hostScheme->url().toString())
                    )
                );
            }
        }
    }
}
