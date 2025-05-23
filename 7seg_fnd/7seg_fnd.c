#include <wiringPi.h>    // wiringPi 라이브러리 헤더 포함
#include <softTone.h>    // softTone 함수 사용을 위한 헤더
#include <stdio.h>       // printf 함수 사용을 위한 헤더
#include <stdlib.h>      // exit 함수 사용을 위한 헤더
#include <pthread.h>     // pthread용 헤더 (뮤텍스 사용)

// 7447 드라이버의 BCD 입력 핀(GPIO 번호) 배열
int gpiopins[4] = {2, 0, 16, 15};    // {A, B, C, D} 비트 순서
int spkr = 26;                       // 스피커 제어 핀

// 0~9까지의 BCD 비트 매핑 (A, B, C, D 순서)
int numberMap[10][4] = {
    {0,0,0,0},  // 0 표시
    {0,0,0,1},  // 1 표시
    {0,0,1,0},  // 2 표시
    {0,0,1,1},  // 3 표시
    {0,1,0,0},  // 4 표시
    {0,1,0,1},  // 5 표시
    {0,1,1,0},  // 6 표시
    {0,1,1,1},  // 7 표시
    {1,0,0,0},  // 8 표시
    {1,0,0,1}   // 9 표시
};

// 뮤텍스 전역 선언 및 초기화
static pthread_mutex_t gpio_mutex = PTHREAD_MUTEX_INITIALIZER;  // GPIO/스피커 동기화용

// GPIO 및 스피커 초기화: wiringPi 모드 설정 및 핀 모드 출력으로 설정
void initFND() {
    wiringPiSetup();                    // wiringPi 모드 설정
    softToneCreate(spkr);               // 스피커 톤 출력을 위한 설정
    for (int i = 0; i < 4; i++) {
        pinMode(gpiopins[i], OUTPUT);   // 각 BCD 입력 핀을 출력 모드로 설정
    }
}

// 주어진 숫자를 7세그먼트에 표시 (뮤텍스로 보호)
void displayNumber(int num) {
    pthread_mutex_lock(&gpio_mutex);          // 뮤텍스 잠금
    for (int i = 0; i < 4; i++) {
        // BCD 비트에 따라 HIGH 또는 LOW 출력
        digitalWrite(gpiopins[i], numberMap[num][i] ? HIGH : LOW);
    }
    pthread_mutex_unlock(&gpio_mutex);        // 뮤텍스 해제
}

// 7세그먼트 표시를 초기화(OFF) 처리 (뮤텍스로 보호)
void clearDisplay() {
    pthread_mutex_lock(&gpio_mutex);           // 뮤텍스 잠금
    for (int i = 0; i < 4; i++) {
        digitalWrite(gpiopins[i], HIGH);       // 모든 비트를 HIGH(OFF)로 설정
    }
    pthread_mutex_unlock(&gpio_mutex);         // 뮤텍스 해제
}

// start부터 0까지 카운트다운하며 1초마다 표시
int countdown(int start) {
    initFND();                                 // GPIO/스피커 초기화
    if (start < 0 || start > 9) {              // 유효 범위 체크
        printf("0~9 사이의 숫자를 입력해야 해\n");
        return 0;
    }
    int current = start;                       // 현재 카운트 저장
    while (current >= 0) {
        printf("현재 숫자: %d\n", current);    // 콘솔 출력
        displayNumber(current);                // 7세그먼트에 숫자 표시
        

        if (current == 0) {
            pthread_mutex_lock(&gpio_mutex);   // 뮤텍스 잠금
            // 0이 되면 스피커로 알람음 출력 (뮤텍스로 보호)
            softToneWrite(spkr, 440);          // 440Hz 알람음
            delay(400);                        // 0.2초 대기

            // 스피커 핀 입력 모드로 전환 (무음)
            softToneWrite(spkr, 0);
            pthread_mutex_unlock(&gpio_mutex); // 뮤텍스 해제
        }
        delay(1000);                           // 1초 대기
        current--;                             // 숫자 1 감소
    }

    clearDisplay();    // 0 완료 후 표시 초기화
    delay(200);        // 잠깐 대기
    return 0;
}

