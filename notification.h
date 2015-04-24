#ifndef WATCHFISH_NOTIFICATION_H
#define WATCHFISH_NOTIFICATION_H

#include <QtCore/QObject>
#include <QtCore/QDateTime>

namespace watchfish
{

class Notification : public QObject
{
	Q_OBJECT
	Q_PROPERTY(uint id READ id CONSTANT)
	Q_PROPERTY(QString sender READ sender)
	Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)
	Q_PROPERTY(QString body READ body NOTIFY bodyChanged)
	Q_PROPERTY(QDateTime timestamp READ timestamp NOTIFY timestampChanged)
	Q_PROPERTY(QString icon READ icon NOTIFY iconChanged)

	explicit Notification(QObject *parent = 0);

public:
	inline uint id() const { return _id; }
	inline QString sender() const { return _sender; }
	inline QString summary() const { return _summary; }
	inline QString body() const { return _body; }
	inline QDateTime timestamp() const { return _timestamp; }
	inline QString icon() const { return _icon; }

signals:
	void summaryChanged();
	void bodyChanged();
	void timestampChanged();
	void iconChanged();

	void closed(int reason);

private:
	friend class NotificationMonitor;

	uint _id;
	QString _sender;
	QString _summary;
	QString _body;
	QDateTime _timestamp;
	QString _icon;
};

}

#endif // WATCHFISH_NOTIFICATION_H
