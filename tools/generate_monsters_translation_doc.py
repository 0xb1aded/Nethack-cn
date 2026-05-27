from __future__ import annotations

import shutil
import subprocess
from collections import OrderedDict
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DOC_PATH = ROOT / "doc" / "monsters_translation_standard_zh_cn.md"
MONSTERS_H = ROOT / "include" / "monsters.h"
OLD_MONST_C = Path(r"d:\Download\Compressed\NetHack-cn-NetHack-cn\NetHack\src\monst.c")
WORK_DIR = ROOT / "tmp" / "generated_monsters_doc"
MON_TYPE_LABELS = {
    "S_ANT": "蚁类",
    "S_BLOB": "斑点怪类",
    "S_COCKATRICE": "鸡蛇类",
    "S_DOG": "犬类",
    "S_EYE": "眼球类",
    "S_FELINE": "猫科类",
    "S_GREMLIN": "小鬼类",
    "S_HUMANOID": "类人生物",
    "S_IMP": "小恶魔类",
    "S_JELLY": "果冻类",
    "S_KOBOLD": "狗头人类",
    "S_LEPRECHAUN": "小矮妖类",
    "S_MIMIC": "拟形怪类",
    "S_NYMPH": "仙女类",
    "S_ORC": "兽人类",
    "S_PIERCER": "锥子怪类",
    "S_QUADRUPED": "四足兽类",
    "S_RODENT": "啮齿类",
    "S_SPIDER": "蛛虫类",
    "S_TRAPPER": "伏击怪类",
    "S_UNICORN": "独角兽与马类",
    "S_VORTEX": "漩涡类",
    "S_WORM": "蠕虫类",
    "S_XAN": "奇虫类",
    "S_LIGHT": "光球类",
    "S_ZRUTY": "兹鲁提类",
    "S_ANGEL": "天使类",
    "S_BAT": "蝙蝠与鸟类",
    "S_CENTAUR": "半人马类",
    "S_DRAGON": "龙类",
    "S_ELEMENTAL": "元素类",
    "S_FUNGUS": "真菌类",
    "S_GNOME": "侏儒类",
    "S_GIANT": "巨人类",
    "S_invisible": "隐形生物类",
    "S_JABBERWOCK": "颊脖龙类",
    "S_KOP": "吉斯通警察类",
    "S_LICH": "巫妖类",
    "S_MUMMY": "木乃伊类",
    "S_NAGA": "纳迦类",
    "S_OGRE": "食人魔类",
    "S_PUDDING": "布丁怪类",
    "S_QUANTMECH": "量子怪类",
    "S_RUSTMONST": "锈怪类",
    "S_SNAKE": "蛇类",
    "S_TROLL": "巨魔类",
    "S_UMBER": "土巨怪类",
    "S_VAMPIRE": "吸血鬼类",
    "S_WRAITH": "幽灵类",
    "S_XORN": "索尔石怪类",
    "S_YETI": "猿兽类",
    "S_ZOMBIE": "僵尸类",
    "S_HUMAN": "人类与精灵类",
    "S_GHOST": "鬼魂类",
    "S_GOLEM": "魔像类",
    "S_DEMON": "恶魔类",
    "S_EEL": "海怪类",
    "S_LIZARD": "蜥蜴类",
    "S_WORM_TAIL": "长蠕虫尾类",
    "S_MIMIC_DEF": "拟形外观类",
}


def run_command(command: str) -> str:
    completed = subprocess.run(
        ["powershell", "-NoProfile", "-Command", command],
        cwd=ROOT,
        check=True,
        text=True,
        capture_output=True,
        encoding="utf-8",
    )
    return completed.stdout


def skip_string(text: str, start: int) -> int:
    i = start + 1
    while i < len(text):
        ch = text[i]
        if ch == "\\":
            i += 2
            continue
        if ch == '"':
            return i + 1
        i += 1
    raise ValueError("unterminated string literal")


def skip_line_comment(text: str, start: int) -> int:
    end = text.find("\n", start)
    return len(text) if end < 0 else end


def skip_block_comment(text: str, start: int) -> int:
    end = text.find("*/", start + 2)
    if end < 0:
        raise ValueError("unterminated block comment")
    return end + 2


def split_top_level_args(text: str) -> list[str]:
    args: list[str] = []
    start = 0
    depth = 0
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == '"':
            i = skip_string(text, i)
            continue
        if text.startswith("//", i):
            i = skip_line_comment(text, i)
            continue
        if text.startswith("/*", i):
            i = skip_block_comment(text, i)
            continue
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "," and depth == 0:
            args.append(text[start:i].strip())
            start = i + 1
        i += 1
    args.append(text[start:].strip())
    return args


