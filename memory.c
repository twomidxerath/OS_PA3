// memory.c (LRU 로직 추가 및 allocate_frame 수정)

#include "simulator.h"
#include "log.h"
#include "util.h"
#include <stdio.h>

// --- LRU 정책 헬퍼 함수 ---

// TLB 엔트리의 접근 시간을 갱신합니다.
void update_tlb_time(uint16_t vpn) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            tlb[i].last_access_time = global_access_time;
            return;
        }
    }
}

// 프레임 엔트리의 접근 시간을 갱신합니다.
void update_frame_time(uint8_t pfn) {
    if (pfn < NUM_FRAMES) {
        frame_table[pfn].last_access_time = global_access_time;
    }
}

// TLB에서 LRU 엔트리의 인덱스를 찾습니다. (가장 작은 time)
int get_lru_tlb_index() {
    uint64_t min_time = -1; // uint64_t의 최댓값
    int lru_index = -1;
    
    for (int i = 0; i < TLB_SIZE; i++) {
        // 1. 유효하지 않은 엔트리가 있으면 바로 그 자리를 사용
        if (!tlb[i].valid) {
            return i;
        }
        
        // 2. 가장 작은 last_access_time을 가진 엔트리를 찾습니다.
        if (tlb[i].last_access_time < min_time) {
            min_time = tlb[i].last_access_time;
            lru_index = i;
        }
    }
    return lru_index;
}

// Frame Table에서 LRU 프레임의 PFN을 찾습니다. (Page Swap 대상)
uint8_t get_lru_eviction_frame() {
    uint64_t min_time = -1;
    uint8_t lru_pfn = 0; 

    for (uint8_t pfn = FIRST_DATA_PFN; pfn < NUM_FRAMES; pfn++) {
        // 할당되지 않은 프레임은 건너뜁니다.
        if (!frame_table[pfn].is_allocated) continue;

        // 페이지 테이블 프레임은 스왑 대상에서 제외합니다.
        if (frame_table[pfn].is_pagetable) continue;

        // 가장 오래된 (가장 작은 시간) 프레임을 찾습니다.
        if (frame_table[pfn].last_access_time < min_time) {
            min_time = frame_table[pfn].last_access_time;
            lru_pfn = pfn;
        }
    }
    
    return lru_pfn; // 0이 반환되면 Eviction 대상이 없다는 뜻
}

// --- allocate_frame 수정 (정책 기반 Eviction) ---
uint8_t allocate_frame(bool is_pagetable) {
    uint8_t new_pfn = 0;

    // 1. 빈 프레임 찾기 (메모리 초기에만 적용)
    if (next_free_frame < NUM_FRAMES) {
        new_pfn = next_free_frame;
        next_free_frame++;
    } else {
        // 2. 메모리가 꽉 찬 경우: 정책에 따라 Eviction 수행
        uint8_t evict_pfn = 0;
        
        if (current_policy == POLICY_RR) {
            evict_pfn = get_rr_eviction_frame();
        } else if (current_policy == POLICY_LRU) {
            evict_pfn = get_lru_eviction_frame(); // **LRU 교체**
        } else {
            fprintf(stderr, "Error: Unknown policy.\n");
            exit(EXIT_FAILURE);
        }

        if (evict_pfn == 0) {
            fprintf(stderr, "Fatal Error: No evictable data frame found.\n");
            exit(EXIT_FAILURE);
        }

        // 스왑 아웃 로직
        new_pfn = evict_pfn;
        invalidate_swapped_out_page(new_pfn);
    }

    // 3. 새 프레임 정보 업데이트 및 LRU 시간 초기화
    frame_table[new_pfn].is_allocated = true;
    frame_table[new_pfn].is_pagetable = is_pagetable;
    frame_table[new_pfn].last_access_time = global_access_time; // 접근 시간 초기화
    // vpn_mapped는 Page Fault 처리 시에 업데이트됨
    
    return new_pfn;
}