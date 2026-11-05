/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 */

#include "LearningQuizDialog.h"
#include "../../LawnApp.h"
#include "../Board.h"
#include "../LawnCommon.h"
#include "GameButton.h"
#include "../../Resources.h"
#include "../../GameConstants.h"
#include "widget/WidgetManager.h"
#include "../../PvzpLib/PvzpCommon.h"
#include "graphics/MemoryImage.h"
#include "graphics/Font.h"
#include "graphics/Image.h"
#include "graphics/GLImage.h"
#include "../../ConstEnums.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace
{
	struct Grade1Hanzi
	{
		const char* ch;
		const char* initial;
		const char* finalSyllable;
	};

	constexpr Grade1Hanzi gGrade1Hanzi[] = {
		{ "妈", "m", "a" }, { "爸", "b", "a" }, { "大", "d", "a" }, { "小", "x", "iao" },
		{ "天", "t", "ian" }, { "地", "d", "i" }, { "人", "r", "en" }, { "手", "sh", "ou" },
		{ "口", "k", "ou" }, { "目", "m", "u" }, { "耳", "er", "er" }, { "日", "r", "i" },
		{ "月", "y", "ue" }, { "水", "sh", "ui" }, { "火", "h", "uo" }, { "山", "sh", "an" },
		{ "石", "sh", "i" }, { "田", "t", "ian" }, { "禾", "h", "e" }, { "花", "h", "ua" },
		{ "草", "c", "ao" }, { "虫", "ch", "ong" }, { "鸟", "n", "iao" }, { "鱼", "y", "u" },
		{ "猫", "m", "ao" }, { "狗", "g", "ou" }, { "牛", "n", "iu" }, { "羊", "y", "ang" },
		{ "马", "m", "a" }, { "鸡", "j", "i" }, { "鸭", "y", "a" }, { "兔", "t", "u" },
		{ "一", "y", "i" }, { "二", "er", "er" }, { "三", "s", "an" }, { "四", "s", "i" },
		{ "五", "w", "u" }, { "六", "l", "iu" }, { "七", "q", "i" }, { "八", "b", "a" },
		{ "九", "j", "iu" }, { "十", "sh", "i" }, { "上", "sh", "ang" }, { "下", "x", "ia" },
		{ "左", "z", "uo" }, { "右", "y", "ou" }, { "中", "zh", "ong" }, { "开", "k", "ai" },
		{ "关", "g", "uan" }, { "学", "x", "ue" }, { "习", "x", "i" }, { "字", "z", "i" },
		{ "文", "w", "en" }, { "读", "d", "u" }, { "写", "x", "ie" }, { "画", "h", "ua" },
	};

	constexpr const char* gInitialPool[] = {
		"b", "p", "m", "f", "d", "t", "n", "l", "g", "k", "h",
		"j", "q", "x", "zh", "ch", "sh", "r", "z", "c", "s", "y", "w"
	};
	constexpr const char* gFinalPool[] = {
		"a", "o", "e", "i", "u", "ü", "ai", "ei", "ui", "ao", "ou", "iu",
		"ie", "üe", "er", "an", "en", "in", "un", "ang", "eng", "ing", "ong",
		"ia", "iao", "ian", "iang", "iong", "ua", "uo", "uai", "uan", "uang"
	};

#include "LearningQuizBankData.inc"

	const Color gOptionColors[4] = {
		Color(255, 168, 120),
		Color(120, 200, 255),
		Color(255, 220, 110),
		Color(190, 160, 255),
	};

	int RandInt(int theMinInclusive, int theMaxInclusive)
	{
		if (theMaxInclusive <= theMinInclusive)
			return theMinInclusive;
		return theMinInclusive + Sexy::Rand(theMaxInclusive - theMinInclusive + 1);
	}

	void ShuffleOptions(LearningQuizQuestion& theQuestion)
	{
		for (int i = 3; i > 0; --i)
		{
			const int j = RandInt(0, i);
			std::swap(theQuestion.mOptions[i], theQuestion.mOptions[j]);
			if (theQuestion.mCorrectIndex == i)
				theQuestion.mCorrectIndex = j;
			else if (theQuestion.mCorrectIndex == j)
				theQuestion.mCorrectIndex = i;
		}
	}

	void FillUniqueDistractors(std::array<std::string, 4>& theOptions, int theCorrectIndex, auto&& theGenerator)
	{
		for (int i = 0; i < 4; ++i)
		{
			if (i == theCorrectIndex)
				continue;
			for (int attempt = 0; attempt < 24; ++attempt)
			{
				std::string aCandidate = theGenerator();
				bool aDup = false;
				for (int j = 0; j < 4; ++j)
				{
					if (!theOptions[j].empty() && theOptions[j] == aCandidate)
					{
						aDup = true;
						break;
					}
				}
				if (!aDup)
				{
					theOptions[i] = std::move(aCandidate);
					break;
				}
			}
			if (theOptions[i].empty())
				theOptions[i] = "?";
		}
	}

	bool NeedsSysFont(const std::string& theText)
	{
		for (unsigned char c : theText)
		{
			if (c >= 0x80)
				return true;
		}
		return false;
	}

	void FillRoundRect(Graphics* g, int x, int y, int w, int h, int r, const Color& theColor)
	{
		g->SetColor(theColor);
		if (r <= 0 || w < r * 2 || h < r * 2)
		{
			g->FillRect(x, y, w, h);
			return;
		}
		g->FillRect(x + r, y, w - r * 2, h);
		g->FillRect(x, y + r, w, h - r * 2);
		g->FillRect(x, y, r, r);
		g->FillRect(x + w - r, y, r, r);
		g->FillRect(x, y + h - r, r, r);
		g->FillRect(x + w - r, y + h - r, r, r);
	}

	bool DecodeUtf8Codepoint(const char*& thePtr, const char* theEnd, int& theOutCp)
	{
		if (thePtr >= theEnd)
			return false;
		const unsigned char c0 = static_cast<unsigned char>(*thePtr++);
		if (c0 < 0x80)
		{
			theOutCp = c0;
			return true;
		}
		if ((c0 & 0xE0) == 0xC0 && thePtr < theEnd)
		{
			theOutCp = ((c0 & 0x1F) << 6) | (static_cast<unsigned char>(*thePtr++) & 0x3F);
			return true;
		}
		if ((c0 & 0xF0) == 0xE0 && thePtr + 1 < theEnd)
		{
			const unsigned char c1 = static_cast<unsigned char>(*thePtr++);
			const unsigned char c2 = static_cast<unsigned char>(*thePtr++);
			theOutCp = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
			return true;
		}
		if ((c0 & 0xF8) == 0xF0 && thePtr + 2 < theEnd)
		{
			const unsigned char c1 = static_cast<unsigned char>(*thePtr++);
			const unsigned char c2 = static_cast<unsigned char>(*thePtr++);
			const unsigned char c3 = static_cast<unsigned char>(*thePtr++);
			theOutCp = ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
			return true;
		}
		theOutCp = '?';
		return true;
	}

	bool LoadQuizFontFile(std::vector<unsigned char>& theOutBytes, stbtt_fontinfo& theOutInfo)
	{
		// Prefer bundled CJK font under the resource root (Android import pack).
		static const char* kBundledRel[] = {
			"properties/fonts/quiz_cjk.ttf",
			"properties/fonts/quiz_cjk.otf",
			"properties/fonts/NotoSansSC-Regular.otf",
			"properties/fonts/NotoSansCJKsc-Regular.otf",
			nullptr
		};
		static const char* kSystemAbs[] = {
#ifdef __ANDROID__
			"/system/fonts/NotoSansCJK-Regular.ttc",
			"/system/fonts/NotoSansSC-Regular.otf",
			"/system/fonts/DroidSansFallback.ttf",
			"/system/fonts/NotoSansCJK-Regular.ttf",
			"/system/fonts/NotoSansCJK-Regular.otf",
			"/product/fonts/NotoSansCJK-Regular.ttc",
			"/product/fonts/NotoSansSC-Regular.otf",
			"/product/fonts/DroidSansFallback.ttf",
			"/system/fonts/NotoSerifCJK-Regular.ttc",
#elif defined(__APPLE__)
			"/System/Library/Fonts/PingFang.ttc",
			"/System/Library/Fonts/STHeiti Light.ttc",
			"/System/Library/Fonts/Hiragino Sans GB.ttc",
#elif defined(__linux__)
			"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
			"/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
			"/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
			"/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
#endif
			nullptr
		};

		auto tryOpenPath = [](const std::string& thePath, std::vector<unsigned char>& theBytes) -> bool
		{
			if (thePath.empty())
				return false;
			std::ifstream aFile(thePath, std::ios::binary);
			if (!aFile)
				return false;
			aFile.seekg(0, std::ios::end);
			const std::streamoff aSize = aFile.tellg();
			if (aSize <= 0)
				return false;
			aFile.seekg(0, std::ios::beg);
			theBytes.resize(static_cast<size_t>(aSize));
			return static_cast<bool>(aFile.read(reinterpret_cast<char*>(theBytes.data()), aSize));
		};

		// U+4E2D 「中」— reject Latin-only fonts (e.g. Roboto) that InitFont but have no CJK.
		constexpr int kCjkProbeCp = 0x4E2D;
		auto tryInitCjkFont = [&](std::vector<unsigned char>& theBytes, stbtt_fontinfo& theInfo) -> bool
		{
			const int aFontCount = std::max(1, stbtt_GetNumberOfFonts(theBytes.data()));
			for (int aIndex = 0; aIndex < aFontCount; ++aIndex)
			{
				const int aOffset = stbtt_GetFontOffsetForIndex(theBytes.data(), aIndex);
				if (aOffset < 0)
					continue;
				stbtt_fontinfo aInfo{};
				if (!stbtt_InitFont(&aInfo, theBytes.data(), aOffset))
					continue;
				if (stbtt_FindGlyphIndex(&aInfo, kCjkProbeCp) == 0)
					continue;
				theInfo = aInfo;
				theInfo.data = theBytes.data();
				return true;
			}
			return false;
		};

		std::vector<std::string> aPaths;
		aPaths.reserve(16);
		for (int i = 0; kBundledRel[i] != nullptr; ++i)
		{
			aPaths.push_back(Sexy::GetResourcePath(kBundledRel[i]));
			aPaths.emplace_back(kBundledRel[i]); // also try CWD for desktop runs
		}
		for (int i = 0; kSystemAbs[i] != nullptr; ++i)
			aPaths.emplace_back(kSystemAbs[i]);

		for (const std::string& aPath : aPaths)
		{
			std::vector<unsigned char> aBytes;
			if (!tryOpenPath(aPath, aBytes))
				continue;
			stbtt_fontinfo aInfo{};
			if (!tryInitCjkFont(aBytes, aInfo))
				continue;

			theOutBytes = std::move(aBytes);
			theOutInfo = aInfo;
			theOutInfo.data = theOutBytes.data();
			return true;
		}
		return false;
	}

	bool GetQuizFont(stbtt_fontinfo*& theOutInfo)
	{
		static bool aTried = false;
		static bool aReady = false;
		static std::vector<unsigned char> aFontBytes;
		static stbtt_fontinfo aFontInfo{};
		if (!aTried)
		{
			aTried = true;
			aReady = LoadQuizFontFile(aFontBytes, aFontInfo);
		}
		if (!aReady)
			return false;
		theOutInfo = &aFontInfo;
		return true;
	}

	std::unique_ptr<MemoryImage> RasterizeUtf8TextStb(const std::string& theUtf8, int thePixelHeight, const Color& theColor, int theMaxWidth)
	{
		stbtt_fontinfo* aFont = nullptr;
		if (!GetQuizFont(aFont) || theUtf8.empty() || thePixelHeight <= 0)
			return nullptr;

		const float aScale = stbtt_ScaleForPixelHeight(aFont, static_cast<float>(thePixelHeight));
		int aAscent = 0, aDescent = 0, aLineGap = 0;
		stbtt_GetFontVMetrics(aFont, &aAscent, &aDescent, &aLineGap);
		const int aLineHeight = std::max(thePixelHeight + 4, static_cast<int>((aAscent - aDescent + aLineGap) * aScale) + 4);

		struct GlyphPlace
		{
			int cp = 0;
			int x = 0;
			int y = 0;
			int w = 0;
			int h = 0;
		};
		std::vector<GlyphPlace> aGlyphs;
		aGlyphs.reserve(theUtf8.size());

		int aPenX = 0;
		int aPenY = 0;
		int aMaxX = 0;
		const char* aPtr = theUtf8.data();
		const char* aEnd = aPtr + theUtf8.size();
		int aPrevCp = 0;
		while (aPtr < aEnd)
		{
			int aCp = 0;
			if (!DecodeUtf8Codepoint(aPtr, aEnd, aCp))
				break;
			if (aCp == '\n')
			{
				aPenX = 0;
				aPenY += aLineHeight;
				aPrevCp = 0;
				continue;
			}

			int aAdvance = 0, aLsb = 0;
			stbtt_GetCodepointHMetrics(aFont, aCp, &aAdvance, &aLsb);
			if (aPrevCp != 0)
				aPenX += static_cast<int>(stbtt_GetCodepointKernAdvance(aFont, aPrevCp, aCp) * aScale);

			int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
			stbtt_GetCodepointBitmapBox(aFont, aCp, aScale, aScale, &x0, &y0, &x1, &y1);
			const int aGlyphW = std::max(1, x1 - x0);
			const int aAdvancePx = std::max(1, static_cast<int>(aAdvance * aScale));

			if (theMaxWidth > 0 && aPenX > 0 && aPenX + aGlyphW > theMaxWidth)
			{
				aPenX = 0;
				aPenY += aLineHeight;
				aPrevCp = 0;
			}

			GlyphPlace aPlace;
			aPlace.cp = aCp;
			aPlace.x = aPenX + x0;
			aPlace.y = aPenY + static_cast<int>(aAscent * aScale) + y0;
			aPlace.w = aGlyphW;
			aPlace.h = std::max(1, y1 - y0);
			aGlyphs.push_back(aPlace);

			aPenX += aAdvancePx;
			aMaxX = std::max(aMaxX, aPenX);
			aPrevCp = aCp;
		}

		const int aWidth = std::max(1, (theMaxWidth > 0 ? std::min(aMaxX, theMaxWidth) : aMaxX) + 4);
		const int aHeight = std::max(thePixelHeight + 8, aPenY + aLineHeight + 4);
		auto aImage = std::make_unique<MemoryImage>(gSexyAppBase);
		aImage->Create(aWidth, aHeight);
		uint32_t* aDest = aImage->GetBits();
		std::fill(aDest, aDest + aWidth * aHeight, 0u);

		const uint32_t aR = static_cast<uint32_t>(theColor.mRed);
		const uint32_t aG = static_cast<uint32_t>(theColor.mGreen);
		const uint32_t aB = static_cast<uint32_t>(theColor.mBlue);
		const uint32_t aA = static_cast<uint32_t>(theColor.mAlpha);

		std::vector<unsigned char> aBitmap;
		for (const GlyphPlace& g : aGlyphs)
		{
			aBitmap.assign(static_cast<size_t>(g.w) * static_cast<size_t>(g.h), 0);
			stbtt_MakeCodepointBitmap(aFont, aBitmap.data(), g.w, g.h, g.w, aScale, aScale, g.cp);
			for (int row = 0; row < g.h; ++row)
			{
				const int dy = g.y + row;
				if (dy < 0 || dy >= aHeight)
					continue;
				for (int col = 0; col < g.w; ++col)
				{
					const int dx = g.x + col;
					if (dx < 0 || dx >= aWidth)
						continue;
					const unsigned char aCoverage = aBitmap[static_cast<size_t>(row) * static_cast<size_t>(g.w) + static_cast<size_t>(col)];
					if (aCoverage == 0)
						continue;
					const uint32_t aOutA = (static_cast<uint32_t>(aCoverage) * aA) / 255u;
					uint32_t& aPixel = aDest[dy * aWidth + dx];
					const uint32_t aOldA = (aPixel >> 24) & 0xFFu;
					if (aOutA >= aOldA)
						aPixel = (aOutA << 24) | (aR << 16) | (aG << 8) | aB;
				}
			}
		}

		aImage->BitsChanged();
		aImage->mHasAlpha = true;
		aImage->mHasTrans = true;
		return aImage;
	}

#ifdef _WIN32
	std::unique_ptr<MemoryImage> RasterizeUtf8TextGdi(const std::string& theUtf8, int thePixelHeight, const Color& theColor, int theMaxWidth)
	{
		if (theUtf8.empty() || thePixelHeight <= 0)
			return nullptr;

		const int aWideLen = MultiByteToWideChar(CP_UTF8, 0, theUtf8.c_str(), -1, nullptr, 0);
		if (aWideLen <= 1)
			return nullptr;
		std::wstring aWide(static_cast<size_t>(aWideLen), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, theUtf8.c_str(), -1, aWide.data(), aWideLen);
		if (!aWide.empty() && aWide.back() == L'\0')
			aWide.pop_back();

		HDC aScreenDC = GetDC(nullptr);
		HDC aMemDC = CreateCompatibleDC(aScreenDC);

		HFONT aFont = CreateFontW(
			-thePixelHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
		);
		HGDIOBJ aOldFont = SelectObject(aMemDC, aFont);

		SIZE aSize{};
		GetTextExtentPoint32W(aMemDC, aWide.c_str(), static_cast<int>(aWide.size()), &aSize);
		const bool aWrap = theMaxWidth > 0 && aSize.cx > theMaxWidth;
		const int aWidth = aWrap ? theMaxWidth : std::max(1, static_cast<int>(aSize.cx) + 4);
		RECT aMeasure{ 0, 0, aWidth, 2048 };
		const UINT aDrawFlags = aWrap
			? (DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX)
			: (DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
		int aHeight = thePixelHeight + 8;
		if (aWrap)
		{
			const int aTextH = DrawTextW(aMemDC, aWide.c_str(), static_cast<int>(aWide.size()), &aMeasure, aDrawFlags | DT_CALCRECT);
			aHeight = std::max(thePixelHeight + 8, aTextH + 4);
		}
		else
		{
			aHeight = std::max(thePixelHeight + 8, static_cast<int>(aSize.cy) + 4);
		}

		BITMAPINFO aBmi{};
		aBmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		aBmi.bmiHeader.biWidth = aWidth;
		aBmi.bmiHeader.biHeight = -aHeight;
		aBmi.bmiHeader.biPlanes = 1;
		aBmi.bmiHeader.biBitCount = 32;
		aBmi.bmiHeader.biCompression = BI_RGB;

		void* aBits = nullptr;
		HBITMAP aBitmap = CreateDIBSection(aMemDC, &aBmi, DIB_RGB_COLORS, &aBits, nullptr, 0);
		HGDIOBJ aOldBmp = SelectObject(aMemDC, aBitmap);
		memset(aBits, 0, static_cast<size_t>(aWidth) * static_cast<size_t>(aHeight) * 4u);

		RECT aRect{ 0, 0, aWidth, aHeight };
		SetBkMode(aMemDC, TRANSPARENT);
		SetTextColor(aMemDC, RGB(255, 255, 255));
		DrawTextW(aMemDC, aWide.c_str(), static_cast<int>(aWide.size()), &aRect, aDrawFlags);

		auto aImage = std::make_unique<MemoryImage>(gSexyAppBase);
		aImage->Create(aWidth, aHeight);
		uint32_t* aDest = aImage->GetBits();
		const uint32_t* aSrc = static_cast<const uint32_t*>(aBits);
		const uint32_t aR = static_cast<uint32_t>(theColor.mRed);
		const uint32_t aG = static_cast<uint32_t>(theColor.mGreen);
		const uint32_t aB = static_cast<uint32_t>(theColor.mBlue);
		const uint32_t aA = static_cast<uint32_t>(theColor.mAlpha);
		for (int i = 0; i < aWidth * aHeight; ++i)
		{
			const uint32_t aCoverage = aSrc[i] & 0xFFu;
			const uint32_t aOutA = (aCoverage * aA) / 255u;
			aDest[i] = (aOutA << 24) | (aR << 16) | (aG << 8) | aB;
		}
		aImage->BitsChanged();
		aImage->mHasAlpha = true;
		aImage->mHasTrans = true;

		SelectObject(aMemDC, aOldBmp);
		SelectObject(aMemDC, aOldFont);
		DeleteObject(aBitmap);
		DeleteObject(aFont);
		DeleteDC(aMemDC);
		ReleaseDC(nullptr, aScreenDC);
		return aImage;
	}
#endif

	std::unique_ptr<MemoryImage> RasterizeUtf8Text(const std::string& theUtf8, int thePixelHeight, const Color& theColor, int theMaxWidth = 0)
	{
#ifdef _WIN32
		if (auto aGdi = RasterizeUtf8TextGdi(theUtf8, thePixelHeight, theColor, theMaxWidth))
			return aGdi;
#endif
		return RasterizeUtf8TextStb(theUtf8, thePixelHeight, theColor, theMaxWidth);
	}

	void DrawCenteredImage(Graphics* g, MemoryImage* theImage, int theCenterX, int theBaselineY)
	{
		if (!theImage)
			return;
		g->DrawImage(theImage, theCenterX - theImage->mWidth / 2, theBaselineY - theImage->mHeight + 6);
	}

	void DrawGameFontCentered(Graphics* g, _Font* theFont, const std::string& theText, int theCenterX, int theY, const Color& theColor)
	{
		if (!theFont)
			return;
		g->SetFont(theFont);
		g->SetColor(theColor);
		g->DrawString(theText, theCenterX - theFont->StringWidth(theText) / 2, theY);
	}
}

LearningQuizQuestion GenerateMixedMathLearningQuiz()
{
	LearningQuizQuestion aQuestion;
	aQuestion.mKind = LearningQuizKind::MathMixed;
	aQuestion.mTitle = "三数加减";
	aQuestion.mPrompt = "从左到右算，中间不能出现负数哦！";

	int a = 1, b = 1, c = 1, aAnswer = 0;
	bool aAdd1 = true, aAdd2 = true;
	bool aFound = false;
	for (int attempt = 0; attempt < 400; ++attempt)
	{
		// Operands are 1..10 (no zero). Final answer may exceed 10.
		a = RandInt(1, 10);
		b = RandInt(1, 10);
		c = RandInt(1, 10);
		aAdd1 = RandInt(0, 1) == 0;
		aAdd2 = RandInt(0, 1) == 0;

		const int aMid = aAdd1 ? (a + b) : (a - b);
		if (aMid < 0)
			continue;
		aAnswer = aAdd2 ? (aMid + c) : (aMid - c);
		if (aAnswer < 0)
			continue;

		aFound = true;
		break;
	}

	if (!aFound)
	{
		// Safe fallback that always satisfies the step rules.
		a = 8; b = 5; c = 3; aAdd1 = true; aAdd2 = false; aAnswer = 10;
	}

	aQuestion.mBigText = std::format("{} {} {} {} {} = ?",
		a, aAdd1 ? '+' : '-', b, aAdd2 ? '+' : '-', c);
	aQuestion.mCorrectIndex = 0;
	aQuestion.mOptions[0] = std::to_string(aAnswer);
	FillUniqueDistractors(aQuestion.mOptions, 0, [aAnswer]() {
		return std::to_string(std::max(0, aAnswer + RandInt(-5, 5)));
	});
	ShuffleOptions(aQuestion);
	return aQuestion;
}

LearningQuizQuestion GeneratePinyinLearningQuiz()
{
	LearningQuizQuestion aQuestion;
	const Grade1Hanzi& aHanzi = gGrade1Hanzi[RandInt(0, static_cast<int>(std::size(gGrade1Hanzi)) - 1)];
	const bool aAskInitial = (std::string(aHanzi.initial) != "er") && RandInt(0, 1) == 0;
	aQuestion.mBigText = aHanzi.ch;
	aQuestion.mCorrectIndex = 0;
	if (aAskInitial)
	{
		aQuestion.mKind = LearningQuizKind::PinyinInitial;
		aQuestion.mTitle = "拼音小课堂";
		aQuestion.mPrompt = "这个字的声母是？";
		aQuestion.mOptions[0] = aHanzi.initial;
		FillUniqueDistractors(aQuestion.mOptions, 0, []() {
			return std::string(gInitialPool[RandInt(0, static_cast<int>(std::size(gInitialPool)) - 1)]);
		});
	}
	else
	{
		aQuestion.mKind = LearningQuizKind::PinyinFinal;
		aQuestion.mTitle = "拼音小课堂";
		aQuestion.mPrompt = "这个字的韵母是？";
		aQuestion.mOptions[0] = aHanzi.finalSyllable;
		FillUniqueDistractors(aQuestion.mOptions, 0, []() {
			return std::string(gFinalPool[RandInt(0, static_cast<int>(std::size(gFinalPool)) - 1)]);
		});
	}
	ShuffleOptions(aQuestion);
	return aQuestion;
}

LearningQuizQuestion GenerateBankLearningQuiz()
{
	LearningQuizQuestion aQuestion;
	if (std::size(gBankQuizEntries) == 0)
		return GenerateMixedMathLearningQuiz();

	const BankQuizEntry& aEntry = gBankQuizEntries[RandInt(0, static_cast<int>(std::size(gBankQuizEntries)) - 1)];
	aQuestion.mKind = LearningQuizKind::Bank;
	aQuestion.mTitle = aEntry.title;
	aQuestion.mPrompt = "请选出正确答案";
	aQuestion.mBigText = aEntry.stem;
	aQuestion.mCorrectIndex = std::clamp(aEntry.correctIndex, 0, 3);
	for (int i = 0; i < 4; ++i)
		aQuestion.mOptions[i] = aEntry.options[i] ? aEntry.options[i] : "?";
	ShuffleOptions(aQuestion);
	return aQuestion;
}

LearningQuizQuestion GenerateRandomLearningQuiz()
{
	// 50% from downloaded bank, 50% randomly generated.
	if (!std::empty(gBankQuizEntries) && RandInt(0, 1) == 0)
		return GenerateBankLearningQuiz();

	// Within generated questions: mostly mixed math, some pinyin.
	if (RandInt(0, 99) < 75)
		return GenerateMixedMathLearningQuiz();
	return GeneratePinyinLearningQuiz();
}

LearningQuizDialog::LearningQuizDialog(LawnApp* theApp, LearningQuizPurpose thePurpose) :
	LawnDialog(theApp, Dialogs::DIALOG_LEARNING_QUIZ, true, "", "", "", Dialog::BUTTONS_NONE)
{
	mApp = theApp;
	mPurpose = thePurpose;
	mDrawStandardBack = false;
	mVerticalCenterText = false;
	mFeedbackColor = Color(60, 140, 80);
	mWidth = 540;
	mHeight = 420;
	mAppearTimer = 18;
	mAnswerResolved = false;
	mResultFxTimer = 0;
	mCloseDelay = 0;
	mFxEmojiImage = nullptr;

	for (int i = 0; i < 4; ++i)
	{
		mOptionButtons[i] = std::make_unique<NewLawnButton>(nullptr, Quiz_Option0 + i, this);
		// NewLawnButton crashes when pressed without button images (null DrawImage).
		// We draw the colorful pads + labels ourselves; buttons only handle hits.
		mOptionButtons[i]->mBtnNoDraw = true;
		mOptionButtons[i]->SetLabel("");
		mOptionButtons[i]->mHasAlpha = true;
		mOptionButtons[i]->mHasTransparencies = true;
	}

	GenerateQuestion();
}

LearningQuizDialog::~LearningQuizDialog()
{
	delete mFxEmojiImage;
	mFxEmojiImage = nullptr;
}

void LearningQuizDialog::RebuildTextImages()
{
	mTitleImage = RasterizeUtf8Text(mQuestion.mTitle, 28, Color(255, 255, 255));
	mPromptImage = RasterizeUtf8Text(mQuestion.mPrompt, 22, Color(80, 100, 75));

	const bool aBank = mQuestion.mKind == LearningQuizKind::Bank;
	const int aBigSize = aBank ? 22 : (NeedsSysFont(mQuestion.mBigText) ? 56 : 40);
	const int aBigMaxW = aBank ? (mWidth - 100) : 0;
	mBigTextImage = RasterizeUtf8Text(mQuestion.mBigText, aBigSize, Color(45, 90, 65), aBigMaxW);
	mFeedbackImage.reset();
	for (int i = 0; i < 4; ++i)
	{
		const int aOptSize = (mQuestion.mOptions[i].size() > 12) ? 20 : 26;
		mOptionLabelImages[i] = RasterizeUtf8Text(mQuestion.mOptions[i], aOptSize, Color(45, 35, 25), mWidth / 2 - 50);
	}
}

void LearningQuizDialog::GenerateQuestion()
{
	mQuestion = GenerateRandomLearningQuiz();
	mFeedback.clear();
	mFeedbackImage.reset();
	mFeedbackTimer = 0;
	mShakeWrongTimer = 0;

	for (auto& aButton : mOptionButtons)
		aButton->SetDisabled(false);

	RebuildTextImages();
	LayoutButtons();
	MarkDirty();
}

void LearningQuizDialog::LayoutButtons()
{
	const int aPad = 30;
	const int aBtnW = (mWidth - aPad * 2 - 18) / 2;
	const int aBtnH = 64;
	const int aStartY = mHeight - 160;

	for (int i = 0; i < 4; ++i)
	{
		const int aCol = i % 2;
		const int aRow = i / 2;
		mOptionButtons[i]->Resize(
			aPad + aCol * (aBtnW + 18),
			aStartY + aRow * (aBtnH + 14),
			aBtnW,
			aBtnH
		);
	}
}

void LearningQuizDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	LawnDialog::AddedToManager(theWidgetManager);
	for (auto& aButton : mOptionButtons)
		AddWidget(aButton.get());
	theWidgetManager->SetFocus(this);
}

void LearningQuizDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	for (auto& aButton : mOptionButtons)
		RemoveWidget(aButton.get());
	LawnDialog::RemovedFromManager(theWidgetManager);

	if (!mApp->mBoard)
		return;

	if (mPurpose == LearningQuizPurpose::Ultimate)
	{
		if (mAnsweredCorrectly)
			mApp->mBoard->FinishLearningQuizUltimate();
		else if (mAnswerResolved)
			mApp->mBoard->FailLearningQuizUltimate();
		else
			mApp->mBoard->mPendingUltimateQuiz = {};
		return;
	}

	if (mAnsweredCorrectly)
		mApp->mBoard->FinishLearningQuizPlant();
	else if (mAnswerResolved)
		mApp->mBoard->FailLearningQuizPlant();
	else
		mApp->mBoard->CancelLearningQuizPlant();
}

void LearningQuizDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	LawnDialog::Resize(theX, theY, theWidth, theHeight);
	LayoutButtons();
}

void LearningQuizDialog::BeginResultFx(bool theCorrect)
{
	mFxSparks.clear();
	delete mFxEmojiImage;
	mFxEmojiImage = nullptr;

	const char* aEmojiPath = theCorrect
		? "properties/quiz_fx/fireworks.png"
		: "properties/quiz_fx/crying.png";
	mFxEmojiImage = gSexyAppBase->GetImage(aEmojiPath, true);
	if (!mFxEmojiImage && theCorrect)
		mFxEmojiImage = gSexyAppBase->GetImage("properties/quiz_fx/party_popper.png", true);

	if (theCorrect)
	{
		static const Color kColors[] = {
			Color(255, 80, 80), Color(255, 200, 60), Color(80, 220, 120),
			Color(80, 160, 255), Color(255, 120, 220), Color(255, 255, 120)
		};
		for (int burst = 0; burst < 3; ++burst)
		{
			const float cx = static_cast<float>(mWidth / 2 + RandInt(-90, 90));
			const float cy = static_cast<float>(140 + RandInt(-30, 40));
			for (int i = 0; i < 18; ++i)
			{
				const float aAngle = (static_cast<float>(i) / 18.0f) * 6.28318f + RandRangeFloat(-0.2f, 0.2f);
				const float aSpeed = RandRangeFloat(2.5f, 6.5f);
				FxSpark aSpark;
				aSpark.x = cx;
				aSpark.y = cy;
				aSpark.vx = std::cos(aAngle) * aSpeed;
				aSpark.vy = std::sin(aAngle) * aSpeed - 1.5f;
				aSpark.maxLife = RandRangeFloat(28.0f, 48.0f);
				aSpark.life = aSpark.maxLife;
				const Color& c = kColors[RandInt(0, static_cast<int>(std::size(kColors)) - 1)];
				aSpark.r = c.mRed; aSpark.g = c.mGreen; aSpark.b = c.mBlue;
				aSpark.size = RandRangeFloat(4.0f, 9.0f);
				mFxSparks.push_back(aSpark);
			}
		}

		if (mApp->mBoard)
		{
			const int aOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 0);
			mApp->AddPvzpParticle(static_cast<float>(BOARD_WIDTH / 2), 120.0f, aOrder, ParticleEffect::PARTICLE_POWIE);
			mApp->AddPvzpParticle(static_cast<float>(BOARD_WIDTH / 2 - 80), 160.0f, aOrder, ParticleEffect::PARTICLE_JACKEXPLODE);
			mApp->AddPvzpParticle(static_cast<float>(BOARD_WIDTH / 2 + 80), 160.0f, aOrder, ParticleEffect::PARTICLE_PRESENT_PICKUP);
		}
		mApp->PlaySample(Sexy::SOUND_EXPLOSION);
	}
	else
	{
		// Falling tear droplets under the cry emoji.
		for (int i = 0; i < 10; ++i)
		{
			FxSpark aTear;
			aTear.x = static_cast<float>(mWidth / 2 + RandInt(-18, 18));
			aTear.y = 150.0f + RandRangeFloat(0.0f, 20.0f);
			aTear.vx = RandRangeFloat(-0.4f, 0.4f);
			aTear.vy = RandRangeFloat(1.2f, 2.8f);
			aTear.maxLife = RandRangeFloat(30.0f, 55.0f);
			aTear.life = aTear.maxLife;
			aTear.r = 90; aTear.g = 170; aTear.b = 255;
			aTear.size = RandRangeFloat(3.0f, 6.0f);
			mFxSparks.push_back(aTear);
		}
		mApp->PlaySample(Sexy::SOUND_BUZZER);
	}

	mResultFxTimer = 55;
	mCloseDelay = 55;
}

