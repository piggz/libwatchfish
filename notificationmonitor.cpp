#include <QtCore/QMessageLogger>
#include <QtCore/QSocketNotifier>
#include <dbus/dbus.h>

#include "notification.h"
#include "notificationmonitor.h"

namespace watchfish
{

Q_LOGGING_CATEGORY(notificationMonitorCat, "watchfish-NotificationMonitor")

class NotificationMonitorPrivate
{
	NotificationMonitor * const q_ptr;
	Q_DECLARE_PUBLIC(NotificationMonitor)

	/** The current set of monitored notifications, indexed by id. */
	QMap<quint32, Notification*> _notifs;
	/** Low level dbus connection used for sniffing. */
	DBusConnection *_conn;
	/** Serials of DBUS method calls of which we are expecting a reply. */
	QHash<quint32, QVariantHash> _pending_confirmation;

	NotificationMonitorPrivate(NotificationMonitor *q);
	~NotificationMonitorPrivate();

	void processIncomingNotification(quint32 id, const QVariantHash &content);
	void processCloseNotification(quint32 id, quint32 reason);

	void sendMessageWithString(const char *service, const char *path, const char *iface, const char *method, const char *arg);
	void addMatchRule(const char *rule);
	void removeMatchRule(const char *rule);

	QVariantHash parseNotifyCall(DBusMessage *msg) const;

	static dbus_bool_t busWatchAdd(DBusWatch *watch, void *data);
	static void busWatchRemove(DBusWatch *watch, void *data);
	static void busWatchToggle(DBusWatch *watch, void *data);

	static DBusHandlerResult busMessageFilter(DBusConnection *conn, DBusMessage *msg, void *user_data);

