#!/usr/bin/env python3
"""Assemble 导入资源/ for Android/desktop first-run import.

Copies main.pak + properties from a source game folder (default: dist/windows-x64-release),
then overlays project extras (quiz_fx, optional fonts).
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path


def copytree_merge(src: Path, dst: Path) -> None:
	dst.mkdir(parents=True, exist_ok=True)
	for path in src.rglob("*"):
		rel = path.relative_to(src)
		target = dst / rel
		if path.is_dir():
			target.mkdir(parents=True, exist_ok=True)
		else:
			target.parent.mkdir(parents=True, exist_ok=True)
			shutil.copy2(path, target)


def main() -> int:
	root = Path(__file__).resolve().parents[1]
	parser = argparse.ArgumentParser()
	parser.add_argument(
		"--source",
		type=Path,
		default=root / "dist" / "windows-x64-release",
		help="Folder that already contains main.pak and properties/",
	)
	parser.add_argument(
		"--out",
		type=Path,
		default=root / "导入资源",
		help="Output import pack folder",
	)
	args = parser.parse_args()

	src = args.source
	out = args.out
	pak = src / "main.pak"
	props = src / "properties"
	if not pak.is_file():
		print(f"missing main.pak under {src}", file=sys.stderr)
		return 1
	if not props.is_dir():
		print(f"missing properties/ under {src}", file=sys.stderr)
		return 1

	if out.exists():
		# Keep README/说明; replace game payload.
		for name in ("main.pak", "properties"):
			target = out / name
			if target.is_file():
				target.unlink()
			elif target.is_dir():
				shutil.rmtree(target)

	out.mkdir(parents=True, exist_ok=True)
	shutil.copy2(pak, out / "main.pak")
	copytree_merge(props, out / "properties")

	# Overlay project extras (quiz FX / optional fonts).
	extra_props = root / "properties"
	if extra_props.is_dir():
		copytree_merge(extra_props, out / "properties")

	readme = out / "使用说明.txt"
	readme.write_text(
		"\n".join(
			[
				"【安卓首次导入】",
				"1. 把整个「导入资源」文件夹拷到手机（任意位置均可）。",
				"2. 打开 PvZ Portable，在资源导入页点「选择资源文件夹」。",
				"3. 选中本文件夹（里面应能看到 main.pak 和 properties）。",
				"4. 导入成功后点「开始游戏」。",
				"",
				"本文件夹已包含：",
				"- main.pak（游戏主资源包）",
				"- properties/（配置、文案、答题特效 quiz_fx 等）",
				"",
				"如需更好的中文显示，可自行放入：",
				"properties/fonts/quiz_cjk.ttf（或 .otf）",
				"安卓一般也能直接使用系统自带中文字体。",
				"",
			]
		),
		encoding="utf-8",
	)

	print(f"assembled import pack -> {out}")
	print(f"  main.pak = {(out / 'main.pak').stat().st_size} bytes")
	print(f"  properties entries = {sum(1 for _ in (out / 'properties').rglob('*'))}")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
