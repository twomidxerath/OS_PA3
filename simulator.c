#include "simulator.h"
#include "log.h"
#include "util.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 전역 변수 정의
uint8_t physical_memory[NUM_FRAMES * FRAME_SIZE];
FrameEntry frame_table[NUM_FRAMES];
TLBEntry tlb[TLB_SIZE];
uint8_t next_rr_tlb = 0;
uint8_t next_rr_frame = FIRST_DATA_PFN;
uint8_t next_free_frame = FIRST_DATA_PFN;
uint8_t root_pd_pfn; 

// 시뮬레이터 초기화
void initialize_simulator(const char *policy_str) {
    if (strcmp(policy_str, "RR") == 0) {
        current_policy = POLICY_RR;
    } else if (strcmp(policy_str, "LRU") == 0) {
        current_policy = POLICY_LRU;
    } else {
        fprintf(stderr, "Error: Invalid replacement policy '%s'. Use RR or LRU.\n", policy_str);
        exit(EXIT_FAILURE);
    }
    printf("Simulator initialized with policy: %s\n", policy_str);

    memset(physical_memory, 0, NUM_FRAMES * FRAME_SIZE);
    memset(frame_table, 0, NUM_FRAMES * sizeof(FrameEntry));
    memset(tlb, 0, TLB_SIZE * sizeof(TLBEntry));

    frame_table[0].is_allocated = true;
    frame_table[1].is_allocated = true;
    root_pd_pfn = 2;
    frame_table[root_pd_pfn].is_allocated = true;
    frame_table[root_pd_pfn].is_pagetable = true; 

    next_free_frame = FIRST_DATA_PFN; 
    next_rr_frame = FIRST_DATA_PFN; 
}

// 파일 읽기 및 시뮬레이션 루프
void simulate_accesses(const char *input_file) {
    FILE *fp = fopen(input_file, "r");
    if (!fp) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    int total_accesses;
    if (fscanf(fp, "%d", &total_accesses) != 1) {
        fprintf(stderr, "Error reading total access count.\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    
    uint16_t va;
    for (int i = 0; i < total_accesses; i++) {
        if (fscanf(fp, "%hx", &va) != 1) {
            break;
        }
        // [중요] 외부 접근 시 최초 Access VA 로그 출력
        log_va_access(va);
        translate_address(va);
        global_access_time++; 
    }
    fclose(fp);
}

// ------------------------------------------------
// 핵심 로직
// ------------------------------------------------

bool tlb_lookup(uint16_t vpn, uint8_t *pfn) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            *pfn = tlb[i].pfn;
            return true;
        }
    }
    return false;
}

void tlb_update(uint16_t vpn, uint8_t pfn) {
    int index;
    if (current_policy == POLICY_RR) {
        index = next_rr_tlb;
        next_rr_tlb = (next_rr_tlb + 1) % TLB_SIZE; 
    } else { 
        index = get_lru_tlb_index(); 
    }
    tlb[index].vpn = vpn;
    tlb[index].pfn = pfn;
    tlb[index].valid = true;
    tlb[index].last_access_time = global_access_time;
}

PTE read_pte(uint8_t pfn, uint8_t index) {
    size_t address = (size_t)pfn * FRAME_SIZE + (size_t)index * PTE_SIZE;
    PTE pte;
    pte.entry_value = physical_memory[address];
    return pte;
}

bool page_table_lookup(uint16_t vpn, uint8_t *pfn_result) {
    uint8_t pd1_idx = (vpn >> 6) & 0x7;
    uint8_t pd2_idx = (vpn >> 3) & 0x7;
    uint8_t pt_idx = vpn & 0x7;

    uint8_t current_pfn = root_pd_pfn;
    PTE pd1_pte = read_pte(current_pfn, pd1_idx);
    if (!get_pte_present(pd1_pte)) return false;
    
    current_pfn = get_pte_pfn(pd1_pte);
    PTE pd2_pte = read_pte(current_pfn, pd2_idx);
    if (!get_pte_present(pd2_pte)) return false;

    current_pfn = get_pte_pfn(pd2_pte);
    PTE pt_pte = read_pte(current_pfn, pt_idx);

    if (get_pte_present(pt_pte)) {
        *pfn_result = get_pte_pfn(pt_pte);
        log_pt_hit(vpn, *pfn_result);
        return true;
    }
    return false;
}

void update_page_table_on_miss(uint16_t vpn, uint8_t data_pfn) {
    uint8_t pd1_idx = (vpn >> 6) & 0x7;
    uint8_t pd2_idx = (vpn >> 3) & 0x7;
    uint8_t pt_idx = vpn & 0x7;

    uint8_t current_pfn = root_pd_pfn;
    uint8_t next_pfn;
    size_t addr;
    
    // 1. PD1
    PTE pd1_pte = read_pte(current_pfn, pd1_idx);
    if (!get_pte_present(pd1_pte)) {
        next_pfn = allocate_frame(true); 
        addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pd1_idx * PTE_SIZE;
        set_pte_entry((PTE*)&physical_memory[addr], next_pfn);
        current_pfn = next_pfn;
    } else {
        current_pfn = get_pte_pfn(pd1_pte);
    }
    
    // 2. PD2
    PTE pd2_pte = read_pte(current_pfn, pd2_idx);
    if (!get_pte_present(pd2_pte)) {
        next_pfn = allocate_frame(true);
        addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pd2_idx * PTE_SIZE;
        set_pte_entry((PTE*)&physical_memory[addr], next_pfn);
        current_pfn = next_pfn;
    } else {
        current_pfn = get_pte_pfn(pd2_pte);
    }

    // 3. PT
    addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pt_idx * PTE_SIZE;
    set_pte_entry((PTE*)&physical_memory[addr], data_pfn);
    
    frame_table[data_pfn].vpn_mapped = vpn; 
}

void translate_address(uint16_t va) {
    uint16_t vpn = va >> OFFSET_BITS;
    uint16_t offset = va & ((1 << OFFSET_BITS) - 1);
    uint8_t pfn;
    
    // 1. TLB Lookup
    if (tlb_lookup(vpn, &pfn)) {
        log_tlb_hit(vpn, pfn);
        if (current_policy == POLICY_LRU) {
            update_tlb_time(vpn); 
            update_frame_time(pfn); 
        }
        log_pa_result((pfn << OFFSET_BITS) | offset);
        return; 
    }
    log_tlb_miss(vpn);
    
    // 2. Page Table Lookup
    if (page_table_lookup(vpn, &pfn)) {
        if (current_policy == POLICY_LRU) update_frame_time(pfn);
        log_tlb_update(vpn, pfn);
        tlb_update(vpn, pfn);
        
        log_va_access(va); // 재접근 로그
        translate_address(va);
        return;
    }
    log_pt_miss(vpn);
    
    // 3. Page Swap
    uint8_t new_pfn = allocate_frame(false); 
    log_pt_update(vpn, new_pfn); 
    update_page_table_on_miss(vpn, new_pfn);
    
    log_tlb_update(vpn, new_pfn);
    tlb_update(vpn, new_pfn); // [수정완료] 변수명 new_pfn으로 통일

    log_va_access(va); // 재접근 로그
    translate_address(va);
}