TARGET = watchfish

TEMPLATE = lib
CONFIG += staticlib
QT += dbus

WATCHFISH_FEATURES = notificationmonitor walltime music calendar
include(libwatchfish.pri)
