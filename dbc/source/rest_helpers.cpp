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
* This header implements the \ref RestHelpers class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <cmath>

#include "monitor.h"
#include "monitors.h"
#include "server.h"
#include "servers.h"
#include "event.h"
#include "events.h"
#include "latency_entry.h"
#include "aggregated_latency_entry.h"
#include "latency_interface_manager.h"
#include "rest_helpers.h"

QJsonObject RestHelpers::convertToJson(const HostScheme& hostScheme, bool includeCustomerId) {
    QJsonObject result;

    result.insert("host_scheme_id", static_cast<double>(hostScheme.hostSchemeId()));

    if (includeCustomerId) {
        result.insert("customer_id", static_cast<double>(hostScheme.customerId()));
    }

    result.insert("url", hostScheme.url().toString());
    result.insert(
        "ssl_expiration_timestamp",
        static_cast<double>(hostScheme.sslExpirationTimestamp())
    );

    return result;
}


QJsonObject RestHelpers::convertToJson(const HostSchemes::HostSchemeHash& hostSchemes, bool includeCustomerId) {
    QJsonObject result;

    for (  HostSchemes::HostSchemeHash::const_iterator it  = hostSchemes.constBegin(),
                                                       end = hostSchemes.constEnd()
         ; it != end
         ; ++it
        ) {
        HostScheme::HostSchemeId hostSchemeId = it.key();
        const HostScheme&        hostScheme   = it.value();

        result.insert(QString::number(hostSchemeId), convertToJson(hostScheme, includeCustomerId));
    }

    return result;
}


QJsonObject RestHelpers::convertToJson(const Monitor& monitor, bool includeCustomerId, bool includeUserOrdering) {
    QJsonObject result;

    result.insert("monitor_id", static_cast<double>(monitor.monitorId()));

    if (includeCustomerId) {
        result.insert("customer_id", static_cast<double>(monitor.customerId()));
    }

    result.insert("host_scheme_id", static_cast<double>(monitor.hostSchemeId()));

    if (includeUserOrdering) {
        result.insert("user_ordering", static_cast<int>(monitor.userOrdering()));
    }

    result.insert("path", monitor.path());
    result.insert("method", Monitor::toString(monitor.method()).toLower());
    result.insert("content_check_mode", Monitor::toString(monitor.contentCheckMode()).toLower());

    QJsonArray keywordsArray;
    const Monitor::KeywordList& keywordList = monitor.keywords();
    for (  Monitor::KeywordList::const_iterator keywordIterator    = keywordList.constBegin(),
                                                keywordEndIterator = keywordList.constEnd()
         ; keywordIterator != keywordEndIterator
         ; ++keywordIterator
        ) {
        keywordsArray.append(QString::fromUtf8(keywordIterator->toBase64()));
    }

    result.insert("keywords", keywordsArray);
    result.insert("post_content_type", Monitor::toString(monitor.contentType()).toLower());
    result.insert("post_user_agent", monitor.userAgent());
    result.insert("post_content", QString::fromUtf8(monitor.postContent().toBase64()));

    return result;
}


QJsonObject RestHelpers::convertToJson(
        const Monitor& monitor,
        const QUrl&    url,
        bool           includeCustomerId,
        bool           includeUserOrdering
    ) {
    QJsonObject result;

    result.insert("monitor_id", static_cast<double>(monitor.monitorId()));

    if (includeCustomerId) {
        result.insert("customer_id", static_cast<double>(monitor.customerId()));
    }

    result.insert("host_scheme_id", static_cast<double>(monitor.hostSchemeId()));

    if (includeUserOrdering) {
        result.insert("user_ordering", static_cast<int>(monitor.userOrdering()));
    }

    result.insert("path", monitor.path());
    result.insert("url", url.toString());
    result.insert("method", Monitor::toString(monitor.method()).toLower());
    result.insert("content_check_mode", Monitor::toString(monitor.contentCheckMode()).toLower());

    QJsonArray keywordsArray;
    const Monitor::KeywordList& keywordList = monitor.keywords();
    for (  Monitor::KeywordList::const_iterator keywordIterator    = keywordList.constBegin(),
                                                keywordEndIterator = keywordList.constEnd()
         ; keywordIterator != keywordEndIterator
         ; ++keywordIterator
        ) {
        keywordsArray.append(QString::fromUtf8(keywordIterator->toBase64()));
    }

    result.insert("keywords", keywordsArray);
    result.insert("post_content_type", Monitor::toString(monitor.contentType()).toLower());
    result.insert("post_user_agent", monitor.userAgent());
    result.insert("post_content", QString::fromUtf8(monitor.postContent().toBase64()));

    return result;
}


