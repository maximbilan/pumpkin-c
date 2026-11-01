"""Packs pumpkin.min.c into the shape of a jack-o'-lantern: pumpkin.c.

The #include lines become the stem. Every other token is laid into the body
row by row, skipping the carved eyes, nose and grin. Each run of a row is
justified so the outline stays crisp, and the spare cells are shared out over
every row so the code ends exactly at the bottom. String literals that don't
fit are split into adjacent literals, and a unary minus stays next to its
operand.

    python3 shape.py [scale]     # writes pumpkin.c and prints it

The scale is where the search starts, in columns per unit (the body is two
units wide). The default reproduces the pumpkin.c in this repo.
"""

import math
import re
import sys

SRC = "pumpkin.min.c"
OUT = "pumpkin.c"
ASPECT = 2.0  # a terminal cell is about twice as tall as it is wide

TOKEN = re.compile(
    r'"(?:\\.|[^"\\])*"'
    r"|'(?:\\.|[^'\\])*'"
    r"|[A-Za-z_]\w*"
    r"|(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?"
    r"|->|\+\+|--|&&|\|\||<=|>=|==|!=|\+=|-=|\*=|/=|<<|>>"
    r"|[^\s]"
)


def tokenize(code):
    return TOKEN.findall(code)


def needs_space(a, b):
    return tokenize(a + b) != tokenize(a) + tokenize(b) or "/*" in a + b or "//" in a + b


UNARY_AFTER = {"=", "(", ",", "return", "*", "/", "+", "-", "<", ">", "?", ":", "&&", "||", "+=", "-=", "*=", "/="}


def glue(tokens):
    """Keeps a unary minus next to its operand: `c=-\n1` compiles, but clang
    warns that it looks like a mistyped -=."""
    units = []
    i = 0
    while i < len(tokens):
        t = tokens[i]
        if t == "-" and i + 1 < len(tokens) and (not units or tokenize(units[-1])[-1] in UNARY_AFTER):
            units.append(t + tokens[i + 1])
            i += 2
        else:
            units.append(t)
            i += 1
    return units


def string_units(lit):
    """The characters of a string literal's body, keeping escapes whole."""
    return re.findall(r"\\[0-7]{1,3}|\\x[0-9a-fA-F]+|\\.|[^\\]", lit[1:-1])


# MARK: the outline


FACE = 1.12  # the carving reads better a little bigger in text


def carved(x, y):
    """Same face as O() in the C code, in front-view coordinates."""
    x, y = x / FACE, (y + 0.03) / FACE
    a, s = abs(x), x / 0.6
    k = 1 - s * s
    t, b = -0.1 - 0.15 * k, -0.14 - 0.31 * k
    return (
        (0.04 < y < 0.4 and abs(a - 0.35) < (0.4 - y) * 0.62)
        or (-0.08 < y < 0.07 and a < (0.07 - y) * 0.6)
        or (k > 0 and b < y < t and not (abs(a - 0.22) < 0.06 and y > t - 0.08) and not (a < 0.06 and y < b + 0.07))
    )


LOBES = [(0.0, 0.0, 0.5, 0.745), (0.34, -0.01, 0.48, 0.72), (-0.34, -0.01, 0.48, 0.72), (0.6, -0.03, 0.4, 0.62), (-0.6, -0.03, 0.4, 0.62)]


