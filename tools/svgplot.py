"""표준 모듈만으로 SVG 그래프를 그린다 (matplotlib 없이).

이 저장소는 외부 라이브러리를 쓰지 않는다. 그래서 그래프 두 가지만 직접 그린다.

    line_chart   x축은 n(로그2), y축은 선형 또는 로그10. 배가 실험용
    dot_chart    x축은 입력 모양 같은 범주, y축은 로그10. 범주마다 점 셋

색은 세 계열(알고리즘 셋 또는 피벗 셋)만 쓴다. 색만으로 구별하지 않도록 계열마다
표식 모양(원 · 네모 · 마름모)을 달리하고, 범례를 늘 붙인다. 흑백으로 인쇄해도
읽힌다. 각 점에는 <title>을 달아 브라우저에서 마우스를 올리면 값이 보인다.
어두운 화면에서는 SVG 안의 스타일이 어두운 색으로 바뀐다.
"""

import math
from xml.sax.saxutils import escape

FONT = ("Pretendard, 'Apple SD Gothic Neo', 'Noto Sans KR', 'Noto Sans CJK KR', "
        "'Malgun Gothic', system-ui, -apple-system, 'Segoe UI', sans-serif")

# 색은 역할로 부른다. 세 계열색은 색각 이상 시뮬레이션까지 검증한 순서다.
LIGHT = {
    "surface": "#fcfcfb", "ink": "#0b0b0b", "ink2": "#52514e", "muted": "#898781",
    "grid": "#e1e0d9", "axis": "#c3c2b7",
    "series": ["#2a78d6", "#eb6834", "#1baf7a"],
}
DARK = {
    "surface": "#1a1a19", "ink": "#ffffff", "ink2": "#c3c2b7", "muted": "#898781",
    "grid": "#2c2c2a", "axis": "#383835",
    "series": ["#3987e5", "#d95926", "#199e70"],
}
MARKERS = ["circle", "square", "diamond"]


def _style():
    def block(p):
        rules = [
            f".surface{{fill:{p['surface']}}}",
            f".ink{{fill:{p['ink']}}}",
            f".ink2{{fill:{p['ink2']}}}",
            f".muted{{fill:{p['muted']}}}",
            f".grid{{stroke:{p['grid']}}}",
            f".axis{{stroke:{p['axis']}}}",
            f".ref{{stroke:{p['ink2']}}}",
            f".ring{{stroke:{p['surface']}}}",
        ]
        for i, color in enumerate(p["series"]):
            rules.append(f".s{i}{{stroke:{color}}} .f{i}{{fill:{color}}}")
        return "".join(rules)

    return ("<style>" + block(LIGHT) +
            "@media (prefers-color-scheme: dark){" + block(DARK) + "}</style>")


# --- 숫자 표기 --------------------------------------------------------------

def fmt_count(v):
    """1234567 -> '1.2M'. 축 눈금용 짧은 표기."""
    for size, suffix in ((1e9, "B"), (1e6, "M"), (1e3, "K")):
        if abs(v) >= size:
            x = v / size
            return (f"{x:.0f}" if x >= 10 or x == int(x) else f"{x:.1f}") + suffix
    return f"{v:g}"


def fmt_n(n):
    """2의 거듭제곱 n을 짧게: 16384 -> '16K', 1048576 -> '1M' (K = 1,024)."""
    for size, suffix in ((1 << 20, "M"), (1 << 10, "K")):
        if n >= size and n % size == 0:
            return f"{n // size}{suffix}"
    return f"{n:,}"


def fmt_ms(v):
    if v >= 100:
        return f"{v:,.0f}"
    if v >= 1:
        return f"{v:.1f}".rstrip("0").rstrip(".")
    return f"{v:g}"


def fmt_plain(v):
    return f"{v:g}"


def text_width(s, size):
    """글자 폭 어림: 한글은 글자 크기만큼, 나머지는 0.58배."""
    w = 0.0
    for ch in s:
        w += size * (1.0 if ord(ch) >= 0x1100 else 0.58)
    return w


# --- 축 ----------------------------------------------------------------------

