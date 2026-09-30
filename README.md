# 과제 1 — 정렬 비교: 병합 · 퀵 · 힙

2026-2 **고급알고리즘**(SIT2001-01) 과제 1. 배운 정렬 둘(병합, 퀵)과 배우지
않은 정렬 하나(힙)를 **하나의 공통 인터페이스**로 묶고, 같은 잣대로 시간 ·
비교 · 이동 · 메모리 · 재귀 깊이 · 안정성을 잰다.

| 정렬 | 구분 | 평균 | 최악 | 추가 공간 | 안정 |
| --- | --- | --- | --- | --- | --- |
| 병합 `mergeSort` | 배운 정렬 (주제 03) | O(n log n) | O(n log n) | O(n) | 안정 |
| 퀵 `quickSort` | 배운 정렬 (주제 04) | O(n log n) | O(n²) | O(log n) 스택 | 불안정 |
| 힙 `heapSort` | **배우지 않은 정렬** | O(n log n) | O(n log n) | O(1) | 불안정 |

- 보고서: `report/REPORT.md` (작성 중)
- 강의 자료: [lec-algorithm.github.io/lecture](https://lec-algorithm.github.io/lecture/)
- 출발점: [lec-algorithm/algorithm-env](https://github.com/lec-algorithm/algorithm-env) template

## 돌려보기

컨테이너(Codespaces) 터미널에서 친다.

```sh
make test      # 유닛 테스트 78개
make run       # 실험 1~4를 표로 (1분 가까이 걸린다)
make charts    # 실험을 CSV로 받아 report/ 아래에 그래프(SVG)를 그린다
```

실험 하나만 돌릴 수도 있다.

```sh
./src/main.out shapes          # 실험 1만 표로
./src/main.out quick --csv     # 실험 3을 CSV로
```

| 명령 | 하는 일 |
| --- | --- |
| `make test` | 유닛 테스트. 하나라도 실패하면 0이 아닌 코드로 끝난다 |
| `make run` | 실험 1~4를 사람이 읽는 표로 |
| `make charts` | 측정값을 `report/data/*.csv`로, 그래프를 `report/figures/*.svg`로 |
| `make sanitize` | ASan · UBSan(메모리 · 미정의 동작 검사기)을 붙여 테스트 |
| `make debug` | 디버그 심볼을 넣어 빌드 (VS Code의 `F5`) |
| `make clean` | 빌드 산출물 정리 |

## 무엇을 재나

| 재는 것 | 방법 |
| --- | --- |
| 시간 | `clock()`. 입력 복사는 빼고 정렬만, 여러 번 잰 값의 가운데 값 |
| 비교 · 이동 | 정렬이 원소를 만지는 길(`sortutil.h`)이 한곳에서 센다. 교환 한 번은 이동 3 |
| 추가 메모리 | 입력 배열 밖에 잡은 작업 공간의 최댓값(바이트) |
| 재귀 깊이 | 원소 둘 이상을 맡은 재귀 호출이 쌓인 최대 깊이 |
| 안정성 | 원소를 `(key, order)`로 만들어, 정렬 뒤 같은 key끼리 입력 순서가 남았는지 실측 |

입력은 무작위 · 정렬됨 · 역순 · 중복많음(값 8가지) 네 모양이다. 난수는
주제 04의 minstd 생성기라 **어느 기계에서든 같은 입력이 나오고, 비교 · 이동
횟수도 같다.** 시간만 기계를 탄다.

| 실험 | 이름 | 조건 | 보려는 것 |
| --- | --- | --- | --- |
| 1 | `shapes` | n = 100,000, 네 입력 모양 | 입력에 따라 누가 흔들리나 |
| 2 | `growth` | 무작위, n = 2¹⁴ → 2²² 두 배씩 | 배가 실험. n log n 앞의 상수, 큰 n에서의 캐시 |
| 3 | `quick` | 피벗 3 × 파티션 2, n = 2,500 → 20,000 | 퀵 정렬이 무너지는 입력과 그 해법 |
| 4 | `heapbuild` | 힙 만들기만, n = 2¹⁰ → 2²² | "힙 만들기는 O(n)"이 맞는가 |

## 파일 구성

```plaintext
src/
├── sort.h                 공개 인터페이스: SortAlgorithm · SortStats · 구현 표
├── sortutil.h · sort.c    구현 전용 도구(비교 · 이동 · 교환을 세는 곳)와 구현 표
├── mergeSort.c            병합 정렬 (수업 코드를 원소 타입과 무관하게 옮김)
├── quickSort.c            퀵 정렬 + 피벗 · 파티션 변형 여섯
├── heapSort.c             힙 정렬 (배우지 않은 정렬)
├── *.pseudo               세 정렬의 의사코드. 구현은 이것을 옮긴 것이다
├── minstd.h · minstd.c    유사난수 생성기 (주제 04와 같은 식)
├── bench.h · bench.c      입력 만들기, 시간 · 안정성 재기
└── main.c                 실험 1~4 (표 또는 CSV)
tests/test_sort.c          유닛 테스트 (표준 C만)
tools/charts.py            CSV → 그래프. 그림은 tools/svgplot.py가 표준 모듈만으로 찍는다
```

## 규약

- 외부 라이브러리를 쓰지 않는다. C는 표준 라이브러리, Python은 표준 모듈만.
- 이 과제는 C로만 낸다. template의 버블 정렬 · Python 예제는 지웠다.
  `tools/`의 Python은 그래프를 그리는 데만 쓴다.
- 실행 파일은 `*.out`으로 만든다. `.gitignore`가 그것만 걸러낸다.
- 컴파일 경고 없이 빌드되고 `make test`가 통과해야 커밋한다.

컨테이너 · VS Code 설정(`.devcontainer/`, `.vscode/`, `Dockerfile`,
`compose.yml`)은 template 그대로다. 쓰는 법은
[algorithm-env의 README](https://github.com/lec-algorithm/algorithm-env)에 있다.

## 변경 기록

[CHANGELOG.md](CHANGELOG.md)
