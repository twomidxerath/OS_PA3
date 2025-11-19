#include "simulator.h"
#include "log.h"
#include "util.h" // 주소 분할 및 PTE 조작 함수 포함
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

    // 시스템 예약 프레임 (PFN 0, 1)
    frame_table[0].is_allocated = true;
    frame_table[1].is_allocated = true;

    // Root Page Directory (PFN 2) 할당 (예시의 PFN 0x002는 PD1에 사용됨을 가정)
    root_pd_pfn = 2;
    frame_table[root_pd_pfn].is_allocated = true;
    frame_table[root_pd_pfn].is_pagetable = true; 

    // PFN 3부터 데이터 페이지 및 동적 PT 할당 시작
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
        translate_address(va);
        global_access_time++; 
    }
    fclose(fp);
}

// ------------------------------------------------
// 핵심 로직 (TLB, Page Table)
// ------------------------------------------------

// TLB Lookup
bool tlb_lookup(uint16_t vpn, uint8_t *pfn) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            *pfn = tlb[i].pfn;
            return true;
        }
    }
    return false;
}

// TLB Update (RR + LRU 지원)
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

// 물리 메모리에서 PTE 읽기
PTE read_pte(uint8_t pfn, uint8_t index) {
    size_t address = (size_t)pfn * FRAME_SIZE + (size_t)index * PTE_SIZE;
    PTE pte;
    pte.entry_value = physical_memory[address];
    return pte;
}

// 3단계 페이지 테이블 탐색
bool page_table_lookup(uint16_t vpn, uint8_t *pfn_result) {
    uint16_t dummy_va = vpn << OFFSET_BITS;
    uint16_t dummy_vpn, dummy_offset;
    uint8_t pd1_idx, pd2_idx, pt_idx;
    
    split_va(dummy_va, &dummy_vpn, &dummy_offset, &pd1_idx, &pd2_idx, &pt_idx);

    // 1. PD1 탐색
    uint8_t current_pfn = root_pd_pfn;
    PTE pd1_pte = read_pte(current_pfn, pd1_idx);
    if (!get_pte_present(pd1_pte)) return false;
    
    // 2. PD2 탐색
    current_pfn = get_pte_pfn(pd1_pte);
    PTE pd2_pte = read_pte(current_pfn, pd2_idx);
    if (!get_pte_present(pd2_pte)) return false;

    // 3. PT 탐색
    current_pfn = get_pte_pfn(pd2_pte);
    PTE pt_pte = read_pte(current_pfn, pt_idx);

    if (get_pte_present(pt_pte)) {
        *pfn_result = get_pte_pfn(pt_pte);
        log_pt_hit(vpn, *pfn_result);
        return true;
    }
    return false;
}

// Page Fault 처리 (테이블 생성 및 업데이트)
void update_page_table_on_miss(uint16_t vpn, uint8_t data_pfn) {
    uint16_t dummy_va = vpn << OFFSET_BITS;
    uint16_t dummy_vpn, dummy_offset;
    uint8_t pd1_idx, pd2_idx, pt_idx;
    split_va(dummy_va, &dummy_vpn, &dummy_offset, &pd1_idx, &pd2_idx, &pt_idx);

    uint8_t current_pfn = root_pd_pfn;
    uint8_t next_pfn;
    
    // 1. PD1 업데이트 경로
    size_t pd1_loc_addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pd1_idx * PTE_SIZE;
    PTE pd1_pte = read_pte(current_pfn, pd1_idx);
    
    if (!get_pte_present(pd1_pte)) {
        next_pfn = allocate_frame(true); 
        PTE *pd1_loc = (PTE*)&physical_memory[pd1_loc_addr];
        set_pte_entry(pd1_loc, next_pfn);
        current_pfn = next_pfn;
    } else {
        current_pfn = get_pte_pfn(pd1_pte);
    }
    
    // 2. PD2 업데이트 경로
    size_t pd2_loc_addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pd2_idx * PTE_SIZE;
    PTE pd2_pte = read_pte(current_pfn, pd2_idx);
    
    if (!get_pte_present(pd2_pte)) {
        next_pfn = allocate_frame(true);
        PTE *pd2_loc = (PTE*)&physical_memory[pd2_loc_addr];
        set_pte_entry(pd2_loc, next_pfn);
        current_pfn = next_pfn;
    } else {
        current_pfn = get_pte_pfn(pd2_pte);
    }

    // 3. 최종 Page Table (PT) 업데이트
    size_t pt_loc_addr = (size_t)current_pfn * FRAME_SIZE + (size_t)pt_idx * PTE_SIZE;
    PTE *pt_loc = (PTE*)&physical_memory[pt_loc_addr];
    set_pte_entry(pt_loc, data_pfn);
    
    frame_table[data_pfn].vpn_mapped = vpn; 
}

// 주소 변환 메인 함수
void translate_address(uint16_t va) {
    uint16_t vpn = va >> OFFSET_BITS;
    uint16_t offset = va & ((1 << OFFSET_BITS) - 1);
    uint8_t pfn_result;
    
    log_va_access(va);
    
    // 1. TLB Lookup
    if (tlb_lookup(vpn, &pfn_result)) {
        log_tlb_hit(vpn, pfn_result);
        
        if (current_policy == POLICY_LRU) {
            update_tlb_time(vpn); 
            update_frame_time(pfn_result); 
        }
        
        uint16_t pa = (pfn_result << OFFSET_BITS) | offset;
        log_pa_result(pa);
        return; 
    }
    
    log_tlb_miss(vpn);
    
    // 2. Page Table Lookup
    if (page_table_lookup(vpn, &pfn_result)) {
        // Page Table Hit (TLB Miss, PT Hit)
        
        if (current_policy == POLICY_LRU) {
            update_frame_time(pfn_result);
        }
        
        log_tlb_update(vpn, pfn_result);
        tlb_update(vpn, pfn_result);
        
        translate_address(va); // 재접근
        return;
    }
    
    // Page Table Miss (Page Fault)
    log_pt_miss(vpn);
    
    // 3. Page Swap & Update
    uint8_t new_data_pfn = allocate_frame(false); // 데이터 페이지 할당
    
    // **[수정]** 로그 기록 및 PTE 업데이트
    log_pt_update(vpn, new_data_pfn); // 누락된 Page Table Update 로그 기록
    
    update_page_table_on_miss(vpn, new_data_pfn);
    
    // TLB 업데이트
    log_tlb_update(vpn, new_data_pfn);
    tlb_update(vpn, new_data_pfn);

    translate_address(va); // 재접근
}