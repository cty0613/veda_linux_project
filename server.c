#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>             // read(), write(), close()
#include <arpa/inet.h>          // sockaddr_in, inet_ntoa()
#include <string.h>
#include <time.h>               // 로그 타임스탬프
#include <wiringPi.h>

#define PORT 9081
#define BUF_SIZE 1024
#define LOG_FILE "log.txt"

// 로그 작성 함수
void write_log(const char* client_ip, const char* request, const char* result) {
    FILE* log_fp = fopen(LOG_FILE, "a");
    if (!log_fp) {
        perror("로그 파일 열기 실패");
        return;
    }
    time_t now = time(NULL);
    char* time_str = strtok(ctime(&now), "\n"); // 개행 제거

    fprintf(log_fp, "[%s] %s | 요청: %s | 결과: %s\n", time_str, client_ip, request, result);
    fclose(log_fp);
}

void* inf_thread(void* arg) {
    int (*func)(int) = (int (*)(int))arg;
    func(0); // 무한 루프 진입
    return NULL;
}

// 동적 라이브러리 함수 실행
char* ExecDLib(const char* libname, const char* funcname, int param) {
    static char result_msg[BUF_SIZE]; // 정적 할당 (호출 후 유효)
    char libpath[300];

    snprintf(libpath, sizeof(libpath), "./lib%s.so", libname);

    void* handle = dlopen(libpath, RTLD_LAZY);
    if (!handle) {
        snprintf(result_msg, sizeof(result_msg), "dlopen error: %s", dlerror());
        return result_msg;
    }

    if (strcmp(funcname, "cdsControl") == 0) {
        // 무한 루프용 함수일 경우 별도 쓰레드 실행
        int (*func)(int);
        *(void**)(&func) = dlsym(handle, funcname);
        if (dlerror()) {
            snprintf(result_msg, sizeof(result_msg), "dlsym error (cdsControl)");
            dlclose(handle);
            return result_msg;
        }

        pthread_t cds_tid;
        pthread_create(&cds_tid, NULL, inf_thread, func);
        pthread_detach(cds_tid);

        snprintf(result_msg, sizeof(result_msg), "CDS 모니터링 시작됨");
        return result_msg;
    }

    int (*func)(int);
    *(void**)(&func) = dlsym(handle, funcname);
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        snprintf(result_msg, sizeof(result_msg), "dlsym error: %s", dlsym_error);
        dlclose(handle);
        return result_msg;
    }

    int result = func(param);
    snprintf(result_msg, sizeof(result_msg), "Result: %d", result);
    dlclose(handle);
    return result_msg;
}

// 클라이언트 처리 쓰레드
void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    getpeername(client_sock, (struct sockaddr*)&client_addr, &len);
    const char* client_ip = inet_ntoa(client_addr.sin_addr);

    const char* welcome_msg =
        "------ Request Usage ------\n"
        "command : libname:function:param (ex: mathlib:add:5)\n"
        "type 'exit' to disconnect\n";

    write(client_sock, welcome_msg, strlen(welcome_msg));

    char buffer[BUF_SIZE];

    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int read_len = read(client_sock, buffer, BUF_SIZE - 1);
        if (read_len <= 0) break;

        buffer[read_len] = '\0'; // 널 종료
        printf("요청 [%s] from %s\n", buffer, client_ip);

        // 연결 종료 명령 처리
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) {
            const char* msg = "서버와의 연결을 종료합니다.\n";
            write(client_sock, msg, strlen(msg));
            printf("클라이언트 %s 연결 종료 요청\n", client_ip);
            break;
        }

        // 요청 파싱: libname:function:param
        char libname[256], funcname[256];
        int param_int;

        if (sscanf(buffer, "%[^:]:%[^:]:%d", libname, funcname, &param_int) != 3) {
            const char* error = "잘못된 요청 형식. 예시: libname:function:int\n";
            write(client_sock, error, strlen(error));
            write_log(client_ip, buffer, "Invalid format");
            continue; // 연결 유지
        }

        // 실행
        const char* exec_result = ExecDLib(libname, funcname, param_int);
        write(client_sock, exec_result, strlen(exec_result));
        write_log(client_ip, buffer, exec_result);
        printf("결과 [%s] to %s\n", exec_result, client_ip);
    }

    close(client_sock);
    printf("클라이언트 연결 종료됨: %s\n", client_ip);
    return NULL;
}


// 서버 세팅 함수
void SetupServer() {
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket create error");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("socket bind error");
        exit(1);
    }

    if (listen(server_sock, 5) < 0) {
        perror("listen error");
        exit(1);
    }

    printf("서버 대기 중 PORT : %d\n", PORT);

    while (1) {
        client_len = sizeof(client_addr);
        int* client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (*client_sock < 0) {
            perror("client accept error");
            free(client_sock);
            continue;
        }

        printf("클라이언트 연결됨: %s\n", inet_ntoa(client_addr.sin_addr));

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_sock) != 0) {
            perror("pthread_create error");
            close(*client_sock);
            free(client_sock);
        }

        pthread_detach(tid);
    }

    close(server_sock);
    printf("서버 종료\n");
}

int main(int argc, char **argv) {
    wiringPiSetup();  // 라즈베리파이 GPIO 초기화
    SetupServer();    // 서버 실행
    return 0;
}
