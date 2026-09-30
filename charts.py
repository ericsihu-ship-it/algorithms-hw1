"""실험을 CSV로 받아 report/ 아래에 그래프(SVG)를 그린다.

    make charts                     # 또는 python3 tools/charts.py
    python3 tools/charts.py --reuse # 다시 재지 않고 report/data/의 CSV로만 그린다

src/main.out <실험> --csv 를 돌려 측정값을 report/data/<실험>.csv에 남기고, 그
파일로 report/figures/*.svg를 그린다. 사람이 읽는 표를 파싱하지 않고 CSV를 쓰는
이유는, 표 모양이 바뀌어도 그래프가 깨지지 않게 하려는 것이다.

표준 모듈만 쓴다. 그림은 tools/svgplot.py가 직접 찍는다.
"""

import csv
import io
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import svgplot  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
BINARY = ROOT / "src" / "main.out"
DATA = ROOT / "report" / "data"
FIGURES = ROOT / "report" / "figures"

ALGOS = ["mergeSort", "quickSort", "cocktailSort"]
SHAPES = [("random", "무작위"), ("sorted", "정렬됨"), ("reversed", "역순"),
          ("few-unique", "중복많음")]  # 실험 1 · 3의 네 입력
PIVOTS = ["first", "random", "median3"]


def load(experiment, reuse):
    """실험 하나를 CSV로 받아 저장하고 행 목록을 돌려준다."""
    path = DATA / f"{experiment}.csv"
    if reuse and path.exists():
        text = path.read_text(encoding="utf-8")
    else:
        if not BINARY.exists():
            subprocess.run(["make", "src/main.out"], cwd=ROOT, check=True)
        print(f"  재는 중: {experiment} ...", flush=True)
        text = subprocess.run([str(BINARY), experiment, "--csv"], cwd=ROOT, check=True,
                              capture_output=True, text=True).stdout
        path.write_text(text, encoding="utf-8")
    rows = list(csv.DictReader(io.StringIO(text)))
    for row in rows:
        for key, value in row.items():
            if key in ("experiment", "input", "algo"):
                continue
            row[key] = float(value) if key == "millis" else int(value)
    return rows


def pick(rows, **cond):
    found = [r for r in rows if all(r[k] == v for k, v in cond.items())]
    if len(found) != 1:
        raise SystemExit(f"CSV에서 {cond}에 맞는 줄이 {len(found)}개다")
    return found[0]


def ms_tip(v):
    return f"{v:,.3f} ms"


def count_tip(v):
    return f"비교 {int(v):,}회"


def figure(name):
    return str(FIGURES / name)


def chart_shapes(rows):
    n = rows[0]["n"]
    labels = [label for _, label in SHAPES]
    for field, fname, title, y_label, y_fmt, tip, label in (
        ("millis", "shapes-time.svg", f"입력 모양별 정렬 시간 (n = {n:,})", "ms",
         svgplot.fmt_ms, ms_tip, lambda v: f"{v:,.0f} ms"),
        ("compares", "shapes-compares.svg", f"입력 모양별 비교 횟수 (n = {n:,})", "비교 횟수",
         svgplot.fmt_count, count_tip, lambda v: f"{v:,.0f}회"),
    ):
        values = [[pick(rows, algo=a, input=s)[field] for s, _ in SHAPES] for a in ALGOS]
        svgplot.dot_chart(
            figure(fname), title=title,
            subtitle="로그 축: 눈금 한 칸이 10배다.",
            desc=f"{title}. " + "; ".join(
                f"{a}: " + ", ".join(f"{lab} {tip(v)}" for lab, v in zip(labels, vals))
                for a, vals in zip(ALGOS, values)),
            categories=labels, panels=[("", values)], series_names=ALGOS,
            y_label=y_label, y_fmt=y_fmt, tip_fmt=tip, label_fmt=label)


