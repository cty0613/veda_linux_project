#include <wiringPi.h>
#include <stdio.h>

#define CDS 	5 	/* GPIO18 */
#define LED 	4 	/* GPIO18 */

int cdsControl(int) {
    pinMode(CDS, INPUT); 	/* Pin 모드를 입력으로 설정 */
    pinMode(LED, OUTPUT); 	/* Pin 모드를 출력으로 설정 */

    while(1){
        if(digitalRead(CDS) == LOW) { 	// 빛이 감지되면(밝으면)(LOW) 
            digitalWrite(LED, LOW); 	/* LED OFF */
        } else {                        // 빛이 감지되지 않으면 (어두우면)(HIGH)
            digitalWrite(LED, HIGH); 	/* LED ON */
        }
    }

    return 0;
    
}

int cdsRead(int) {
    pinMode(CDS, INPUT); 	/* Pin 모드를 입력으로 설정 */
    return digitalRead(CDS);
}

// int main( )
// {
//     wiringPiSetup();
//     while(1) {
//         int value = cdsControl(); 		/* 조도 센서 사용을 위한 함수 호출 */
//         printf("LIGHT DETECTION : %d\n", value);
//     }
    
//     return 0;
// }