class Scale:
    """값을 화면 좌표로 옮긴다. kind는 'linear' · 'log2' · 'log10'."""

    def __init__(self, kind, lo, hi, a, b):
        self.kind, self.lo, self.hi, self.a, self.b = kind, lo, hi, a, b

    def _t(self, v):
        if self.kind == "linear":
            return v
        return math.log(v, 2 if self.kind == "log2" else 10)

    def __call__(self, v):
        t0, t1 = self._t(self.lo), self._t(self.hi)
        return self.a + (self._t(v) - t0) / (t1 - t0) * (self.b - self.a)


def log10_bounds(values):
    lo = min(v for v in values if v > 0)
    hi = max(values)
    return 10 ** math.floor(math.log10(lo)), 10 ** math.ceil(math.log10(hi))


def log10_ticks(lo, hi):
    ticks = []
    e = round(math.log10(lo))
    while 10 ** e <= hi * 1.0001:
        ticks.append(10 ** e)
        e += 1
    return ticks


def linear_ticks(lo, hi, count=5):
    span = hi - lo
    raw = span / count
    mag = 10 ** math.floor(math.log10(raw))
    step = min((m * mag for m in (1, 2, 2.5, 5, 10) if m * mag >= raw), default=mag * 10)
    first = math.ceil(lo / step) * step
    ticks = []
    v = first
    while v <= hi + step * 1e-9:
        ticks.append(round(v, 10))
        v += step
    return ticks


# --- SVG 조각 ----------------------------------------------------------------

class Svg:
    def __init__(self, width, height, title, desc):
        self.w, self.h = width, height
        self.title, self.desc = title, desc
        self.parts = []

    def add(self, s):
        self.parts.append(s)

    def text(self, x, y, s, cls="ink2", size=12, anchor="start", weight=None, extra=""):
        w = f' font-weight="{weight}"' if weight else ""
        self.add(f'<text class="{cls}" x="{x:.1f}" y="{y:.1f}" font-size="{size}" '
                 f'text-anchor="{anchor}"{w}{extra}>{escape(s)}</text>')

    def hline(self, x0, x1, y, cls):
        self.add(f'<line class="{cls}" x1="{x0:.1f}" x2="{x1:.1f}" y1="{y:.1f}" y2="{y:.1f}" '
                 f'stroke-width="1" shape-rendering="crispEdges"/>')

    def vline(self, x, y0, y1, cls):
        self.add(f'<line class="{cls}" x1="{x:.1f}" x2="{x:.1f}" y1="{y0:.1f}" y2="{y1:.1f}" '
                 f'stroke-width="1" shape-rendering="crispEdges"/>')

    def marker(self, x, y, i, tip=None, r=4.5):
        """계열 i의 표식. 둘레에 바탕색 2px 고리를 둘러 선과 겹쳐도 읽히게 한다."""
        shape = MARKERS[i % len(MARKERS)]
        attrs = f'class="f{i} ring" stroke-width="2"'
        if shape == "circle":
            body = f'<circle {attrs} cx="{x:.1f}" cy="{y:.1f}" r="{r:.1f}"/>'
        elif shape == "square":
            s = r * 0.9
            body = (f'<rect {attrs} x="{x - s:.1f}" y="{y - s:.1f}" width="{2 * s:.1f}" '
                    f'height="{2 * s:.1f}" rx="1"/>')
        else:
            s = r * 1.25
            body = (f'<path {attrs} d="M{x:.1f},{y - s:.1f} L{x + s:.1f},{y:.1f} '
                    f'L{x:.1f},{y + s:.1f} L{x - s:.1f},{y:.1f} Z"/>')
        if tip:
            # 보이지 않는 넓은 원을 두어 작은 표식도 마우스로 잡기 쉽게 한다.
            self.add(f'<g><title>{escape(tip)}</title>{body}'
                     f'<circle cx="{x:.1f}" cy="{y:.1f}" r="12" fill="transparent"/></g>')
        else:
            self.add(body)

    def header(self, title, subtitle, left):
        self.text(left, 28, title, cls="ink", size=15, weight="600")
        if subtitle:
            self.text(left, 48, subtitle, cls="ink2", size=12)

    def legend(self, names, left, y, kind):
        """범례: 선 그래프는 짧은 선 + 표식, 점 그래프는 표식만."""
        x = left
        for i, name in enumerate(names):
            if kind == "line":
                self.add(f'<line class="s{i}" x1="{x:.1f}" x2="{x + 22:.1f}" y1="{y - 4:.1f}" '
                         f'y2="{y - 4:.1f}" stroke-width="2" stroke-linecap="round"/>')
                self.marker(x + 11, y - 4, i, r=3.5)
                x += 28
            else:
                self.marker(x + 5, y - 4, i, r=4)
                x += 14
            self.text(x, y, name, cls="ink2", size=12)
            x += text_width(name, 12) + 18

    def render(self):
        head = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {self.w} {self.h}" '
                f'width="{self.w}" height="{self.h}" role="img" aria-labelledby="title desc" '
                f'font-family="{escape(FONT)}">'
                f'<title id="title">{escape(self.title)}</title>'
                f'<desc id="desc">{escape(self.desc)}</desc>' + _style() +
                f'<rect class="surface" width="{self.w}" height="{self.h}" rx="10"/>')
        return head + "".join(self.parts) + "</svg>\n"


