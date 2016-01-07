CONFIG += link_pkgconfig

contains(WATCHFISH_FEATURES, notificationmonitor) {
	PKGCONFIG += dbus-1
        HEADERS += $$PWD/notificationmonitor.h $$PWD/notificationmonitor_p.h $$PWD/notification.h
        SOURCES += $$PWD/notificationmonitor.cpp $$PWD/notification.cpp
}

contains(WATCHFISH_FEATURES, walltime) {
	PKGCONFIG += timed-qt5
        HEADERS += $$PWD/walltimemonitor.h $$PWD/walltimemonitor_p.h
        SOURCES += $$PWD/walltimemonitor.cpp
}

contains(WATCHFISH_FEATURES, music) {
	PKGCONFIG += mpris-qt5
        HEADERS += $$PWD/musiccontroller.h $$PWD/musiccontroller_p.h
        SOURCES += $$PWD/musiccontroller.cpp
}
