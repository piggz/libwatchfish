#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>

#include "soundprofile.h"

namespace watchfish
{

Q_LOGGING_CATEGORY(soundProfileCat, "watchfish-SoundProfile")


SoundProfile::SoundProfile(QObject *parent): QObject(parent) {
}

SoundProfile::~SoundProfile() {
}

void SoundProfile::setProfile(watchfish::SoundProfile::AvailableProfiles p) {
    qCDebug(soundProfileCat) << Q_FUNC_INFO << p;
#if defined(UUITK_EDITION)

    QDBusReply<QDBusObjectPath> findUserByNameReply = QDBusConnection::systemBus().call(
        QDBusMessage::createMethodCall(
            "org.freedesktop.Accounts",
            "/org/freedesktop/Accounts",
            "org.freedesktop.Accounts",
            "FindUserByName"
            ) << "phablet"
        );
    if (!findUserByNameReply.isValid()) {
        qCWarning(soundProfileCat) << "!isValid" << findUserByNameReply.error().message();
        return;
    }

    QDBusInterface iface(
        "org.freedesktop.Accounts",
        findUserByNameReply.value().path(),
        "org.freedesktop.DBus.Properties",
        QDBusConnection::systemBus()
    );

    if (!iface.isValid()) {
        qCWarning(soundProfileCat) << "Failed to initialize DBus interface for user path";
        return;
    }

    QDBusReply<void> setPropertyReply = iface.call(
        "Set",
        "com.lomiri.touch.AccountsService.Sound",
        "SilentMode",
        QVariant::fromValue(QDBusVariant(p == watchfish::SoundProfile::Silent))
    );

    if (!setPropertyReply.isValid()) {
        qCWarning(soundProfileCat) << "Failed to set SilentMode:" << setPropertyReply.error().message();
        return;
    }

#elif defined(MER_EDITION_SAILFISH)

    QString profile;
    switch(p) {
    case watchfish::SoundProfile::General:
        profile = "general";
        break;
    case watchfish::SoundProfile::Silent:
        profile = "silent";
        break;
    }


    QDBusReply<bool> res = QDBusConnection::sessionBus().call(
        QDBusMessage::createMethodCall("com.nokia.profiled", "/com/nokia/profiled", "com.nokia.profiled", "set_profile")
        << profile);
    if (res.isValid()) {
        if (!res.value()) {
            qCWarning(soundProfileCat) << "Unable to set profile" << profile << res.error().message();
        }
    }
    else {
        qCWarning(soundProfileCat) << res.error().message();
    }


#else
    qCWarning(soundProfileCat) << Q_FUNC_INFO << "Not implemented for platform";
#endif

}

} // namespace
