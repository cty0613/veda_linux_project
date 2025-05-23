# VEDA 2기 리눅스 프로그래밍 프로젝트
한화 VEDA 2기 리눅스 프로그래밍 (TCP서버 원격 장치 제어 서버 & 클라이언트) 프로젝트 레포지토리입니다.


## 주요 기능
1. LED 제어 : 

   GPIO18(OUTPUT, PWM) 사용하여 클라이언트의 요청에 따라 LED를 ON/OFF 합니다.
   
2. CDS(조도 센서) :
   
   GPIO24(INPUT)를 사용하여 Pull-Up 저항과 연결된 CDS의 저항을 HIGH/LOW로 빛의 밝기를 인식합니다 클라이언트에 제공합니다.

   GPIO23(OUTPUT) 사용하여 CDS의 값에 따라 어두우면 LED를 ON, 밝으면 LED를 OFF 합니다.

3. 부저(BUZZER) :

   GPIO12(OUTPUT)를 사용하여 연결된 부저를 통해 클라이언트의 요청에 따라 특정 음색을 재생합니다.

4. 7SEGMENT(FND) 제어 :

   GPIO14, 15, 17, 27과 연결된 7447 드라이버와 7SEGMENT(FND)를 사용하여 숫자를 표시합니다.

   클라이언트의 요청에 따라 카운트다운이 시작되며, 남은 시간(N초)를 7SEGMENT에 표시하고, 0이되면 부저를 울립니다.

5. 요청/응답 로깅 및 로그뷰어 :

   서버와 클라이언트 사이에 이뤄지는 TCP 요청과 응답의 내용을 `server.log`에 기록합니다.

   http/html 서버를 통해 웹 브라우저로 이 로그의 내용을 조회할 수 있습니다.


## 실행 환경
### 서버
RaspberryPi 4B+ 8GB
RaspberryPi OS (64bit, ARM)

### 클라이언트
Linux/UNIX


## 프로젝트 빌드 및 실행

!! 라이브리러 및 서버 프로그램 빌드 시 wiringPi가 설치되어있어야 함!

1. 서버 프로그램 빌드
    ```makefile
    make server http_server
    ```

2. 클라이언트 프로그램 빌드
    ```makefile
    make client
    ```

3. 동적 라이브러리 빌드
    ```makefile
    make
    ```


4. 서버/클라이언트 실행하기
    -  서버 실행
        ```bash
        bash start_service.sh
        ```

    -  클라이언트 실행 (로컬 네트워크 상 다른 기기에서)
        ```bash
        ./client
        ```

## 클라이언트 요청 방법

### LED 제어
```bash
led_pwm ledCtrl 0        # LED 끄기
led_pwm ledCtrl 1        # LED 약한 밝기로 켜기
led_pwm ledCtrl 2        # LED 중간 밝기로 켜기
led_pwm ledCtrl 3        # LED 최대 밝기로 켜기
```

### CDS (조도 센서)
```bash
cds_led cdsCtrl 0        # CDS값에 따라 LED 자동제어 활성화
cds_led cdsRead 0        # CDS값 읽기
```

### 부저
```bash
buzzer_music play 0      # 부저로 Loveholic - Loveholic 재생
buzzer_music play 1      # 부저로 IU - 블루밍 음악 재생
buzzer_music play 2      # 부저로 Glen Check - 60s Cardin 음악 재생
```
### 7SEG(FND)
```bash
7seg_fnd countdowm 5     # 5초 카운트 다운
7seg_fnd countdowm 9     # 9초 카운트 다운
```

## 로그 뷰어
`http://[라즈베리파이의 로컬 IPv4 주소]:8080`로 접속

예시 : 라즈베리파이 주소가 192.168.0.76 이라면
`http://192.168.0.76:8080`로 접속


