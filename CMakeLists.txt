cmake_minimum_required(VERSION 3.16)

# set(FLAVOR kirigami)
# set(FLAVOR uuitk)
# set(FLAVOR silica)
# set(WATCHFISH_FEATURES "notificationmonitor;walltime;music;calendar;voicecall;volume;soundprofile")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(libwatchfish STATIC)

# Make sure to choose same Qt version as rest of project
if(TARGET Qt6::Core)
    set(QT_VERSION_MAJOR 6)
    find_package(Qt6 REQUIRED COMPONENTS Core DBus)
elseif(TARGET Qt5::Core)
    set(QT_VERSION_MAJOR 5)
    find_package(Qt5 REQUIRED COMPONENTS Core DBus)
else()
    # Use Qt6 first if available
    find_package(Qt6 COMPONENTS Core DBus QUIET)
    if(Qt6_FOUND)
        set(QT_VERSION_MAJOR 6)
    else()
        find_package(Qt5 REQUIRED COMPONENTS Core Xml)
        set(QT_VERSION_MAJOR 5)
    endif()
endif()

if(QT_VERSION_MAJOR EQUAL 5)
    macro(qt_add_dbus_interface outfiles interface basename)
        qt5_add_dbus_interface(${outfiles} ${interface} ${basename})
    endmacro()
endif()

message(STATUS "libwatchfish: using Qt ${QT_VERSION_MAJOR}")

find_package(PkgConfig REQUIRED)

target_include_directories(libwatchfish PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(libwatchfish PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
)

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

    pkg_check_modules(TIMED REQUIRED timed-qt${QT_VERSION_MAJOR})
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
    if(QT_VERSION_MAJOR EQUAL 6)
        pkg_check_modules(MPRIS REQUIRED ambermpris6)
    else()
        pkg_check_modules(MPRIS REQUIRED ambermpris)
    endif()
    target_include_directories(libwatchfish PUBLIC ${MPRIS_INCLUDE_DIRS})
    target_link_libraries(libwatchfish PUBLIC ${MPRIS_LIBRARIES})

    list(APPEND SOURCES musiccontroller.cpp)
    list(APPEND HEADERS musiccontroller.h musiccontroller_p.h)

    if(FLAVOR STREQUAL "silica")
        target_compile_definitions(libwatchfish PUBLIC MER_EDITION_SAILFISH)
        list(APPEND SOURCES musiccontroller_sailfish.cpp)
        qt_add_dbus_interface(DBUS_INTERFACES_GEN com.Meego.MainVolume2.xml mainvolume2_interface)
    elseif(FLAVOR STREQUAL "uuitk")
        target_compile_definitions(libwatchfish PUBLIC UUITK_EDITION)
        list(APPEND SOURCES musiccontroller_ubuntu.cpp)
    else()
        list(APPEND SOURCES musiccontroller_kirigami.cpp)
    endif()
endif()

# Feature: calendar
if(WATCHFISH_FEATURES MATCHES "calendar")
    if(FLAVOR STREQUAL "silica")
        pkg_check_modules(LIBMKCAL REQUIRED libmkcal-qt${QT_VERSION_MAJOR})
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
    elseif(FLAVOR STREQUAL "uuitk")
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Organizer)
        target_compile_definitions(libwatchfish PUBLIC UUITK_EDITION)
        target_link_libraries(libwatchfish PUBLIC Qt${QT_VERSION_MAJOR}::Organizer)

        list(APPEND HEADERS
            calendarsource.h
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
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Contacts)
        target_compile_definitions(libwatchfish PUBLIC MER_EDITION_SAILFISH)
        target_link_libraries(libwatchfish PUBLIC Qt${QT_VERSION_MAJOR}::Contacts)

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
        qt_add_dbus_interface(DBUS_INTERFACES_GEN org.nemomobile.voicecall.VoiceCallManager.xml voicecallmanager_interface)
        qt_add_dbus_interface(DBUS_INTERFACES_GEN org.nemomobile.voicecall.VoiceCall.xml voicecall_interface)
    elseif(FLAVOR STREQUAL "uuitk") # ubuntu touch
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Contacts)
        target_compile_definitions(libwatchfish PUBLIC UUITK_EDITION)
        target_link_libraries(libwatchfish PUBLIC Qt${QT_VERSION_MAJOR}::Contacts)

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
        target_include_directories(libwatchfish PUBLIC /usr/include/telepathy-qt${QT_VERSION_MAJOR}/)
        target_link_libraries(libwatchfish PUBLIC telepathy-qt${QT_VERSION_MAJOR})
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
    qt_add_dbus_interface(DBUS_INTERFACES_GEN com.Meego.MainVolume2.xml mainvolume2_interface)
endif()

target_sources(libwatchfish PUBLIC ${HEADERS} PRIVATE ${SOURCES} ${DBUS_INTERFACES_GEN})
