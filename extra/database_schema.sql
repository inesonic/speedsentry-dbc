-- ---------------------------------------------------------------------------------------------------------------------
-- Copyright 2021 - 2023 Inesonic, LLC.
--
-- GNU Public License, Version 3:
--   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
--  License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
--  version.
--  
--  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
--  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
--  details.
--  
--  You should have received a copy of the GNU General Public License along with this program.  If not, see
--  <https://www.gnu.org/licenses/>.
-- ---------------------------------------------------------------------------------------------------------------------
-- SQL script that builds our SpeedSentry database
-- ---------------------------------------------------------------------------------------------------------------------
--
-- Notes:
--   Assumed user DbC and DbCAdmin has been created using a command such as:
--   CREATE ROLE DbC NOSUPERUSER
--                   NOCREATEDB
--                   NOCREATEROLE
--                   NOINHERIT
--                   LOGIN
--                   CONNECTION LIMIT 32
--                   ENCRYPTED PASSWORD 'password';
--
--   CREATE ROLE DbCAdmin NOSUPERUSER
--                        NOCREATEDB
--                        NOCREATEROLE
--                        NOINHERIT
--                        LOGIN
--                        CONNECTION LIMIT 1
--                        ENCRYPTED PASSWORD 'password';
--
-- Drop and create our SpeedSentry database

DROP DATABASE IF EXISTS speedsentry_backend;
CREATE DATABASE speedsentry_backend WITH ENCODING = 'UTF8';
GRANT CONNECT ON DATABASE speedsentry_backend TO DbC;
GRANT CONNECT,CREATE,TEMPORARY ON DATABASE speedsentry_backend TO DbCAdmin;

\c speedsentry_backend;

-- ---------------------------------------------------------------------------------------------------------------------
-- Regions table
-- The regions table tracks the regions where polling servers reside.  We maintain a 1-to-many relationship between
-- regions and polling servers.  A single region can contain zero or more polling servers.

