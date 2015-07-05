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
#so that it can run on systems not connected to real instrumentation


QT       += core gui network
CONFIG   += qt thread c++11

#If you want to compile with all virtual hardware, uncomment the following line
CONFIG += nohardware

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport printsupport

TARGET = qtftm
TEMPLATE = app

INCLUDEPATH += "/usr/Qt/5.4/gcc_64/include"
INCLUDEPATH += "/usr/Qt/5.4/gcc_64"


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

#This line should reflect which spectrometer the code is being compiled for
DEFINES += QTFTM_SPECTROMETER=1

DISTFILES += \
    Notes.txt

#note: on linux, /usr/lib64 should have symbolic links set up to point to the qwt library if not using
#pre-compiled qwt6 libraries (as of openSUSE 13.2, the pre-compiled libraries are built against Qt4,
#so to use this program the libraries must be built manually against Qt5
#example links (all in /usr/lib64, e.g, as root, ln -s /usr/local/qwt-6.1.3-svn/lib/libqwt.so /usr/lib64/libswt.so)
#libqwt.so -> /usr/local/qwt-6.1.3-svn/lib/libqwt.so
#libqwt.so.6 -> /usr/local/qwt-6.1.3-svn/lib/libqwt.so
#libqwt.so.6.1 -> /usr/local/qwt-6.1.3-svn/lib/libqwt.so
#libqwt.so.6.1.3 -> /usr/local/qwt-6.1.3-svn/lib/libqwt.so

#headers for qwt6 should be linked in /usr/local/include/qwt6:
#example: ln -s /usr/local/qwt-6.1.3/include /usr/local/include/qwt6
unix:!macx: LIBS += -L/usr/local/lib64 -lqwt -lgsl -lgslcblas -lm -llabjackusb


#Hardware definitions
nohardware {
####################################################
#       DO NOT MODIFY VALUES IN THIS SECTION       #
#This sets definitions for nohardware configuration#
# You can choose your hardware in the next section #
#   Add new variables here if new hardware added   #
####################################################
SCOPE=0
GPIB=0
ATTN=0
PDG=0
MD=0
FC=0
FC_CHANNELS=4
IOB=0
FTMSYNTH=0
DRSYNTH=0
PGEN=0
PGEN_GAS=0
PGEN_DC=1
PGEN_MW=2
PGEN_DR=3
PGEN_CHANNELS=8
HVPS=0
####################################################
} else {

####################################################
#              HARDWARE DEFINITIONS                #
# Choose hardware here. Be sure to read each entry #
#If you add a new implementation, add comment below#
#If you add new hardware, make new variables below #
####################################################

####################################################
#Oscilloscope. 0 = virtual, 1 = DPO3012            #
####################################################
SCOPE=1

#####################################################
#GPIB Controller. 0 = virtual, 1 = Prologix GPIB-LAN#
#####################################################
GPIB=1

#####################################################
#Attenuator. 0 = virtual, 1 = Aeroflex Weinschel Attenuator
#####################################################
ATTN=1

#####################################################
#Pin switch delay generator. 0 = virtual, 1 = Antonucci
#####################################################
PDG=1

#####################################################
#Motor driver. 0 = virtual, 1 = Antonucci
#####################################################
MD=1

#####################################################
#flow controller. 0 = virtual, 1 = MKS 647C
#####################################################
FC=1
FC_CHANNELS=4

#####################################################
#IO Board. 0 = virtual, 1 = LabJack U3
#####################################################
IOB=1

#####################################################
#FTM Synth. 0 = virtual, 1 = HP8673, 2 = HP8340
#####################################################
FTMSYNTH=1

#####################################################
#DR Synth. 0 = virtual, 1 = HP8673, 2 = HP8340
#####################################################
DRSYNTH=2

#####################################################
#pulse generator. 0 = virtual, 1 = QC 9518
#Indicate numbers of Gas, DC, MW, and DR channels
#Also be sure to indicate total number of channels
#####################################################
PGEN=1
PGEN_GAS=0
PGEN_DC=1
PGEN_MW=2
PGEN_DR=3
PGEN_CHANNELS=8

#####################################################
#HV power supply (discharge). 0 = virtual, 1 = ???? (not implemented as of 2015-07-04)
#####################################################
HVPS=0
}


######################################################
#only modify the lines below if you know what you're doing!
#For instance, if new hardware is added to program, you
#should make new variables above for both configurations
#and add appropriate defines below. See headers of
#hardware base classes for examples of how to use these
######################################################
DEFINES += QTFTM_OSCILLOSCOPE=$$SCOPE
DEFINES += QTFTM_GPIBCONTROLLER=$$GPIB
DEFINES += QTFTM_ATTENUATOR=$$ATTN
DEFINES += QTFTM_PDG=$$PDG
DEFINES += QTFTM_MOTORDRIVER=$$MD
DEFINES += QTFTM_FLOWCONTROLLER=$$FC QTFTM_FLOW_NUMCHANNELS=$$FC_CHANNELS
DEFINES += QTFTM_IOBOARD=$$IOB
DEFINES += QTFTM_FTMSYNTH=$$FTMSYNTH
DEFINES += QTFTM_DRSYNTH=$$DRSYNTH
DEFINES += QTFTM_PGEN=$$PGEN QTFTM_PGEN_GASCHANNEL=$$PGEN_GAS QTFTM_PGEN_DCCHANNEL=$$PGEN_DC QTFTM_PGEN_MWCHANNEL=$$PGEN_MW \
	QTFTM_PGEN_DRCHANNEL=$$PGEN_DR QTFTM_PGEN_NUMCHANNELS=$$PGEN_CHANNELS
DEFINES += QTFTM_HVPS=$$HVPS

equals(SCOPE, "0") {
	RESOURCES += virtualdata.qrc
}
