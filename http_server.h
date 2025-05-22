// http_server.h
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

// HTTP 서버를 실행하는 함수 선언
// port: 열 포트 번호, html_path: 서비스할 HTML 파일 경로
int http_server_start(int port, const char *html_path);

#endif // HTTP_SERVER_H
