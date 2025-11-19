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
extern uint8_t next_free_frame; 
extern uint8_t root_pd_pfn;

// 핵심 시뮬레이터 함수 (simulator.c)
void initialize_simulator(const char *policy_str);
void simulate_accesses(const char *input_file);
void translate_address(uint16_t va);

// 메모리 관리 함수 (memory.c)
uint8_t allocate_frame(bool is_pagetable); 
void invalidate_swapped_out_page(uint8_t pfn);

// LRU/RR 헬퍼 함수 (memory.c)
int get_lru_tlb_index();
uint8_t get_lru_eviction_frame();
uint8_t get_rr_eviction_frame();
void update_tlb_time(uint16_t vpn);
void update_frame_time(uint8_t pfn);

#endif