#include "simulator.h"
#include "log.h"
#include "util.h"
#include <stdio.h>

// --- 헬퍼 함수: 스왑 아웃 시 TLB와 Page Table 무효화 ---
void invalidate_swapped_out_page(uint8_t pfn) {
    uint16_t vpn = frame_table[pfn].vpn_mapped;
    
    // 1. TLB 무효화 (Valid bit = 0)
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            tlb[i].valid = false;
        }
    }

    // 2. Page Table 무효화 (Present bit = 0)
    // VPN을 이용해 해당 PTE 위치를 찾아야 합니다.
    uint16_t dummy_va = vpn << OFFSET_BITS;
    uint16_t dummy_vpn, dummy_offset;
    uint8_t pd1_idx, pd2_idx, pt_idx;
    
    split_va(dummy_va, &dummy_vpn, &dummy_offset, &pd1_idx, &pd2_idx, &pt_idx);

    // PD1 탐색
    uint8_t current_pfn = root_pd_pfn;
    size_t pd1_addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pd1_idx * PTE_SIZE;
    if (!(physical_memory[pd1_addr] & 0x80)) return; // PD1 부재 시 중단
    
    // PD2 탐색
    current_pfn = physical_memory[pd1_addr] & 0x7F;
    size_t pd2_addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pd2_idx * PTE_SIZE;
    if (!(physical_memory[pd2_addr] & 0x80)) return; // PD2 부재 시 중단

    // PT 탐색 및 Present bit 클리어
    current_pfn = physical_memory[pd2_addr] & 0x7F;
    size_t pt_addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pt_idx * PTE_SIZE;
    
    // Present bit (최상위 비트)을 0으로 설정
    physical_memory[pt_addr] &= 0x7F; 
}


// --- LRU/RR 공통 및 헬퍼 함수 ---

void update_tlb_time(uint16_t vpn) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            tlb[i].last_access_time = global_access_time;
            return;
        }
    }
}

void update_frame_time(uint8_t pfn) {
    if (pfn < NUM_FRAMES) {
        frame_table[pfn].last_access_time = global_access_time;
    }
}

int get_lru_tlb_index() {
    uint64_t min_time = -1; 
    int lru_index = -1;
    
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!tlb[i].valid) return i; // 빈 곳이 있으면 우선 사용
        if (tlb[i].last_access_time < min_time) {
            min_time = tlb[i].last_access_time;
            lru_index = i;
        }
    }
    return lru_index;
}

uint8_t get_rr_eviction_frame() {
    uint8_t start_frame = next_rr_frame;
    uint8_t evict_frame = 0;
    
    for (int i = 0; i < NUM_FRAMES; i++) {
        evict_frame = start_frame;
        start_frame = (start_frame + 1) % NUM_FRAMES;

        if (evict_frame < 3) continue; // 0, 1, 2 (Root PD) 보호
        
        if (frame_table[evict_frame].is_allocated && !frame_table[evict_frame].is_pagetable) {
            next_rr_frame = start_frame;
            return evict_frame;
        }
    }
    return 0;
}

uint8_t get_lru_eviction_frame() {
    uint64_t min_time = -1;
    uint8_t lru_pfn = 0; 

    for (uint8_t pfn = 3; pfn < NUM_FRAMES; pfn++) { // 0,1,2 보호
        if (!frame_table[pfn].is_allocated) continue;
        if (frame_table[pfn].is_pagetable) continue;

        if (frame_table[pfn].last_access_time < min_time) {
            min_time = frame_table[pfn].last_access_time;
            lru_pfn = pfn;
        }
    }
    return lru_pfn;
}

// --- 프레임 할당 (Eviction 포함) ---
uint8_t allocate_frame(bool is_pagetable) {
    uint8_t new_pfn = 0;

    // 1. 빈 프레임이 있으면 사용
    if (next_free_frame < NUM_FRAMES) {
        new_pfn = next_free_frame;
        next_free_frame++;
    } else {
        // 2. 메모리가 가득 차면 Eviction 수행
        uint8_t evict_pfn = 0;
        
        if (current_policy == POLICY_RR) {
            evict_pfn = get_rr_eviction_frame();
        } else if (current_policy == POLICY_LRU) {
            evict_pfn = get_lru_eviction_frame();
        } else {
            fprintf(stderr, "Error: Unknown policy.\n");
            return 0;
        }

        if (evict_pfn == 0) {
            fprintf(stderr, "Fatal Error: No evictable data frame found.\n");
            return 0;
        }

        // 스왑 아웃 처리 (TLB/PT 무효화)
        invalidate_swapped_out_page(evict_pfn);
        new_pfn = evict_pfn;
    }

    // 3. 메타데이터 업데이트
    frame_table[new_pfn].is_allocated = true;
    frame_table[new_pfn].is_pagetable = is_pagetable;
    frame_table[new_pfn].last_access_time = global_access_time;
    
    return new_pfn;
}