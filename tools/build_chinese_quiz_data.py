#!/usr/bin/env python3
"""Generate LearningQuizChineseData.inc from a grade-1 lexicon."""

from __future__ import annotations

from pathlib import Path

TONES = {
	"a": "āáǎà",
	"o": "ōóǒò",
	"e": "ēéěè",
	"i": "īíǐì",
	"u": "ūúǔù",
	"ü": "ǖǘǚǜ",
}


def apply_tone(syllable: str, tone: int) -> str:
	if tone < 1 or tone > 4:
		return syllable
	for ch in "aoe":
		if ch in syllable:
			i = syllable.index(ch)
			return syllable[:i] + TONES[ch][tone - 1] + syllable[i + 1 :]
	# iu/ui: mark the latter vowel
	for i in range(len(syllable) - 1, -1, -1):
		c = syllable[i]
		if c in TONES:
			return syllable[:i] + TONES[c][tone - 1] + syllable[i + 1 :]
	return syllable


def make_base(initial: str, final: str) -> tuple[str, str, str]:
	if initial == "er" or final == "er":
		return "er", "", "er"
	ini = "" if initial in ("",) else initial
	fin = final
	if ini in ("j", "q", "x") and fin.startswith("ü"):
		fin = "u" + fin[1:]
	if ini == "y":
		# y + i/ü forms
		if fin.startswith("ü"):
			base = "yu" + fin[1:]
			return base, "y", final
		if fin.startswith("i"):
			base = fin if fin != "i" else "yi"
			return base, "y", final
		base = "y" + fin
		return base, "y", final
	if ini == "w":
		if fin.startswith("u"):
			base = fin if fin != "u" else "wu"
			return base, "w", final
		base = "w" + fin
		return base, "w", final
	base = ini + fin
	return base, ini, final


