cmake_minimum_required(VERSION 3.16)

# set(FLAVOR kirigami)
# set(FLAVOR uuitk)
# set(FLAVOR silica)
# set(WATCHFISH_FEATURES "notificationmonitor;walltime;music;calendar;voicecall;volume")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the library type
add_library(libwatchfish STATIC)

find_package(PkgConfig REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core DBus)

# Link with Qt5 DBus
target_include_directories(libwatchfish PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libwatchfish PUBLIC Qt5::Core Qt5::DBus)

# Collect source files based on features
set(SOURCES)
set(HEADERS)



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
    list(APPEND DBUS_INTERFACES
        com.Meego.MainVolume2.xml
    )
endif()

# Feature: calendar
if(WATCHFISH_FEATURES MATCHES "calendar")
    if(FLAVOR STREQUAL "silica")
        pkg_check_modules(LIBMKCAL REQUIRED libmkcal-qt5)

        if(EXISTS /usr/include/KF5/KCalendarCore)
            pkg_check_modules(KF5CalendarCore REQUIRED KF5CalendarCore)
            target_compile_definitions(libwatchfish PUBLIC KF5CALENDARCORE)
        endif()
    endif()

    if(MER_EDITION_SAILFISH)
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
        list(APPEND DBUS_INTERFACES
            org.nemomobile.voicecall.VoiceCallManager.xml
            org.nemomobile.voicecall.VoiceCall.xml
        )
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
    list(APPEND DBUS_INTERFACES
        com.Meego.MainVolume2.xml
    )
endif()

# Add sources and headers to the library
target_sources(libwatchfish PUBLIC ${SOURCES} ${HEADERS})

# Install DBus interfaces if defined
if(DBUS_INTERFACES)
    foreach(DBUS_FILE ${DBUS_INTERFACES})
        install(FILES ${DBUS_FILE} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dbus-1/interfaces)
    endforeach()
endif()


