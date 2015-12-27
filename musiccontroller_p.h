#ifndef WATCHFISH_MUSICCONTROLLER_P_H
#define WATCHFISH_MUSICCONTROLLER_P_H

#include <MprisQt/mpris.h>
#include <MprisQt/mprismanager.h>

#include "musiccontroller.h"

namespace watchfish
{

class MusicControllerPrivate : public QObject
{
	Q_OBJECT

public:
	MusicControllerPrivate(MusicController *q);
	~MusicControllerPrivate();

public:
	MprisManager *manager;
	MusicController::Status curStatus;
	QString curTitle;
	QString curAlbum;
	QString curArtist;
	QString curAlbumArt;
	int curDuration;

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

