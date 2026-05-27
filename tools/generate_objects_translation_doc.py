from __future__ import annotations

import subprocess
from collections import OrderedDict
from pathlib import Path
import shutil


ROOT = Path(__file__).resolve().parent.parent
DOC_PATH = ROOT / "doc" / "objects_translation_standard_zh_cn.md"
OLD_OBJECTS_C = Path(r"d:\Download\Compressed\NetHack-cn-NetHack-cn\NetHack\src\objects.c")
WORK_DIR = ROOT / "tmp" / "generated_objects_doc"


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


def build_current_meta(temp_dir: Path) -> list[list[str]]:
    c_path = temp_dir / "extract_current_meta_aligned.c"
    exe_path = temp_dir / "extract_current_meta_aligned.exe"
    source = r'''
#include <stdio.h>
#include "config.h"
#ifdef MAIL_STRUCTURES
#undef MAIL_STRUCTURES
#endif
#include "weight.h"
#include "obj.h"
#include "prop.h"
#include "skills.h"
#include "color.h"
#include "objclass.h"

static struct objdescr descs[NUM_OBJECTS + 1] = {
#define OBJECTS_DESCR_INIT
#include "objects.h"
#undef OBJECTS_DESCR_INIT
};

static struct objclass classes[NUM_OBJECTS + 1] = {
#define OBJECTS_INIT
#include "objects.h"
#undef OBJECTS_INIT
};

int main(void) {
    size_t n = sizeof(descs) / sizeof(descs[0]);
    for (size_t i = 0; i + 1 < n; ++i) {
        printf("%zu\t%d\t%s\t%s\t%s\t%s\n",
            i,
            (int) classes[i].oc_class,
            descs[i].oc_ename ? descs[i].oc_ename : "",
            descs[i].oc_edescr ? descs[i].oc_edescr : "",
            descs[i].oc_name ? descs[i].oc_name : "",
            descs[i].oc_descr ? descs[i].oc_descr : "");
    }
    return 0;
}
'''
    c_path.write_text(source, encoding="ascii", newline="\n")
    run_command(
        f"clang -x c -Iinclude -I. -o '{exe_path}' '{c_path}'"
    )
    output = run_command(f"& '{exe_path}'")
    rows = [line.rstrip("\n").split("\t") for line in output.splitlines()]
    return [row for row in rows if len(row) >= 6 and any(cell.strip() for cell in row[2:])]


def build_old_rows(temp_dir: Path) -> list[list[str]]:
    c_path = temp_dir / "extract_old_objects.c"
    exe_path = temp_dir / "extract_old_objects.exe"
    source = f'''
#include <stdio.h>
#include "{OLD_OBJECTS_C.as_posix()}"
int main(void) {{
    size_t n = sizeof(obj_descr) / sizeof(obj_descr[0]);
    for (size_t i = 0; i < n; ++i) {{
        printf("%zu\\t%s\\t%s\\t%s\\n", i,
            obj_descr[i].oc_ename ? obj_descr[i].oc_ename : "",
            obj_descr[i].oc_name ? obj_descr[i].oc_name : "",
            obj_descr[i].oc_descr ? obj_descr[i].oc_descr : "");
    }}
    return 0;
}}
'''
    c_path.write_text(source, encoding="ascii", newline="\n")
    run_command(
        f"clang -x c -I'{OLD_OBJECTS_C.parent.parent / 'include'}' "
        f"-I'{OLD_OBJECTS_C.parent.parent}' -o '{exe_path}' '{c_path}'"
    )
    output = run_command(f"& '{exe_path}'")
    rows = [line.rstrip("\n").split("\t") for line in output.splitlines()]
    return [row for row in rows if any(cell.strip() for cell in row[1:])]


def norm(value: str) -> str:
    return value.strip()


def esc(value: str) -> str:
    return value.replace("\\", "\\\\").replace("|", "\\|")


def dedupe_rows(rows: list[tuple[str, str, str]]) -> list[tuple[str, str, str]]:
    seen: set[tuple[str, str, str]] = set()
    unique_rows: list[tuple[str, str, str]] = []
    for row in rows:
        if row in seen:
            continue
        seen.add(row)
        unique_rows.append(row)
    return unique_rows


