import argparse
import random
import math
import sys

# --- 시스템 상수 정의 ---
N = 512             # 총 고유 페이지 수 (VPN 0x000 ~ 0x1FF)
TOTAL_ACCESSES = 10000 # 시뮬레이터 테스트를 위한 총 접근 횟수
OFFSET_BITS = 3     # Offset 비트 수 (3 bits for 8-Byte page size)

# --- 헬퍼 함수: VA 생성 ---
def generate_va(vpn):
    """VPN과 랜덤 오프셋을 결합하여 12-bit VA를 생성합니다."""
    # 3-bit 오프셋 (0 ~ 7)을 균등하게 생성
    offset = random.randint(0, (1 << OFFSET_BITS) - 1)
    
    # VA = (VPN << OFFSET_BITS) | Offset
    va = (vpn << OFFSET_BITS) | offset
    return va

# --- 1. Uniform Distribution (균등 분포) ---
def generate_uniform_trace(n_pages, total_accesses):
    """균등 분포에 따라 메모리 접근 트레이스를 생성합니다."""
    trace = []
    # 0부터 N-1 (511)까지의 VPN을 균등하게 선택
    for _ in range(total_accesses):
        vpn = random.randint(0, n_pages - 1)
        va = generate_va(vpn)
        trace.append(va)
    return trace

# --- 2. Zipfian Distribution (집피안 분포) ---
def generate_zipfian_trace(n_pages, s, total_accesses):
    """Zipfian 분포에 따라 메모리 접근 트레이스를 생성합니다."""
    trace = []
    
    # 1. 일반화된 조화 수 (Harmonic Number) H_N,s 계산: H_N,s = sum(1/i^s)
    h_n_s = sum(1.0 / math.pow(i, s) for i in range(1, n_pages + 1))
    
    # 2. 확률 질량 함수 (PMF) 및 누적 분포 함수 (CDF) 계산
    # PMF(k) = (1/k^s) / H_N,s
    pmf = []
    cdf = []
    cumulative_prob = 0.0
    
    for rank in range(1, n_pages + 1):
        prob = (1.0 / math.pow(rank, s)) / h_n_s
        pmf.append(prob)
        cumulative_prob += prob
        cdf.append(cumulative_prob)
    
    # 3. Zipfian 분포에 따른 VPN 선택 (CDF를 이용한 역변환)
    # 랭크를 VPN에 매핑 (가장 인기 있는 랭크 1이 VPN 0에 매핑된다고 가정)
    ranks_to_vpn = {rank: rank - 1 for rank in range(1, n_pages + 1)}
    
    for _ in range(total_accesses):
        # 0.0 ~ 1.0 사이의 난수 생성
        r = random.random()
        
        # CDF 테이블에서 랭크 찾기
        selected_rank = 0
        for i in range(n_pages):
            if r <= cdf[i]:
                selected_rank = i + 1
                break
        
        # 랭크를 VPN으로 변환
        vpn = ranks_to_vpn[selected_rank]
        
        # VA 생성 및 트레이스에 추가
        va = generate_va(vpn)
        trace.append(va)
        
    return trace

# --- 메인 실행 로직 ---
def main():
    parser = argparse.ArgumentParser(
        description="가상 메모리 시뮬레이터용 메모리 접근 트레이스 생성기",
        formatter_class=argparse.RawTextHelpFormatter
    )
    # --dist 또는 -d 옵션
    parser.add_argument(
        '--dist', '-d',
        type=str,
        required=True,
        choices=['uniform', 'zipfian'],
        help='생성할 분포 유형: "uniform" 또는 "zipfian".'
    )
    # --s 또는 -s 옵션
    parser.add_argument(
        '--s', '-s',
        type=float,
        default=1.0,
        help='Zipfian 분포의 skew 파라미터 (s). 기본값은 1.0.'
    )

    args = parser.parse_args()
    
    if args.dist == 'uniform':
        trace = generate_uniform_trace(N, TOTAL_ACCESSES)
    elif args.dist == 'zipfian':
        trace = generate_zipfian_trace(N, args.s, TOTAL_ACCESSES)
    else:
        # 이 경로에는 도달하지 않아야 함 (choices로 제한됨)
        sys.exit("Invalid distribution type.")

    # --- 출력 형식: 첫 줄은 총 횟수, 그 다음부터 각 VA (hex) ---
    print(len(trace))
    for va in trace:
        print(f"0x{va:03x}")
        
    # 마지막 줄은 빈 줄 (Trailing newline)
    print()

if __name__ == '__main__':
    main()