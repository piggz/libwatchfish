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

#ifndef WATCHFISH_VOICECALLCONTROLLER_BASE_H
#define WATCHFISH_VOICECALLCONTROLLER_BASE_H

#include <QObject>
#include <QString>
#include <QtCore/QLoggingCategory>

namespace watchfish
{

Q_DECLARE_LOGGING_CATEGORY(voiceCallControllerCat)

class VoiceCallControllerBase : public QObject
{
        Q_OBJECT

        Q_PROPERTY(bool inCall READ inCall NOTIFY inCallChanged)
        Q_PROPERTY(bool ringing READ ringing NOTIFY ringingChanged)
        Q_PROPERTY(QString callerId READ callerId NOTIFY callerIdChanged)

public:
        explicit VoiceCallControllerBase(QObject *parent = 0);
        ~VoiceCallControllerBase();

        virtual bool inCall() const;
        virtual bool ringing() const;

        virtual QString callerId() const;

        virtual void hangup();
        virtual void silence();
        virtual void answer();
        virtual QString findPersonByNumber(const QString &number);

signals:
        void inCallChanged();
        void ringingChanged();
        void callerIdChanged();

};

} // namespace

#endif // WATCHFISH_VOICECALLCONTROLLER_BASE_H