QJsonObject RestHelpers::convertToJson(const Monitors::MonitorList& monitorList, bool includeCustomerId) {
    QJsonObject monitorArray;
    for (  Monitors::MonitorList::const_iterator monitorIterator    = monitorList.constBegin(),
                                                 monitorEndIterator = monitorList.constEnd()
         ; monitorIterator != monitorEndIterator
         ; ++monitorIterator
        ) {
        monitorArray.insert(
            QString::number(monitorIterator->userOrdering()),
            RestHelpers::convertToJson(*monitorIterator, includeCustomerId, true)
        );
    }

    return monitorArray;
}


MonitorUpdater::MonitorEntries RestHelpers::convertToMonitorEntries(
        MonitorUpdater::Errors& errors,
        const QJsonArray&       monitorData
    ) {
    MonitorUpdater::MonitorEntries result;

    unsigned numberEntries = static_cast<unsigned>(monitorData.size());
    for (MonitorUpdater::UserOrdering userOrdering=0 ; userOrdering<numberEntries ; ++userOrdering) {
        const QJsonValue& value = monitorData.at(userOrdering);
        if (value.isObject()) {
            bool                  okToUseEntry;
            MonitorUpdater::Entry entry = convertToMonitorEntry(
                errors,
                value.toObject(),
                userOrdering,
                &okToUseEntry
            );

            if (okToUseEntry) {
                result.append(entry);
            }
        } else {
            errors.append(MonitorUpdater::Error(userOrdering, QString("expected dictionary")));
        }
    }

    return result;
}


MonitorUpdater::MonitorEntries RestHelpers::convertToMonitorEntries(
        MonitorUpdater::Errors& errors,
        const QJsonObject&      monitorData
    ) {
    MonitorUpdater::MonitorEntries result;

    QSet<MonitorUpdater::UserOrdering> foundEntries;
    for (  QJsonObject::const_iterator entryIterator    = monitorData.constBegin(),
                                       entryEndIterator = monitorData.constEnd()
         ; entryIterator != entryEndIterator
         ; ++entryIterator
        ) {
        bool                         success;
        MonitorUpdater::UserOrdering userOrdering = entryIterator.key().toUShort(&success);

        if (success) {
            if (!foundEntries.contains(userOrdering)) {
                foundEntries.insert(userOrdering);
                QJsonValue value = entryIterator.value();
                if (value.isObject()) {
                    bool                  okToUseEntry;
                    MonitorUpdater::Entry entry = convertToMonitorEntry(
                        errors,
                        value.toObject(),
                        userOrdering,
                        &okToUseEntry
                    );

                    if (okToUseEntry) {
                        result.append(entry);
                    }
                } else {
                    errors.append(MonitorUpdater::Error(userOrdering, QString("expected dictionary")));
                }
            } else {
                errors.append(MonitorUpdater::Error(userOrdering, QString("duplicate entry")));
            }
        } else {
            errors.append(MonitorUpdater::Error(0xFFFF, QString("invalid key \"%1\"").arg(entryIterator.key())));
        }
    }

    return result;
}


