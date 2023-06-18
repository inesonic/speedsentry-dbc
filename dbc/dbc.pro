##-*-makefile-*-########################################################################################################
# Copyright 2021 - 2023 Inesonic, LLC
#
# GNU Public License, Version 3:
#   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
#   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
#   version.
#   
#   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
#   details.
#   
#   You should have received a copy of the GNU General Public License along with this program.  If not, see
#   <https://www.gnu.org/licenses/>.
########################################################################################################################

########################################################################################################################
# Basic build characteristics
#

TEMPLATE = app
QT += core gui network sql charts
CONFIG += console
CONFIG += c++14

########################################################################################################################
# Headers
#

INCLUDEPATH += include
HEADERS = include/metatypes.h \
          include/log.h \
          include/dbc.h \
          include/cache_base.h \
          include/cache.h \
          include/database_manager.h \
          include/sql_helpers.h \
          include/region.h \
          include/regions.h \
          include/server.h \
          include/servers.h \
          include/server_administrator.h \
          include/customer_secret.h \
          include/customer_secrets.h \
          include/host_scheme.h \
          include/host_schemes.h \
          include/scheme_host.h \
          include/scheme_host_path.h \
          include/monitor.h \
          include/monitors.h \
          include/monitor_updater.h \
          include/customer_capabilities.h \
          include/customers_capabilities.h \
          include/customer_mapping.h \
          include/event.h \
          include/events.h \
          include/event_processor.h \
          include/short_latency_entry.h \
          include/latency_entry.h \
          include/aggregated_latency_entry.h \
          include/latency_interface.h \
          include/latency_aggregator.h \
          include/latency_interface_manager.h \
          include/plotter_base.h \
          include/latency_plotter.h \
          include/plot_mailbox.h \
          include/rest_helpers.h \
          include/active_resources.h \
          include/resource.h \
          include/resources.h \
          include/resource_plotter.h \
          include/region_manager.h \
          include/server_manager.h \
          include/host_scheme_manager.h \
          include/monitor_manager.h \
          include/customer_capabilities_manager.h \
          include/event_manager.h \
          include/customer_mapping_manager.h \
          include/latency_manager.h \
          include/multiple_manager.h \
          include/resource_manager.h \
          include/customer_authenticator.h \
          include/customer_rest_api_v1.h \
          include/outbound_rest_api.h \
          include/outbound_rest_api_factory.h \

########################################################################################################################
# Source files
#

SOURCES = source/main.cpp \
          source/metatypes.cpp \
          source/log.cpp \
          source/dbc.cpp \
          source/cache_base.cpp \
          source/database_manager.cpp \
          source/sql_helpers.cpp \
          source/regions.cpp \
          source/server.cpp \
          source/servers.cpp \
          source/server_administrator.cpp \
          source/customer_secret.cpp \
          source/customer_secrets.cpp \
          source/host_schemes.cpp \
          source/monitor.cpp \
          source/monitors.cpp \
          source/monitor_updater.cpp \
          source/customers_capabilities.cpp \
          source/customer_mapping.cpp \
          source/event.cpp \
          source/events.cpp \
          source/event_processor.cpp \
          source/latency_interface.cpp \
          source/latency_aggregator.cpp \
          source/latency_aggregator_private.cpp \
          source/latency_interface_manager.cpp \
          source/plotter_base.cpp \
          source/latency_plotter.cpp \
          source/plot_mailbox.cpp \
          source/rest_helpers.cpp \
          source/active_resources.cpp \
          source/resources.cpp \
          source/resource_plotter.cpp \
          source/region_manager.cpp \
          source/server_manager.cpp \
          source/host_scheme_manager.cpp \
          source/monitor_manager.cpp \
          source/customer_capabilities_manager.cpp \
          source/event_manager.cpp \
          source/customer_mapping_manager.cpp \
          source/latency_manager.cpp \
          source/multiple_manager.cpp \
          source/resource_manager.cpp \
          source/customer_authenticator.cpp \
          source/customer_rest_api_v1.cpp \
          source/outbound_rest_api.cpp \
          source/outbound_rest_api_factory.cpp \

########################################################################################################################
# Private headers
#

INCLUDEPATH += source
HEADERS += source/latency_aggregator_private.h \

########################################################################################################################
# Libraries
#

INCLUDEPATH += $${INEREST_API_IN_V1_INCLUDE}
INCLUDEPATH += $${INEREST_API_OUT_V1_INCLUDE}
INCLUDEPATH += $${INECRYPTO_INCLUDE}
INCLUDEPATH += $${INEXTEA_INCLUDE}

LIBS += -L$${INEREST_API_IN_V1_LIBDIR} -linerest_api_in_v1
LIBS += -L$${INEREST_API_OUT_V1_LIBDIR} -linerest_api_out_v1
LIBS += -L$${INECRYPTO_LIBDIR} -linecrypto
LIBS += -L$${INEXTEA_LIBDIR} -linextea

########################################################################################################################
# Locate build intermediate and output products
#

TARGET = dbc

CONFIG(debug, debug|release) {
    unix:DESTDIR = build/debug
    win32:DESTDIR = build/Debug
} else {
    unix:DESTDIR = build/release
    win32:DESTDIR = build/Release
}

OBJECTS_DIR = $${DESTDIR}/objects
MOC_DIR = $${DESTDIR}/moc
RCC_DIR = $${DESTDIR}/rcc
UI_DIR = $${DESTDIR}/ui
