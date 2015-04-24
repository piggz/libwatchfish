#include <QtCore/QDebug>
#include <QtCore/QSocketNotifier>
#include <dbus/dbus.h>

#include "notification.h"
#include "notificationmonitor.h"

using namespace watchfish;

namespace
{

NotificationMonitor *global_monitor = 0;

DBusConnection *bus_connection;

QHash<quint32, QVariantHash> pending_confirmation;

dbus_bool_t bus_watch_add(DBusWatch *watch, void *data)
{
	NotificationMonitor *monitor = static_cast<NotificationMonitor*>(data);
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
		qWarning() << "Can't add this type of watch" << flags;
		return FALSE;
	}

	QSocketNotifier *notifier = new QSocketNotifier(socket, type, monitor);
	dbus_watch_set_data(watch, notifier, NULL);

	notifier->setEnabled(dbus_watch_get_enabled(watch));

	notifier->connect(notifier, &QSocketNotifier::activated,
			[watch]() {
		dbus_watch_handle(watch, dbus_watch_get_flags(watch));

		while (dbus_connection_get_dispatch_status(bus_connection) == DBUS_DISPATCH_DATA_REMAINS) {
			dbus_connection_dispatch(bus_connection);
		}
	});

	return TRUE;
}

void bus_watch_remove(DBusWatch *watch, void *data)
{
	QSocketNotifier *notifier = static_cast<QSocketNotifier*>(dbus_watch_get_data(watch));
	Q_UNUSED(data);
	delete notifier;
}

void bus_watch_toggle(DBusWatch *watch, void *data)
{
	QSocketNotifier *notifier = static_cast<QSocketNotifier*>(dbus_watch_get_data(watch));
	Q_UNUSED(data);
	notifier->setEnabled(dbus_watch_get_enabled(watch));
}