# --- 선 그래프 ---------------------------------------------------------------

def line_chart(path, *, title, subtitle, desc, xs, series, y_scale="linear",
               y_min=None, y_max=None, y_label="", x_label="n (K = 1,024)",
               y_fmt=fmt_plain, x_fmt=fmt_n, tip_fmt=None, refs=(), end_labels=True,
               width=720, height=420):
    """xs는 2의 거듭제곱들. series는 [(이름, [y ...]), ...]."""
    svg = Svg(width, height, title, desc)
    left, right, top, bottom = 64, 112 if end_labels else 24, 116, 52
    x0, x1 = left, width - right
    y0, y1 = height - bottom, top

    all_y = [y for _, ys in series for y in ys] + [r[0] for r in refs]
    if y_scale == "log10":
        lo, hi = log10_bounds(all_y)
        lo = y_min if y_min is not None else lo
        hi = y_max if y_max is not None else hi
        ticks = log10_ticks(lo, hi)
    else:
        lo = y_min if y_min is not None else 0.0
        hi = y_max if y_max is not None else max(all_y) * 1.1
        ticks = linear_ticks(lo, hi)
        hi = max(hi, ticks[-1])
    sx = Scale("log2", xs[0], xs[-1], x0, x1)
    sy = Scale(y_scale, lo, hi, y0, y1)

    svg.header(title, subtitle, 20)
    svg.legend([name for name, _ in series], 20, 76, "line")

    for t in ticks:
        y = sy(t)
        svg.hline(x0, x1, y, "grid")
        svg.text(x0 - 8, y + 4, y_fmt(t), cls="muted", size=11, anchor="end")
    svg.hline(x0, x1, y0, "axis")
    for x in xs:
        svg.vline(sx(x), y0, y0 + 4, "axis")
        svg.text(sx(x), y0 + 18, x_fmt(x), cls="muted", size=11, anchor="middle")
    svg.text((x0 + x1) / 2, height - 10, x_label, cls="muted", size=11, anchor="middle")
    if y_label:
        svg.text(20, top - 16, y_label, cls="muted", size=11)

    for value, label in refs:
        y = sy(value)
        svg.add(f'<line class="ref" x1="{x0:.1f}" x2="{x1:.1f}" y1="{y:.1f}" y2="{y:.1f}" '
                f'stroke-width="1"/>')
        svg.text(x1 - 4, y - 6, label, cls="ink2", size=11, anchor="end")

    for i, (name, ys) in enumerate(series):
        pts = " ".join(f"{sx(x):.1f},{sy(y):.1f}" for x, y in zip(xs, ys))
        svg.add(f'<polyline class="s{i}" points="{pts}" fill="none" stroke-width="2" '
                f'stroke-linejoin="round" stroke-linecap="round"/>')
    for i, (name, ys) in enumerate(series):
        for x, y in zip(xs, ys):
            tip = f"{name} · n = {x:,} · " + (tip_fmt(y) if tip_fmt else y_fmt(y))
            svg.marker(sx(x), sy(y), i, tip=tip)

    if end_labels:
        # 끝값 이름표. 서로 14px보다 가까우면 범례에 맡기고 붙이지 않는다.
        ends = sorted(((sy(ys[-1]), name) for name, ys in series))
        for k, (y, name) in enumerate(ends):
            near_prev = k > 0 and abs(y - ends[k - 1][0]) < 14
            near_next = k + 1 < len(ends) and abs(ends[k + 1][0] - y) < 14
            if not (near_prev or near_next):
                svg.text(x1 + 10, y + 4, name, cls="ink2", size=12)

    with open(path, "w", encoding="utf-8") as f:
        f.write(svg.render())


