TARGET = watchfish

TEMPLATE = lib
CONFIG += staticlib
QT += dbus

WATCHFISH_FEATURES = notificationmonitor walltime music
include(libwatchfish.pri)
