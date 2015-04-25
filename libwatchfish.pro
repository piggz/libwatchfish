TARGET = watchfish

TEMPLATE = lib
CONFIG += staticlib

CONFIG += link_pkgconfig
PKGCONFIG += dbus-1
INCLUDEPATH += /usr/include/dbus-1.0

HEADERS = notificationmonitor.h \
    notification.h
SOURCES = notificationmonitor.cpp \
    notification.cpp

