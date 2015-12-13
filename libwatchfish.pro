TARGET = watchfish

TEMPLATE = lib
CONFIG += staticlib
QT += dbus

CONFIG += link_pkgconfig
PKGCONFIG += dbus-1 timed-qt5

HEADERS = notificationmonitor.h notificationmonitor_p.h notification.h \
    walltimemonitor.h walltimemonitor_p.h

SOURCES = notificationmonitor.cpp notification.cpp \
    walltimemonitor.cpp
