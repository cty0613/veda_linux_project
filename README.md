### VEDA 2기 리눅스 프로그래밍 프로젝트

### 빌드하기

1. 서버
    ```makefile
    make server
    ```

2. 클라이언트
    ```makefile
    make client
    ```

3. 라이브러리
    ```makefile
    make clean
    make
    ```


### 실행하기
1. 서버 실행
    ```bash
    sudo ./server
    ```

2. 클라이언트 실행
    ```bash
    ./client
    ```

3. 요청 보내기
    - `led_pwm` : GPIO18에 연결된 LED PWM 제어
        - `ledPwmCtrl`
            - `0` : 끄기
            - `1` : 약하게
            - `2` : 중간
            - `3` : 최대
    - `cds_led` : 조도 센서
        - `cdsControl` : 조도값으로 LED 제어
        - `cdsRead` : 조도값 읽기
    - `buzzer_music` : 부저 제어
        - `musicPlay` : 음악 재생
            - `0` : 학교종
            - `1` : 아이유 블루밍
            - `2` : GOD 촛불하나
    - `7seg_fnd` : 7seg 제어
        - `countDown` : 카운트 다운 시작
            - `5`: 5초 카운트 다운

