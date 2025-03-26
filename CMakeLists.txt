cmake_minimum_required(VERSION 3.16)

# set(FLAVOR kirigami)
# set(FLAVOR uuitk)
# set(FLAVOR silica)
# set(WATCHFISH_FEATURES "notificationmonitor;walltime;music;calendar;voicecall;volume;soundprofile")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(libwatchfish STATIC)

find_package(PkgConfig REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core DBus)

target_include_directories(libwatchfish PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(libwatchfish PUBLIC Qt5::Core Qt5::DBus)

set(SOURCES)
set(HEADERS)
set(DBUS_INTERFACES_GEN)

# Feature: soundprofile
if(WATCHFISH_FEATURES MATCHES "soundprofile")
    LIST(APPEND SOURCES
        soundprofile.cpp
    )
    LIST(APPEND HEADERS
        soundprofile.h
    )
endif()

# Feature: notificationmonitor
if(WATCHFISH_FEATURES MATCHES "notificationmonitor")

    pkg_check_modules(DBUS REQUIRED dbus-1)

    target_include_directories(libwatchfish PUBLIC ${DBUS_INCLUDE_DIRS})
    target_link_libraries(libwatchfish PUBLIC ${DBUS_LIBRARIES})

    list(APPEND SOURCES
        notificationmonitor.cpp
        notification.cpp
    )
    list(APPEND HEADERS
        notificationmonitor.h
        notificationmonitor_p.h
        notification.h
    )
endif()

# Feature: walltime
if(WATCHFISH_FEATURES MATCHES "walltime")

    pkg_check_modules(TIMED REQUIRED timed-qt5)
    target_include_directories(libwatchfish PUBLIC ${TIMED_INCLUDE_DIRS})
    target_link_libraries(libwatchfish PUBLIC ${TIMED_LIBRARIES})

    list(APPEND SOURCES
        walltimemonitor.cpp
    )
    list(APPEND HEADERS
        walltimemonitor.h
        walltimemonitor_p.h
    )
endif()

# Feature: music
if(WATCHFISH_FEATURES MATCHES "music")
    pkg_check_modules(MPRIS REQUIRED mpris-qt5)
    target_include_directories(libwatchfish PUBLIC ${MPRIS_INCLUDE_DIRS})
    target_link_libraries(libwatchfish PUBLIC ${MPRIS_LIBRARIES})

    list(APPEND SOURCES
        musiccontroller.cpp
    )
    list(APPEND HEADERS
        musiccontroller.h
        musiccontroller_p.h
    )
    qt5_add_dbus_interface(DBUS_INTERFACES_GEN com.Meego.MainVolume2.xml mainvolume2_interface)
endif()

# Feature: calendar
if(WATCHFISH_FEATURES MATCHES "calendar")
    if(FLAVOR STREQUAL "silica")
        pkg_check_modules(LIBMKCAL REQUIRED libmkcal-qt5)
        target_include_directories(libwatchfish PUBLIC ${LIBMKCAL_INCLUDE_DIRS})
        target_link_libraries(libwatchfish PUBLIC ${LIBMKCAL_LIBRARIES})

        if(EXISTS /usr/include/KF5/KCalendarCore/)
            pkg_check_modules(KF5CALENDARCORE REQUIRED KF5CalendarCore)
            target_include_directories(libwatchfish PUBLIC ${KF5CALENDARCORE_INCLUDE_DIRS})
            target_link_libraries(libwatchfish PUBLIC ${KF5CALENDARCORE_LIBRARIES})
            add_definitions(-DKF5CALENDARCORE)
        endif()

        list(APPEND HEADERS
            calendarsource.h
            calendarsource_p.h
            calendarevent.h
        )
    else()
        list(APPEND HEADERS
            calendarsource.h
            calendarevent.h
        )
    endif()

    list(APPEND SOURCES
        calendarsource.cpp
        calendarevent.cpp
    )
endif()

# Feature: voicecall
if(WATCHFISH_FEATURES MATCHES "voicecall")
    if(FLAVOR STREQUAL "silica")
        find_package(Qt5 COMPONENTS Contacts)
        target_compile_definitions(libwatchfish PUBLIC MER_EDITION_SAILFISH)
        target_link_libraries(libwatchfish PUBLIC Qt5::Contacts)

        list(APPEND SOURCES
            voicecallcontroller_sailfish.cpp
            voicecallcontrollerbase.cpp
        )
        list(APPEND HEADERS
            voicecallcontroller_sailfish.h
            voicecallcontroller.h
            voicecallcontroller_p.h
            voicecallcontrollerbase.h
        )
        qt5_add_dbus_interface(DBUS_INTERFACES_GEN org.nemomobile.voicecall.VoiceCallManager.xml voicecallmanager_interface)
        qt5_add_dbus_interface(DBUS_INTERFACES_GEN org.nemomobile.voicecall.VoiceCall.xml voicecall_interface)
    elseif(FLAVOR STREQUAL "uuitk") # ubuntu touch
        find_package(Qt5 COMPONENTS Contacts)
        target_compile_definitions(libwatchfish PUBLIC UUITK_EDITION)
        target_link_libraries(libwatchfish PUBLIC Qt5::Contacts)

        list(APPEND SOURCES
            voicecallcontroller_ubuntu.cpp
            callchannelobserver.cpp
            voicecallcontrollerbase.cpp
        )
        list(APPEND HEADERS
            voicecallcontroller.h
            voicecallcontroller_ubuntu.h
            callchannelobserver.h
            voicecallcontrollerbase.h
        )
        target_include_directories(libwatchfish PUBLIC /usr/include/telepathy-qt5/)
        target_link_libraries(libwatchfish PUBLIC telepathy-qt5)
    endif()
endif()

# Feature: volume
if(WATCHFISH_FEATURES MATCHES "volume")
    list(APPEND SOURCES
        volumecontroller.cpp
    )
    list(APPEND HEADERS
        volumecontroller.h
        volumecontroller_p.h
    )
    qt5_add_dbus_interface(DBUS_INTERFACES_GEN com.Meego.MainVolume2.xml mainvolume2_interface)
endif()

target_sources(libwatchfish PUBLIC ${HEADERS} PRIVATE ${SOURCES} ${DBUS_INTERFACES_GEN})
