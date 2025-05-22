#include <stdio.h>
#include <stdlib.h>

#include <dlfcn.h>              // dlib load
#include <unistd.h>             // read(), write(), close()
#include <pthread.h>            // pthread

#include <arpa/inet.h>          // network socket
#include <string.h>             // str 

#include <wiringPi.h>           // wiringPiSetup()
#include <time.h>               // for log timestamp
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>

#define PORT 9081
#define BACKLOG 10              // 최대 대기 큐 길이
#define LOG_FILE "server.log"   // 로그 파일 경로

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;  // 로그 동기화용 뮤텍스
pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;  // 맵 접근용 뮤텍스

// 함수 실행 파리미터 전달용 DTO
typedef struct {
    int     clinet_socket;  // 클라리언트 소켓
    char    libname[256];  // 로드할 라이브러리 이름
    char    funcname[256]; // 호출할 함수 이름
    int     arg;            // 호출 함수에 전달할 정수 인자
} func_req_dto;

// 함수별 뮤텍스 노드 (연결리스트)
typedef struct func_mutex_node {
    char name[256];
    pthread_mutex_t mutex;
    struct func_mutex_node *next;
} func_mutex_node;

// 함수별 뮤텍스 노드의 헤드 (연결리스트)
func_mutex_node *mutex_list_head = NULL;

// 호출되는 함수에 대응되는 뮤텍스 포인터를 반환하는 함수
pthread_mutex_t *get_func_mutex(const char *funcname){
    func_mutex_node *node;  // 검색할 뮤텍스의 헤드 노드
    pthread_mutex_t *m; // 반환할 뮤텍스

    // 뮤텍스 맵에 안전 접근
    pthread_mutex_lock(&map_mutex);
    
    // 기존에 생성된 함수 뮤텍스 노드 검색
    for (node = mutex_list_head; node; node = node->next){
        if(strcmp(node->name, funcname) == 0){ // 함수 발견
            m = &node->mutex;
            pthread_mutex_unlock(&map_mutex);
            return m;
        }
    }

    // 검색 결과가 없으면 새 노드 생성
    node = malloc(sizeof(*node));   // 메모리 할당
    strcpy(node->name, funcname);   // 함수 이름 뮤텍스 노드에 저장
    pthread_mutex_init(&node->mutex, NULL); // 기본 속성 뮤텍스 초기화 (사용 준비)
    
    // 새로운 뮤텍스 노드를 리스트에 삽입
    node->next = mutex_list_head;
    mutex_list_head = node;

    m = &node->mutex;
    pthread_mutex_unlock(&map_mutex);
    return m;
    
}

// 로그 기록용 함수
void LOG(const char *format, ...) {
    pthread_mutex_lock(&log_mutex);                  // 로그 기록 전 락 획득
    FILE *fp = fopen(LOG_FILE, "a");                 // 로그 파일 append 모드로 열기
    if (!fp) {
        perror("fopen");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    // 현재 시간 문자열 생성
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_now);

    // 시간 찍고 나머지 메시지 출력
    fprintf(fp, "[%s] ", timebuf);
    va_list args;
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);
    fprintf(fp, "\n");

    fclose(fp);                                      // 파일 닫기
    pthread_mutex_unlock(&log_mutex);                // 락 해제
}

void* func_thread(void *arg){
    func_req_dto *req = (func_req_dto *)arg;

    // 요청한 함수에 대한 뮤텍스 획득 & 잠금 (같은 함수 요청끼리만 직렬화)
    pthread_mutex_t *fmutex = get_func_mutex(req->funcname);
    pthread_mutex_lock(fmutex); // 이미 실행중이면 여기서 블락



    LOG("REQUEST from socket %d: lib=%s, func=%s, arg=%d",
              req->clinet_socket, req->libname, req->funcname, req->arg);

    // 동적 라이브러리 로드 "./lib<name>.so"
    char libpath[512];
    snprintf(libpath, sizeof(libpath), "./lib%s.so", req->libname);
    void *handle = dlopen(libpath, RTLD_LAZY);

    if(!handle){
        char errmsg[BUFSIZ];
        snprintf(errmsg, sizeof(errmsg), "dlopen error: %s", dlerror());
        LOG("ERROR: %s", errmsg);
        dprintf(req->clinet_socket, "ERROR: cannot load dlib %s\n", req->libname);
        goto cleanup;
    }

    // 동적 라이브러리 내 함수 심볼 조회
    typedef int (*func_t)(int);
    dlerror(); // handle에서 발생한 에러 리셋
    func_t func = (func_t)dlsym(handle, req->funcname);
    char *dlsym_err = dlerror();
    if(dlsym_err) {
        LOG("ERROR: dlsym error: %s", dlsym_err);
        dprintf(req->clinet_socket, "ERROR: no func symbol at dlib, %s\n", req->funcname);
        dlclose(handle);
        goto cleanup;
    }

    // 로드한 함수 실행 (스레드 내부에서)
    int result = func(req->arg);

    // 클라이언트에 함수 결과 응답 전송
    LOG("RESPONSE to socket %d: %d", req->clinet_socket, result);
    dprintf(req->clinet_socket, "RESULT: %d\n", result);

    dlclose(handle);

    cleanup:
        pthread_mutex_unlock(fmutex);
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

    close(client_socket);
    LOG("Disconnected client from %d", client_socket);
    return NULL;
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