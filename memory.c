#include "simulator.h"
#include "log.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h> // [수정] exit, EXIT_FAILURE 사용을 위해 필수

// --- 헬퍼 함수들 (allocate_frame보다 먼저 정의되어야 함) ---

void invalidate_swapped_out_page(uint8_t pfn) {
    uint16_t vpn = frame_table[pfn].vpn_mapped;
    
    // TLB 무효화
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            tlb[i].valid = false;
        }
    }

    // Page Table 무효화
    uint16_t dummy_va = vpn << OFFSET_BITS;
    uint16_t dummy_vpn, dummy_offset;
    uint8_t pd1_idx, pd2_idx, pt_idx;
    
    split_va(dummy_va, &dummy_vpn, &dummy_offset, &pd1_idx, &pd2_idx, &pt_idx);

    uint8_t current_pfn = root_pd_pfn;
    size_t pd1_addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pd1_idx * PTE_SIZE;
    if (!(physical_memory[pd1_addr] & 0x80)) return;
    
    current_pfn = physical_memory[pd1_addr] & 0x7F;
    size_t pd2_addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pd2_idx * PTE_SIZE;
    if (!(physical_memory[pd2_addr] & 0x80)) return;

    current_pfn = physical_memory[pd2_addr] & 0x7F;
    size_t pt_addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pt_idx * PTE_SIZE;
    
    physical_memory[pt_addr] &= 0x7F; // Present bit 클리어
}

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
        if (!tlb[i].valid) return i;
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

        if (evict_frame < 3) continue; // 0, 1, 2 보호
        
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

    for (uint8_t pfn = 3; pfn < NUM_FRAMES; pfn++) {
        if (!frame_table[pfn].is_allocated) continue;
        if (frame_table[pfn].is_pagetable) continue;

        if (frame_table[pfn].last_access_time < min_time) {
            min_time = frame_table[pfn].last_access_time;
            lru_pfn = pfn;
        }
    }
    return lru_pfn;
}

// --- 프레임 할당 (메인 함수) ---
uint8_t allocate_frame(bool is_pagetable) {
    uint8_t new_pfn = 0;

    if (next_free_frame < NUM_FRAMES) {
        new_pfn = next_free_frame;
        next_free_frame++;
    } else {
        uint8_t evict_pfn = 0;
        
        if (current_policy == POLICY_RR) {
            evict_pfn = get_rr_eviction_frame();
        } else if (current_policy == POLICY_LRU) {
            evict_pfn = get_lru_eviction_frame();
        } else {
            fprintf(stderr, "Error: Unknown policy.\n");
            exit(EXIT_FAILURE);
        }

        if (evict_pfn == 0) {
            fprintf(stderr, "Fatal Error: No evictable data frame found.\n");
            exit(EXIT_FAILURE);
        }

        invalidate_swapped_out_page(evict_pfn);
        new_pfn = evict_pfn;
    }

    frame_table[new_pfn].is_allocated = true;
    frame_table[new_pfn].is_pagetable = is_pagetable;
    frame_table[new_pfn].last_access_time = global_access_time;
    
    return new_pfn;
}