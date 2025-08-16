#ifndef DB_H
#define DB_H

#pragma once
#include <QString>
#include <QList>
#include <QSqlDatabase>

namespace Db {

struct SettingsRow {
    int aircon = 24;
    int aircon_flag = 0;
    int ambient = 0;        // 0:red,1:yellow,2:green,3:rainbow
    int ambient_flag = 0;
    int window_flag = 0;
    int wiper_flag = 0;
    int wallpaper = 0;      // 0~5
    int wallpaper_flag = 0;
}; // smart momde structure

// Account structure
struct Account {
    int id = -1;        // accounts.iD (PK)
    QString name;       // account name (UNIQUE)
    QString avatar;     // btn image dir(.qrc or file dir)
};

// === 계정 속성 갱신 ===
bool setAccountAvatar(int accountId, const QString& path);

// === dir ===
QString baseDir();                 // 절대 경로: <실행파일>/db/database
QString masterPath();              // <실행파일>/db/database/master.db

// === init ===
bool initDirs();
bool initMaster();                 // accounts 테이블(+avatar 컬럼) 보장

// === Account List/create ===
QList<Account> listAccounts();                                  // read all account
bool createAccount(const QString& name, int* outId = nullptr);  // create ( MAX : 5 )

// === Read Avatar(account) ===
bool getAccount(int id, Account* out);

// === users/<id>.db 경로 ===
// === delete account ===
QString userDbPath(int accountId);
static bool resetAccountsSequenceIfEmpty(QSqlDatabase& db);
bool deleteAccount(int accountId);

bool initUserDb(int accountId);                      // settings 테이블/행 보장
bool loadSettings(int accountId, SettingsRow* out);  // id=1 로드
bool saveSettings(int accountId, const SettingsRow& row); // id=1 저장

} // namespace Db

#endif // DB_H
