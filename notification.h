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

#ifndef WATCHFISH_NOTIFICATION_H
#define WATCHFISH_NOTIFICATION_H

#include <QtCore/QObject>
#include <QtCore/QDateTime>

namespace watchfish
{

class NotificationPrivate;

class Notification : public QObject
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(Notification)

	/** Notification ID */
	Q_PROPERTY(uint id READ id CONSTANT)
	/** Name of sender program */
	Q_PROPERTY(QString sender READ sender WRITE setSender NOTIFY senderChanged)
	Q_PROPERTY(QString summary READ summary WRITE setSummary NOTIFY summaryChanged)
	Q_PROPERTY(QString body READ body WRITE setBody NOTIFY bodyChanged)
	Q_PROPERTY(QDateTime timestamp READ timestamp WRITE setTimestamp NOTIFY timestampChanged)
	/** Icon file path */
	Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)
	Q_ENUMS(CloseReason)

public:
	explicit Notification(uint id, QObject *parent = 0);
	~Notification();

	enum CloseReason {
		Expired = 1,
		DismissedByUser = 2,
		DismissedByProgram = 3,
		ClosedOther = 4
	};

	uint id() const;

	QString sender() const;
	void setSender(const QString &sender);

	QString summary() const;
	void setSummary(const QString &summary);

	QString body() const;
	void setBody(const QString &body);

	QDateTime timestamp() const;
	void setTimestamp(const QDateTime &dt);

	QString icon() const;
	void setIcon(const QString &icon);

signals:
	void senderChanged();
	void summaryChanged();
	void bodyChanged();
	void timestampChanged();
	void iconChanged();

	void closed(CloseReason reason);

private:
	NotificationPrivate * const d_ptr;
};

}

#endif // WATCHFISH_NOTIFICATION_H
