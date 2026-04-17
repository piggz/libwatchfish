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

#include <QtCore/QDir>
#include <QtCore/QCryptographicHash>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif

#include "mprismetadata.h"
#include "musiccontroller.h"
#include "musiccontroller_p.h"

namespace watchfish
{

Q_LOGGING_CATEGORY(musicControllerCat, "watchfish-MusicController")

MusicControllerPrivate::MusicControllerPrivate(MusicController *q)
	: controller(new Amber::MprisController(this)), q_ptr(q)
{
	connect(controller, &Amber::MprisController::currentServiceChanged,
			this, &MusicControllerPrivate::handleCurrentServiceChanged);
	connect(controller, &Amber::MprisController::playbackStatusChanged,
			this, &MusicControllerPrivate::handlePlaybackStatusChanged);
	connect(controller, &Amber::MprisController::shuffleChanged,
			q, &MusicController::shuffleChanged);
	connect(controller, &Amber::MprisController::loopStatusChanged,
			q, &MusicController::repeatChanged);

	connect(controller->metaData(), &Amber::MprisMetaData::metaDataChanged,
			this, &MusicControllerPrivate::handleMetadataChanged);
}



QString MusicControllerPrivate::stripAlbumArtComponent(const QString& component)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	static QRegularExpression rsb("\\[.*\\]");
	static QRegularExpression rfb("\\{.*\\}");
	static QRegularExpression rrb("\\(.*\\)");
	static QRegularExpression stripB("^[()_{}\\[\\]!@#$^&*+=|\\\\/\"'?<>~`\\s\\t]*");
	static QRegularExpression stripE("[()_{}\\[\\]!@#$^&*+=|\\\\/\"'?<>~`\\s\\t]*$");
#else
	static QRegExp rsb("\\[.*\\]");
	static QRegExp rfb("{.*}");
	static QRegExp rrb("\\(.*\\)");
	static QRegExp stripB("^[()_{}[]!@#$^&*+=|\\\\/\"'?<>~`\\s\\t]*");
	static QRegExp stripE("[()_{}[]!@#$^&*+=|\\\\/\"'?<>~`\\s\\t]*$");
#endif
	QString s(component);

	if (s.isEmpty()) {
		return QString(" ");
	}

	s = s.replace(rsb, "");
	s = s.replace(rfb, "");
	s = s.replace(rrb, "");
	s = s.replace(stripB, "");
	s = s.replace(stripE, "");
	s = s.replace("  ", " ");
	s = s.replace("\t", " ");

	return s.toLower();
}

QString MusicControllerPrivate::findAlbumArt(const QString &artist, const QString &album)
{
	QDir dir(QDir::homePath() + "/.cache/media-art/");
	QByteArray first_hash = QCryptographicHash::hash(stripAlbumArtComponent(artist).toUtf8(),
													 QCryptographicHash::Md5).toHex();
	QByteArray second_hash = QCryptographicHash::hash(stripAlbumArtComponent(album).toUtf8(),
													  QCryptographicHash::Md5).toHex();
    QString file = QString("album-%1-%2.jpeg").arg(first_hash.constData(), second_hash.constData());
	qCDebug(musicControllerCat()) << "checking for albumart in" << file;
	if (dir.exists(file)) {
		return dir.absoluteFilePath(file);
	}

	// Now try with an empty artist name
	first_hash = QCryptographicHash::hash(QString(" ").toUtf8(), QCryptographicHash::Md5).toHex();
    file = QString("album-%1-%2.jpeg").arg(first_hash.constData(), second_hash.constData());
	qCDebug(musicControllerCat()) << "checking for albumart in" << file;
	if (dir.exists(file)) {
		return dir.absoluteFilePath(file);
	}

	return QString();
}

void MusicControllerPrivate::updateStatus()
{
	Q_Q(MusicController);
	QString service = controller->currentService();
	MusicController::Status newStatus;

	if (service.isEmpty()) {
		newStatus =  MusicController::StatusNoPlayer;
	} else {
		switch (controller->playbackStatus()) {
		case Amber::Mpris::Playing:
			newStatus = MusicController::StatusPlaying;
			break;
		case Amber::Mpris::Paused:
			newStatus = MusicController::StatusPaused;
			break;
		default:
			newStatus = MusicController::StatusStopped;
			break;
		}
	}

	if (newStatus != curStatus) {
		curStatus = newStatus;
		emit q->statusChanged();
	}
}

