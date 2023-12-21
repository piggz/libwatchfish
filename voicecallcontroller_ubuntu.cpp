/*
 *  libwatchfish - library with common functionality for SailfishOS smartwatch connector programs.
 *  Copyright (C) 2016 Javier S. Pedro <dev.git@javispedro.com>
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

#include "voicecallcontroller.h"
#include "callchannelobserver.h"
#include "voicecallcontrollerbase.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>

namespace watchfish
{

VoiceCallController::VoiceCallController(QObject *parent): VoiceCallControllerBase(parent)
{

    // Calls
    m_telepathyMonitor = new TelepathyMonitor(this);
    connect(m_telepathyMonitor, &TelepathyMonitor::incomingCall, this, &VoiceCallController::incomingCall);
    connect(m_telepathyMonitor, &TelepathyMonitor::callStarted, this, &VoiceCallController::callStarted);
    connect(m_telepathyMonitor, &TelepathyMonitor::callEnded, this, &VoiceCallController::callEnded);
}

QString VoiceCallController::findPersonByNumber(const QString &number) {
    if ((m_callerId == number) && m_callerName != "") {
        return m_callerName;
    }
    return number;
}

void VoiceCallController::incomingCall(uint cookie, const QString &number, const QString &name) {
    m_lastCookie = cookie;
    m_callerId = number;
    m_callerName = name;

    m_ringing = true;
    m_inCall = true;
    emit inCallChanged();
    emit ringingChanged();
    emit callerIdChanged();
}

bool VoiceCallController::inCall() const {
    return m_inCall;
}

bool VoiceCallController::ringing() const {
    return m_ringing;
}

QString VoiceCallController::callerId() const {
    return m_callerId;
}

void VoiceCallController::answer() {
    m_telepathyMonitor->accept(m_lastCookie);
    m_ringing = false;
    m_inCall = true;
    emit inCallChanged();
    emit ringingChanged();
}

void VoiceCallController::hangup() {
    hangupCall(m_lastCookie);
}

void VoiceCallController::callStarted(uint cookie) {
    m_lastCookie = cookie;
    m_ringing = false;
    m_inCall = true;
    emit inCallChanged();
    emit ringingChanged();
}

void VoiceCallController::callEnded(uint cookie, bool missed) {
    m_lastCookie = cookie;
    m_ringing = false;
    m_inCall = false;
    emit inCallChanged();
    emit ringingChanged();
}

void VoiceCallController::hangupCall(uint cookie) {
    m_telepathyMonitor->hangupCall(cookie);
    m_ringing = false;
    m_inCall = false;
    emit inCallChanged();
    emit ringingChanged();
}

} // namespace
