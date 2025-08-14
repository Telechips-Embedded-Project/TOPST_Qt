#ifndef UTILS_H
#define UTILS_H

// 현재 좌표(lat1, lon1)에서 목표 좌표(lat2, lon2)까지 진행 방향(방위각) 계산
// 반환값: 북쪽 = 0°, 시계방향으로 증가
double calculateBearing(double lat1, double lon1, double lat2, double lon2);

#endif