def find_matching_paren(text: str, open_index: int) -> int:
    depth = 1
    i = open_index + 1
    while i < len(text):
        ch = text[i]
        if ch == '"':
            i = skip_string(text, i)
            continue
        if text.startswith("//", i):
            i = skip_line_comment(text, i)
            continue
        if text.startswith("/*", i):
            i = skip_block_comment(text, i)
            continue
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError("unmatched '('")


def iter_mon_calls(text: str):
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == '"':
            i = skip_string(text, i)
            continue
        if text.startswith("//", i):
            i = skip_line_comment(text, i)
            continue
        if text.startswith("/*", i):
            i = skip_block_comment(text, i)
            continue
        if text.startswith("MON(", i):
            open_index = i + 3
            close_index = find_matching_paren(text, open_index)
            yield i, open_index, close_index
            i = close_index + 1
            continue
        i += 1


def parse_c_string(token: str) -> str | None:
    token = token.strip()
    if len(token) < 2 or token[0] != '"' or token[-1] != '"':
        return None
    result: list[str] = []
    i = 1
    while i < len(token) - 1:
        ch = token[i]
        if ch != "\\":
            result.append(ch)
            i += 1
            continue
        i += 1
        if i >= len(token) - 1:
            return None
        esc = token[i]
        mapping = {
            "\\": "\\",
            '"': '"',
            "n": "\n",
            "r": "\r",
            "t": "\t",
            "0": "\0",
        }
        result.append(mapping.get(esc, esc))
        i += 1
    return "".join(result)


def parse_name_macro(arg_text: str) -> tuple[str, list[str]] | None:
    stripped = arg_text.strip()
    if stripped.startswith("NAM("):
        kind = "NAM"
    elif stripped.startswith("NAMS("):
        kind = "NAMS"
    else:
        return None

    open_index = stripped.find("(")
    close_index = find_matching_paren(stripped, open_index)
    if stripped[close_index + 1 :].strip():
        return None
    inner = stripped[open_index + 1 : close_index]
    parts = split_top_level_args(inner)
    names = [parse_c_string(part) for part in parts]
    if any(name is None for name in names):
        return None
    if kind == "NAM" and len(names) != 1:
        return None
    if kind == "NAMS" and len(names) != 3:
        return None
    return kind, [name for name in names if name is not None]


def norm(value: str) -> str:
    return value.strip()


def esc(value: str) -> str:
    return value.replace("\\", "\\\\").replace("|", "\\|")


def dedupe_rows(rows: list[tuple[str, str, str, str, str, str, str]]) -> list[tuple[str, str, str, str, str, str, str]]:
    seen: set[tuple[str, str, str, str, str, str, str]] = set()
    unique_rows: list[tuple[str, str, str, str, str, str, str]] = []
    for row in rows:
        if row in seen:
            continue
        seen.add(row)
        unique_rows.append(row)
    return unique_rows


def strip_if0_blocks(text: str) -> str:
    lines = text.splitlines(keepends=True)
    out: list[str] = []
    skip_depth = 0

    for line in lines:
        stripped = line.lstrip()
        if stripped.startswith("#if 0"):
            skip_depth += 1
            continue
        if skip_depth:
            if stripped.startswith("#if"):
                skip_depth += 1
                continue
            if stripped.startswith("#endif"):
                skip_depth -= 1
                continue
            continue
        out.append(line)
    return "".join(out)


def normalize_mon_symbol(sym_arg: str) -> str:
    return norm(sym_arg)


def symbol_label(sym_arg: str) -> str:
    sym = normalize_mon_symbol(sym_arg)
    if not sym:
        return "未知类型 (UNKNOWN_CLASS)"
    prefix = MON_TYPE_LABELS.get(sym)
    return f"{prefix} ({sym})" if prefix else sym


def build_current_rows() -> list[dict[str, object]]:
    text = strip_if0_blocks(MONSTERS_H.read_text(encoding="utf-8"))
    rows: list[dict[str, object]] = []

    for _, open_index, close_index in iter_mon_calls(text):
        args = split_top_level_args(text[open_index + 1 : close_index])
        if len(args) < 3:
            continue
        english_names = parse_name_macro(args[0])
        chinese_names = parse_name_macro(args[1])
        sym_arg = normalize_mon_symbol(args[2])
        if english_names is None or chinese_names is None:
            continue
        english_kind, english_values = english_names
        chinese_kind, chinese_values = chinese_names
        rows.append(
            {
                "section": symbol_label(sym_arg),
                "kind": english_kind,
                "english": english_values,
                "chinese": chinese_values,
            }
        )
    return rows


