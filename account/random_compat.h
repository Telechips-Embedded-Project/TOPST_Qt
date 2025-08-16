#ifndef RANDOM_COMPAT_H
#define RANDOM_COMPAT_H

#pragma once
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
  #include <QRandomGenerator>
  inline double rand01() { return QRandomGenerator::global()->generateDouble(); }
  inline quint32 rand32() { return QRandomGenerator::global()->generate(); }
#else
  #include <QDateTime>
  #include <cstdlib>
  inline void ensureSeed() {
    static bool seeded = false;
    if (!seeded) {
      qsrand(uint(QDateTime::currentMSecsSinceEpoch() & 0xffffffff));
      seeded = true;
    }
  }
  inline double rand01() { ensureSeed(); return double(qrand()) / double(RAND_MAX); }
  inline quint32 rand32() { ensureSeed(); return quint32(qrand()); }
#endif

#endif // RANDOM_COMPAT_H