void LearningQuizDialog::UpdateResultFx()
{
	for (FxSpark& s : mFxSparks)
	{
		s.x += s.vx;
		s.y += s.vy;
		s.vy += mAnsweredCorrectly ? 0.12f : 0.08f;
		s.life -= 1.0f;
	}
	mFxSparks.erase(
		std::remove_if(mFxSparks.begin(), mFxSparks.end(), [](const FxSpark& s) { return s.life <= 0.0f; }),
		mFxSparks.end()
	);
}

void LearningQuizDialog::DrawResultFx(Graphics* g)
{
	if (mFxEmojiImage)
	{
		const float aPulse = 1.0f + 0.12f * std::sin(static_cast<float>(mResultFxTimer) * 0.35f);
		const int aW = static_cast<int>(mFxEmojiImage->mWidth * 1.8f * aPulse);
		const int aH = static_cast<int>(mFxEmojiImage->mHeight * 1.8f * aPulse);
		g->DrawImage(mFxEmojiImage, mWidth / 2 - aW / 2, 118 - aH / 2, aW, aH);
	}

	for (const FxSpark& s : mFxSparks)
	{
		const float aAlpha = std::clamp(s.life / std::max(1.0f, s.maxLife), 0.0f, 1.0f);
		const int aSize = std::max(2, static_cast<int>(s.size * aAlpha));
		g->SetColor(Color(s.r, s.g, s.b, static_cast<int>(255 * aAlpha)));
		g->FillRect(static_cast<int>(s.x) - aSize / 2, static_cast<int>(s.y) - aSize / 2, aSize, aSize);
	}
}