# (char, initial, final, tone, lookalike_group)
# tone 0 = light tone
RAW: list[tuple[str, str, str, int, int]] = [
	("人", "r", "en", 2, 1), ("入", "r", "u", 4, 1),
	("大", "d", "a", 4, 2), ("太", "t", "ai", 4, 2), ("天", "t", "ian", 1, 2),
	("土", "t", "u", 3, 3), ("士", "sh", "i", 4, 3),
	("日", "r", "i", 4, 4), ("目", "m", "u", 4, 4), ("白", "b", "ai", 2, 4), ("自", "z", "i", 4, 4),
	("木", "m", "u", 4, 5), ("禾", "h", "e", 2, 5), ("米", "m", "i", 3, 5),
	("刀", "d", "ao", 1, 7), ("力", "l", "i", 4, 7),
	("己", "j", "i", 3, 8), ("已", "y", "i", 3, 8),
	("未", "w", "ei", 4, 9), ("末", "m", "o", 4, 9),
	("贝", "b", "ei", 4, 10), ("见", "j", "ian", 4, 10),
	("乌", "w", "u", 1, 11), ("鸟", "n", "iao", 3, 11),
	("问", "w", "en", 4, 12), ("间", "j", "ian", 1, 12),
	("晴", "q", "ing", 2, 13), ("清", "q", "ing", 1, 13), ("请", "q", "ing", 3, 13), ("情", "q", "ing", 2, 13),
	("地", "d", "i", 4, 14), ("池", "ch", "i", 2, 14),
	("跑", "p", "ao", 3, 15), ("抱", "b", "ao", 4, 15), ("把", "b", "a", 3, 15),
	("妈", "m", "a", 1, 16), ("吗", "m", "a", 0, 16), ("马", "m", "a", 3, 16),
	("河", "h", "e", 2, 17), ("何", "h", "e", 2, 17),
	("林", "l", "in", 2, 18), ("森", "s", "en", 1, 18),
	("星", "x", "ing", 1, 19), ("醒", "x", "ing", 3, 19),
	("蓝", "l", "an", 2, 20), ("篮", "l", "an", 2, 20),
	("花", "h", "ua", 1, 21), ("华", "h", "ua", 2, 21),
	("草", "c", "ao", 3, 22), ("早", "z", "ao", 3, 22),
	("羊", "y", "ang", 2, 23), ("美", "m", "ei", 3, 23),
	("牛", "n", "iu", 2, 24), ("午", "w", "u", 3, 24),
	("干", "g", "an", 1, 25), ("千", "q", "ian", 1, 25),
	("上", "sh", "ang", 4, 27), ("止", "zh", "i", 3, 27),
	("下", "x", "ia", 4, 28), ("不", "b", "u", 4, 28),
	("中", "zh", "ong", 1, 29), ("申", "sh", "en", 1, 29),
	("小", "x", "iao", 3, 30), ("少", "sh", "ao", 3, 30),
	("多", "d", "uo", 1, 31), ("夕", "x", "i", 1, 31),
	("儿", "er", "er", 2, 32), ("几", "j", "i", 3, 32),
	("听", "t", "ing", 1, 33), ("厅", "t", "ing", 1, 33),
	("字", "z", "i", 4, 37), ("学", "x", "ue", 2, 37),
	("文", "w", "en", 2, 38), ("父", "f", "u", 4, 38),
	("母", "m", "u", 3, 39), ("每", "m", "ei", 3, 39),
	("手", "sh", "ou", 3, 41), ("毛", "m", "ao", 2, 41),
	("足", "z", "u", 2, 42), ("走", "z", "ou", 3, 42),
	("心", "x", "in", 1, 43), ("必", "b", "i", 4, 43),
	("想", "x", "iang", 3, 44), ("相", "x", "iang", 1, 44),
	("东", "d", "ong", 1, 45), ("车", "ch", "e", 1, 45),
	("西", "x", "i", 1, 46), ("四", "s", "i", 4, 46),
	("北", "b", "ei", 3, 48), ("比", "b", "i", 3, 48),
	("春", "ch", "un", 1, 49), ("看", "k", "an", 4, 49),
	("风", "f", "eng", 1, 53), ("凤", "f", "eng", 4, 53),
	("云", "y", "un", 2, 54), ("运", "y", "un", 4, 54),
	("雨", "y", "u", 3, 55), ("两", "l", "iang", 3, 55),
	("雪", "x", "ue", 3, 56), ("电", "d", "ian", 4, 56),
	("山", "sh", "an", 1, 57), ("出", "ch", "u", 1, 57),
	("水", "sh", "ui", 3, 58), ("永", "y", "ong", 3, 58),
	("火", "h", "uo", 3, 59), ("灭", "m", "ie", 4, 59),
	("石", "sh", "i", 2, 60), ("右", "y", "ou", 4, 60),
	("田", "t", "ian", 2, 61), ("由", "y", "ou", 2, 61), ("甲", "j", "ia", 3, 61),
	("声", "sh", "eng", 1, 62), ("生", "sh", "eng", 1, 62), ("升", "sh", "eng", 1, 62),
	("深", "sh", "en", 1, 63), ("身", "sh", "en", 1, 63),
	("真", "zh", "en", 1, 64), ("针", "zh", "en", 1, 64),
	("正", "zh", "eng", 4, 65), ("证", "zh", "eng", 4, 65),
	("成", "ch", "eng", 2, 66), ("城", "ch", "eng", 2, 66),
	("青", "q", "ing", 1, 67), ("清", "q", "ing", 1, 67),
	("京", "j", "ing", 1, 68), ("惊", "j", "ing", 1, 68),
	("分", "f", "en", 1, 69), ("风", "f", "eng", 1, 69),
	("根", "g", "en", 1, 70), ("跟", "g", "en", 1, 70),
	("很", "h", "en", 3, 71), ("狠", "h", "en", 3, 71),
	("边", "b", "ian", 1, 73), ("变", "b", "ian", 4, 73),
	("方", "f", "ang", 1, 75), ("放", "f", "ang", 4, 75), ("房", "f", "ang", 2, 75),
	("忙", "m", "ang", 2, 76), ("茫", "m", "ang", 2, 76),
	("平", "p", "ing", 2, 78), ("评", "p", "ing", 2, 78),
	("坐", "z", "uo", 4, 80), ("座", "z", "uo", 4, 80), ("做", "z", "uo", 4, 80),
	("从", "c", "ong", 2, 81), ("丛", "c", "ong", 2, 81),
	("松", "s", "ong", 1, 82), ("送", "s", "ong", 4, 82),
	("找", "zh", "ao", 3, 84), ("照", "zh", "ao", 4, 84),
	("包", "b", "ao", 1, 87), ("抱", "b", "ao", 4, 87), ("饱", "b", "ao", 3, 87),
	("刀", "d", "ao", 1, 89), ("到", "d", "ao", 4, 89), ("倒", "d", "ao", 3, 89),
	("桃", "t", "ao", 2, 90), ("逃", "t", "ao", 2, 90),
	("脑", "n", "ao", 3, 91), ("恼", "n", "ao", 3, 91),
	("老", "l", "ao", 3, 92), ("姥", "l", "ao", 3, 92),
	("知", "zh", "i", 1, 93), ("只", "zh", "i", 3, 93), ("支", "zh", "i", 1, 93),
	("吃", "ch", "i", 1, 94), ("尺", "ch", "i", 3, 94),
	("师", "sh", "i", 1, 95), ("狮", "sh", "i", 1, 95), ("诗", "sh", "i", 1, 95),
	("资", "z", "i", 1, 96), ("姿", "z", "i", 1, 96),
	("次", "c", "i", 4, 97), ("刺", "c", "i", 4, 97),
	("丝", "s", "i", 1, 98), ("思", "s", "i", 1, 98),
	("那", "n", "a", 4, 99), ("哪", "n", "a", 3, 99),
	("拉", "l", "a", 1, 100), ("啦", "l", "a", 0, 100),
	("里", "l", "i", 3, 101), ("理", "l", "i", 3, 101),
	("你", "n", "i", 3, 102), ("泥", "n", "i", 2, 102),
	("年", "n", "ian", 2, 103), ("念", "n", "ian", 4, 103),
	("连", "l", "ian", 2, 104), ("莲", "l", "ian", 2, 104),
	("路", "l", "u", 4, 105), ("露", "l", "u", 4, 105),
	# high-frequency singles
	("爸", "b", "a", 4, 0), ("我", "w", "o", 3, 0), ("他", "t", "a", 1, 0), ("她", "t", "a", 1, 0),
	("一", "y", "i", 1, 0), ("二", "er", "er", 4, 0), ("三", "s", "an", 1, 0), ("五", "w", "u", 3, 0),
	("六", "l", "iu", 4, 0), ("七", "q", "i", 1, 0), ("八", "b", "a", 1, 0), ("九", "j", "iu", 3, 0), ("十", "sh", "i", 2, 0),
	("头", "t", "ou", 2, 0), ("耳", "er", "er", 3, 0), ("牙", "y", "a", 2, 0), ("舌", "sh", "e", 2, 0),
	("哥", "g", "e", 1, 0), ("弟", "d", "i", 4, 0), ("姐", "j", "ie", 3, 0), ("妹", "m", "ei", 4, 0),
	("家", "j", "ia", 1, 0), ("门", "m", "en", 2, 0), ("窗", "ch", "uang", 1, 0),
	("书", "sh", "u", 1, 0), ("画", "h", "ua", 4, 0), ("笔", "b", "i", 3, 0), ("纸", "zh", "i", 3, 0),
	("红", "h", "ong", 2, 0), ("黄", "h", "uang", 2, 0), ("绿", "l", "ü", 4, 0), ("黑", "h", "ei", 1, 0),
	("猫", "m", "ao", 1, 0), ("狗", "g", "ou", 3, 0), ("鱼", "y", "u", 2, 0), ("虫", "ch", "ong", 2, 0),
	("飞", "f", "ei", 1, 0), ("跳", "t", "iao", 4, 0), ("笑", "x", "iao", 4, 0), ("哭", "k", "u", 1, 0),
	("吃", "ch", "i", 1, 0), ("喝", "h", "e", 1, 0), ("睡", "sh", "ui", 4, 0), ("玩", "w", "an", 2, 0),
	("高", "g", "ao", 1, 0), ("低", "d", "i", 1, 0), ("远", "y", "uan", 3, 0), ("近", "j", "in", 4, 0),
	("新", "x", "in", 1, 0), ("旧", "j", "iu", 4, 0), ("快", "k", "uai", 4, 0), ("慢", "m", "an", 4, 0),
	("左", "z", "uo", 3, 0), ("前", "q", "ian", 2, 0), ("后", "h", "ou", 4, 0),
	("外", "w", "ai", 4, 0), ("是", "sh", "i", 4, 0), ("在", "z", "ai", 4, 0), ("有", "y", "ou", 3, 0),
	("和", "h", "e", 2, 0), ("也", "y", "e", 3, 0), ("会", "h", "ui", 4, 0), ("能", "n", "eng", 2, 0),
	("来", "l", "ai", 2, 0), ("去", "q", "u", 4, 0), ("长", "zh", "ang", 3, 0), ("年", "n", "ian", 2, 0),
	("月", "y", "ue", 4, 0), ("明", "m", "ing", 2, 0), ("光", "g", "uang", 1, 0), ("亮", "l", "iang", 4, 0),
	("树", "sh", "u", 4, 0), ("叶", "y", "e", 4, 0), ("果", "g", "uo", 3, 0), ("瓜", "g", "ua", 1, 0),
	("江", "j", "iang", 1, 0), ("海", "h", "ai", 3, 0), ("湖", "h", "u", 2, 0),
	("船", "ch", "uan", 2, 0), ("桥", "q", "iao", 2, 0), ("街", "j", "ie", 1, 0),
	("衣", "y", "i", 1, 0), ("裤", "k", "u", 4, 0), ("鞋", "x", "ie", 2, 0), ("帽", "m", "ao", 4, 0),
	("饭", "f", "an", 4, 0), ("菜", "c", "ai", 4, 0), ("肉", "r", "ou", 4, 0), ("蛋", "d", "an", 4, 0),
	("床", "ch", "uang", 2, 0), ("桌", "zh", "uo", 1, 0), ("椅", "y", "i", 3, 0), ("灯", "d", "eng", 1, 0),
	("校", "x", "iao", 4, 0), ("班", "b", "an", 1, 0), ("课", "k", "e", 4, 0),
	("同", "t", "ong", 2, 0), ("友", "y", "ou", 3, 0), ("朋", "p", "eng", 2, 0),
	("开", "k", "ai", 1, 0), ("关", "g", "uan", 1, 0), ("读", "d", "u", 2, 0), ("写", "x", "ie", 3, 0),
	("说", "sh", "uo", 1, 0), ("话", "h", "ua", 4, 0), ("口", "k", "ou", 3, 0), ("男", "n", "an", 2, 0),
	("女", "n", "ü", 3, 0), ("南", "n", "an", 2, 0), ("秋", "q", "iu", 1, 0), ("冬", "d", "ong", 1, 0),
	("夏", "x", "ia", 4, 0), ("花", "h", "ua", 1, 0),
]


