#include <wiringPi.h>
#include <softTone.h>
#include <stdio.h>
#include <pthread.h>

#define C1 523  //도
#define C1_ 554
#define D1 587  //레
#define D1_ 622
#define E1 659  //미
#define F1 698  //파
#define F1_ 740
#define G1 784  //솔
#define G1_ 831
#define A1 880  //라
#define A1_ 932
#define B1 988  //시
#define C2 1046 //도
#define C2_ 1109
#define D2 1178 //레
#define D2_ 1245
#define E2 1319 //미
#define F2 1397 //파
#define F2_ 1480
#define G2 1568 //솔
#define G2_ 1661
#define A2 1760 //라
#define A2_ 1865
#define B2 1976
#define C3 (1046*2)

#define XX 99   //쉼표
#define W0 1600 // 0 알람음
#define W1 1200 // 1 

#define DLY_1 16 // 온음표
#define DLY_2 8  // 2분음표
#define DLY_4 4  // 4분음표
#define DLY_4d 6 // 4 점 분음표
#define DLY_8 2  // 8분음표
#define DLY_8d 3 // 8 점 분음표
#define DLY_16 1 // 16분음표

#define BPM          120                      // 분당 박자 수 설정
#define NOTE_UNIT_MS (60000 / BPM / 4)        // 16분음표(ms) = (1분=60000ms ÷ BPM) ÷ 4

#define SPKR_PIN 	26 	/* GPIO25 */

// 뮤텍스 전역 선언 및 초기화
static pthread_mutex_t tone_mutex = PTHREAD_MUTEX_INITIALIZER;

int sng1_notes[19] = { // 러브홀릭 - 러브홀릭
    XX, A1, G1_, A1, E1,
    XX, C2_, B1, A1,  G1_,
    XX, C2_, C2, C2_, D2, C2_,
    B1, C2_, G1_
};

int sng1_length[19] = {
    DLY_8, DLY_4, DLY_8, DLY_8, DLY_4,
    DLY_8, DLY_4, DLY_8, DLY_8, DLY_4,
    DLY_8, DLY_4, DLY_8, DLY_4, DLY_4, DLY_4,
    DLY_4, DLY_8, DLY_4
};

int sng2_notes[25]= //  아이유 블루밍
    {C1,E1,G1,XX,A1,
     C2,E2,C2,D2,E2,
     XX,C2,C2,C2,C2,
     C2,C2,C2,C2,A1,
     G1,D2,E2,XX,C2};

int sng2_length[25] = {
    DLY_16, DLY_8, DLY_8, DLY_8, DLY_8d, 
    DLY_8d, DLY_8d, DLY_8d, DLY_4,  DLY_8, 
    DLY_4d, DLY_8,  DLY_8,  DLY_8,  DLY_8, 
    DLY_8,  DLY_8,  DLY_8, DLY_8, DLY_8, 
    DLY_8d, DLY_8,  DLY_8, DLY_16, DLY_4
};  // 각 음표 길이 매크로 배열, ‘d’는 점음표(1.5배 길이)

int sng3_notes[44] = { // 60s Cardin
    G1, XX, A1, XX, C2, 
    E2, E2, C2, D2, XX, C2,
    G1, XX, A1, XX, C2,
    E2, E2, C2, D2, XX, C2,
    G1, XX, A1, XX, C2, 
    E2, E2, C2, D2, XX, C2,
    G1, XX, A1, XX, C2,
    E2, G2, C2, C3, XX, D2
};

int sng3_length[44] = {
    DLY_8, DLY_16, DLY_16, DLY_8, DLY_8,
    DLY_16, DLY_16, DLY_16, DLY_16, DLY_8, DLY_8,
    DLY_8, DLY_16, DLY_16, DLY_8, DLY_8,
    DLY_16, DLY_16, DLY_16, DLY_16, DLY_8, DLY_8,
    DLY_8, DLY_16, DLY_16, DLY_8, DLY_8,
    DLY_16, DLY_16, DLY_16, DLY_16, DLY_8, DLY_8,
    DLY_8, DLY_16, DLY_16, DLY_8, DLY_8,
    DLY_16, DLY_16, DLY_16, DLY_16, DLY_8, DLY_8
};