void LearningQuizDialog::FinishAndClose()
{
	mApp->KillDialog(Dialogs::DIALOG_LEARNING_QUIZ);
}

void LearningQuizDialog::Update()
{
	LawnDialog::Update();
	if (mAppearTimer > 0)
		--mAppearTimer;
	if (mFeedbackTimer > 0)
		--mFeedbackTimer;
	if (mShakeWrongTimer > 0)
		--mShakeWrongTimer;
	if (mResultFxTimer > 0)
	{
		--mResultFxTimer;
		UpdateResultFx();
	}
	if (mCloseDelay > 0)
	{
		--mCloseDelay;
		if (mCloseDelay == 0)
			FinishAndClose();
	}
	MarkDirty();
}

void LearningQuizDialog::Draw(Graphics* g)
{
	const int aPop = mAppearTimer > 0 ? (mAppearTimer * 2) : 0;
	const int aShakeX = (mShakeWrongTimer > 0) ? (((mShakeWrongTimer / 2) % 2 == 0) ? -5 : 5) : 0;

	FillRoundRect(g, 10, 14, mWidth, mHeight, 18, Color(40, 70, 55, 55));
	FillRoundRect(g, aShakeX, aPop, mWidth, mHeight - aPop, 18, Color(255, 250, 235));

	g->SetColor(Color(110, 195, 145));
	g->FillRect(aShakeX, aPop, mWidth, 62);
	g->SetColor(Color(255, 255, 255, 70));
	g->FillRect(aShakeX, aPop, mWidth, 22);
	g->SetColor(Color(90, 175, 125));
	g->FillRect(aShakeX, aPop + 62, mWidth, 4);

	g->SetColor(Color(255, 185, 120));
	g->FillRect(18 + aShakeX, 78 + aPop, 12, 12);
	g->SetColor(Color(140, 205, 255));
	g->FillRect(mWidth - 30 + aShakeX, 78 + aPop, 12, 12);

	if (mTitleImage)
		DrawCenteredImage(g, mTitleImage.get(), mWidth / 2 + aShakeX, 44 + aPop);
	else
		DrawGameFontCentered(g, FONT_HOUSEOFTERROR20, mQuestion.mTitle, mWidth / 2 + aShakeX, 40 + aPop, Color::White);

	if (mPromptImage)
		DrawCenteredImage(g, mPromptImage.get(), mWidth / 2 + aShakeX, 96 + aPop);
	else
		DrawGameFontCentered(g, FONT_DWARVENTODCRAFT18, mQuestion.mPrompt, mWidth / 2 + aShakeX, 92 + aPop, Color(90, 110, 90));

	FillRoundRect(g, 36 + aShakeX, 112 + aPop, mWidth - 72, 108, 14, Color(255, 255, 255));
	g->SetColor(Color(165, 220, 185));
	g->FillRect(36 + aShakeX, 112 + aPop, mWidth - 72, 8);
	g->FillRect(36 + aShakeX, 212 + aPop, mWidth - 72, 8);

	if (mBigTextImage)
		DrawCenteredImage(g, mBigTextImage.get(), mWidth / 2 + aShakeX, 188 + aPop);
	else
		DrawGameFontCentered(g, FONT_HOUSEOFTERROR28, mQuestion.mBigText, mWidth / 2 + aShakeX, 180 + aPop, Color(50, 90, 70));

	if (!mFeedback.empty() && mFeedbackTimer > 0)
	{
		if (!mFeedbackImage)
			mFeedbackImage = RasterizeUtf8Text(mFeedback, 20, mFeedbackColor);
		if (mFeedbackImage)
			DrawCenteredImage(g, mFeedbackImage.get(), mWidth / 2 + aShakeX, 248 + aPop);
		else
			DrawGameFontCentered(g, FONT_DWARVENTODCRAFT15, mFeedback, mWidth / 2 + aShakeX, 244 + aPop, mFeedbackColor);
	}

	if (mAnswerResolved)
		DrawResultFx(g);

	for (int i = 0; i < 4; ++i)
	{
		NewLawnButton* aBtn = mOptionButtons[i].get();
		Color aPad = gOptionColors[i];
		if (aBtn->mIsOver && !aBtn->mDisabled)
		{
			aPad.mRed = std::min(255, aPad.mRed + 20);
			aPad.mGreen = std::min(255, aPad.mGreen + 20);
			aPad.mBlue = std::min(255, aPad.mBlue + 20);
		}
		FillRoundRect(g, aBtn->mX, aBtn->mY, aBtn->mWidth, aBtn->mHeight, 12, aPad);
		g->SetColor(Color(255, 255, 255, 90));
		g->FillRect(aBtn->mX + 8, aBtn->mY + 4, aBtn->mWidth - 16, 10);

		if (mOptionLabelImages[i])
		{
			DrawCenteredImage(
				g,
				mOptionLabelImages[i].get(),
				aBtn->mX + aBtn->mWidth / 2,
				aBtn->mY + aBtn->mHeight / 2 + 14
			);
		}
		else
		{
			DrawGameFontCentered(
				g,
				FONT_HOUSEOFTERROR20,
				mQuestion.mOptions[i],
				aBtn->mX + aBtn->mWidth / 2,
				aBtn->mY + aBtn->mHeight / 2 + 8,
				Color(45, 35, 25)
			);
		}
	}
}

