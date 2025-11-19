#include "simulator.h"
#include "log.h"
#include "util.h" // 주소 분할을 위해 포함
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 전역 변수 정의 (simulator.h에서 extern으로 선언됨)
// ------------------------------------------------
// LRU 관리를 위한 전역 시간 카운터
// (main.c에서 이미 정의되었으므로 여기서는 extern으로 사용)
// ReplacementPolicy current_policy; 
// uint64_t global_access_time;     

// 물리 메모리 시뮬레이션 및 메타데이터
uint8_t physical_memory[NUM_FRAMES * FRAME_SIZE];
FrameEntry frame_table[NUM_FRAMES];

// TLB 및 RR/LRU 포인터
TLBEntry tlb[TLB_SIZE];
uint8_t next_rr_tlb = 0;
uint8_t next_rr_frame = FIRST_DATA_PFN; // RR 포인터는 데이터 영역부터 시작
uint8_t next_free_frame = FIRST_DATA_PFN; // 초기에 사용할 수 있는 다음 프레임

// Root Page Directory의 PFN
uint8_t root_pd_pfn; 

// --- 임시/최소한의 메모리 관리 함수 (나중에 memory.c로 분리 예정) ---
// 이 단계에서는 초기화에 필요한 최소한의 할당 로직만 구현합니다.
uint8_t get_free_frame() {
    // 메모리가 꽉 찼는지 확인
    if (next_free_frame >= NUM_FRAMES) {
        // 실제 Page Swap 로직 (Part 1, 2)이 필요하지만, 초기화 단계에서는 메모리 부족으로 처리
        return 0; // 임시: 0은 유효하지 않은 PFN으로 가정 (PFN 1, 2는 시스템용)
    }
    return next_free_frame++;
}

// ------------------------------------------------
// 핵심 시뮬레이터 함수 구현
// ------------------------------------------------

// 시뮬레이터 초기화 및 정책 설정
void initialize_simulator(const char *policy_str) {
    // 1. 정책 설정
    if (strcmp(policy_str, "RR") == 0) {
        current_policy = POLICY_RR;
    } else if (strcmp(policy_str, "LRU") == 0) {
        current_policy = POLICY_LRU;
    } else {
        fprintf(stderr, "Error: Invalid replacement policy '%s'. Use RR or LRU.\n", policy_str);
        exit(EXIT_FAILURE);
    }
    printf("Simulator initialized with policy: %s\n", policy_str);

    // 2. 물리 메모리 및 TLB 초기화
    memset(physical_memory, 0, NUM_FRAMES * FRAME_SIZE);
    memset(frame_table, 0, NUM_FRAMES * sizeof(FrameEntry));
    memset(tlb, 0, TLB_SIZE * sizeof(TLBEntry));

    // 3. 시스템 예약 프레임 처리 (PFN 0, 1은 예약, 2는 Root PD로 설정)
    // 예시에서 0x000, 0x001은 bitmask에 사용되고 0x002는 Page Directory1에 사용됨
    
    // PFN 0과 1은 예약 (할당됨으로 표시)
    frame_table[0].is_allocated = true;
    frame_table[1].is_allocated = true;

    // 4. Root Page Directory 할당 (PFN 2)
    root_pd_pfn = 2;
    frame_table[root_pd_pfn].is_allocated = true;
    frame_table[root_pd_pfn].is_pagetable = true; 
    // root_pd_pfn은 RR 교체 대상에서 제외되어야 함

    // next_free_frame 포인터를 FIRST_DATA_PFN(3)부터 시작하도록 설정
    // 이 시점에서는 0, 1, 2가 사용되었으므로 next_free_frame은 3부터 시작해야 함
    next_free_frame = FIRST_DATA_PFN; // PFN 3부터 데이터 페이지 사용 가능

    // RR 프레임 포인터도 데이터 영역 시작부터 시작
    next_rr_frame = FIRST_DATA_PFN; 
}

