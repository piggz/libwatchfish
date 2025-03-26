CONFIG += link_pkgconfig
equals(FLAVOR, "silica") {
    QT += contacts
}

contains(WATCHFISH_FEATURES, soundprofile{
	HEADERS += $$PWD/soundprofile.h
	SOURCES += $$PWD/soundprofile.cpp
}

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
        DBUS_INTERFACES += $$PWD/com.Meego.MainVolume2.xml
}

contains(WATCHFISH_FEATURES, calendar) {
        equals(FLAVOR, "silica") {
            PKGCONFIG += libmkcal-qt5
            exists( /usr/include/KF5/KCalendarCore ) {
                PKGCONFIG += KF5CalendarCore
                DEFINES += KF5CALENDARCORE
            }
        } else {
            #QT += KContacts
        }
        contains (DEFINES, MER_EDITION_SAILFISH) {
            HEADERS += $$PWD/calendarsource.h $$PWD/calendarsource_p.h $$PWD/calendarevent.h
        } else {
            HEADERS += $$PWD/calendarsource.h $$PWD/calendarevent.h
        }
	SOURCES += $$PWD/calendarsource.cpp $$PWD/calendarevent.cpp
}

contains(WATCHFISH_FEATURES, voicecall) {

	equals(FLAVOR, "silica") {
		HEADERS += $$PWD/voicecallcontroller_sailfish.h $$PWD/voicecallcontroller.h $$PWD/voicecallcontroller_p.h $$PWD/voicecallcontrollerbase.h
		SOURCES += $$PWD/voicecallcontroller_sailfish.cpp $$PWD/voicecallcontrollerbase.cpp
		DBUS_INTERFACES += $$PWD/org.nemomobile.voicecall.VoiceCallManager.xml $$PWD/org.nemomobile.voicecall.VoiceCall.xml
		DEFINES += MER_EDITION_SAILFISH
	} else {
		HEADERS += $$PWD/voicecallcontroller.h $$PWD/voicecallcontroller_ubuntu.h $$PWD/callchannelobserver.h $$PWD/voicecallcontrollerbase.h
		SOURCES += $$PWD/voicecallcontroller_ubuntu.cpp $$PWD/callchannelobserver.cpp $$PWD/voicecallcontrollerbase.cpp
		INCLUDEPATH += /usr/include/telepathy-qt5/
		LIBS += -ltelepathy-qt5
		QT += contacts
		DEFINES += UUITK_EDITION
	}
}

contains (WATCHFISH_FEATURES, volume) {
	HEADERS += $$PWD/volumecontroller.h $$PWD/volumecontroller_p.h
	SOURCES += $$PWD/volumecontroller.cpp
	DBUS_INTERFACES += com.Meego.MainVolume2.xml
}
