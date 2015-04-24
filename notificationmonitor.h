#ifndef NOTIFICATIONMONITOR_H
#define NOTIFICATIONMONITOR_H

#include <QtCore/QObject>
#include <QtCore/QMap>

namespace watchfish
{

class Notification;

class NotificationMonitor : public QObject
{
	Q_OBJECT

public:
	~NotificationMonitor();

	static NotificationMonitor *instance();

	void processIncomingNotification(quint32 id, const QVariantHash &content);
	void processCloseNotification(quint32 id, quint32 reason);

signals:
	void notification(Notification *n);

private:
	explicit NotificationMonitor(QObject *parent = 0);
	QMap<quint32, Notification*> _notifs;
};

}

#endif // NOTIFICATIONMONITOR_H
