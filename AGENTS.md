# AGENTS.md

2026-2 고급알고리즘(SIT2001-01) **과제 1 — 정렬 비교(병합 · 퀵 · 칵테일 셰이커)** 저장소.
이 문서는 이 저장소에서 작업하는 AI 도구를 위한 가이드다.
[algorithm-env](https://github.com/lec-algorithm/algorithm-env) template에서 시작했다.

## 이 저장소의 범위

- 정렬은 셋이다: `mergeSort` · `quickSort` · `cocktailShakerSort`. 퀵 정렬만 실험 3을
  위해 피벗 · 파티션 변형 여섯(`QUICK_VARIANTS`)을 더 가지고, 실험 4의 기준선으로
  버블 정렬(`BASELINE_ALGORITHMS`)이 있다. 정렬을 늘리지 않는다.
- 병합과 퀵은 수업 코드(algorithm-code)와 **같은 절차**여야 한다. 테스트가
  슬라이드의 비교 · 이동 횟수(병합 22 · 68, 퀵 25 · 21 등)를 그대로 확인한다.
  세는 규칙을 바꾸면 이 테스트가 깨진다.
- 보고서는 `report/REPORT.md`에서 PDF로 낸다. 측정값은 `report/data/*.csv`,
  그래프는 `report/figures/*.svg`이고 `make charts`가 다시 만든다. 손으로 고치지 않는다.

## 구조와 규약

```plaintext
src/    sort.h · sortutil.h · sort.c · mergeSort.c · quickSort.c · cocktailShakerSort.c · bubbleSort.c
        minstd.h · minstd.c · bench.h · bench.c · main.c · *.pseudo
tests/  test_sort.c
tools/  charts.py · svgplot.py
```

- **외부 라이브러리를 쓰지 않는다.** C는 표준 라이브러리만, Python은 표준 모듈만.
- **C로만 낸다.** `tools/`의 Python은 그래프용이다.
- 정렬 코드는 원소를 `sortutil.h`의 함수(`sortCompare` · `sortCopy` · `sortSwap`)로만
  만진다. 비교 · 이동을 한곳에서 세기 위해서다.
- 의사코드(`src/*.pseudo`)가 기준이다. 절차를 바꾸면 의사코드도 같이 고친다.
- 난수는 `minstd`만 쓴다(`rand()` 금지). 비교 · 이동 횟수가 기계와 무관하게 재현되어야 한다.
- 실행 파일은 `*.out`으로 만든다. C는 camelCase, Python은 snake_case.
- **컴파일 경고 없이** 빌드되어야 한다(`-Wall -Wextra`).

## 실행

```sh
make test       # 커밋 전에 반드시 통과
make run        # 실험 1~4 (1분 가까이 걸린다)
make charts     # report/data · report/figures 다시 만들기
make sanitize   # ASan · UBSan
```

## Git

- 한 커밋에는 한 가지 주제만 담는다.
- **커밋 전에 `make test`가 통과해야 한다.**
- 커밋 메시지 제목은 영어 명령형 한 줄