// 입력 파일 읽기 및 주소 접근 시뮬레이션
void simulate_accesses(const char *input_file) {
    FILE *fp = fopen(input_file, "r");
    if (!fp) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    int total_accesses;
    // 첫 번째 줄에서 총 액세스 횟수를 읽습니다.
    if (fscanf(fp, "%d", &total_accesses) != 1) {
        fprintf(stderr, "Error reading total access count from input file.\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    
    uint16_t va;
    // 두 번째 줄부터 가상 주소(VA)를 읽어 시뮬레이션을 반복합니다.
    for (int i = 0; i < total_accesses; i++) {
        // VA는 16진수 포맷(0x...)으로 읽습니다.
        if (fscanf(fp, "%hx", &va) != 1) {
            fprintf(stderr, "Error reading virtual address at line %d.\n", i + 2);
            break;
        }
        
        // --- 핵심 주소 변환 함수 호출 ---
        translate_address(va);
        global_access_time++; // 접근 시간 갱신
    }

    fclose(fp);
}

// 가상 주소(VA)를 처리하는 주 로직 (다음 단계에서 구현할 핵심 함수)
void translate_address(uint16_t va) {
    // ----------------------------------------------------
    // TODO: Phase 1의 핵심 로직 (TLB -> PT -> Swap) 구현 예정
    // ----------------------------------------------------
    uint16_t vpn, offset;
    uint8_t pd1_idx, pd2_idx, pt_idx;

    // 1. VA 분리 및 로깅
    split_va(va, &vpn, &offset, &pd1_idx, &pd2_idx, &pt_idx);
    log_va_access(va);

    // *이후 로직은 다음 단계에서 Part 1의 RR 정책을 기반으로 구현됩니다.*
    
    // 이 임시 코드는 컴파일 테스트를 위한 것입니다.
    // printf("Processing VA: 0x%03x, VPN: 0x%03x, PD1: %u, PD2: %u, PT: %u, Offset: %u\n", 
    //        va, vpn, pd1_idx, pd2_idx, pt_idx, offset);
}
// ... (기존 simulator.c 내용) ...
#include "util.h" 

// TLB에서 VPN을 찾아 PFN을 반환 (Hit 시 true)
bool tlb_lookup(uint16_t vpn, uint8_t *pfn) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            *pfn = tlb[i].pfn;
            // RR 정책에서는 접근 시간 갱신이 필요 없음
            return true;
        }
    }
    return false;
}

// TLB에 새로운 엔트리를 Round-Robin 정책으로 업데이트합니다.
void tlb_update(uint16_t vpn, uint8_t pfn) {
    int index = next_rr_tlb;
    
    // RR 교체 위치 결정 및 포인터 업데이트
    // Part 2 (LRU)에서는 이 부분이 get_lru_tlb_index()로 대체됩니다.
    next_rr_tlb = (next_rr_tlb + 1) % TLB_SIZE; 
    
    // 업데이트
    tlb[index].vpn = vpn;
    tlb[index].pfn = pfn;
    tlb[index].valid = true;
    // LRU 시간 갱신 (Part 2에서 사용)
    tlb[index].last_access_time = global_access_time;
}

// 물리 메모리에서 PTE를 읽어옵니다.
// pfn: PTE 배열이 시작하는 프레임 번호
// index: 해당 프레임 내의 PTE 인덱스 (0~7)
PTE read_pte(uint8_t pfn, uint8_t index) {
    // PTE 배열 시작 주소: pfn * FRAME_SIZE
    // 실제 PTE 주소: pfn * FRAME_SIZE + index * PTE_SIZE
    size_t address = (size_t)pfn * FRAME_SIZE + (size_t)index * PTE_SIZE;
    PTE pte;
    
    // 물리 메모리는 uint8_t 배열이므로, 1 Byte를 직접 읽어옵니다.
    pte.entry_value = physical_memory[address];
    return pte;
}

// 3단계 페이지 테이블을 탐색하여 PFN을 반환합니다. (Hit 시 true)
bool page_table_lookup(uint16_t vpn, uint8_t *pfn_result) {
    uint16_t dummy_vpn, dummy_offset;
    uint8_t pd1_idx, pd2_idx, pt_idx;
    
    // 1. VA를 분리합니다. (vpn은 이미 주어졌지만, 인덱스 추출을 위해 다시 분리)
    split_va(0, &dummy_vpn, &dummy_offset, &pd1_idx, &pd2_idx, &pt_idx); // vpn을 인수로 전달해야 하지만, util.c의 구조를 가정하여 vpn을 그대로 사용

    // 실제로는 split_va가 vpn만으로 인덱스를 추출하도록 수정해야 하지만, 
    // 여기서는 vpn을 통해 다시 인덱스를 추출하는 로직을 사용합니다.
    pd1_idx = (vpn >> (PT_BITS + PD2_BITS)) & 0x7;
    pd2_idx = (vpn >> PT_BITS) & 0x7;
    pt_idx = vpn & 0x7;


    // 2. PD1 (Root Page Directory) 탐색
    uint8_t current_pfn = root_pd_pfn; // PFN 2
    PTE pd1_pte = read_pte(current_pfn, pd1_idx);
    
    if (!get_pte_present(pd1_pte)) {
        // PD1 Miss (Page Fault) -> PD2와 PT를 위한 프레임 할당 필요
        return false;
    }
    
    // 3. PD2 탐색
    current_pfn = get_pte_pfn(pd1_pte);
    PTE pd2_pte = read_pte(current_pfn, pd2_idx);

    if (!get_pte_present(pd2_pte)) {
        // PD2 Miss (Page Fault) -> PT를 위한 프레임 할당 필요
        return false;
    }

    // 4. 최종 Page Table 탐색
    current_pfn = get_pte_pfn(pd2_pte);
    PTE pt_pte = read_pte(current_pfn, pt_idx);

    if (get_pte_present(pt_pte)) {
        // PT Hit!
        *pfn_result = get_pte_pfn(pt_pte);
        log_pt_hit(vpn, *pfn_result);
        return true;
    } else {
        // PT Miss (Page Fault) -> 데이터 페이지를 위한 프레임 할당 필요
        return false;
    }
}