def body(x, y):
    return any(((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1 for cx, cy, rx, ry in LOBES)


def mask(scale):
    """Rows of (start, end) runs that make up the pumpkin body; scale is
    columns per unit (the body is 2 units wide, 1.49 tall)."""
    half_w = math.ceil(scale) + 1
    half_h = math.ceil(0.745 * scale / ASPECT) + 1
    rows = []
    for r in range(-half_h, half_h + 1):
        y = -(r * ASPECT) / scale
        runs, start = [], None
        for c in range(-half_w, half_w + 2):
            x = (c + 0.5) / scale
            inside = c <= half_w and body(x, y) and not carved(x, y)
            if inside and start is None:
                start = c
            elif not inside and start is not None:
                runs.append((start + half_w, c + half_w))
                start = None
        # Skip the narrow cap rows so the stem sits on a proper shoulder.
        if runs and (rows or sum(e - s for s, e in runs) >= 28):
            rows.append(runs)
    return rows, 2 * half_w + 1


# MARK: packing


def pack(tokens, rows, reserve=0.0):
    """Fills the runs with tokens. Returns the lines, or None if they don't fit."""
    tokens = list(tokens)
    lines = []
    for runs in rows:
        line = []
        for start, end in runs:
            width = end - start
            cap = width - int(width * reserve + 0.5)
            seg, used = [], 0
            while tokens:
                tok = tokens[0]
                gap = 1 if seg and needs_space(seg[-1], tok) else 0
                if used + gap + len(tok) <= cap:
                    seg.append(tok)
                    used += gap + len(tok)
                    tokens.pop(0)
                    continue
                room = cap - used - gap
                if tok.startswith('"') and room >= 3:
                    units = string_units(tok)
                    n = 0
                    while n < len(units) and len("".join(units[: n + 1])) + 2 <= room:
                        n += 1
                    if 0 < n < len(units):
                        head = '"' + "".join(units[:n]) + '"'
                        seg.append(head)
                        used += gap + len(head)
                        tokens[0] = '"' + "".join(units[n:]) + '"'
                break
            line.append((start, width, seg))
        lines.append(line)
    if tokens:
        return None
    return lines


def justify(seg, width):
    if not seg:
        return " " * width
    gaps = [1 if needs_space(a, b) else 0 for a, b in zip(seg, seg[1:])]
    extra = width - sum(map(len, seg)) - sum(gaps)
    if len(seg) == 1:
        return seg[0].ljust(width)
    n = len(gaps)
    # Spread the spare columns evenly over the gaps.
    for i in range(n):
        gaps[i] += extra * (i + 1) // n - extra * i // n
    out = seg[0]
    for g, tok in zip(gaps, seg[1:]):
        out += " " * g + tok
    return out


def render(lines, total_width):
    text = []
    for line in lines:
        row = [" "] * total_width
        for start, width, seg in line:
            s = justify(seg, width)
            row[start : start + width] = list(s)
        text.append("".join(row).rstrip())
    return text


def main():
    src = open(SRC).read()
    includes = [l.strip() for l in src.splitlines() if l.startswith("#")]
    code = "\n".join(l for l in src.splitlines() if not l.startswith("#"))
    tokens = glue(tokenize(code))

    def fit(scale):
        """Packs at this scale, handing the spare cells to every row (the
        largest reserve that still fits). Returns (lines, empty runs left at
        the end) or None."""
        rows, total = mask(scale)
        if not pack(tokens, rows):
            return None
        lo, hi = 0.0, 0.25
        for _ in range(30):
            mid = (lo + hi) / 2
            if pack(tokens, rows, mid):
                lo = mid
            else:
                hi = mid
        lines = pack(tokens, rows, lo)
        flat = [seg for line in lines for _, _, seg in line]
        trailing = len(flat) - 1 - max(i for i, seg in enumerate(flat) if seg)
        return lines, trailing, total

    scale = float(sys.argv[1]) if len(sys.argv) > 1 else 43.0
    best = None
    while scale < 60:
        got = fit(scale)
        if got:
            if got[1] == 0:
                best = (scale, got)
                break
            best = best or (scale, got)
        scale += 0.05
    scale, (lines, _, total) = best
    body_lines = render(lines, total)

    # The stem: the includes, leaning right as they rise.
    mid = total // 2
    stem = []
    for i, inc in enumerate(includes):
        lean = (len(includes) - 1 - i) // 2
        stem.append(" " * (mid - len(inc) // 2 + lean) + inc)
    text = "\n".join(stem + body_lines) + "\n"
    open(OUT, "w").write(text)
    cap = sum(w for line in lines for _, w, _ in line)
    print(text)
    print(f"scale {scale:.2f}: {len(stem) + len(body_lines)} rows x {max(map(len, text.splitlines()))} cols, "
          f"{cap} cells for {sum(map(len, tokens))} chars of tokens", file=sys.stderr)


main()
