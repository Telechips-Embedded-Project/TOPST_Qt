#include "db.h"

#include "system_status.h"

#include <QDir>
#include <QCoreApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QFile>

namespace {
constexpr int kMaxAccounts = 5; // MAX account : 5

// QSqlDatabase connection name (같은 이름이면 동일 연결 재사용)
QString masterConnName() { return QStringLiteral("master"); }
QString userConnName(int id) { return QStringLiteral("user_%1").arg(id); }

// SQLite 파일을 열고(또는 이미 열린 연결을 재사용) outDb에 반환하는 헬퍼
// - path: 열고 싶은 .db 파일 절대경로
// - connName: Qt 내부 커넥션 이름(동일 이름/경로면 재사용)
// return: 성공 시 true, 실패 시 false
bool ensureOpenSqlite(const QString& path, const QString& connName, QSqlDatabase* outDb) {
    // 1) 같은 이름의 연결이 이미 등록되어 있으면 재사용 시도
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        // 같은 파일(path)로 열려 있다면 그대로 반환
        if (db.isValid() && db.isOpen() && db.databaseName() == path) { *outDb = db; return true; }
        // 다른 파일이었다면 정리 후 새로 염
        db.close(); QSqlDatabase::removeDatabase(connName);
    }

    // 2) 새 연결 생성 (QSQLITE)
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(path);
    if (!db.open()) { qWarning() << "[DB] open failed:" << path << db.lastError().text(); return false; }

    /*
    // 가벼운 튜닝(선택)
    {
        QSqlQuery q(db);
        q.exec("PRAGMA synchronous=NORMAL");
        q.exec("PRAGMA journal_mode=WAL"); // 이 설정 때문에 .db-wal/.db-shm 파일이 같이 생김
    }
    */

    *outDb = db;
    return true;
}

// 간단한 SQL(DDL/단문)을 실행하는 유틸 — 실패 시 로그 남기고 false 반환
bool execSimple(QSqlDatabase& db, const char* sql) {
    QSqlQuery q(db);
    if (!q.exec(QString::fromUtf8(sql))) {
        qWarning() << "[DB] SQL failed:" << sql << q.lastError().text();
        return false;
    }
    return true;
}

} // namespace