// Page Fault 발생 시 Page Table 구조를 업데이트합니다.
// 이 함수는 Page Fault가 발생한 단계까지의 PT/PD 구조를 동적으로 할당하고 PTE를 업데이트합니다.
void update_page_table_on_miss(uint16_t vpn, uint8_t data_pfn) {
    uint8_t pd1_idx, pd2_idx, pt_idx;
    // VA 분리 (split_va의 구조를 가정하고 vpn을 직접 파싱)
    pd1_idx = (vpn >> (PT_BITS + PD2_BITS)) & 0x7;
    pd2_idx = (vpn >> PT_BITS) & 0x7;
    pt_idx = vpn & 0x7;

    uint8_t current_pfn = root_pd_pfn; // Root Page Directory PFN (PFN 2)
    PTE pd1_pte, pd2_pte, pt_pte;
    uint8_t next_pfn;
    
    // 1. PD1 (Root) 업데이트 경로
    pd1_pte = read_pte(current_pfn, pd1_idx);
    if (!get_pte_present(pd1_pte)) {
        // PD1 미스: PD2를 저장할 새 프레임 할당
        next_pfn = allocate_frame(true); // is_pagetable = true
        // PD1 업데이트: (현재 pfn, index, 다음 pfn)
        PTE *pd1_loc = (PTE*)&physical_memory[current_pfn * FRAME_SIZE + pd1_idx * PTE_SIZE];
        set_pte_entry(pd1_loc, next_pfn);
        current_pfn = next_pfn;
    } else {
        current_pfn = get_pte_pfn(pd1_pte);
    }
    
    // 2. PD2 업데이트 경로
    pd2_pte = read_pte(current_pfn, pd2_idx);
    if (!get_pte_present(pd2_pte)) {
        // PD2 미스: 최종 Page Table을 저장할 새 프레임 할당
        next_pfn = allocate_frame(true); // is_pagetable = true
        // PD2 업데이트
        PTE *pd2_loc = (PTE*)&physical_memory[current_pfn * FRAME_SIZE + pd2_idx * PTE_SIZE];
        set_pte_entry(pd2_loc, next_pfn);
        current_pfn = next_pfn;
    } else {
        current_pfn = get_pte_pfn(pd2_pte);
    }

    // 3. 최종 Page Table (PT) 업데이트
    // PT 업데이트: data_pfn을 PTE에 저장하고 Present Bit을 1로 설정
    PTE *pt_loc = (PTE*)&physical_memory[current_pfn * FRAME_SIZE + pt_idx * PTE_SIZE];
    set_pte_entry(pt_loc, data_pfn);
    
    // Frame Table에 역매핑 정보 기록 (스왑 아웃 시 필요)
    frame_table[data_pfn].vpn_mapped = vpn; 
}


