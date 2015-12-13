TARGET = watchfish

TEMPLATE = lib
CONFIG += staticlib

CONFIG += link_pkgconfig
PKGCONFIG += dbus-1 timed-qt5

HEADERS = notificationmonitor.h notification.h \
    walltimemonitor.h
SOURCES = notificationmonitor.cpp notification.cpp \
    walltimemonitor.cpp
