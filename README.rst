========================
Inesonic SpeedSentry DBC
========================
The Inesonic SpeedSentry DBC project provides the database controller (DBC) for
SpeedSentry.  The project provides three key services:

* Data aggregation from multiple polling servers.  Polling servers forward
  latency data to the DBC at periodic intervals.

* A REST API that can be used by both customers and a website to obtain data
  and control the SpeedSentry backend.

* A command and control REST API you can use to control the SpeedSentry
  backend.  The command-and-control API will forward commands to polling
  servers and provide data polling servers can use for configuration.

The DBC requires the Qt libraries and employs the QMAKE build environment.  The
DBC relies on the following projects:

* https://github.com/inesonic/inecrypto.git

* https://github.com/inesonic/inerest_api_in_v1.git

* https://github.com/inesonic/inerest_api_out_v1.git

* https://github.com/inesonic/inextea.git

You must set the following QMAKE variables on the QMAKE command line:

* INECRYPTO_INCLUDE
  
* INECRYPTO_LIBDIR
  
* INEREST_API_IN_V1_INCLUDE
  
* INEREST_API_IN_V1_LIBDIR
  
* INEREST_API_OUT_V1_INCLUDE
  
* INEREST_API_OUT_V1_LIBDIR
  
* INEXTEA_INCLUDE
  
* INEXTEA_LIBDIR

You can control the DBC and, by extension, polling servers using the command
line provided by the https://github.com/inesonic/speedsentry-command.git
project.

You can use the speedsentry-polling-server project at
https://github.com/inesonic/speedsentry-polling-server.git to implement one or
more SpeedSentry polling servers for your application.


Licensing
=========
This code is licensed under the GNU Public License, version 3.0.
