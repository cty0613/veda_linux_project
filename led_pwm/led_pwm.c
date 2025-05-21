#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>

#define LED_PIN 1 	/* GPIO18  */


void initPWM() {
    wiringPiSetup();
    pinMode(LED_PIN, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);      // 마크-스페이스 방식 (권장)
    pwmSetClock(384);             // 분주비 설정 (PWM 주기 조정용)
    pwmSetRange(1000);            // 듀티사이클 범위 설정 (0~1000)

}

int ledPwmCtrl(int brightness) {
    initPWM();
    if (brightness == 2 ) { // NORMAL BRIGHTNESS
        pwmWrite(LED_PIN, 300);               // 30% 밝기
        return 0;
    } else if (brightness == 3 ) { // BRIGHTEST BRIGHTNESS
        pwmWrite(LED_PIN, 1000);              // 100% 밝기
        return 0;
    } else if (brightness == 1 ) { // DIM BRIGHTNESS
        pwmWrite(LED_PIN, 100);               // 10% 밝기
        return 0;
    } else if (brightness == 0) {  // OFF
        pwmWrite(LED_PIN, 0);
        return 0;
    } else {
        printf("Error : brightness value must be 0 ~ 3");
        pwmWrite(LED_PIN, 0);
        return 0;
    }   
}

// int main() {
//     ledPwmCtrl(3);
//     return 0;
// }