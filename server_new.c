#include <stdio.h>
#include <stdlib.h>

#include <dlfcn.h>              // dlib
#include <unistd.h>             // read(), write(), close()
#include <pthread.h>

#include <arpa/inet.h>          // sockaddr_in, inet_ntoa()
#include <string.h>             // str 

#include <wiringPi.h>

#define PORT 9081

// 함수 실행 파리미터 전달용 DTO
typedef struct {
    int     clinet_socket;  // 클라리언트 소켓
    char    libname[256];  // 로드할 라이브러리 이름
    char    funcname[256]; // 호출할 함수 이름
    int     arg;            // 호출 함수에 전달할 정수 인자
} func_req_dto;

void* func_thread(void *arg){
    func_req_dto *req = (func_req_dto *)arg;

    // 동적 라이브러리 로드 "./lib<name>.so"
    char libpath[512];
    snprintf(libpath, sizeof(libpath), "./lib%s.so", req->libname);
    void *handle = dlopen(libpath, RTLD_LAZY);

    if(!handle){
        char errmsg[BUFSIZ];
        snprintf(errmsg, sizeof(errmsg), "dlopen error: %s", dlerror());
        dprintf(req->clinet_socket, "ERROR: cannot load dlib %s\n", req->libname);
        goto cleanup;
    }

    // 동적 라이브러리 내 함수 심볼 조회
    typedef int (*func_t)(int);
    dlerror(); // handle에서 발생한 에러 리셋
    func_t func = (func_t)dlsym(handle, req->funcname);
    char *dlsym_err = dlerror();
    if(dlsym_err) {
        dprintf(req->clinet_socket, "ERROR: no func symbol at dlib, %s\n", req->funcname);
        dlclose(handle);
        goto cleanup;
    }

    // 로드한 함수 실행 (스레드 내부에서)
    int result = func(req->arg);

    // 클라이언트에 함수 결과 응답 전송
    dprintf(req->clinet_socket, "RESULT: %d\n", result);

    dlclose(handle);

    cleanup:
        free(req);
        return NULL;
}

void* client_thread(void *arg){
    int client_socket = *(int*)arg; // 전달받은 클라이언트 소켓
    free(arg);                      // 전달할때 사용한 포인터 해제

    char buf[BUFSIZ];
    ssize_t n;

    // 클라이언트에서 오는 요청 대기(루프 반복)
    while( (n = read(client_socket, buf, sizeof(buf)-1)) > 0) {
        buf[n] = '\0';
        
        // 요청 형식 파싱 >> libname funcname arg
        char *saveptr;
        char *libname = strtok_r(buf, " \t\n", &saveptr);
        char *funcname = strtok_r(NULL, " \t\n", &saveptr);
        char *argstr = strtok_r(NULL, " \t\n", &saveptr);
        if (!libname || !funcname || !argstr) {
            dprintf(client_socket, "ERROR: invalid request format\n");
            continue;
        }

        // 함수 실행용 스레드에 전달할 구조체 및 변수 준비
        func_req_dto *req = malloc(sizeof(func_req_dto));
        strcpy(req->libname, libname);
        strcpy(req->funcname, funcname);
        req->arg = atoi(argstr);
        req->clinet_socket = client_socket;
        
        // 함수 실행용 스레드 생성
        pthread_t tid;
        pthread_create(&tid, NULL, func_thread, req);
        pthread_detach(tid);
    }
    
}

int serverSetup() {
    struct sockaddr_in server_addr; // 서버 및 클라이언트 주소 설정 구조체
    socklen_t client_len;                        
    char msg_buffer[BUFSIZ];
    
    // 1. 서버 소켓 생성 (IPv4, TCP)
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket create error");
        exit(1);
    }

    // 2. 서버 주소 설정 (server_addr)
    memset(&server_addr, 0, sizeof(server_addr));   // 구조체 초기화
    server_addr.sin_family = AF_INET;               // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;       // 모든 IP 허용
    server_addr.sin_port = htons(PORT);             // 포트 설정

    // 3. 소켓에 IP주소/포트 할당
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("socket bind error");
        exit(1);
    }

    // 4. listen: 클라이언트 접속 대기
    if (listen(server_sock, 5) < 0) {
        perror("server listen error");
        exit(1);
    }

    printf("%d 포트에서 서버 설정 완료... 대기중\n", PORT);
    
    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (*client_sock < 0) {
            perror("cliend accept error");
            free(client_sock);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, client_sock);
        pthread_detach(tid);
    }

    close(server_sock);
    
}

int main() {
    wiringPiSetup();
    serverSetup();
    
    return 0;
}