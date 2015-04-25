#ifndef NOTIFICATIONMONITOR_H
#define NOTIFICATIONMONITOR_H

#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>

#include "notification.h"

namespace watchfish
{

Q_DECLARE_LOGGING_CATEGORY(notificationMonitorCat)

class NotificationMonitorPrivate;

class NotificationMonitor : public QObject
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(NotificationMonitor)

public:
	explicit NotificationMonitor(QObject *parent = 0);
	~NotificationMonitor();

signals:
	void notification(Notification *n);

private:
	Q_PRIVATE_SLOT(d_func(), void handleBusSocketActivated())
	NotificationMonitorPrivate * const d_ptr;
};

}

#endif // NOTIFICATIONMONITOR_H