int play(int music_sel) {
    softToneCreate(SPKR_PIN); 	/* 톤 출력을 위한 GPIO 설정 */
    if (music_sel == 0) { // 학교 종이 퉁퉁퉁
        int numNotes = sizeof(sng1_notes) / sizeof(sng1_notes[0]);  // 배열 길이 계산
            for (int i = 0; i < numNotes; i++) {
                int freq = (sng1_notes[i] == XX) ? 0 : sng1_notes[i] / 2;  // 쉼표면 0, 아니면 주파수

                pthread_mutex_lock(&tone_mutex);                // 뮤텍스 잠금
                softToneWrite(SPKR_PIN, freq);                  // 주파수 또는 무음 출력
                pthread_mutex_unlock(&tone_mutex);              // 뮤텍스 해제

                delay(sng2_length[i] * NOTE_UNIT_MS);           // 음 길이만큼 대기

                // 음 끝난 뒤 무음으로 리셋
                pthread_mutex_lock(&tone_mutex);                // 뮤텍스 잠금
                softToneWrite(SPKR_PIN, 0);                     // 무음 출력
                pthread_mutex_unlock(&tone_mutex);              // 뮤텍스 해제

                delay(NOTE_UNIT_MS / 4);                        // 음 사이 짧은 공백
        }
    } else if (music_sel == 1) { // 아이유 블루밍
        int numNotes = sizeof(sng2_notes) / sizeof(sng2_notes[0]);  // 배열 길이 계산
            for (int i = 0; i < numNotes; i++) {
                int freq = (sng2_notes[i] == XX) ? 0 : sng2_notes[i] / 2;  // 쉼표면 0, 아니면 주파수

                pthread_mutex_lock(&tone_mutex);                // 뮤텍스 잠금
                softToneWrite(SPKR_PIN, freq);                  // 주파수 또는 무음 출력
                pthread_mutex_unlock(&tone_mutex);              // 뮤텍스 해제

                delay(sng2_length[i] * NOTE_UNIT_MS);           // 음 길이만큼 대기

                // 음 끝난 뒤 무음으로 리셋
                pthread_mutex_lock(&tone_mutex);                // 뮤텍스 잠금
                softToneWrite(SPKR_PIN, 0);                     // 무음 출력
                pthread_mutex_unlock(&tone_mutex);              // 뮤텍스 해제

                delay(NOTE_UNIT_MS / 4);                        // 음 사이 짧은 공백
        }
    } else if (music_sel == 2) { // 60's cardin
        int numNotes = sizeof(sng3_notes) / sizeof(sng3_notes[0]);
        for (int i = 0; i < numNotes; i++) {
            int freq = (sng3_notes[i] == XX) ? 0 : sng3_notes[i] / 2;  // 쉼표면 0, 아니면 주파수

            pthread_mutex_lock(&tone_mutex);                 // 뮤텍스 잠금
            softToneWrite(SPKR_PIN, freq);                   // 주파수 또는 무음 출력
            pthread_mutex_unlock(&tone_mutex);               // 뮤텍스 해제

            delay(sng3_length[i] * NOTE_UNIT_MS/1.5);        // 음 길이만큼 대기

            // 음 끝난 뒤 무음으로 리셋
            pthread_mutex_lock(&tone_mutex);                 // 뮤텍스 잠금
            softToneWrite(SPKR_PIN, 0);                      // 무음 출력
            pthread_mutex_unlock(&tone_mutex);               // 뮤텍스 해제

            delay(NOTE_UNIT_MS / 4);                         // 음 사이 짧은 공백
        }
    } else {
        printf("[buzzer_music] 0, 1, 2 만 입력가능함\n");
    }

    return 0;
}