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

#include "calendarsource.h"

#include <QDateTime>

#ifdef MER_EDITION_SAILFISH
#include "calendarsource_p.h"
#include <KCalendarCore/OccurrenceIterator>
#include <KCalendarCore/Event>
#endif

namespace watchfish
{

Q_LOGGING_CATEGORY(calendarSourceCat, "watchfish-CalendarSource")

#ifdef MER_EDITION_SAILFISH

CalendarSourcePrivate::CalendarSourcePrivate(CalendarSource *q)
#ifdef KF5CALENDARCORE
	: calendar(new mKCal::ExtendedCalendar(QTimeZone::systemTimeZone())),
#else
	: calendar(new mKCal::ExtendedCalendar(KDateTime::Spec::LocalZone())),
#endif
	  calendarStorage(calendar->defaultStorage(calendar)),
	  q_ptr(q)
{
	calendarStorage->registerObserver(this);
	if (!calendarStorage->open()) {
		qCWarning(calendarSourceCat) << "Cannot open calendar database";
	}
}

CalendarSourcePrivate::~CalendarSourcePrivate()
{
	calendarStorage->unregisterObserver(this);
}

void CalendarSourcePrivate::storageModified(mKCal::ExtendedStorage *storage, const QString &info)
{
	Q_Q(CalendarSource);
	Q_UNUSED(storage);
	qCDebug(calendarSourceCat) << "Storage modified:" << info;
	emit q->changed();
}

void CalendarSourcePrivate::storageProgress(mKCal::ExtendedStorage *storage, const QString &info)
{
	Q_UNUSED(storage);
	Q_UNUSED(info);
	// Nothing to do
}

void CalendarSourcePrivate::storageFinished(mKCal::ExtendedStorage *storage, bool error, const QString &info)
{
	Q_UNUSED(storage);
	Q_UNUSED(error);
	Q_UNUSED(info);
	// Nothing to do
}

CalendarEvent CalendarSourcePrivate::convertToEvent(
#ifdef KF5CALENDARCORE
    const KCalendarCore::Incidence::Ptr &incidence
#else
    const KCalCore::Incidence::Ptr &incidence
#endif
, const QDateTime &startDate, const QDateTime &endDate) {
	CalendarEvent event;

	event.setUid(incidence->uid());
    event.setStart(startDate);
    event.setEnd(endDate);
	auto alarmList = incidence->alarms();
	if (!alarmList.empty() && alarmList[0]->enabled()) {
		QDateTime nextAlarm = alarmList[0]->nextRepetition(QDateTime::currentDateTime());
		if (nextAlarm.isValid())
			event.setAlertTime(nextAlarm);
	}
	event.setAllDay(incidence->allDay());
	event.setTitle(incidence->summary());
	event.setLocation(incidence->location());

	return event;
}



CalendarSource::CalendarSource(QObject *parent)
	: QObject(parent), d_ptr(new CalendarSourcePrivate(this))
{
}


CalendarSource::~CalendarSource()
{
	delete d_ptr;
}

QList<CalendarEvent> CalendarSource::fetchEvents(const QDate &start, const QDate &end,
												 bool startInclusive, bool endInclusive)
{
	Q_D(CalendarSource);
	int count;
	count = d->calendarStorage->loadRecurringIncidences();
	qCDebug(calendarSourceCat) << "Loaded" << count << "recurring events";
	count = d->calendarStorage->load(start, end);
	qCDebug(calendarSourceCat) << "Loaded" << count << "normal events";

	QList<CalendarEvent> events;

    KCalendarCore::OccurrenceIterator it(*d->calendar, QDateTime(start), QDateTime(end));
    while (it.hasNext()) {
        it.next();
        if (!d->calendar->isVisible(it.incidence()) || it.incidence()->type() != KCalendarCore::IncidenceBase::TypeEvent) {
            continue;
        }
        events.append(d->convertToEvent(it.incidence(), it.occurrenceStartDate(), it.occurrenceStartDate())); //!TODO end date is newer api
    }

	qCDebug(calendarSourceCat) << "Returning" << events.size() << "events";
	return events;
}

#else
CalendarSource::CalendarSource(QObject *parent)
    : QObject(parent)
{

}

CalendarSource::~CalendarSource()
{
}

QList<CalendarEvent> CalendarSource::fetchEvents(const QDate &start, const QDate &end,
                                                 bool startInclusive, bool endInclusive)
{
    QList<CalendarEvent> events;
    return events;
}
#endif

}
