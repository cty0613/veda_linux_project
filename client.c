#include <stdio.h>        // printf, fgets
#include <stdlib.h>       // exit
#include <string.h>       // memset, strlen
#include <unistd.h>       // read, write, close
#include <arpa/inet.h>    // sockaddr_in, inet_pton
#include <pthread.h>      // pthread_create, pthread_exit
#include <signal.h>       // signal, SIG_IGN

#define SERVER_IP   "192.168.0.76"
#define SERVER_PORT 9081
#define BUF_SIZE    1024

int sockfd;

// SIGINT외 시그널 핸들링 (무시)
void ignore_signals() {
    for (int sig = 1; sig < NSIG; ++sig) {  // NSIG는 전체 시그널 갯수
        if (sig == SIGKILL || sig == SIGSTOP || sig == SIGINT)
            continue;         // SIGKILL, SIGSTOP, SIGINT만 허용
        signal(sig, SIG_IGN); // 그 외 모든 시그널 무시
    }
}

// TCP 요청 스레드
void *send_thread(void *arg) {
    char sendbuf[BUF_SIZE];
    while (1) {
        if (!fgets(sendbuf, BUF_SIZE, stdin)) break;
        size_t len = strlen(sendbuf);
        if (len > 0 && sendbuf[len-1] == '\n') sendbuf[len-1] = '\0';
        if (strcmp(sendbuf, "exit") == 0) break;
        if (write(sockfd, sendbuf, strlen(sendbuf)) < 0) {
            perror("write 실패");
            break;
        }
    }
    close(sockfd);
    return NULL;
}

// TCP 응답 스레드
void *recv_thread(void *arg) {
    char recvbuf[BUF_SIZE];
    ssize_t n;
    while ((n = read(sockfd, recvbuf, BUF_SIZE-1)) > 0) {
        recvbuf[n] = '\0';
        printf("Server Response: %s\n", recvbuf);
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;
    pthread_t tid_send, tid_recv;

    // 시그널 무시 설정
    ignore_signals();

    // 소켓 생성
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket 생성 실패");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton 실패");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 서버에 연결
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect 실패");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Connected: %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("Input Request (Ctrl+C to exit)\n");

    // 전송/수신 스레드 생성
    pthread_create(&tid_send, NULL, send_thread, NULL);
    pthread_create(&tid_recv, NULL, recv_thread, NULL);

    // 전송 스레드 종료 대기
    pthread_join(tid_send, NULL);
    pthread_cancel(tid_recv); // 수신 스레드 종료 요청

    printf("Client Terminated\n");
    return 0;
}