void LearningQuizDialog::ButtonPress([[maybe_unused]] int theId)
{
	mApp->PlaySample(Sexy::SOUND_TAP);
}

void LearningQuizDialog::ButtonDepress(int theId)
{
	if (theId < Quiz_Option0 || theId > Quiz_Option3)
		return;
	if (mAnswerResolved)
		return;

	const int aIndex = theId - Quiz_Option0;
	const bool aCorrect = (aIndex == mQuestion.mCorrectIndex);
	mAnswerResolved = true;
	mAnsweredCorrectly = aCorrect;

	for (auto& aButton : mOptionButtons)
		aButton->SetDisabled(true);

	if (aCorrect)
	{
		mFeedback = (mPurpose == LearningQuizPurpose::Ultimate) ? "太棒了！大招发动！" : "太棒了！可以种下啦";
		mFeedbackColor = Color(40, 150, 70);
		mFeedbackTimer = 55;
		mFeedbackImage = RasterizeUtf8Text(mFeedback, 20, mFeedbackColor);
		mApp->PlayFoley(FoleyType::FOLEY_PLANT);
		BeginResultFx(true);
	}
	else
	{
		mFeedback = (mPurpose == LearningQuizPurpose::Ultimate)
			? "答错了，大招进入冷却"
			: "答错了，取消种植并扣25阳光";
		mFeedbackColor = Color(200, 80, 70);
		mFeedbackTimer = 55;
		mFeedbackImage = RasterizeUtf8Text(mFeedback, 20, mFeedbackColor);
		mShakeWrongTimer = 18;
		BeginResultFx(false);
	}
}

void LearningQuizDialog::KeyDown(KeyCode theKey)
{
	if (mAnswerResolved)
		return;
	if (theKey == KEYCODE_ESCAPE)
	{
		// Escape cancels without penalty / without ultimate cooldown.
		mApp->KillDialog(Dialogs::DIALOG_LEARNING_QUIZ);
		return;
	}
	if (theKey >= '1' && theKey <= '4')
	{
		ButtonDepress(Quiz_Option0 + (theKey - '1'));
		return;
	}
}
