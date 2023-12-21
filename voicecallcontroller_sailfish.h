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

#ifndef WATCHFISH_VOICECALLCONTROLLER_SAILFISH_H
#define WATCHFISH_VOICECALLCONTROLLER_SAILFISH_H

#include <QtCore/QLoggingCategory>
#include "voicecallcontrollerbase.h"

namespace watchfish
{

Q_DECLARE_LOGGING_CATEGORY(voiceCallControllerCat)

class VoiceCallControllerPrivate;

class VoiceCallController : public VoiceCallControllerBase
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(VoiceCallController)

public:
	explicit VoiceCallController(QObject *parent = 0);
	~VoiceCallController();

    virtual bool inCall() const override;
    virtual bool ringing() const override;
    virtual QString callerId() const override;

    virtual void hangup() override;
    virtual void silence() override;
    virtual void answer() override;
    virtual QString findPersonByNumber(const QString &number) override;

private:
	VoiceCallControllerPrivate * const d_ptr;
};

} // namespace


#endif // WATCHFISH_VOICECALLCONTROLLER_H