void MusicControllerPrivate::updateAlbumArt()
{
	Q_Q(MusicController);
	QString newAlbumArt = findAlbumArt(curArtist, curAlbum);
	if (newAlbumArt != curAlbumArt) {
		curAlbumArt = newAlbumArt;
		emit q->albumArtChanged();
	}
}

void MusicControllerPrivate::updateMetadata()
{
	Q_Q(MusicController);
	Amber::MprisMetaData *metadata = controller->metaData();
	bool checkAlbumArt = false;

	QString newArtist = metadata->contributingArtist().toString(),
			newAlbum = metadata->albumTitle().toString(),
			newTitle = metadata->title().toString();

	if (newArtist != curArtist) {
		curArtist = newArtist;
		checkAlbumArt = true;
		emit q->artistChanged();
	}

	if (newAlbum != curAlbum) {
		curAlbum = newAlbum;
		checkAlbumArt = true;
		emit q->albumChanged();
	}

	if (newTitle != curTitle) {
		curTitle = newTitle;
		emit q->titleChanged();
	}

	if (checkAlbumArt) {
		updateAlbumArt();
	}

	int newDuration = metadata->duration().toULongLong();
	if (newDuration != curDuration) {
		curDuration = newDuration;
		emit q->durationChanged();
	}

	emit q->metadataChanged();
}

void MusicControllerPrivate::handleCurrentServiceChanged()
{
	Q_Q(MusicController);
	qCDebug(musicControllerCat()) << controller->currentService();
	updateStatus();
	emit q->serviceChanged();
}

void MusicControllerPrivate::handlePlaybackStatusChanged()
{
	qCDebug(musicControllerCat()) << controller->playbackStatus();
	updateStatus();
}

void MusicControllerPrivate::handleMetadataChanged()
{
	updateMetadata();
}

MusicController::MusicController(QObject *parent)
    : QObject(parent), d_ptr(new MusicControllerPrivate(this))
{
}

MusicController::~MusicController()
{

	delete d_ptr;
}


MusicController::Status MusicController::status() const
{
	Q_D(const MusicController);
	return d->curStatus;
}

Amber::MprisMetaData MusicController::metadata() const
{
	Q_D(const MusicController);
	return d->controller->metaData();
}

QString MusicController::service() const
{
	Q_D(const MusicController);
	return d->controller->currentService();
}

QString MusicController::title() const
{
	Q_D(const MusicController);
	return d->curTitle;
}

QString MusicController::album() const
{
	Q_D(const MusicController);
	return d->curAlbum;
}

QString MusicController::artist() const
{
	Q_D(const MusicController);
	return d->curArtist;
}

QString MusicController::albumArt() const
{
	Q_D(const MusicController);
	return d->curAlbumArt;
}

int MusicController::duration() const
{
	Q_D(const MusicController);
	return d->curDuration;
}

MusicController::RepeatStatus MusicController::repeat() const
{
	Q_D(const MusicController);
	switch (d->controller->loopStatus()) {
	case Amber::Mpris::LoopStatus::LoopNone:
	default:
		return RepeatNone;
	case Amber::Mpris::LoopStatus::LoopTrack:
		return RepeatTrack;
	case Amber::Mpris::LoopStatus::LoopPlaylist:
		return RepeatPlaylist;
	}
}

bool MusicController::shuffle() const
{
	Q_D(const MusicController);
	return d->controller->shuffle();
}

void MusicController::play()
{
	Q_D(MusicController);
	d->controller->play();
}

void MusicController::pause()
{
	Q_D(MusicController);
	d->controller->pause();
}

void MusicController::playPause()
{
	Q_D(MusicController);
	d->controller->playPause();
}

void MusicController::next()
{
	Q_D(MusicController);
	d->controller->next();
}

void MusicController::previous()
{
	Q_D(MusicController);
	d->controller->previous();
}

}
