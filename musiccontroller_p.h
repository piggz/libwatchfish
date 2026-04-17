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

#ifndef WATCHFISH_MUSICCONTROLLER_P_H
#define WATCHFISH_MUSICCONTROLLER_P_H

#include <AmberMpris/mpris.h>
#include <AmberMpris/mpriscontroller.h>
#include <QtDBus/QDBusInterface>

#if !defined(MER_EDITION_SAILFISH) && !defined(UUITK_EDITION)
#include <pulse/pulseaudio.h>
#endif

#include "musiccontroller.h"

namespace watchfish
{

class MusicControllerPrivate : public QObject
{
	Q_OBJECT

public:
	explicit MusicControllerPrivate(MusicController *q);
	~MusicControllerPrivate();

public:
	Amber::MprisController *controller;
	MusicController::Status curStatus;
	QString curTitle;
	QString curAlbum;
	QString curArtist;
	QString curAlbumArt;
	int curDuration;

#if defined(MER_EDITION_SAILFISH)
    QDBusConnection *_pulseBus = nullptr;
    uint _maxVolume = 0;
    void connectPulseBus();
#elif defined(UUITK_EDITION)
    QDBusInterface *_accountsIface = nullptr;
    void connectAccountsBus();
#else
    // Native PulseAudio (libpulse) state for Kirigami/desktop
    pa_mainloop     *_paMainloop = nullptr;
    pa_context      *_paContext  = nullptr;
    int              _cachedVolume = -1; // 0-100, -1 = unknown
    bool connectPulse();
    void disconnectPulse();
    // Iterate the mainloop until the operation completes or timeout
    bool iterateMainloop(pa_operation *op);
#endif

private:
	static QString stripAlbumArtComponent(const QString& component);
	static QString findAlbumArt(const QString &artist, const QString &album);
	void updateStatus();
	void updateAlbumArt();
	void updateMetadata();

private slots:
	void handleCurrentServiceChanged();
	void handlePlaybackStatusChanged();
	void handleMetadataChanged();

private:
	MusicController * const q_ptr;
	Q_DECLARE_PUBLIC(MusicController)
};

}

#endif // WATCHFISH_MUSICCONTROLLER_P_H