# --- 점 그래프 (범주 × 계열, 로그 축) -------------------------------------------

def dot_chart(path, *, title, subtitle, desc, categories, panels, series_names,
              y_label="", y_fmt=fmt_count, tip_fmt=None, label_max=True,
              label_fmt=None, width=720, height=420):
    """panels는 [(패널 제목, [[계열0의 범주별 값], [계열1 ...], ...]), ...].
    패널이 둘 이상이면 같은 y축을 나눠 쓰는 작은 그래프를 나란히 그린다.
    막대 대신 점을 쓰는 이유: 로그 축에서 막대 길이는 값의 비율을 말하지 않는다."""
    svg = Svg(width, height, title, desc)
    multi = len(panels) > 1
    left, right, top, bottom = 64, 20, 140 if multi else 116, 40
    y0, y1 = height - bottom, top
    all_v = [v for _, rows in panels for row in rows for v in row]
    lo, hi = log10_bounds(all_v)
    sy = Scale("log10", lo, hi, y0, y1)
    ticks = log10_ticks(lo, hi)

    svg.header(title, subtitle, 20)
    svg.legend(series_names, 20, 76, "dot")
    if y_label:
        # 패널이 여럿이면 패널 제목 줄 위에 따로 둔다 (제목과 겹치지 않게).
        svg.text(20, top - (40 if multi else 16), y_label, cls="muted", size=11)

    gap = 28
    panel_w = (width - left - right - gap * (len(panels) - 1)) / len(panels)
    top_value = max(all_v)
    for p, (panel_title, rows) in enumerate(panels):
        px0 = left + p * (panel_w + gap)
        px1 = px0 + panel_w
        if len(panels) > 1:
            svg.text(px0, top - 14, panel_title, cls="ink", size=12, weight="600")
        for t in ticks:
            svg.hline(px0, px1, sy(t), "grid")
            if p == 0:
                svg.text(px0 - 8, sy(t) + 4, y_fmt(t), cls="muted", size=11, anchor="end")
        svg.hline(px0, px1, y0, "axis")
        band = panel_w / len(categories)
        count = len(rows)
        step = min(22.0, band / (count + 1))
        for c, cat in enumerate(categories):
            cx = px0 + band * (c + 0.5)
            svg.text(cx, y0 + 18, cat, cls="muted", size=11, anchor="middle")
            for i, row in enumerate(rows):
                x = cx + (i - (count - 1) / 2) * step
                v = row[c]
                tip = f"{panel_title + ' · ' if panel_title else ''}{series_names[i]} · {cat} · " + \
                    (tip_fmt(v) if tip_fmt else y_fmt(v))
                svg.marker(x, sy(v), i, tip=tip, r=5)
                if label_max and v == top_value:
                    # 가장 큰 값 하나에만 값을 적는다. 오른쪽에 자리가 없으면 왼쪽에.
                    label = (label_fmt or tip_fmt or y_fmt)(v)
                    if x + 9 + text_width(label, 11) > width - 8:
                        svg.text(x - 9, sy(v) + 4, label, cls="ink", size=11, anchor="end")
                    else:
                        svg.text(x + 9, sy(v) + 4, label, cls="ink", size=11)

    with open(path, "w", encoding="utf-8") as f:
        f.write(svg.render())
