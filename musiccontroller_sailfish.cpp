/*
 *  libwatchfish - library with common functionality for SailfishOS smartwatch connector programs.
 *  Copyright (C) 2015 Javier S. Pedro <dev.git@javispedro.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Volume control for Sailfish OS via PulseAudio P2P DBus (com.Meego.MainVolume2).
// Volume is step-based (0..StepCount), returned as int directly.

#include <QDBusMessage>
#include <QDBusReply>

#include "musiccontroller.h"
#include "musiccontroller_p.h"

namespace watchfish
{

MusicControllerPrivate::~MusicControllerPrivate()
{
    if (_pulseBus != nullptr) {
        qDebug() << "Disconnecting from PulseAudio P2P DBus";
        QDBusConnection::disconnectFromBus("org.PulseAudio1");
        delete _pulseBus;
    }
}

void MusicControllerPrivate::connectPulseBus()
{
    if (_pulseBus) {
        if (!_pulseBus->isConnected()) {
            delete _pulseBus;
        } else {
            return;
        }
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        "org.PulseAudio1", "/org/pulseaudio/server_lookup1",
        "org.freedesktop.DBus.Properties", "Get");
    call << "org.PulseAudio.ServerLookup1" << "Address";

    QDBusReply<QDBusVariant> lookupReply = QDBusConnection::sessionBus().call(call);
    if (!lookupReply.isValid()) {
        qDebug() << "Cannot connect to PulseAudio bus";
        return;
    }

    qDebug() << "PulseAudio Bus address:" << lookupReply.value().variant().toString();
    _pulseBus = new QDBusConnection(
        QDBusConnection::connectToPeer(lookupReply.value().variant().toString(), "org.PulseAudio1"));

    if (_maxVolume == 0) {
        call = QDBusMessage::createMethodCall(
            "com.Meego.MainVolume2", "/com/meego/mainvolume2",
            "org.freedesktop.DBus.Properties", "Get");
        call << "com.Meego.MainVolume2" << "StepCount";

        QDBusReply<QDBusVariant> volumeMaxReply = _pulseBus->call(call);
        if (volumeMaxReply.isValid()) {
            _maxVolume = volumeMaxReply.value().variant().toUInt();
            qDebug() << "Max volume:" << _maxVolume;
        } else {
            qWarning() << "Could not read volume max, cannot adjust volume:"
                       << volumeMaxReply.error().message();
        }
    }
}

int MusicController::volume() const
{
    if (d_ptr->_pulseBus == nullptr) {
        qWarning() << "PulseAudio bus not available";
        return -1;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        "com.Meego.MainVolume2", "/com/meego/mainvolume2",
        "org.freedesktop.DBus.Properties", "Get");
    call << "com.Meego.MainVolume2" << "CurrentStep";

    QDBusReply<QDBusVariant> volumeReply = d_ptr->_pulseBus->call(call);
    if (volumeReply.isValid()) {
        return static_cast<int>(volumeReply.value().variant().toUInt());
    }
    return -1;
}

void MusicController::setVolume(const uint newVolume)
{
    qDebug() << "Setting volume:" << newVolume;
    d_ptr->connectPulseBus();

    if (d_ptr->_pulseBus == nullptr) {
        qWarning() << "PulseAudio bus not available";
        return;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        "com.Meego.MainVolume2", "/com/meego/mainvolume2",
        "org.freedesktop.DBus.Properties", "Set");
    call << "com.Meego.MainVolume2" << "CurrentStep"
         << QVariant::fromValue(QDBusVariant(newVolume));

    QDBusError err = d_ptr->_pulseBus->call(call);
    if (err.isValid()) {
        qWarning() << err.message();
    }
}

void MusicController::volumeUp()
{
    d_ptr->connectPulseBus();
    int curVolume = volume();
    if (curVolume < 0) {
        return;
    }
    uint newVolume = static_cast<uint>(curVolume) + 1;
    if (newVolume >= d_ptr->_maxVolume) {
        qDebug() << "Cannot increase volume beyond maximum" << d_ptr->_maxVolume;
        return;
    }
    setVolume(newVolume);
}

void MusicController::volumeDown()
{
    d_ptr->connectPulseBus();
    int curVolume = volume();
    if (curVolume <= 0) {
        qDebug() << "Cannot decrease volume beyond 0";
        return;
    }
    setVolume(static_cast<uint>(curVolume) - 1);
}

} // namespace watchfish
