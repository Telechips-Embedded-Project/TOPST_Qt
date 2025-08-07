#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#include <stdbool.h>

#define SHM_KEY 0x8888

typedef struct {
    int brightness_level;   // 0: off, 1: low, 2: mid, 3: high
    char color[16];         // "red", "yellow", etc.
    bool error_flag;        // LED 제어 실패 여부 등
} ambient_state_t;

typedef struct {
    int level;              // 0: off, 1: low, 2: mid, 3: high, 4: fast 등
    // bool is_on;          // 삭제 예정
    bool error_flag;        // 장치 에러 여부(보류)
} device_state_t;

typedef struct {
    float co2;
    float temperature;
    float humidity;
    float lux;
    float rain;
} sensor_state_t;

typedef struct {
    ambient_state_t ambient;
    device_state_t aircon;
    device_state_t window;
    device_state_t headlamp;
    device_state_t wiper;
    char music_file[64];    // 현재 재생 중인 음악 파일명
    sensor_state_t sensor;
} system_status_t;

#endif