MonitorUpdater::Entry RestHelpers::convertToMonitorEntry(
        MonitorUpdater::Errors&      errors,
        const QJsonObject&           monitorData,
        MonitorUpdater::UserOrdering userOrdering,
        bool*                        ok
    ) {
    unsigned                         numberFields     = 0;
    bool                             okToUseEntry     = true;
    MonitorUpdater::Method           method           = MonitorUpdater::Method::GET;
    MonitorUpdater::ContentCheckMode contentCheckMode = MonitorUpdater::ContentCheckMode::NO_CHECK;
    MonitorUpdater::ContentType      contentType      = MonitorUpdater::ContentType::TEXT;
    QString                          uriString;
    MonitorUpdater::KeywordList      keywords;
    QString                          userAgent;
    QByteArray                       postContent;

    if (monitorData.contains("uri")) {
        QJsonValue uriValue = monitorData.value("uri");
        if (uriValue.isString()) {
            uriString = uriValue.toString();
        } else {
            errors.append(MonitorUpdater::Error(userOrdering, QString("uri must be a string")));
            okToUseEntry = false;
        }

        ++numberFields;
    } else {
        errors.append(MonitorUpdater::Error(userOrdering, QString("missing required field \"uri\"")));
        okToUseEntry = false;
    }

    if (monitorData.contains("method")) {
        QJsonValue methodValue = monitorData.value("method");
        if (methodValue.isString()) {
            bool isOK;
            method = Monitor::toMethod(methodValue.toString(), &isOK);
            if (!isOK) {
                okToUseEntry = false;
                errors.append(MonitorUpdater::Error(userOrdering, QString("invalid method, use \"get\" or \"post\"")));
            }
        } else {
            okToUseEntry = false;
            errors.append(MonitorUpdater::Error(userOrdering, QString("method fields must be a string")));
        }

        ++numberFields;
    }

    if (monitorData.contains("content_check_mode")) {
        QJsonValue contentCheckModeValue = monitorData.value("content_check_mode");
        if (contentCheckModeValue.isString()) {
            bool isOK;
            contentCheckMode = Monitor::toContentCheckMode(contentCheckModeValue.toString(), &isOK);
            if (!isOK) {
                okToUseEntry = false;
                errors.append(
                    MonitorUpdater::Error(
                        userOrdering,
                        QString(
                            "invalid content_check_mode, use \"no_check\", \"content_match\", \"all_keywords\", "
                            "\"any_keywords\", or \"smart_content_match\""
                        )
                    )
                );
            }
        } else {
            okToUseEntry = false;
            errors.append(MonitorUpdater::Error(userOrdering, QString("content_check_mode fields must be a string")));
        }

        ++numberFields;
    }

    if (monitorData.contains("post_content_type")) {
        QJsonValue contentTypeValue = monitorData.value("post_content_type");
        if (contentTypeValue.isString()) {
            bool isOK;
            contentType = Monitor::toContentType(contentTypeValue.toString(), &isOK);
            if (!isOK) {
                okToUseEntry = false;
                errors.append(
                    MonitorUpdater::Error(
                        userOrdering,
                        QString("invalid post_content_type, use \"text\", \"json\", or \"xml\"")
                    )
                );
            }
        } else {
            okToUseEntry = false;
            errors.append(
                MonitorUpdater::Error(userOrdering, QString("post_content_type fields must be a string"))
            );
        }

        ++numberFields;
    }

    if (monitorData.contains("keywords")) {
        QJsonValue keywordsValue = monitorData.value("keywords");
        bool       isOK          = keywordsValue.isArray();
        if (isOK) {
            QJsonArray keywordsArray = keywordsValue.toArray();
            keywords.reserve(keywordsArray.size());
            QJsonArray::const_iterator keywordsIterator    = keywordsArray.constBegin();
            QJsonArray::const_iterator keywordsEndIterator = keywordsArray.constEnd();

            while (isOK && keywordsIterator != keywordsEndIterator) {
                QByteArray::FromBase64Result base64Result = QByteArray::fromBase64Encoding(
                    keywordsIterator->toString().toUtf8(),
                    QByteArray::Base64Option::AbortOnBase64DecodingErrors
                );

                if (base64Result) {
                    keywords.append(*base64Result);
                    ++keywordsIterator;
                } else {
                    okToUseEntry = false;
                    isOK         = false;
                    errors.append(
                        MonitorUpdater::Error(
                            userOrdering,
                            QString("keyword entries should be base64 encoded as per RFC4648.")
                        )
                    );
                }
            }
        } else {
            okToUseEntry = false;
            errors.append(
                MonitorUpdater::Error(
                    userOrdering,
                    QString("keywords must be an array of RFC4648 base64 encoded values.")
                )
            );
        }

        ++numberFields;
    }

    if (monitorData.contains("post_user_agent")) {
        QJsonValue userAgentValue = monitorData.value("post_user_agent");
        if (userAgentValue.isString()) {
            userAgent = userAgentValue.toString();
        } else {
            okToUseEntry = false;
            errors.append(MonitorUpdater::Error(userOrdering, QString("post_user_agent must be a string.")));
        }

        ++numberFields;
    }

    if (monitorData.contains("post_content")) {
        QJsonValue postContentValue = monitorData.value("post_content");
        if (postContentValue.isString()) {
            QByteArray::FromBase64Result base64Result = QByteArray::fromBase64Encoding(
                postContentValue.toString().toUtf8(),
                QByteArray::Base64Option::AbortOnBase64DecodingErrors
            );

            if (base64Result) {
                postContent = *base64Result;
            } else {
                okToUseEntry = false;
                errors.append(
                    MonitorUpdater::Error(
                        userOrdering,
                        QString("post_user_agent should be base64 encoded as per RFC4648.")
                    )
                );
            }

        } else {
            okToUseEntry = false;
            errors.append(
                MonitorUpdater::Error(
                    userOrdering,
                    QString("post_user_agent must be a string holding RFC4648 base64 encoded values.")
                )
            );
        }

        ++numberFields;
    }

    if (numberFields != static_cast<unsigned>(monitorData.size())) {
        okToUseEntry = false;
        errors.append(MonitorUpdater::Error(userOrdering, QString("unexpected entries")));
    }

    if (ok != nullptr) {
        *ok = okToUseEntry;
    }

    return MonitorUpdater::Entry(
        userOrdering,
        QUrl(uriString),
        method,
        contentCheckMode,
        keywords,
        contentType,
        userAgent,
        postContent
    );
}