QVariantHash parse_notify_call(DBusMessage *msg)
{
	QVariantHash r;
	DBusMessageIter iter, sub;
	const char *app_name, *app_icon, *summary, *body;
	quint32 replaces_id;
	qint32 expire_timeout;

	if (strcmp(dbus_message_get_signature(msg), "susssasa{sv}i") != 0) {
		qWarning() << "Invalid signature";
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

DBusHandlerResult message_filter(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
	NotificationMonitor *monitor = static_cast<NotificationMonitor*>(user_data);
	DBusError error = DBUS_ERROR_INIT;
	Q_UNUSED(conn);
	switch (dbus_message_get_type(msg)) {
	case DBUS_MESSAGE_TYPE_METHOD_CALL:
		if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "Notify")) {
			quint32 serial = dbus_message_get_serial(msg);
			QVariantHash content = parse_notify_call(msg);
			pending_confirmation.insert(serial, content);
		}
		break;
	case DBUS_MESSAGE_TYPE_METHOD_RETURN:
		if (pending_confirmation.contains(dbus_message_get_reply_serial(msg))) {
			quint32 id;
			if (dbus_message_get_args(msg, &error, DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID)) {
				QVariantHash content = pending_confirmation.take(dbus_message_get_reply_serial(msg));
				monitor->processIncomingNotification(id, content);
			} else {
				qWarning() << "Could not parse notification method return";
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
				monitor->processCloseNotification(id, reason);
			} else {
				qWarning() << "Failed to parse notification signal arguments";
			}

		}
		break;
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

void send_message_with_string(const char *service, const char *path, const char *iface, const char *method, const char *arg)
{
	DBusMessage *msg = dbus_message_new_method_call(service, path, iface, method);
	Q_ASSERT(msg);
	dbus_message_set_no_reply(msg, TRUE);
	dbus_message_append_args(msg,
							 DBUS_TYPE_STRING, &arg,
							 DBUS_TYPE_INVALID);
	dbus_connection_send(bus_connection, msg, NULL);
	dbus_message_unref(msg);
}

void add_match_rule(const char *rule)
{
	send_message_with_string("org.freedesktop.DBus", "/",
							 "org.freedesktop.DBus", "AddMatch", rule);
}

void remove_match_rule(const char *rule)
{
	send_message_with_string("org.freedesktop.DBus", "/",
							 "org.freedesktop.DBus", "RemoveMatch", rule);
}

}

NotificationMonitor::NotificationMonitor(QObject *parent) :
	QObject(parent)
{
	Q_ASSERT(!bus_connection);
	DBusError error = DBUS_ERROR_INIT;
	bus_connection = dbus_bus_get_private(DBUS_BUS_SESSION, &error);
	if (!bus_connection) {
		qWarning() << "Could not connect to the session bus";
		return;
	}

	dbus_connection_set_exit_on_disconnect(bus_connection, FALSE);

	dbus_connection_set_watch_functions(bus_connection, bus_watch_add,
										bus_watch_remove, bus_watch_toggle,
										this, NULL);

	add_match_rule("type='method_call',interface='org.freedesktop.Notifications',member='Notify',eavesdrop='true'");
	add_match_rule("type='method_return',sender='org.freedesktop.Notifications',eavesdrop='true'");
	add_match_rule("type='signal',sender='org.freedesktop.Notifications',path='/org/freedesktop/Notifications',interface='org.freedesktop.Notifications',member='NotificationClosed'");

	dbus_bool_t result = dbus_connection_add_filter(bus_connection, message_filter,
													this, NULL);
	if (!result) {
		qWarning() << "Could not add filter";
	}

	qDebug() << "Starting notification monitor";
}

NotificationMonitor::~NotificationMonitor()
{
	Q_ASSERT(bus_connection);

	remove_match_rule("type='method_call',interface='org.freedesktop.Notifications',member='Notify',eavesdrop='true'");
	remove_match_rule("type='method_return',sender='org.freedesktop.Notifications',eavesdrop='true'");
	remove_match_rule("type='signal',sender='org.freedesktop.Notifications',path='/org/freedesktop/Notifications',interface='org.freedesktop.Notifications',member='NotificationClosed'");

	dbus_connection_remove_filter(bus_connection, message_filter, this);

	dbus_connection_close(bus_connection);
	dbus_connection_unref(bus_connection);
	bus_connection = NULL;
}

NotificationMonitor *NotificationMonitor::instance()
{
	if (!global_monitor) {
		global_monitor = new NotificationMonitor;
	}
	return global_monitor;
}

void NotificationMonitor::processIncomingNotification(quint32 id, const QVariantHash &content)
{
	qDebug() << "Incoming notification" << id << content;
	Notification *n = _notifs.value(id, 0);
	if (n) {
		QString s = content["summary"].toString();
		if (n->_summary != s) {
			n->_summary = s;
			emit n->summaryChanged();
		}
		s = content["body"].toString();
		if (n->_body != s) {
			n->_body = s;
			emit n->bodyChanged();
		}
		s = content["icon"].toString();
		if (n->_icon != s) {
			n->_icon = s;
			emit n->iconChanged();
		}
		QDateTime dt = content["timestamp"].toDateTime();
		if (dt.isValid() && n->_timestamp != dt) {
			n->_timestamp = dt;
			emit n->timestampChanged();
		}
	} else {
		n = new Notification(this);
		n->_id = id;
		n->_sender = content["sender"].toString();
		n->_summary = content["summary"].toString();
		n->_body = content["body"].toString();
		n->_timestamp = content["timestamp"].toDateTime();
		n->_icon = content["icon"].toString();

		if (!n->_timestamp.isValid()) {
			n->_timestamp = QDateTime::currentDateTime();
		}

		_notifs.insert(id, n);

		emit notification(n);
	}
}

void NotificationMonitor::processCloseNotification(quint32 id, quint32 reason)
{
	qDebug() << "Close notification" << id << reason;
	Notification *n = _notifs.value(id, 0);
	if (n) {
		_notifs.remove(id);
		emit n->closed(reason);
		n->deleteLater();
	} else {
		qDebug() << " but it is not found";
	}
}