namespace Db {



// base dir
QString baseDir()
{
    // db 디렉터리 지정
    QDir dir(QCoreApplication::applicationDirPath() + "/db");
    return dir.filePath("database");
}
// make master.db in dir(baseDir)
QString masterPath() {
    return baseDir() + "/master.db";
}

QString userDbPath(int accountId) {
    return baseDir() + QString("/users/%1.db").arg(accountId);
}

// mkpath ( database/ , database/users/ )
bool initDirs() {
    QDir d; bool ok = d.mkpath(baseDir()); ok &= d.mkpath(baseDir() + "/users"); return ok;
}

// master.db 초기화: 폴더 보장 + DB 연결 + accounts 테이블(avatar 포함) 보장
// 또한, 기존 DB에서 avatar 컬럼이 없으면 ALTER TABLE로 추가(마이그레이션)
bool initMaster() {
    if (!initDirs()) return false;

    QSqlDatabase db;
    if (!ensureOpenSqlite(masterPath(), masterConnName(), &db)) return false;

    // accounts 테이블 스키마 정의 (id/name/timestamps/avatar)
    const char* createTbl =
        "CREATE TABLE IF NOT EXISTS accounts ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " name TEXT UNIQUE NOT NULL,"
        " created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        " last_login TEXT,"
        " avatar TEXT"
        ")";
    if (!execSimple(db, createTbl)) return false;

    // 기존 DB 마이그레이션: 오래된 DB에서 avatar 컬럼이 비어 있으면 추가
    {
        QSqlQuery qi(db);
        bool hasAvatar = false;
        if (qi.exec("PRAGMA table_info(accounts)")) {
            while (qi.next()) {
                // PRAGMA table_info 반환 형식: (cid, name, type, notnull, dflt_value, pk)
                if (qi.value(1).toString() == "avatar") { hasAvatar = true; break; }
            }
        }
        if (!hasAvatar) {
            QSqlQuery q(db);
            if (!q.exec("ALTER TABLE accounts ADD COLUMN avatar TEXT")) {
                qWarning() << "[DB] add avatar column failed:" << q.lastError().text();
            }
        }
    }

    return true;
}

// accounts 테이블에서 모든 계정을 읽어 UI 복원에 사용
QList<Account> listAccounts() {
    QList<Account> out;
    QSqlDatabase db;
    if (!ensureOpenSqlite(masterPath(), masterConnName(), &db)) return out;

    // avatar까지 읽어온다
    QSqlQuery q(db);
    // avatar까지 함께 읽어와야 버튼 이미지가 재실행 시에도 고정
    if (!q.exec("SELECT id, name, IFNULL(avatar,'') FROM accounts ORDER BY id ASC")) {
        qWarning() << "[DB] listAccounts failed:" << q.lastError().text();
        return out;
    }
    while (q.next()) {
        Account a;
        a.id     = q.value(0).toInt();
        a.name   = q.value(1).toString();
        a.avatar = q.value(2).toString();
        out.push_back(a);
    }
    return out;
}

// 생성 직후(또는 최초 복원 시 1회)에 avatar 경로를 고정 저장
bool setAccountAvatar(int accountId, const QString& path) {
    if (accountId <= 0) return false;
    QSqlDatabase db;
    if (!ensureOpenSqlite(masterPath(), masterConnName(), &db)) return false;

    QSqlQuery q(db);
    q.prepare("UPDATE accounts SET avatar=? WHERE id=?");
    q.addBindValue(path);
    q.addBindValue(accountId);
    if (!q.exec()) {
        qWarning() << "[DB] set avatar failed:" << q.lastError().text();
        return false;
    }
    return true;
}

// 새 계정 생성: 1) 5개 제한 확인 → 2) INSERT → 3) outId 반환
bool createAccount(const QString& name, int* outId) {
    if (name.trimmed().isEmpty()) return false; // 빈 이름 방지

    // 1) 현재 계정 수 조회 — 최대 개수 제한: 5
    QSqlDatabase db;
    if (!ensureOpenSqlite(masterPath(), masterConnName(), &db)) return false;

    QSqlQuery qc(db);
    if (!qc.exec("SELECT COUNT(*) FROM accounts")) return false;
    int cnt = 0; if (qc.next()) cnt = qc.value(0).toInt();
    if (cnt >= kMaxAccounts) { qWarning() << "[DB] max accounts reached"; return false; }

    // 2) INSERT 실행
    QSqlQuery q(db);
    q.prepare("INSERT INTO accounts(name, created_at) VALUES(?, CURRENT_TIMESTAMP)");
    q.addBindValue(name.trimmed());
    if (!q.exec()) { qWarning() << "[DB] insert account failed:" << q.lastError().text(); return false; }

    // 3) 새 계정 id 획득
    int id = q.lastInsertId().toInt();
    if (outId) *outId = id;

    initUserDb(id);
    return true;
}

// Helper : Read account (avatar) by id from DB
bool getAccount(int id, Account* out)
{
    if (!out || id <= 0) return false;
    QSqlDatabase db;
    if (!ensureOpenSqlite(masterPath(), masterConnName(), &db)) return false;

    QSqlQuery q(db);
    q.prepare("SELECT id, name, IFNULL(avatar,'') FROM accounts WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) return false;
    if (!q.next())  return false;

    out->id     = q.value(0).toInt();
    out->name   = q.value(1).toString();
    out->avatar = q.value(2).toString();
    return true;
}

static bool resetAccountsSequenceIfEmpty(QSqlDatabase& db) {
    QSqlQuery qc(db);
    if (!qc.exec("SELECT COUNT(*) FROM accounts")) return false;
    int cnt = 0; if (qc.next()) cnt = qc.value(0).toInt();
    if (cnt == 0) {
        QSqlQuery q(db);
        if (!q.exec("DELETE FROM sqlite_sequence WHERE name='accounts'")) return false;
        // 선택: q.exec("VACUUM");
    }
    return true;
}

bool deleteAccount(int accountId)
{
    if (accountId <= 0) return false;

    // 1) master에서 계정 삭제
    QSqlDatabase db;
    if (!ensureOpenSqlite(masterPath(), masterConnName(), &db)) return false;

    QSqlQuery q(db);
    q.prepare("DELETE FROM accounts WHERE id=?");
    q.addBindValue(accountId);
    if (!q.exec()) {
        qWarning() << "[DB] delete account failed:" << q.lastError().text();
        return false;
    }

    // 2) user DB 파일 삭제
    if (QSqlDatabase::contains(userConnName(accountId))) {
        QSqlDatabase udb = QSqlDatabase::database(userConnName(accountId));
        if (udb.isOpen()) udb.close();
        QSqlDatabase::removeDatabase(userConnName(accountId));
    }
    QFile::remove(userDbPath(accountId));

    resetAccountsSequenceIfEmpty(db);
    return true;
}

// Settings_page를 붙일 때 여기서 settings 테이블을 만들고 id=1 행을 보장한다.
bool initUserDb(int accountId) {
    QSqlDatabase db;
    if (!ensureOpenSqlite(userDbPath(accountId), userConnName(accountId), &db))
        return false;

    const char* ddl =
        "CREATE TABLE IF NOT EXISTS settings ("
        " id INTEGER PRIMARY KEY,"
        " aircon INTEGER, aircon_flag INTEGER,"
        " ambient INTEGER, ambient_flag INTEGER,"
        " window_flag INTEGER, wiper_flag INTEGER,"
        " wallpaper INTEGER, wallpaper_flag INTEGER"
        ")";
    if (!execSimple(db, ddl)) return false;

    // id=1 보장
    QSqlQuery q(db);
    if (!q.exec("SELECT COUNT(*) FROM settings WHERE id=1")) return false;
    if (q.next() && q.value(0).toInt() == 0) {
        QSqlQuery ins(db);
        ins.prepare("INSERT INTO settings "
                    "(id,aircon,aircon_flag,ambient,ambient_flag,window_flag,wiper_flag,wallpaper,wallpaper_flag) "
                    "VALUES (1,24,0,0,0,0,0,0,0)");
        if (!ins.exec()) return false;
    }
    return true;
}

bool loadSettings(int accountId, SettingsRow* r) {
    if (!r) return false;
    QSqlDatabase db;
    if (!ensureOpenSqlite(userDbPath(accountId), userConnName(accountId), &db))
        return false;
    QSqlQuery q(db);
    if (!q.exec("SELECT aircon,aircon_flag,ambient,ambient_flag,window_flag,wiper_flag,wallpaper,wallpaper_flag "
                "FROM settings WHERE id=1")) return false;
    if (!q.next()) return false;
    r->aircon         = q.value(0).toInt();
    r->aircon_flag    = q.value(1).toInt();
    r->ambient        = q.value(2).toInt();
    r->ambient_flag   = q.value(3).toInt();
    r->window_flag    = q.value(4).toInt();
    r->wiper_flag     = q.value(5).toInt();
    r->wallpaper      = q.value(6).toInt();
    r->wallpaper_flag = q.value(7).toInt();
    return true;
}

bool saveSettings(int accountId, const SettingsRow& r) {
    QSqlDatabase db;
    if (!ensureOpenSqlite(userDbPath(accountId), userConnName(accountId), &db))
        return false;
    QSqlQuery u(db);
    u.prepare("UPDATE settings SET "
              "aircon=?, aircon_flag=?, ambient=?, ambient_flag=?, "
              "window_flag=?, wiper_flag=?, wallpaper=?, wallpaper_flag=? "
              "WHERE id=1");
    u.addBindValue(r.aircon);
    u.addBindValue(r.aircon_flag);
    u.addBindValue(r.ambient);
    u.addBindValue(r.ambient_flag);
    u.addBindValue(r.window_flag);
    u.addBindValue(r.wiper_flag);
    u.addBindValue(r.wallpaper);
    u.addBindValue(r.wallpaper_flag);
    return u.exec();
}

} // namespace Db