QJsonObject RestHelpers::convertToJson(const MonitorUpdater::Errors& errors) {
    QJsonObject result;
    QJsonArray  errorData;

    if (errors.isEmpty()) {
        result.insert("status", "OK");
    } else {
        result.insert("status", "failed, see errors");

        for (MonitorUpdater::Errors::const_iterator it=errors.constBegin(),end=errors.constEnd() ; it!=end ; ++it) {
            const MonitorUpdater::Error& error = *it;

            QJsonObject errorEntry;
            if (error.userOrdering() != 0xFFFF) {
                errorEntry.insert("user_ordering", static_cast<int>(error.userOrdering()));
            }

            errorEntry.insert("text", error.errorMessage());
            errorData.append(errorEntry);
        }
    }

    result.insert("errors", errorData);

    return result;
}


QJsonObject RestHelpers::convertToJson(
        const CustomerCapabilities& customerCapabilities,
        bool                        includeCustomerId,
        bool                        includeBlacklistField,
        bool                        includeDomainExpirationField
    ) {
    QJsonObject result;

    if (includeCustomerId) {
        result.insert("customer_id", static_cast<double>(customerCapabilities.customerId()));
    }

    result.insert("maximum_number_monitors", customerCapabilities.maximumNumberMonitors());
    result.insert("polling_interval", customerCapabilities.pollingInterval());
//    result.insert("expiration_days", customerCapabilities.expirationDays());
    result.insert("customer_active", customerCapabilities.customerActive());
    result.insert("multi_region_checking", customerCapabilities.multiRegionChecking());
    result.insert("supports_wordpress", customerCapabilities.supportsWordPress());
    result.insert("supports_rest_api", customerCapabilities.supportsRestApi());
    result.insert("supports_content_checking", customerCapabilities.supportsContentChecking());
    result.insert("supports_keyword_checking", customerCapabilities.supportsKeywordChecking());
    result.insert("supports_post_method", customerCapabilities.supportsPostMethod());
    result.insert("supports_latency_tracking", customerCapabilities.supportsLatencyTracking());
    result.insert("supports_ssl_expiration_checking", customerCapabilities.supportsSslExpirationChecking());
    result.insert("supports_ping_based_polling", customerCapabilities.supportsPingBasedPolling());

    if (includeBlacklistField) {
        result.insert("supports_blacklist_checking", customerCapabilities.supportsBlackListChecking());
    }

    if (includeDomainExpirationField) {
        result.insert("supports_domain_expiration_checking", customerCapabilities.supportsDomainExpirationChecking());
    }

    result.insert("supports_maintenance_mode", customerCapabilities.supportsMaintenanceMode());
    result.insert("supports_rollups", customerCapabilities.supportsRollups());
    result.insert("paused", customerCapabilities.paused());

    return result;
}