def chart_growth(rows):
    xs = sorted({r["n"] for r in rows})

    def series(field):
        return [(a, [pick(rows, algo=a, n=x)[field] for x in xs]) for a in ALGOS]

    svgplot.line_chart(
        figure("growth-compares.svg"),
        title="n을 두 배씩 늘릴 때 비교 횟수 (무작위 입력)",
        subtitle="로그-로그 축. 기울기가 차수다: 칵테일은 기울기 2(n²), 병합 · 퀵은 1을 조금 넘는다(n log n).",
        desc="n = 1K부터 32K까지 두 배씩 늘리며 센 비교 횟수. 칵테일 정렬은 n이 두 배면 4배, "
             "병합 · 퀵은 2.1~2.2배로 자란다.",
        xs=xs, series=series("compares"), y_scale="log10",
        y_label="비교 횟수", y_fmt=svgplot.fmt_count, tip_fmt=count_tip)

    svgplot.line_chart(
        figure("growth-time.svg"),
        title="n을 두 배씩 늘릴 때 걸린 시간 (무작위 입력)",
        subtitle="로그-로그 축. 선 사이의 세로 간격이 곧 배수다. n이 커질수록 벌어진다.",
        desc="n = 1K부터 32K까지 두 배씩 늘리며 잰 시간. 칵테일 정렬만 기울기가 2다.",
        xs=xs, series=series("millis"), y_scale="log10",
        y_label="ms", y_fmt=svgplot.fmt_ms, tip_fmt=ms_tip)


def chart_quick(rows):
    n = max(r["n"] for r in rows)
    labels = [label for _, label in SHAPES]
    panels = []
    for part, panel in (("2way", "2-way 파티션 (수업 코드)"), ("3way", "3-way 파티션")):
        values = [[pick(rows, algo=f"{p}/{part}", input=s, n=n)["compares"] for s, _ in SHAPES]
                  for p in PIVOTS]
        panels.append((panel, values))
    svgplot.dot_chart(
        figure("quick-compares.svg"),
        title=f"퀵 정렬: 피벗 × 파티션별 비교 횟수 (n = {n:,})",
        subtitle="색은 피벗 고르는 법. 로그 축이라 위로 한 칸이면 10배 더 비교한 것이다.",
        desc="2-way 파티션은 중복많음 입력에서 피벗과 무관하게 비교가 수천만 번으로 치솟는다. "
             "3-way 파티션은 중복에 강하지만 첫 원소 피벗은 역순에서 무너진다.",
        categories=labels, panels=panels, series_names=PIVOTS,
        y_label="비교 횟수", y_fmt=svgplot.fmt_count, tip_fmt=count_tip, label_max=False,
        height=440)


def chart_turtle(rows):
    n = max(r["n"] for r in rows)
    shapes = [("random", "무작위"), ("sorted", "정렬됨"), ("reversed", "역순"),
              ("turtle", "거북이"), ("rabbit", "토끼")]
    names = ["bubbleSort", "cocktailSort"]
    values = [[pick(rows, algo=a, input=s, n=n)["compares"] for s, _ in shapes] for a in names]
    svgplot.dot_chart(
        figure("turtle-compares.svg"),
        title=f"버블 정렬(기준선) vs 칵테일 정렬: 비교 횟수 (n = {n:,})",
        subtitle="거북이(맨 끝의 최솟값)에서 버블은 n²/2번, 칵테일은 3n번쯤 비교한다. 로그 축이다.",
        desc="두 정렬 모두 flag로 일찍 멈춘다. 거북이 입력에서 버블 정렬은 약 3,200만 번, "
             "칵테일 정렬은 약 2만 4천 번 비교한다. 토끼와 정렬됨에서는 둘이 같다.",
        categories=[label for _, label in shapes], panels=[("", values)], series_names=names,
        y_label="비교 횟수", y_fmt=svgplot.fmt_count, tip_fmt=count_tip, label_max=False,
        colors=[3, 2])


def main():
    reuse = "--reuse" in sys.argv[1:]
    DATA.mkdir(parents=True, exist_ok=True)
    FIGURES.mkdir(parents=True, exist_ok=True)

    chart_shapes(load("shapes", reuse))
    chart_growth(load("growth", reuse))
    chart_quick(load("quick", reuse))
    chart_turtle(load("turtle", reuse))
    print(f"그래프를 {FIGURES.relative_to(ROOT)}/ 에, 측정값을 {DATA.relative_to(ROOT)}/ 에 썼다.")


if __name__ == "__main__":
    main()
