#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "types.h"
#include "log.h"
#include <stdio.h>

// 시뮬레이터 전역 상태 (main.c에 정의됨)
extern ReplacementPolicy current_policy; 
extern uint64_t global_access_time;     

// 물리 메모리 및 메타데이터 (simulator.c에 정의됨)
extern uint8_t physical_memory[NUM_FRAMES * FRAME_SIZE];
extern FrameEntry frame_table[NUM_FRAMES];

// TLB 및 포인터 (simulator.c에 정의됨)
extern TLBEntry tlb[TLB_SIZE];
extern uint8_t next_rr_tlb;
extern uint8_t next_rr_frame;
extern uint8_t next_free_frame; // [수정] memory.c에서 사용하기 위해 추가
extern uint8_t root_pd_pfn;

// 함수 선언
void initialize_simulator(const char *policy_str);
void simulate_accesses(const char *input_file);
void translate_address(uint16_t va);

// 메모리 관리 함수 (memory.c)
uint8_t allocate_frame(bool is_pagetable); 

// LRU/RR 헬퍼 함수 (memory.c) - 순서 문제 해결을 위해 선언 추가
int get_lru_tlb_index();
uint8_t get_lru_eviction_frame();
uint8_t get_rr_eviction_frame(); // [수정] 추가됨
void update_tlb_time(uint16_t vpn);
void update_frame_time(uint8_t pfn);

#endif