QJsonObject RestHelpers::convertToJson(const Event& event, bool includeCustomerId, bool includeHash) {
    QJsonObject result;

    if (includeCustomerId) {
        result.insert("customer_id", static_cast<double>(event.customerId()));
    }

    result.insert("event_id", static_cast<double>(event.eventId()));
    result.insert("monitor_id", static_cast<double>(event.monitorId()));
    result.insert("timestamp", static_cast<double>(event.unixTimestamp()));
    result.insert("event_type", Event::toString(event.eventType()).toLower());
    result.insert("message", event.message());

    if (includeHash) {
        result.insert("hash", QString::fromLatin1(event.hash().toBase64()));
    }

    return result;

}


QJsonArray RestHelpers::convertToJson(const Events::EventList& events, bool includeCustomerId, bool includeHash) {
    QJsonArray result;

    for (Events::EventList::const_iterator it=events.constBegin(),end=events.constEnd() ; it!=end ; ++it) {
        result.append(convertToJson(*it, includeCustomerId, includeHash));
    }

    return result;
}


QJsonObject RestHelpers::convertToJson(
        const LatencyEntry&           entry,
        const Servers::ServersById&   serversById,
        const Monitors::MonitorsById& monitorsById,
        bool                          includeServerId,
        bool                          includeRegionId,
        bool                          includeCustomerId
    ) {
    QJsonObject result;

    result.insert("monitor_id", static_cast<double>(entry.monitorId()));
    result.insert("timestamp", static_cast<double>(entry.unixTimestamp()));
    result.insert("latency", entry.latencyMicroseconds() * 1.0E-6);

    if (includeServerId) {
        result.insert("server_id", entry.serverId());
    }

    if (includeRegionId) {
        if (serversById.contains(entry.serverId())) {
            const Server& server = serversById.value(entry.serverId());
            result.insert("region_id", server.regionId());
        } else {
            result.insert("region_id", Region::invalidRegionId);
        }
    }

    if (includeCustomerId) {
        if (monitorsById.contains(entry.monitorId())) {
            const Monitor& monitor = monitorsById.value(entry.monitorId());
            result.insert("customer_id", static_cast<double>(monitor.customerId()));
        } else {
            result.insert("customer_id", static_cast<double>(CustomerCapabilities::invalidCustomerId));
        }
    }

    return result;
}


