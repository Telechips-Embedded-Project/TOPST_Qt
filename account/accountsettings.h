#ifndef ACCOUNTSETTINGS_H
#define ACCOUNTSETTINGS_H

#include <QString>

struct AccountSettings {
    QString username;
    QString theme;
    QString preferredPage;
    QString ambientColor;
    QString airconLevel;
    int volume = 50;
};

#endif // ACCOUNTSETTINGS_H
