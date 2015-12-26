#ifndef WATCHFISH_MUSICCONTROLLER_P_H
#define WATCHFISH_MUSICCONTROLLER_P_H

#include <QtDBus/QDBusServiceWatcher>
#include <QtDBus/QDBusContext>

#include "musiccontroller.h"

namespace watchfish
{

class MusicControllerPrivate : public QObject, protected QDBusContext
{
	Q_OBJECT

public:
	MusicControllerPrivate(MusicController *q);
	~MusicControllerPrivate();

private:
	MusicController * const q_ptr;
	Q_DECLARE_PUBLIC(MusicController)
};

}

#endif // WATCHFISH_MUSICCONTROLLER_P_H