QJsonObject RestHelpers::convertToJson(
        const AggregatedLatencyEntry& entry,
        const Servers::ServersById&   serversById,
        const Monitors::MonitorsById& monitorsById,
        bool                          includeServerId,
        bool                          includeRegionId,
        bool                          includeCustomerId
    ) {
    QJsonObject result = convertToJson(
        static_cast<const LatencyEntry&>(entry),
        serversById,
        monitorsById,
        includeServerId,
        includeRegionId,
        includeCustomerId
    );

    result.insert("average", entry.meanLatency() * 1.0E-6);
    result.insert("variance", entry.varianceLatency() * 1.0E-12);
    result.insert("minimum", entry.minimumLatency() * 1.0E-6);
    result.insert("maximum", entry.maximumLatency() * 1.0E-6);
    result.insert("number_samples", static_cast<double>(entry.numberSamples()));
    result.insert("start_timestamp", static_cast<double>(entry.startTimestamp()));
    result.insert("end_timestamp", static_cast<double>(entry.endTimestamp()));

    return result;
}


QJsonArray RestHelpers::convertToJson(
        const LatencyInterfaceManager::LatencyEntryList& entries,
        const Servers::ServersById&                      serversById,
        const Monitors::MonitorsById&                    monitorsById,
        bool                                             includeServerId,
        bool                                             includeRegionId,
        bool                                             includeCustomerId
    ) {
    QJsonArray result;

    for (  LatencyInterfaceManager::LatencyEntryList::const_iterator it  = entries.constBegin(),
                                                                     end = entries.constEnd()
         ; it != end
         ; ++it
        ) {
        const LatencyEntry& entry = *it;
        result.append(
            convertToJson(
                entry,
                serversById,
                monitorsById,
                includeServerId,
                includeRegionId,
                includeCustomerId
            )
        );
    }

    return result;
}


QJsonArray RestHelpers::convertToJson(
        const LatencyInterfaceManager::AggregatedLatencyEntryList& entries,
        const Servers::ServersById&                                serversById,
        const Monitors::MonitorsById&                              monitorsById,
        bool                                                       includeServerId,
        bool                                                       includeRegionId,
        bool                                                       includeCustomerId
    ) {
    QJsonArray result;

    for (  LatencyInterfaceManager::AggregatedLatencyEntryList::const_iterator it  = entries.constBegin(),
                                                                               end = entries.constEnd()
         ; it != end
         ; ++it
        ) {
        const AggregatedLatencyEntry& entry = *it;
        result.append(
            convertToJson(
                entry,
                serversById,
                monitorsById,
                includeServerId,
                includeRegionId,
                includeCustomerId
            )
        );
    }

    return result;
}


QJsonObject RestHelpers::convertToJson(const Resource& resource, bool includeCustomerId) {
    QJsonObject result;

    if (includeCustomerId) {
        result.insert("customer_id", static_cast<double>(resource.customerId()));
    }

    result.insert("value_type", static_cast<int>(resource.valueType()));
    result.insert("value", resource.value());
    result.insert("timestamp", static_cast<double>(resource.unixTimestamp()));

    return result;
}


QJsonArray RestHelpers::convertToJson(
        const Resources::ResourceList& resources,
        bool                           fullEntry,
        bool                           includeCustomerId
    ) {
    QJsonArray result;

    if (fullEntry) {
        for (  Resources::ResourceList::const_iterator it = resources.constBegin(), end = resources.constEnd()
             ; it != end
             ; ++it
            ) {
            result.append(convertToJson(*it, includeCustomerId));
        }
    } else {
        for (  Resources::ResourceList::const_iterator it = resources.constBegin(), end = resources.constEnd()
             ; it != end
             ; ++it
            ) {
            QJsonObject entry;

            entry.insert("value", it->value());
            entry.insert("timestamp", static_cast<double>(it->unixTimestamp()));

            result.append(entry);
        }
    }

    return result;
}
