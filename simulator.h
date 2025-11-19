#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "types.h" // 위에 정의한 타입들을 포함
#include "log.h"   // 로깅 함수 포함
#include <stdio.h>

// 시뮬레이터 전역 상태
extern ReplacementPolicy current_policy; // 현재 교체 정책 (RR/LRU)
extern uint64_t global_access_time;     // LRU 관리를 위한 전역 시간 카운터

// 물리 메모리 시뮬레이션 및 메타데이터 (Frame Table)
extern uint8_t physical_memory[NUM_FRAMES * FRAME_SIZE];
extern FrameEntry frame_table[NUM_FRAMES];

// TLB 및 RR/LRU 포인터
extern TLBEntry tlb[TLB_SIZE];
extern uint8_t next_rr_tlb;     // RR TLB 교체용 포인터
extern uint8_t next_rr_frame;   // RR Frame 교체용 포인터

// Root Page Directory의 PFN (모든 프로세스가 단일 프로세스이므로 전역으로 관리)
extern uint8_t root_pd_pfn;


// 핵심 시뮬레이터 함수
void initialize_simulator(const char *policy_str); // 시뮬레이터 초기화 및 정책 설정
void simulate_accesses(const char *input_file);   // 입력 파일 읽기 및 주소 접근 시뮬레이션
void translate_address(uint16_t va);             // 가상 주소(VA)를 처리하는 주 로직


// 메모리 및 TLB 관리 함수 (내부적으로 호출)
uint8_t allocate_frame(bool is_pagetable); // 새로운 물리 프레임 할당 (Swap 로직 포함)
void update_lru_time(uint8_t pfn);         // 특정 프레임의 LRU 시간을 갱신
void update_tlb_time(uint16_t vpn);         // 특정 TLB 엔트리의 LRU 시간을 갱신

// simulator.h

// ... (기존 선언 유지) ...

// LRU 헬퍼 함수 선언 (memory.c에 구현됨)
int get_lru_tlb_index();
uint8_t get_lru_eviction_frame();
void update_tlb_time(uint16_t vpn);
void update_frame_time(uint8_t pfn);

#endif