	void handleBusSocketActivated();
};

NotificationMonitorPrivate::NotificationMonitorPrivate(NotificationMonitor *q)
	: q_ptr(q)
{
	DBusError error = DBUS_ERROR_INIT;
	_conn = dbus_bus_get_private(DBUS_BUS_SESSION, &error);
	if (!_conn) {
		qCWarning(notificationMonitorCat) << "Could not connect to the session bus";
		return;
	}

	dbus_connection_set_exit_on_disconnect(_conn, FALSE);

	dbus_connection_set_watch_functions(_conn, busWatchAdd, busWatchRemove,
										busWatchToggle, this, NULL);

	addMatchRule("type='method_call',interface='org.freedesktop.Notifications',member='Notify',eavesdrop='true'");
	addMatchRule("type='method_return',sender='org.freedesktop.Notifications',eavesdrop='true'");
	addMatchRule("type='signal',sender='org.freedesktop.Notifications',path='/org/freedesktop/Notifications',interface='org.freedesktop.Notifications',member='NotificationClosed'");

	dbus_bool_t result = dbus_connection_add_filter(_conn, busMessageFilter,
													this, NULL);
	if (!result) {
		qCWarning(notificationMonitorCat) << "Could not add filter";
	}

	qCDebug(notificationMonitorCat) << "Starting notification monitor";
}

NotificationMonitorPrivate::~NotificationMonitorPrivate()
{
	QMap<quint32, Notification*>::iterator it = _notifs.begin();
	while (it != _notifs.end()) {
		delete it.value();
	}

	removeMatchRule("type='method_call',interface='org.freedesktop.Notifications',member='Notify',eavesdrop='true'");
	removeMatchRule("type='method_return',sender='org.freedesktop.Notifications',eavesdrop='true'");
	removeMatchRule("type='signal',sender='org.freedesktop.Notifications',path='/org/freedesktop/Notifications',interface='org.freedesktop.Notifications',member='NotificationClosed'");

	dbus_connection_remove_filter(_conn, busMessageFilter, this);

	dbus_connection_close(_conn);
	dbus_connection_unref(_conn);
}

void NotificationMonitorPrivate::processIncomingNotification(quint32 id, const QVariantHash &content)
{
	Q_Q(NotificationMonitor);
	qCDebug(notificationMonitorCat) << "Incoming notification" << id << content;

	Notification *n = _notifs.value(id, 0);

	bool is_new_notification = !n;
	if (is_new_notification) {
		n = new Notification(id, q);
	}

	n->setSender(content["sender"].toString());
	n->setSummary(content["summary"].toString());
	n->setBody(content["body"].toString());
	n->setIcon(content["icon"].toString());

	QDateTime timestamp = content["timestamp"].toDateTime();
	if (timestamp.isValid()) {
		n->setTimestamp(timestamp);
	} else if (is_new_notification) {
		n->setTimestamp(QDateTime::currentDateTime());
	}

	if (is_new_notification) {
		_notifs.insert(id, n);
		emit q->notification(n);
	}
}

void NotificationMonitorPrivate::processCloseNotification(quint32 id, quint32 reason)
{
	qCDebug(notificationMonitorCat) << "Close notification" << id << reason;
	Notification *n = _notifs.value(id, 0);
	if (n) {
		_notifs.remove(id);
		emit n->closed(static_cast<Notification::CloseReason>(reason));
		n->deleteLater();
	} else {
		qCDebug(notificationMonitorCat) << " but it is not found";
	}
}

void NotificationMonitorPrivate::sendMessageWithString(const char *service, const char *path, const char *iface, const char *method, const char *arg)
{
	DBusMessage *msg = dbus_message_new_method_call(service, path, iface, method);
	Q_ASSERT(msg);
	dbus_message_set_no_reply(msg, TRUE);
	dbus_message_append_args(msg,
							 DBUS_TYPE_STRING, &arg,
							 DBUS_TYPE_INVALID);
	dbus_connection_send(_conn, msg, NULL);
	dbus_message_unref(msg);
}

void NotificationMonitorPrivate::addMatchRule(const char *rule)
{
	sendMessageWithString("org.freedesktop.DBus", "/",
						  "org.freedesktop.DBus", "AddMatch", rule);
}

void NotificationMonitorPrivate::removeMatchRule(const char *rule)
{
	sendMessageWithString("org.freedesktop.DBus", "/",
						  "org.freedesktop.DBus", "RemoveMatch", rule);
}

QVariantHash NotificationMonitorPrivate::parseNotifyCall(DBusMessage *msg) const
{
	QVariantHash r;
	DBusMessageIter iter, sub;
	const char *app_name, *app_icon, *summary, *body;
	quint32 replaces_id;
	qint32 expire_timeout;

	if (strcmp(dbus_message_get_signature(msg), "susssasa{sv}i") != 0) {
		qCWarning(notificationMonitorCat) << "Invalid signature";
		return r;
	}

	dbus_message_iter_init(msg, &iter);
	Q_ASSERT(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING);
	dbus_message_iter_get_basic(&iter, &app_name);
	dbus_message_iter_next(&iter);
	Q_ASSERT(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32);
	dbus_message_iter_get_basic(&iter, &replaces_id);
	dbus_message_iter_next(&iter);
	Q_ASSERT(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING);
	dbus_message_iter_get_basic(&iter, &app_icon);
	dbus_message_iter_next(&iter);
	dbus_message_iter_get_basic(&iter, &summary);
	dbus_message_iter_next(&iter);
	dbus_message_iter_get_basic(&iter, &body);
	dbus_message_iter_next(&iter);

	QStringList actions;
	dbus_message_iter_recurse(&iter, &sub);
	while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING) {
		const char *action;
		dbus_message_iter_get_basic(&sub, &action);
		actions.append(QString::fromUtf8(action));
		dbus_message_iter_next(&sub);
	}
	r.insert("actions", QVariant::fromValue(actions));
	dbus_message_iter_next(&iter);

	dbus_message_iter_recurse(&iter, &sub);
	while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_DICT_ENTRY) {
		DBusMessageIter entry, value;
		const char *key;

		dbus_message_iter_recurse(&sub, &entry);
		dbus_message_iter_get_basic(&entry, &key);
		dbus_message_iter_next(&entry);

		dbus_message_iter_recurse(&entry, &value);
		if (strcmp(key, "category") == 0 && dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_STRING) {
			const char *s;
			dbus_message_iter_get_basic(&value, &s);
			r.insert("category", QString::fromUtf8(s));
		} else if (strcmp(key, "x-nemo-timestamp") == 0 && dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_STRING) {
			const char *s;
			dbus_message_iter_get_basic(&value, &s);
			r.insert("timestamp", QDateTime::fromString(QString::fromUtf8(s), Qt::ISODate));
		}

		dbus_message_iter_next(&sub);
	}

	dbus_message_iter_next(&iter);
	Q_ASSERT(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INT32);
	dbus_message_iter_get_basic(&iter, &expire_timeout);

	r.insert("sender", QString::fromUtf8(app_name));
	r.insert("app_icon", QString::fromUtf8(app_icon));
	r.insert("summary", QString::fromUtf8(summary));
	r.insert("body", QString::fromUtf8(body));

	if (strlen(app_icon) > 0) {
		r.insert("icon", QString::fromLocal8Bit(app_icon));
	}

	return r;
}

