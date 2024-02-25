#-------------------------------------------------
#
# Project created by QtCreator 2013-05-26T19:28:00
#
#-------------------------------------------------
#This project is only set up to work properly on UNIX, because the default save path starts with /
#It could be easily modified to run on windows; simply modify savePath and appDataPath in main.cpp
#Both directories need to be writable by the user running the program
#
#This version allows compile-time selection of hardware, and also has virtual hardware implementations
#so that it can run on systems not connected to real instrumentation.
#In order to compile this program, you must copy the config.pri.template file to config.pri
#config.pri will be ignored by git, and can safely be used to store the local configuration
#Do not modify config.pri.template unless you are making changes relevant for all instruments (eg adding new hardware)


QT       += core gui network
CONFIG   += qt c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport printsupport

TARGET = qtftm
TEMPLATE = app

include(config.pri) {
	DISTFILES += config.pri
} else {
	error("Configuration file (config.pri) missing! Build aborted.")
}


include(wizard.pri)
include(gui.pri)
include(acquisition.pri)
include(hardware.pri)
include(data.pri)
include(settings.pri)
include(implementations.pri)

SOURCES += main.cpp

RESOURCES += \
    icons.qrc

DISTFILES += \
    config.pri \
    config.pri.template \
    README