CREATE TABLE regions (
    region_id   SMALLSERIAL NOT NULL PRIMARY KEY,
    region_name VARCHAR(64) NOT NULL UNIQUE
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE regions TO DbC;
GRANT ALL PRIVILEGES ON SEQUENCE regions_region_id_seq TO DbC;
GRANT ALL PRIVILEGES ON TABLE regions TO DbCAdmin;
GRANT ALL PRIVILEGES ON SEQUENCE regions_region_id_seq TO DbCAdmin;

-- ---------------------------------------------------------------------------------------------------------------------
-- Servers table
-- The servers table tracks a specific polling server.  Each server must reside in a defined region.
--
-- Servers are tracked by their identifier and can be active, inactive, or defunct.  An active server can accept
-- customer monitors.  An inactive server exists but is not testing customer sites.  Inactive servers are generally in
-- the process of being configured, upgraded, or taken down.  A defunct server no longer exists and is only being
-- tracked due to latency data that references it.
--
-- We also track information reported by the server regarding their workloads so that we can maintain balanced workloads
-- within each region as well as determine when new servers need to be provisioned.

CREATE TYPE server_status AS ENUM('UNKNOWN','ACTIVE','INACTIVE','DEFUNCT');

CREATE TABLE servers (
    server_id            SMALLSERIAL NOT NULL PRIMARY KEY,
    region_id            SMALLINT NOT NULL,
    identifier           VARCHAR(48) NOT NULL UNIQUE,
    status               server_status NOT NULL DEFAULT 'UNKNOWN',
    monitor_service_rate REAL DEFAULT NULL,
    cpu_loading          REAL DEFAULT NULL,
    memory_loading       REAL DEFAULT NULL,
    CONSTRAINT servers_region_fk_constraint
        FOREIGN KEY (region_id) REFERENCES regions (region_id)
        ON DELETE CASCADE ON UPDATE NO ACTION
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE servers TO DbC;
GRANT ALL PRIVILEGES ON SEQUENCE servers_server_id_seq TO DbC;
GRANT ALL PRIVILEGES ON TABLE servers TO DbCAdmin;
GRANT ALL PRIVILEGES ON SEQUENCE servers_server_id_seq TO DbCAdmin;

-- ---------------------------------------------------------------------------------------------------------------------
-- Customer Capabilities table
-- The customer capabilities table tracks what functions we allow each customer to use.  This table is also treated as
-- the master customer table for our backend.
--
-- The "flags" field is a bit field indicating detailed information about what the customer can and can-not do.  See
-- the source code for the detailed definition of this field.
--
-- The expiration_days field is currently unused.  Data retention for all customers is currently the same.

CREATE TABLE customer_capabilities (
    customer_id      INTEGER NOT NULL PRIMARY KEY,
    number_monitors  INTEGER NOT NULL,
    polling_interval SMALLINT NOT NULL,
    expiration_days  INTEGER NOT NULL,
    flags            INTEGER NOT NULL
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE customer_capabilities TO DbC;
GRANT ALL PRIVILEGES ON TABLE customer_capabilities TO DbCAdmin;

-- ---------------------------------------------------------------------------------------------------------------------
-- Customer Mapping table
-- The customer mapping table tracks which polling servers are serving which customers.  We also track which server
-- is the "primary" server for this customer.  The primary server is the polling server that is performing per
-- host/scheme ping testing as well as checking SSL expiration data for this customer.

CREATE TABLE customer_mapping (
    customer_id    INTEGER NOT NULL,
    server_id      SMALLINT NOT NULL,
    primary_server BOOLEAN NOT NULL,
    PRIMARY KEY    (customer_id, server_id),
    CONSTRAINT customer_mapping_custmer_capabilities_fk_constraint
        FOREIGN KEY (customer_id) REFERENCES customer_capabilities (customer_id)
        ON DELETE CASCADE ON UPDATE NO ACTION,
    CONSTRAINT customer_servers_fk_constraint
        FOREIGN KEY (server_id) REFERENCES servers (server_id)
        ON DELETE CASCADE ON UPDATE NO ACTION
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE customer_mapping TO DbC;
GRANT ALL PRIVILEGES ON TABLE customer_mapping TO DbCAdmin;

-- ---------------------------------------------------------------------------------------------------------------------
-- Host/Scheme table
-- The host/scheme table stores information about a single authority referenced by a customer.  The table also stores
-- the last reported SSL expiration data.
--
-- Note that monitors are tied to host/scheme entries in a 1 to many relationship where one host scheme may have one or
-- more associated monitors.

CREATE TABLE host_scheme (
    host_scheme_id           SERIAL PRIMARY KEY,
    customer_id              INTEGER NOT NULL,
    host                     VARCHAR(128) NOT NULL,
    ssl_expiration_timestamp BIGINT NOT NULL,
    CONSTRAINT host_scheme_customer_capabilities_fk_constraint
        FOREIGN KEY (customer_id) REFERENCES customer_capabilities (customer_id)
        ON DELETE CASCADE ON UPDATE NO ACTION
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE host_scheme TO DbC;
GRANT ALL PRIVILEGES ON SEQUENCE host_scheme_host_scheme_id_seq TO DbC;
GRANT ALL PRIVILEGES ON TABLE host_scheme TO DbCAdmin;
GRANT ALL PRIVILEGES ON SEQUENCE host_scheme_host_scheme_id_seq TO DbCAdmin;

-- ---------------------------------------------------------------------------------------------------------------------
-- Monitor table
-- The monitor table stores information about a single monitor.  Each monitor entry is tied to a hosthost/scheme table
-- which indicates the authority for the monitor.  The monitor maintains just the specific path under each authority.
--
-- * The user_ordering entry indicates the location in the GUI where the monitor is defined.
-- * The path indicates the path under the authority for the monitor.
-- * The content check mode indicates if the contents returned by the path should be checked for changes or for
--   keywords.
-- * The keywords entry is a binary blob holding the keywords.  Each keyword is stored as a 2 byte length (little
--   endian) followed by the actual keyword.
-- * The method entry indicates if an HTTP GET or POST should be used to check the endpoint.
-- * The post_content_type indicates the content type to be reported by the polling server when issuing POSTs.  The
--   value is ignored for GETs.
-- * The post_user_agent entry indicates the user agent string to be provided in the header.
-- * The post_content type entry holds the raw content to be provided.

CREATE TYPE monitor_content_check_mode AS ENUM('NO_CHECK','CONTENT_MATCH','ANY_KEYWORDS','ALL_KEYWORDS','SMART_CONTENT_MATCH');
CREATE TYPE monitor_method AS ENUM('GET', 'POST');
CREATE TYPE monitor_post_content_type AS ENUM('JSON','XML','TEXT');

CREATE TABLE monitor (
    monitor_id         SERIAL PRIMARY KEY,
    customer_id        INTEGER NOT NULL,
    host_scheme_id     INTEGER NOT NULL,
    user_ordering      SMALLINT NOT NULL,
    path               TEXT NOT NULL,
    method             monitor_method NOT NULL,
    content_check_mode monitor_content_check_mode NOT NULL,
    keywords           BYTEA NOT NULL,
    post_content_type  monitor_post_content_type NOT NULL,
    post_user_agent    VARCHAR(128) DEFAULT NULL,
    post_content       BYTEA,
    CONSTRAINT monitor_host_scheme_fk_constraint
        FOREIGN KEY (host_scheme_id) REFERENCES host_scheme (host_scheme_id)
        ON DELETE CASCADE ON UPDATE NO ACTION,
    CONSTRAINT monitor_customer_constraint
        FOREIGN KEY (customer_id) REFERENCES customer_capabilities (customer_id)
        ON DELETE CASCADE ON UPDATE NO ACTION
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE monitor TO DbC;
GRANT ALL PRIVILEGES ON SEQUENCE monitor_monitor_id_seq TO DbC;
GRANT ALL PRIVILEGES ON TABLE monitor TO DbCAdmin;
GRANT ALL PRIVILEGES ON SEQUENCE monitor_monitor_id_seq TO DbCAdmin;

-- ---------------------------------------------------------------------------------------------------------------------
-- Latency Seconds table
-- The latency seconds table stores raw latency values reported by the polling servers.  The time stamp is defined as
-- the seconds since 12:00:00 January 1, 2021 (UTC) to keep the value under 32-bits.
-- The latency value is latency in microseconds.

CREATE TABLE latency_seconds (
    monitor_id INTEGER NOT NULL,
    server_id  SMALLINT NOT NULL,
    timestamp  INTEGER NOT NULL,
    latency    INTEGER NOT NULL,
    PRIMARY KEY (monitor_id, server_id, timestamp),
    CONSTRAINT latency_seconds_monitor_fk_constraint
        FOREIGN KEY (monitor_id) REFERENCES monitor (monitor_id)
        ON DELETE CASCADE ON UPDATE NO ACTION,
    CONSTRAINT latency_seconds_server_fk_constraint
        FOREIGN KEY (server_id) REFERENCES servers (server_id)
        ON DELETE CASCADE ON UPDATE NO ACTION
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE latency_seconds TO DbC;
GRANT ALL PRIVILEGES ON TABLE latency_seconds TO DbCAdmin;

-- ---------------------------------------------------------------------------------------------------------------------
-- Latency Aggregated table
-- The latency aggregated table is used to store older latency values.  Latency and timestamp are random samples taken
-- over a sample period defined by the start and end timestamp values.

CREATE TABLE latency_aggregated (
    monitor_id       INTEGER NOT NULL,
    server_id        SMALLINT NOT NULL,
    timestamp        INTEGER NOT NULL,
    latency          INTEGER NOT NULL,
    start_timestamp  INTEGER NOT NULL,
    end_timestamp    INTEGER NOT NULL,
    mean_latency     DOUBLE PRECISION NOT NULL,
    variance_latency DOUBLE PRECISION NOT NULL,
    minimum_latency  INTEGER NOT NULL,
    maximum_latency  INTEGER NOT NULL,
    number_samples   INTEGER NOT NULL,
    PRIMARY KEY (monitor_id, server_id, start_timestamp),
    CONSTRAINT latency_aggregated_monitor_fk_constraint
        FOREIGN KEY (monitor_id) REFERENCES monitor (monitor_id)
        ON DELETE CASCADE ON UPDATE NO ACTION,
    CONSTRAINT latency_aggregated_servers_fk_constraint
        FOREIGN KEY (server_id) REFERENCES servers (server_id)
        ON DELETE CASCADE ON UPDATE NO ACTION
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE latency_aggregated TO DbC;
GRANT ALL PRIVILEGES ON TABLE latency_aggregated TO DbCAdmin;

-- ---------------------------------------------------------------------------------------------------------------------
-- Event table
-- The event table stores information about an event reported by a polling server.  Only events that are reported to the
-- customer are stored.

CREATE TYPE event_event_type AS ENUM(
    'WORKING',
    'NO_RESPONSE',
    'CONTENT_CHANGED',
    'KEYWORDS',
    'SSL_CERTIFICATE_EXPIRING',
    'SSL_CERTIFICATE_RENEWED',
    'CUSTOMER_1',
    'CUSTOMER_2',
    'CUSTOMER_3',
    'CUSTOMER_4',
    'CUSTOMER_5',
    'CUSTOMER_6',
    'CUSTOMER_7',
    'CUSTOMER_8',
    'CUSTOMER_9',
    'CUSTOMER_10',
    'TRANSACTION',
    'INQUIRY',
    'SUPPORT_REQUEST',
    'STORAGE_LIMIT_REACHED'
);

CREATE TABLE event (
    event_id    SERIAL,
    monitor_id  INTEGER NOT NULL,
    customer_id INTEGER NOT NULL,
    timestamp   INTEGER NOT NULL,
    event_type  event_event_type NOT NULL,
    message     VARCHAR(128) DEFAULT NULL,
    hash        BYTEA,
    PRIMARY KEY (event_id),
    CONSTRAINT event_customer_capabilities_fk_constraint
        FOREIGN KEY (customer_id) REFERENCES customer_capabilities (customer_id)
        ON DELETE CASCADE ON UPDATE NO ACTION,
    CONSTRAINT event_monitor_fk_constraint
        FOREIGN KEY (monitor_id) REFERENCES monitor (monitor_id) ON DELETE CASCADE ON UPDATE NO ACTION
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE event TO DbC;
GRANT ALL PRIVILEGES ON SEQUENCE event_event_id_seq TO DbC;
GRANT ALL PRIVILEGES ON TABLE event TO DbCAdmin;
GRANT ALL PRIVILEGES ON SEQUENCE event_event_id_seq TO DbCAdmin;

-- ---------------------------------------------------------------------------------------------------------------------
-- Monitor Status table
-- The monitor status table holds the last reported status of a given monitor.

CREATE TYPE monitor_status_status AS ENUM('UNKNOWN','WORKING','FAILED');

CREATE TABLE monitor_status (
    monitor_id INTEGER,
    status     monitor_status_status NOT NULL,
    CONSTRAINT monitor_status_monitor_fk_constraint
        FOREIGN KEY (monitor_id) REFERENCES monitor (monitor_id)
        ON DELETE CASCADE ON UPDATE NO ACTION
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE monitor_status TO DbC;
GRANT ALL PRIVILEGES ON TABLE monitor_status TO DbCAdmin;

-- ---------------------------------------------------------------------------------------------------------------------
-- Customer Secrets table
-- The customer secrets table stores the customer unique REST API secret used by each customer.  Note that the data is
-- encrypted at rest and cached by the database controller.

CREATE TABLE customer_secrets (
    customer_id INTEGER NOT NULL,
    secret      BYTEA NOT NULL,
    CONSTRAINT customer_secrets_customer_fk_constraint
        FOREIGN KEY (customer_id) REFERENCES customer_capabilities (customer_id)
        ON DELETE CASCADE ON UPDATE NO ACTION
);

GRANT SELECT,INSERT,UPDATE,DELETE ON TABLE customer_secrets TO DbC;
GRANT ALL PRIVILEGES ON TABLE customer_secrets TO DbCAdmin;
