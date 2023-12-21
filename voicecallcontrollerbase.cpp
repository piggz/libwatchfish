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

#include "voicecallcontrollerbase.h"
#include <QString>
#include <QtCore/QLoggingCategory>

namespace watchfish
{

Q_LOGGING_CATEGORY(voiceCallControllerCat, "watchfish-VoiceCallController")


VoiceCallControllerBase::VoiceCallControllerBase(QObject *parent): QObject(parent) {
}

VoiceCallControllerBase::~VoiceCallControllerBase() {
}

bool VoiceCallControllerBase::inCall() const
{
    return false;
}

bool VoiceCallControllerBase::ringing() const
{
    return false;
}

QString VoiceCallControllerBase::callerId() const {
    return QString();
}

void VoiceCallControllerBase::hangup() {
}
void VoiceCallControllerBase::silence() {
}
void VoiceCallControllerBase::answer() {
}

QString VoiceCallControllerBase::findPersonByNumber(const QString &number) {
    return number;
}

} // namespace