dbus_bool_t NotificationMonitorPrivate::busWatchAdd(DBusWatch *watch, void *data)
{
	NotificationMonitorPrivate *self = static_cast<NotificationMonitorPrivate*>(data);
	NotificationMonitor *monitor = self->q_func();
	int socket = dbus_watch_get_socket(watch);
	int flags = dbus_watch_get_flags(watch);

	QSocketNotifier::Type type;
	switch (flags) {
	case DBUS_WATCH_READABLE:
		type = QSocketNotifier::Read;
		break;
	case DBUS_WATCH_WRITABLE:
		type = QSocketNotifier::Write;
		break;
	default:
		qCWarning(notificationMonitorCat) << "Can't add this type of watch" << flags;
		return FALSE;
	}

	QSocketNotifier *notifier = new QSocketNotifier(socket, type, monitor);
	dbus_watch_set_data(watch, notifier, NULL);

	notifier->setEnabled(dbus_watch_get_enabled(watch));
	notifier->setProperty("dbus-watch", QVariant::fromValue<void*>(watch));

	notifier->connect(notifier, SIGNAL(activated(int)),
					  monitor, SLOT(handleBusSocketActivated()));

	return TRUE;
}

void NotificationMonitorPrivate::busWatchRemove(DBusWatch *watch, void *data)
{
	QSocketNotifier *notifier = static_cast<QSocketNotifier*>(dbus_watch_get_data(watch));
	Q_UNUSED(data);
	delete notifier;
}

void NotificationMonitorPrivate::busWatchToggle(DBusWatch *watch, void *data)
{
	QSocketNotifier *notifier = static_cast<QSocketNotifier*>(dbus_watch_get_data(watch));
	Q_UNUSED(data);
	notifier->setEnabled(dbus_watch_get_enabled(watch));
}

DBusHandlerResult NotificationMonitorPrivate::busMessageFilter(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
	NotificationMonitorPrivate *self = static_cast<NotificationMonitorPrivate*>(user_data);
	DBusError error = DBUS_ERROR_INIT;
	Q_UNUSED(conn);
	switch (dbus_message_get_type(msg)) {
	case DBUS_MESSAGE_TYPE_METHOD_CALL:
		if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "Notify")) {
			quint32 serial = dbus_message_get_serial(msg);
			QVariantHash content = self->parseNotifyCall(msg);
			self->_pending_confirmation.insert(serial, content);
		}
		break;
	case DBUS_MESSAGE_TYPE_METHOD_RETURN:
		if (self->_pending_confirmation.contains(dbus_message_get_reply_serial(msg))) {
			quint32 id;
			if (dbus_message_get_args(msg, &error, DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID)) {
				QVariantHash content = self->_pending_confirmation.take(dbus_message_get_reply_serial(msg));
				self->processIncomingNotification(id, content);
			} else {
				qCWarning(notificationMonitorCat) << "Could not parse notification method return";
			}
		}
		break;
	case DBUS_MESSAGE_TYPE_SIGNAL:
		if (dbus_message_is_signal(msg, "org.freedesktop.Notifications", "NotificationClosed")) {
			quint32 id, reason;
			if (dbus_message_get_args(msg, &error,
									  DBUS_TYPE_UINT32, &id,
									  DBUS_TYPE_UINT32, &reason,
									  DBUS_TYPE_INVALID)) {
				self->processCloseNotification(id, reason);
			} else {
				qCWarning(notificationMonitorCat) << "Failed to parse notification signal arguments";
			}

		}
		break;
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

void NotificationMonitorPrivate::handleBusSocketActivated()
{
	Q_Q(NotificationMonitor);
	QSocketNotifier *notifier = static_cast<QSocketNotifier*>(q->sender());
	DBusWatch *watch = static_cast<DBusWatch*>(notifier->property("dbus-watch").value<void*>());

	dbus_watch_handle(watch, dbus_watch_get_flags(watch));

	while (dbus_connection_get_dispatch_status(_conn) == DBUS_DISPATCH_DATA_REMAINS) {
		dbus_connection_dispatch(_conn);
	}
}

NotificationMonitor::NotificationMonitor(QObject *parent) :
	QObject(parent), d_ptr(new NotificationMonitorPrivate(this))
{

}

NotificationMonitor::~NotificationMonitor()
{
	delete d_ptr;
}

}

#include "moc_notificationmonitor.cpp"