def main() -> int:
	seen: set[str] = set()
	rows: list[tuple[str, str, str, str, int, int]] = []
	for ch, ini, fin, tone, gid in RAW:
		if ch in seen:
			continue
		seen.add(ch)
		base, ini_s, fin_s = make_base(ini, fin)
		py = apply_tone(base, tone) if tone >= 1 else base
		rows.append((ch, py, ini_s, fin_s, tone, gid))

	out = Path(__file__).resolve().parents[1] / "src" / "Lawn" / "Widget" / "LearningQuizChineseData.inc"
	lines = [
		"// Generated by tools/build_chinese_quiz_data.py — grade-1 lexicon with lookalike groups.",
		"struct QuizHanzi {",
		"\tconst char* ch;",
		"\tconst char* pinyin;",
		"\tconst char* initial;",
		"\tconst char* finalNoTone;",
		"\tint tone;",
		"\tint lookalikeGroup;",
		"};",
		"constexpr QuizHanzi gQuizHanzi[] = {",
	]
	for ch, py, ini, fin, tone, gid in rows:
		lines.append(f'\t{{ "{ch}", "{py}", "{ini}", "{fin}", {tone}, {gid} }},')
	lines.append("};")
	out.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
	print(f"wrote {len(rows)} entries -> {out}")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
