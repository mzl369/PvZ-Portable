#!/usr/bin/env python3
"""Convert 题库/*.json into LearningQuizBankData.inc for the learning quiz."""

from __future__ import annotations

import json
import sys
from pathlib import Path


def esc(s: str) -> str:
	return (
		s.replace("\\", "\\\\")
		.replace('"', '\\"')
		.replace("\r", " ")
		.replace("\n", " ")
	)


def main() -> int:
	root = Path(__file__).resolve().parents[1]
	bank_dir = root / "题库"
	out = root / "src" / "Lawn" / "Widget" / "LearningQuizBankData.inc"
	if not bank_dir.is_dir():
		print(f"missing bank dir: {bank_dir}", file=sys.stderr)
		return 1

	entries: list[tuple[str, str, list[str], int]] = []
	for path in sorted(bank_dir.glob("*.json")):
		data = json.loads(path.read_text(encoding="utf-8"))
		subject = (data.get("meta") or {}).get("subject", "")
		title = "数学练习" if subject == "math" else "语文练习"
		for q in data.get("questions") or []:
			typ = q.get("type")
			stem = (q.get("stem") or "").strip()
			if not stem:
				continue
			if typ == "single":
				opts = q.get("options") or []
				if not isinstance(opts, list) or len(opts) != 4:
					continue
				if not all(isinstance(o, str) and o.strip() for o in opts):
					continue
				ans = q.get("answer")
				if not (isinstance(ans, str) and len(ans) == 1 and ans.upper() in "ABCD"):
					continue
				idx = ord(ans.upper()) - ord("A")
				entries.append((title, stem, [o.strip() for o in opts], idx))
			elif typ == "judge":
				ans = q.get("answer")
				if isinstance(ans, bool):
					correct = 0 if ans else 1
				elif isinstance(ans, str) and ans.lower() in ("true", "false"):
					correct = 0 if ans.lower() == "true" else 1
				else:
					continue
				entries.append((title, stem + "（判断对错）", ["对", "错", "不确定", "再看看"], correct))

	seen: set[tuple[str, tuple[str, ...]]] = set()
	unique: list[tuple[str, str, list[str], int]] = []
	for title, stem, opts, idx in entries:
		key = (stem, tuple(opts))
		if key in seen:
			continue
		seen.add(key)
		unique.append((title, stem, opts, idx))

	lines = [
		"// Auto-generated from 题库/*.json — re-run tools/build_quiz_bank.py to refresh.",
		"struct BankQuizEntry {",
		"\tconst char* title;",
		"\tconst char* stem;",
		"\tconst char* options[4];",
		"\tint correctIndex;",
		"};",
		"constexpr BankQuizEntry gBankQuizEntries[] = {",
	]
	for title, stem, opts, idx in unique:
		lines.append(
			'\t{ "%s", "%s", { "%s", "%s", "%s", "%s" }, %d },'
			% (esc(title), esc(stem), esc(opts[0]), esc(opts[1]), esc(opts[2]), esc(opts[3]), idx)
		)
	lines.append("};")
	out.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
	print(f"wrote {len(unique)} questions -> {out}")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
