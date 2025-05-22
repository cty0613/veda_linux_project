#include <stdio.h>        // printf, fgets
#include <stdlib.h>       // exit
#include <string.h>       // memset, strlen
#include <unistd.h>       // read, write, close
#include <arpa/inet.h>    // sockaddr_in, inet_pton
#include <pthread.h>      // pthread_create, pthread_exit

#define SERVER_IP   "192.168.0.76"  // 서버 IP
#define SERVER_PORT 9081         // 서버 포트
#define BUF_SIZE    1024         // 버퍼 크기

int sockfd;  // 전역으로 선언해서 두 스레드 모두 사용

// --- 요청 전송 전용 스레드 함수 ---
void *send_thread(void *arg) {
    char sendbuf[BUF_SIZE];
    while (1) {
        // 1) 사용자 입력 대기
        if (!fgets(sendbuf, BUF_SIZE, stdin)) {
            break;  // EOF거나 에러면 종료
        }

        // 2) 개행문자 제거
        size_t len = strlen(sendbuf);
        if (len > 0 && sendbuf[len-1] == '\n') {
            sendbuf[len-1] = '\0';
        }

        // 3) "exit" 입력 시 루프 탈출
        if (strcmp(sendbuf, "exit") == 0) {
            break;
        }
        
        // 4) 서버로 전송
        if (write(sockfd, sendbuf, strlen(sendbuf)) < 0) {
            perror("write 실패");
            break;
        }
    }
    // 입력 스레드 종료 시 소켓 닫기
    close(sockfd);
    return NULL;
}

// --- 응답 수신 전용 스레드 함수 ---
void *recv_thread(void *arg) {
    char recvbuf[BUF_SIZE];
    ssize_t n;
    while ((n = read(sockfd, recvbuf, BUF_SIZE-1)) > 0) {
        recvbuf[n] = '\0';  // 널 종료
        printf("서버 응답: %s\n", recvbuf);
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;
    pthread_t tid_send, tid_recv;

    // 1) 소켓 생성
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket 생성 실패");
        exit(EXIT_FAILURE);
    }

    // 2) 서버 주소 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton 실패");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 3) 서버에 연결
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect 실패");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("서버에 연결됨: %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("요청을 입력하세요 (exit 입력 시 종료)\n");

    // 4) 전송·수신 스레드 생성
    pthread_create(&tid_send, NULL, send_thread, NULL);
    pthread_create(&tid_recv, NULL, recv_thread, NULL);

    // 5) 메인 스레드는 두 스레드의 종료를 대기
    pthread_join(tid_send, NULL);
    // 전송 스레드가 종료되면 수신 스레드도 같이 끝나도록
    pthread_cancel(tid_recv);

    printf("클라이언트 종료\n");
    return 0;
}
