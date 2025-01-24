#ifndef SOUND_PROFILE__H
#define SOUND_PROFILE__H

#include <QObject>
#include <QtCore/QLoggingCategory>

namespace watchfish
{

Q_DECLARE_LOGGING_CATEGORY(soundProfileCat)


class SoundProfile : public QObject {
    Q_OBJECT

public:
    explicit SoundProfile(QObject *parent = 0);
    ~SoundProfile();

    enum AvailableProfiles {
        Silent,
        General
    };
    Q_ENUM(AvailableProfiles)

    void setProfile(watchfish::SoundProfile::AvailableProfiles p);

    

};

}

#endif // SOUND_PROFILE__H
