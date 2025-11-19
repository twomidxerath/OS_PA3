#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // getopt 함수 사용
#include <string.h>
#include "simulator.h"
#include "log.h"

// 전역 변수 실제 정의
ReplacementPolicy current_policy;
uint64_t global_access_time = 0;

int main(int argc, char *argv[]) {
    int opt;
    char *policy_str = NULL;
    char *input_file = NULL;
    char *log_file = NULL;

    // 인자 파싱 (-p, -f, -l)
    while ((opt = getopt(argc, argv, "p:f:l:")) != -1) {
        switch (opt) {
            case 'p': policy_str = optarg; break;
            case 'f': input_file = optarg; break;
            case 'l': log_file = optarg; break;
            default:
                fprintf(stderr, "Usage: %s -p <RR|LRU> -f <input> -l <output>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!policy_str || !input_file || !log_file) {
        fprintf(stderr, "Error: Missing arguments.\nUsage: %s -p <RR|LRU> -f <input> -l <output>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    initialize_simulator(policy_str);
    open_log_file(log_file);
    simulate_accesses(input_file);
    close_log_file();
    
    return 0;
}