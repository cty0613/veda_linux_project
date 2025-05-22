#include "http_server.h"
#include <stdio.h>          // fopen, fread, fwrite
#include <stdlib.h>         // malloc, free
#include <string.h>         // strcmp, strstr, memset
#include <unistd.h>         // read, write, close
#include <arpa/inet.h>      // htons, sockaddr_in
#include <sys/socket.h>     // socket, bind, listen, accept
#include <netinet/in.h>     // sockaddr_in

#define HTTP_PORT 8080      // HTTP 포트
#define BACKLOG   5         // listen 대기 큐 길이
#define BUF_SIZE  2048      // 버퍼 크기

// 디스크의 파일(path)을 읽어 (body, size)로 반환
static int serve_file(int clientfd, const char *path, const char *content_type) {
    FILE *fp = fopen(path, "r");              // 파일 열기
    if (!fp) return -1;                       // 없으면 -1 반환

    fseek(fp, 0, SEEK_END);                   
    long size = ftell(fp);                    // 파일 크기 계산
    rewind(fp);                               

    // HTTP 헤더 작성
    char header[256];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n\r\n",
        content_type, size);
    write(clientfd, header, hlen);            // 헤더 전송

    // 본문 전송
    char buf[BUF_SIZE];
    size_t n;
    while ((n = fread(buf, 1, BUF_SIZE, fp)) > 0) {
        write(clientfd, buf, n);               // 청크 단위 전송
    }
    fclose(fp);                                // 파일 닫기
    return 0;
}

int http_server_start(int port, const char *unused) {
    int sockfd, clientfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    char req[BUF_SIZE];

    // 1) 소켓 생성
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return -1; }
    // 2) 주소 재사용
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // 3) bind
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;           
    serv_addr.sin_addr.s_addr = INADDR_ANY;   
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind"); close(sockfd); return -1;
    }
    // 4) listen
    if (listen(sockfd, BACKLOG) < 0) {
        perror("listen"); close(sockfd); return -1;
    }
    printf("HTTP Server listening on port %d\n", port);

    // 5) 요청 처리 루프
    while (1) {
        clientfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);
        if (clientfd < 0) { perror("accept"); continue; }

        // 5-1) 요청 헤더 첫 줄 읽기
        int r = read(clientfd, req, BUF_SIZE - 1);
        if (r <= 0) { close(clientfd); continue; }
        req[r] = '\0';

        // 5-2) GET 요청의 경로 파싱 (예: "GET /path HTTP/1.1")
        char *path = NULL;
        if (strncmp(req, "GET ", 4) == 0) {
            path = strtok(req + 4, " ");        // 첫 번째 공백에서 잘라 경로 얻음
        }

        // 5-3) 라우팅: "/" → index.html, "/server.log" → 로그 파일
        if (!path || strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            // index.html 반환
            if (serve_file(clientfd, "index.html", "text/html; charset=utf-8") < 0) {
                // 404 처리
                const char *nf = "HTTP/1.1 404 Not Found\r\n\r\n";
                write(clientfd, nf, strlen(nf));
            }
        }
        else if (strcmp(path, "/server.log") == 0) {
            // server.log 반환
            if (serve_file(clientfd, "server.log", "text/plain; charset=utf-8") < 0) {
                const char *nf = "HTTP/1.1 404 Not Found\r\n\r\n";
                write(clientfd, nf, strlen(nf));
            }
        }
        else {
            // 기타 경로 → 404
            const char *nf = "HTTP/1.1 404 Not Found\r\n\r\n";
            write(clientfd, nf, strlen(nf));
        }

        close(clientfd);                       // 연결 종료
    }

    close(sockfd);
    return 0;
}

// 2) 기존 main은 그대로 두어도 되지만 이제 http_server_start 호출
int main(int argc, char *argv[]) {
    // 기본 값: 포트 8080, index.html
    return http_server_start(8080, "index.html");
}
