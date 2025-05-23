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

#ifndef WATCHFISH_MUSICCONTROLLER_H
#define WATCHFISH_MUSICCONTROLLER_H

#include <QObject>
#include <QtCore/QLoggingCategory>

namespace watchfish
{

Q_DECLARE_LOGGING_CATEGORY(musicControllerCat)

class MusicControllerPrivate;

class MusicController : public QObject
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(MusicController)

public:
	explicit MusicController(QObject *parent = 0);
	~MusicController();

	enum Status {
		StatusNoPlayer = 0,
		StatusStopped,
		StatusPaused,
		StatusPlaying
	};
    Q_ENUM(Status)

	enum RepeatStatus {
		RepeatNone = 0,
		RepeatTrack,
		RepeatPlaylist
	};

    Q_INVOKABLE Status status() const;
    Q_INVOKABLE QString service() const;

    Q_INVOKABLE QVariantMap metadata() const;

    Q_INVOKABLE QString title() const;
    Q_INVOKABLE QString album() const;
    Q_INVOKABLE QString artist() const;

    Q_INVOKABLE QString albumArt() const;

    Q_INVOKABLE int duration() const;

    Q_INVOKABLE RepeatStatus repeat() const;
    Q_INVOKABLE bool shuffle() const;

    Q_INVOKABLE int volume() const;

public slots:
	void play();
	void pause();
	void playPause();
	void next();
	void previous();

    void setVolume(const uint newVolume);
	void volumeUp();
	void volumeDown();

signals:
	void statusChanged();
	void serviceChanged();
	void metadataChanged();
	void titleChanged();
	void albumChanged();
	void artistChanged();
	void albumArtChanged();
	void durationChanged();
	void repeatChanged();
	void shuffleChanged();
	void volumeChanged();

private:
	MusicControllerPrivate * const d_ptr;
};

}

#endif // WATCHFISH_MUSICCONTROLLER_H
