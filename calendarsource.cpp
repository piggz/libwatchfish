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

#elif defined(UUITK_EDITION)

    #include <QDebug>
    #include <QOrganizerEventOccurrence>
    #include <QOrganizerEventTime>
    #include <QOrganizerItem>
    #include <QOrganizerItemAudibleReminder>
    #include <QOrganizerItemDescription>
    #include <QOrganizerItemDetail>
    #include <QOrganizerItemEmailReminder>
    #include <QOrganizerItemFetchRequest>
    #include <QOrganizerItemLocation>
    #include <QOrganizerItemReminder>
    #include <QOrganizerItemType>
    #include <QOrganizerItemVisualReminder>
    #include <QOrganizerManager>

    #define MANAGER           "eds"
    #define MANAGER_FALLBACK  "memory"

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

#elif defined(UUITK_EDITION)
CalendarSource::CalendarSource(QObject *parent)
    : QObject(parent)
{
    QString envManager(qgetenv("ALARM_BACKEND"));
    if (envManager.isEmpty())
        envManager = MANAGER;
    if (!QOrganizerManager::availableManagers().contains(envManager)) {
        envManager = MANAGER_FALLBACK;
    }
    m_manager = new QOrganizerManager(envManager);
    m_manager->setParent(this);

    connect(m_manager, &QOrganizerManager::itemsAdded, this, &CalendarSource::changed);
    connect(m_manager, &QOrganizerManager::itemsRemoved, this, &CalendarSource::changed);
    connect(m_manager, &QOrganizerManager::itemsChanged, this, &CalendarSource::changed);
}

CalendarSource::~CalendarSource()
{
}

QList<CalendarEvent> CalendarSource::fetchEvents(const QDate &start, const QDate &end,
                                                 bool startInclusive, bool endInclusive)
{
    QList<CalendarEvent> events;

    QDateTime startTime = QDateTime(start, QTime(0, 0));
    QDateTime endTime = QDateTime(end, QTime(23, 59, 59));

    QOrganizerItemFetchRequest request;
    request.setManager(m_manager);
    request.setStartDate(startTime);
    request.setEndDate(endTime);
    request.start();
    request.waitForFinished();

//    qDebug() << "Fetch error:" << request.error();
    const QList<QOrganizerItem> items = request.items();
//    qDebug() << "items count:" << items.count();

    for (const QOrganizerItem &item : items) {

        QOrganizerItemType::ItemType type = item.type();
        if (type != QOrganizerItemType::TypeEvent &&
            type != QOrganizerItemType::TypeEventOccurrence) {
            continue;
        }
        QOrganizerEventTime timeDetail = static_cast<QOrganizerEventTime>(
            item.detail(QOrganizerItemDetail::TypeEventTime)
        );

        QOrganizerItemDescription descDetail = static_cast<QOrganizerItemDescription>(
            item.detail(QOrganizerItemDetail::TypeDescription)
        );
        QOrganizerItemLocation locationDetail = static_cast<QOrganizerItemLocation>(
            item.detail(QOrganizerItemDetail::TypeLocation)
        );
        QOrganizerItemReminder reminderDetail = static_cast<QOrganizerItemReminder>(
            item.detail(QOrganizerItemDetail::TypeReminder)
        );

        QOrganizerItemVisualReminder visualReminder = static_cast<QOrganizerItemVisualReminder>(
            item.detail(QOrganizerItemDetail::TypeVisualReminder)
        );
        QOrganizerItemAudibleReminder audibleReminder = static_cast<QOrganizerItemAudibleReminder>(
            item.detail(QOrganizerItemDetail::TypeAudibleReminder)
        );

        CalendarEvent ev;
        ev.setUid(item.guid());
        ev.setStart(timeDetail.startDateTime());
        ev.setEnd(timeDetail.endDateTime());
        ev.setTitle(item.displayLabel());
        ev.setLocation(locationDetail.label());
        ev.setDescription(descDetail.description());
        ev.setAllDay(timeDetail.isAllDay());
        if (!reminderDetail.isEmpty()) {
            ev.setAlertTime(timeDetail.startDateTime().addSecs(-reminderDetail.secondsBeforeStart()));
        }
        if (!visualReminder.isEmpty()) {
            ev.setAlertTime(timeDetail.startDateTime().addSecs(-visualReminder.secondsBeforeStart()));
        }
        if (!audibleReminder.isEmpty()) {
            ev.setAlertTime(timeDetail.startDateTime().addSecs(-audibleReminder.secondsBeforeStart()));
        }
        events.append(ev);
    }

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