def build_old_name_set() -> set[str]:
    text = OLD_MONST_C.read_text(encoding="utf-8")
    names: set[str] = set()
    for _, open_index, close_index in iter_mon_calls(text):
        args = split_top_level_args(text[open_index + 1 : close_index])
        if len(args) < 1:
            continue
        parsed = parse_name_macro(args[0])
        if parsed is not None:
            _, english_names = parsed
            for name in english_names:
                if name:
                    names.add(name)
            continue
        english = parse_c_string(args[0])
        if english is not None and english:
            names.add(english)
    return names


def render_doc(rows: list[dict[str, object]], old_names: set[str]) -> str:
    sections: OrderedDict[str, list[tuple[str, str, str, str, str, str, str]]] = OrderedDict()
    new_count = 0

    for row in rows:
        section = row["section"]
        if section not in sections:
            sections[section] = []
        kind = row["kind"]
        english_names = row["english"]
        chinese_names = row["chinese"]
        if kind == "NAM":
            male_en = "*"
            male_cn = "*"
            female_en = "*"
            female_cn = "*"
            neutral_en = norm(english_names[0]) if english_names else ""
            neutral_cn = norm(chinese_names[0]) if chinese_names else ""
            note = "5.0.0 新增" if neutral_en and neutral_en not in old_names else ""
        else:
            male_en = norm(english_names[0]) if len(english_names) > 0 else ""
            female_en = norm(english_names[1]) if len(english_names) > 1 else ""
            neutral_en = norm(english_names[2]) if len(english_names) > 2 else ""
            male_cn = norm(chinese_names[0]) if len(chinese_names) > 0 else ""
            female_cn = norm(chinese_names[1]) if len(chinese_names) > 1 else ""
            neutral_cn = norm(chinese_names[2]) if len(chinese_names) > 2 else ""
            note_parts: list[str] = []
            if male_en and male_en not in old_names:
                note_parts.append(male_en)
            if female_en and female_en not in old_names:
                note_parts.append(female_en)
            if neutral_en and neutral_en not in old_names:
                note_parts.append(neutral_en)
            note = "5.0.0 新增：" + "，".join(note_parts) if note_parts else ""
        if note:
            new_count += 1
        sections[section].append((male_en, male_cn, female_en, female_cn, neutral_en, neutral_cn, note))

    lines: list[str] = []
    lines.append("## 怪物简中译名标准（monsters.h）")
    lines.append("")
    lines.append(f"当前有效怪物条目共 {len(rows)} 条，其中备注为 `5.0.0 新增` 的条目共 {new_count} 条。")
    lines.append("")
    lines.append("### 按怪物类型分组")
    lines.append("")

    for section, section_rows in sections.items():
        unique_rows = dedupe_rows(section_rows)
        if not unique_rows:
            continue
        lines.append(f"#### {section}")
        lines.append("")
        lines.append("| 男性英文名 | 男性中文名 | 女性英文名 | 女性中文名 | 中性英文名 | 中性中文名 | 备注 |")
        lines.append("| --- | --- | --- | --- | --- | --- | --- |")
        for male_en, male_cn, female_en, female_cn, neutral_en, neutral_cn, note in unique_rows:
            lines.append(
                "| " + " | ".join(
                    esc(x)
                    for x in [male_en, male_cn, female_en, female_cn, neutral_en, neutral_cn, note]
                ) + " |"
            )
        lines.append("")

    return "\n".join(lines) + "\n"


def main() -> None:
    if not MONSTERS_H.exists():
        raise FileNotFoundError(f"找不到 monsters.h：{MONSTERS_H}")
    if not OLD_MONST_C.exists():
        raise FileNotFoundError(f"找不到旧版 monst.c：{OLD_MONST_C}")

    if WORK_DIR.exists():
        shutil.rmtree(WORK_DIR, ignore_errors=True)
    WORK_DIR.mkdir(parents=True, exist_ok=True)

    try:
        rows = build_current_rows()
        old_names = build_old_name_set()
        DOC_PATH.write_text(render_doc(rows, old_names), encoding="utf-8", newline="\n")
        print(DOC_PATH)
    finally:
        shutil.rmtree(WORK_DIR, ignore_errors=True)


if __name__ == "__main__":
    main()