def render_doc(meta_rows: list[list[str]], old_rows: list[list[str]]) -> str:
    old_enames = {norm(row[1]) for row in old_rows if norm(row[1])}
    old_cnames = {norm(row[2]) for row in old_rows if norm(row[2])}
    old_cdescs = {norm(row[3]) for row in old_rows if norm(row[3])}

    def is_new(ename: str, edesc: str, cname: str, cdesc: str) -> bool:
        if ename and ename in old_enames:
            return False
        if cname and cname in old_cnames:
            return False
        if not ename and not cname:
            if edesc and edesc in old_cdescs:
                return False
            if cdesc and cdesc in old_cdescs:
                return False
        return True

    section_order = [
        "通用占位",
        "杂项物体",
        "武器",
        "防具",
        "戒指",
        "护符",
        "工具",
        "食物",
        "药水",
        "卷轴",
        "魔法书",
        "魔杖",
        "金币",
        "宝石与石头",
        "岩石",
        "铁球",
        "铁链",
        "毒液",
    ]
    class_map = {
        1: "杂项物体",
        2: "武器",
        3: "防具",
        4: "戒指",
        5: "护符",
        6: "工具",
        7: "食物",
        8: "药水",
        9: "卷轴",
        10: "魔法书",
        11: "魔杖",
        12: "金币",
        13: "宝石与石头",
        14: "岩石",
        15: "铁球",
        16: "铁链",
        17: "毒液",
    }
    sections: OrderedDict[str, dict[str, list[tuple[str, str, str]]]] = OrderedDict(
        (title, {"names": [], "descrs": []}) for title in section_order
    )

    new_count = 0
    for row in meta_rows:
        _, class_id, ename, edesc, cname, cdesc = [norm(x) for x in row[:6]]
        class_num = int(class_id)
        if ename.startswith("generic "):
            title = "通用占位"
        else:
            title = class_map.get(class_num, f"未分类({class_num})")
            if title not in sections:
                sections[title] = {"names": [], "descrs": []}

        note = "5.0.0 新增" if is_new(ename, edesc, cname, cdesc) else ""
        if note:
            new_count += 1
        if ename or cname:
            sections[title]["names"].append((ename, cname, note))
        if edesc or cdesc:
            sections[title]["descrs"].append((edesc, cdesc, note))

    lines: list[str] = []
    lines.append("## 物品简中译名标准（objects.h）")
    lines.append("")
    lines.append(f"当前有效条目共 {len(meta_rows)} 条，其中备注为 `5.0.0 新增` 的条目共 {new_count} 条。")
    lines.append("")

    for title, payload in sections.items():
        payload["names"] = dedupe_rows(payload["names"])
        payload["descrs"] = dedupe_rows(payload["descrs"])
        if not payload["names"] and not payload["descrs"]:
            continue
        lines.append(f"### {title}")
        lines.append("")
        if payload["names"]:
            lines.append("#### 物品名称")
            lines.append("")
            lines.append("| 英文 | 中文 | 备注 |")
            lines.append("| --- | --- | --- |")
            for ename, cname, note in payload["names"]:
                lines.append("| " + " | ".join(esc(x) for x in [ename, cname, note]) + " |")
            lines.append("")
        if payload["descrs"]:
            lines.append("#### 未鉴定物品名称")
            lines.append("")
            lines.append("| 英文 | 中文 | 备注 |")
            lines.append("| --- | --- | --- |")
            for edesc, cdesc, note in payload["descrs"]:
                lines.append("| " + " | ".join(esc(x) for x in [edesc, cdesc, note]) + " |")
            lines.append("")

    return "\n".join(lines) + "\n"


def main() -> None:
    if not OLD_OBJECTS_C.exists():
        raise FileNotFoundError(f"找不到旧版 objects.c：{OLD_OBJECTS_C}")

    if WORK_DIR.exists():
        shutil.rmtree(WORK_DIR, ignore_errors=True)
    WORK_DIR.mkdir(parents=True, exist_ok=True)

    try:
        temp_dir = WORK_DIR
        meta_rows = build_current_meta(temp_dir)
        old_rows = build_old_rows(temp_dir)
        DOC_PATH.write_text(render_doc(meta_rows, old_rows), encoding="utf-8", newline="\n")
        print(DOC_PATH)
    finally:
        shutil.rmtree(WORK_DIR, ignore_errors=True)


if __name__ == "__main__":
    main()
