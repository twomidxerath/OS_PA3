#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

//////////////////////////////////////////////////
// 1. 메모리 시스템 사양 상수 (12-bit VA)
//////////////////////////////////////////////////

#define VA_BITS 12                  // 가상 주소: 12 비트
#define PA_BITS 10                  // 물리 메모리 1024 Bytes = 2^10이므로 10 비트
#define FRAME_SIZE_BITS 3           // 페이지/프레임 크기 8 Bytes = 2^3이므로 3 비트
#define FRAME_SIZE 8                // 페이지/프레임 크기 (8 Bytes)

// TLB 사양
#define TLB_SIZE 16                 // TLB 엔트리 수

// 물리 메모리 사양
#define NUM_FRAMES 128              // 총 물리 프레임 수 (1024 / 8)
#define PFN_BITS 7                  // PFN은 7 비트 (log2(128))

// 주소 분할 사양
#define OFFSET_BITS FRAME_SIZE_BITS // 오프셋: 3 비트
#define VPN_BITS (VA_BITS - OFFSET_BITS) // VPN: 9 비트

// 3-Level Paging 구조 (9-bit VPN을 3비트씩 분할)
#define PD1_BITS 3                  // Root Page Directory Index
#define PD2_BITS 3                  // Level 2 Page Directory Index
#define PT_BITS 3                   // Page Table Index

#define PT_ENTRY_COUNT 8            // Page Table 내 엔트리 수 (8 Bytes / 1 Byte)
#define PTE_SIZE 1                  // Page Table Entry 크기 (1 Byte)

// 물리 프레임 할당 시작 PFN (예시에서 0x000, 0x001 등을 제외하고 0x003부터 시작 가능성 언급)
#define FIRST_DATA_PFN 3            // 데이터 프레임 할당 시작 (운영체제 예약 영역 제외)

//////////////////////////////////////////////////
// 2. 핵심 데이터 구조체 정의
//////////////////////////////////////////////////

// 페이지 테이블 엔트리 (1 Byte: 1-bit Present + 7-bit PFN)
typedef struct {
    uint8_t entry_value;
} PTE;

// TLB 엔트리 구조체
typedef struct {
    uint16_t vpn;           // 가상 페이지 번호 (9 비트)
    uint8_t pfn;            // 물리 프레임 번호 (7 비트)
    bool valid;             // 유효 비트

    // RR/LRU 정책 관리를 위한 필드
    uint64_t last_access_time; // LRU 정책용: 마지막 접근 시간
    uint8_t rr_index;       // RR 정책용: 교체 순서 (사용하지 않아도 됨, next_rr_tlb로 관리)
} TLBEntry;

// 물리 프레임 메타데이터 구조체
typedef struct {
    bool is_allocated;      // 할당 여부
    bool is_pagetable;      // 이 프레임이 페이지 테이블 자체를 저장하는지 (Swap 방지용)
    uint16_t vpn_mapped;    // 이 프레임에 매핑된 VPN (역매핑 정보)

    // RR/LRU 정책 관리를 위한 필드
    uint64_t last_access_time; // LRU 정책용: 마지막 접근 시간
    uint8_t rr_index;       // RR 정책용: 교체 순서 (사용하지 않아도 됨, next_rr_frame으로 관리)
} FrameEntry;

// 정책 구분용 Enum
typedef enum {
    POLICY_RR,
    POLICY_LRU
} ReplacementPolicy;

#endif