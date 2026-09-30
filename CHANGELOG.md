# Changelog

이 저장소(2026-2 고급알고리즘 과제 1 — 정렬 비교)의 변경 기록. 형식은
[Keep a Changelog](https://keepachangelog.com/ko/1.1.0/)를 따르고,
버전은 [유의적 버전](https://semver.org/lang/ko/)을 따른다.

## [1.0.0] - 2026-09-30

제출본. 배우지 않은 정렬을 힙 정렬에서 칵테일 셰이커 정렬로 바꾸고, 측정 결과와
보고서를 넣었다.

### Added

- **칵테일 셰이커 정렬** (`src/cocktailShakerSort.c` · `.pseudo`). 버블 정렬을 앞 · 뒤로
  번갈아 돌리고, 교환이 없으면 멈춘다. 안정 · 제자리.
- **기준선 버블 정렬** (`src/bubbleSort.c`, `BASELINE_ALGORITHMS`). 수업 코드에 flag를
  넣은 판. 칵테일 정렬과 다른 점이 "뒤로 가는 회전" 하나만 남도록 했다.
- **거북이 · 토끼 입력** (`SHAPE_TURTLE` · `SHAPE_RABBIT`)과 **실험 4 `turtle`**.
- 테스트 11개 추가(89개): 거북이에서 버블 n(n-1)/2 · 칵테일 3n-6 비교, 두 정렬의
  이동이 같음, 버블 정렬의 슬라이드 숫자(이동 87 · 75, flag가 있을 때 정렬된 입력 비교 9).
- **보고서** (`report/REPORT.md`, `report/REPORT.pdf`)와 측정값 · 그래프 (`report/data`, `report/figures`).

### Changed

- 실험 1은 n = 10,000, 실험 2는 n = 2^10 → 2^15로 줄였다. 칵테일 정렬이 O(n²)이다.
- 실험 1 · 3은 네 입력 모양(무작위 · 정렬됨 · 역순 · 중복많음)만 쓴다.

### Removed

- 힙 정렬(`heapSort`, `buildMaxHeap`)과 실험 4 `heapbuild`.

## [0.1.0] - 2026-09-28

코드를 먼저 넣었다. 측정 결과와 보고서는 다음 판에 넣는다.

### Added

- **공통 인터페이스** (`src/sort.h`). 함수 포인터를 담은 구조체 `SortAlgorithm`과
  구현 표 `SORT_ALGORITHMS`로 세 정렬을 묶었다. 비교는 `qsort` 규약의 함수
  포인터로 받아 원소 타입을 모른다. 그래서 `(key, order)` 원소로 안정성을 잰다.
- **세는 곳을 한곳으로** (`src/sortutil.h`). 정렬은 원소를 `sortCompare` ·
  `sortCopy` · `sortSwap`으로만 만진다. 비교 · 이동 · 추가 메모리 · 재귀 깊이를
  정렬 코드가 아니라 여기서 센다.
- **세 정렬**: `mergeSort` · `quickSort` · `heapSort`. 병합과 퀵은 수업 코드
  (algorithm-code)의 절차를 원소 타입과 무관하게 옮겼고, 힙은 새로 썼다. 각
  정렬의 의사코드를 `src/*.pseudo`에 두었다.
- **퀵 정렬 변형 여섯** (`QUICK_VARIANTS`). 피벗(첫 원소 · 랜덤 · 셋 중 가운데)
  × 파티션(수업의 2-way · 3-way). 본 비교의 `quickSort`는 랜덤 · 2-way다.
- **유사난수 생성기** (`src/minstd.c`). 주제 04와 같은 minstd라 입력 배열과
  랜덤 피벗이 어느 기계에서든 같다. 비교 · 이동 횟수가 재현된다.
- **실험 넷** (`src/main.c`). 입력 모양별 · 배가 실험 · 퀵 정렬 변형 ·
  힙 만들기. 표로 찍거나 `--csv`로 찍는다.
- **그래프** (`tools/charts.py`, `tools/svgplot.py`). 표준 모듈만으로 SVG를
  그린다. `make charts`.
- **유닛 테스트 78개** (`tests/test_sort.c`). 구현 표를 훑으므로 정렬을 더 넣어도
  그대로 돈다. 기본 케이스 · `qsort` 대조 · `n = 0..300` 전수 · 안정성 주장과
  실측의 일치 · **슬라이드의 비교 · 이동 횟수 재현**(병합 22 · 68, 퀵 25 · 21 등)
  · 힙 성질과 "힙 만들기 비교 ≤ 2n" · minstd 수열까지 본다.
- `make sanitize`: ASan · UBSan을 붙여 테스트한다.

### Changed

- `Makefile`: 실험 프로그램과 테스트가 헤더나 옆 파일이 바뀌어도 다시 빌드되도록
  의존 파일을 모두 적었다. `make charts` · `make sanitize`를 더했다.
- `.vscode/launch.json`: `F5`(디버그 빌드)는 실험 1만 돌린다. 넷을 다 돌리면
  몇 분 걸린다.

### Removed

- template의 버블 정렬 예제와 Python 구현. 이 과제는 C로만 낸다.