// 가상 주소(VA)를 처리하는 주 로직
void translate_address(uint16_t va) {
    uint16_t vpn, offset;
    uint8_t pd1_idx, pd2_idx, pt_idx;
    uint8_t pfn_result;
    
    // VA 분리 (VA 0x104의 경우, VPN 0x020, Offset 0x4)
    // util.c에서 구현된 split_va를 사용해야 하지만, 현재는 인덱스 추출만 필요하므로 vpn만 사용
    vpn = va >> OFFSET_BITS;
    offset = va & ((1 << OFFSET_BITS) - 1);
    
    log_va_access(va);
    
    // 1. TLB Lookup
    if (tlb_lookup(vpn, &pfn_result)) {
        // TLB Hit
        log_tlb_hit(vpn, pfn_result);
        
        // PA 계산: PA = PFN * FRAME_SIZE + Offset
        uint16_t pa = (pfn_result << OFFSET_BITS) | offset;
        log_pa_result(pa);
        return; // 성공적인 접근 종료
    }
    
    // TLB Miss
    log_tlb_miss(vpn);
    
    // 2. Page Table Lookup (3-Level)
    if (page_table_lookup(vpn, &pfn_result)) {
        // Page Table Hit (TLB Miss, PT Hit)
        
        // 3. TLB Update 및 재접근 (Access again)
        log_tlb_update(vpn, pfn_result);
        tlb_update(vpn, pfn_result);
        
        // 재접근 (재귀 호출)
        translate_address(va); 
        return;
    }
    
    // Page Table Miss (Page Fault)
    log_pt_miss(vpn);
    
    // 4. Page Swap & Page Table/TLB Update
    
    // 새 데이터 페이지를 위한 프레임 할당 (스왑 발생 가능)
    uint8_t new_data_pfn = allocate_frame(false); // is_pagetable = false
    
    // Page Table 구조 동적 생성/업데이트
    update_page_table_on_miss(vpn, new_data_pfn);
    
    // Page Table 업데이트 기록 및 TLB 업데이트
    log_pt_update(vpn, new_data_pfn);
    log_tlb_update(vpn, new_data_pfn);
    tlb_update(vpn, new_data_pfn);

    // 5. 재접근 (Access again)
    translate_address(va);
}

// simulator.c (LRU 정책 통합)

// ... (기존 tlb_lookup, page_table_lookup 등 함수 유지) ...

// TLB에 새로운 엔트리를 정책에 따라 업데이트합니다.
void tlb_update(uint16_t vpn, uint8_t pfn) {
    int index;
    
    if (current_policy == POLICY_RR) {
        // RR: 순환 인덱스 사용
        index = next_rr_tlb;
        next_rr_tlb = (next_rr_tlb + 1) % TLB_SIZE; 
    } else { // POLICY_LRU
        // LRU: LRU 인덱스 탐색 사용
        index = get_lru_tlb_index(); 
    }

    // 업데이트
    tlb[index].vpn = vpn;
    tlb[index].pfn = pfn;
    tlb[index].valid = true;
    
    // LRU 시간 갱신 (RR이든 LRU든, 실제 TLB 엔트리가 업데이트될 때 시간 기록)
    tlb[index].last_access_time = global_access_time;
}

// 가상 주소(VA)를 처리하는 주 로직
void translate_address(uint16_t va) {
    // ... (주소 분리 및 로그 va access) ...
    
    // 1. TLB Lookup
    if (tlb_lookup(vpn, &pfn_result)) {
        // TLB Hit
        log_tlb_hit(vpn, pfn_result);
        
        // **LRU 갱신:** LRU 정책일 경우에만 접근 시간을 갱신합니다.
        if (current_policy == POLICY_LRU) {
            update_tlb_time(vpn); // TLB 엔트리 갱신
            update_frame_time(pfn_result); // 프레임 엔트리 갱신
        }
        
        // ... (PA 계산 및 로그) ...
        return; 
    }
    
    // TLB Miss
    log_tlb_miss(vpn);
    
    // 2. Page Table Lookup (3-Level)
    if (page_table_lookup(vpn, &pfn_result)) {
        // Page Table Hit (TLB Miss, PT Hit)
        
        // **LRU 갱신:** LRU 정책일 경우 프레임 접근 시간 갱신
        if (current_policy == POLICY_LRU) {
            update_frame_time(pfn_result); // 프레임 엔트리 갱신
        }
        
        // 3. TLB Update 및 재접근 (Access again)
        log_tlb_update(vpn, pfn_result);
        tlb_update(vpn, pfn_result);
        
        // 재접근 (재귀 호출)
        translate_address(va); 
        return;
    }
    
    // Page Table Miss (Page Fault)
    log_pt_miss(vpn);
    
    // 4. Page Swap & Page Table/TLB Update
    uint8_t new_data_pfn = allocate_frame(false); // is_pagetable = false
    
    // Frame Table에 역매핑 정보 기록
    frame_table[new_data_pfn].vpn_mapped = vpn;

    update_page_table_on_miss(vpn, new_data_pfn);
    
    // Page Table 업데이트 기록 및 TLB 업데이트
    log_pt_update(vpn, new_data_pfn);
    log_tlb_update(vpn, new_data_pfn);
    tlb_update(vpn, new_data_pfn);

    // 5. 재접근 (Access again)
    translate_address(va);
}