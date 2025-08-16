#include "navi_utils.h"
#include <QtMath>

double calculateBearing(double lat1, double lon1, double lat2, double lon2)
{
    double lat1Rad = qDegreesToRadians(lat1);
    double lat2Rad = qDegreesToRadians(lat2);
    double deltaLonRad = qDegreesToRadians(lon2 - lon1);

    double y = sin(deltaLonRad) * cos(lat2Rad);
    double x = cos(lat1Rad) * sin(lat2Rad)
             - sin(lat1Rad) * cos(lat2Rad) * cos(deltaLonRad);

    double bearingRad = atan2(y, x);
    double bearingDeg = qRadiansToDegrees(bearingRad);

    // 0 ~ 360 범위로 보정
    return fmod(bearingDeg + 360.0, 360.0);
}
