#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include "accountsettings.h"
#include <QString>

class AccountManager
{
public:
    static AccountSettings load(const QString &accountName);
};

#endif // ACCOUNTMANAGER_H
