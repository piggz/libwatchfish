#include "notification.h"

namespace watchfish
{

struct NotificationPrivate
{
	uint id;
	QString sender;
	QString summary;
	QString body;
	QDateTime timestamp;
	QString icon;
};

Notification::Notification(uint id, QObject *parent) : QObject(parent), d_ptr(new NotificationPrivate)
{
	Q_D(Notification);
	d->id = id;
}

Notification::~Notification()
{
}

uint Notification::id() const
{
	Q_D(const Notification);
	return d->id;
}

QString Notification::sender() const
{
	Q_D(const Notification);
	return d->sender;
}

void Notification::setSender(const QString &sender)
{
	Q_D(Notification);
	if (sender != d->sender) {
		d->sender = sender;
		emit senderChanged();
	}
}

QString Notification::summary() const
{
	Q_D(const Notification);
	return d->summary;
}

void Notification::setSummary(const QString &summary)
{
	Q_D(Notification);
	if (summary != d->summary) {
		d->summary = summary;
		emit summaryChanged();
	}
}

QString Notification::body() const
{
	Q_D(const Notification);
	return d->body;
}

void Notification::setBody(const QString &body)
{
	Q_D(Notification);
	if (body != d->body) {
		d->body = body;
		emit bodyChanged();
	}
}

QDateTime Notification::timestamp() const
{
	Q_D(const Notification);
	return d->timestamp;
}

void Notification::setTimestamp(const QDateTime &dt)
{
	Q_D(Notification);
	if (dt != d->timestamp) {
		d->timestamp = dt;
		emit timestampChanged();
	}
}

QString Notification::icon() const
{
	Q_D(const Notification);
	return d->icon;
}

void Notification::setIcon(const QString &icon)
{
	Q_D(Notification);
	if (icon != d->icon) {
		d->icon = icon;
		emit iconChanged();
	}
}

}
