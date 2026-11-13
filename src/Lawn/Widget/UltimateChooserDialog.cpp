/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 */

#include "UltimateChooserDialog.h"
#include "../SeedPacket.h"
#include "../../LawnApp.h"
#include "../Board.h"
#include "../../GameConstants.h"
#include "../../Resources.h"
#include "widget/WidgetManager.h"

namespace
{
	constexpr int SLOT_COLS = 3;
	constexpr int SLOT_GAP_X = 28;
	constexpr int SLOT_GAP_Y = 22;
	constexpr int PANEL_PAD_X = 48;
	constexpr int PANEL_PAD_Y = 40;

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
}

UltimateChooserDialog::UltimateChooserDialog() :
	LawnDialog(gLawnApp, Dialogs::DIALOG_ULTIMATE_CHOOSER, true, "", "", "", Dialog::BUTTONS_NONE)
{
	mApp = gLawnApp;
	mDrawStandardBack = false;
	mVerticalCenterText = false;
	mClip = false;
	mHoverType = UltimateType::NUM_ULTIMATE_TYPES;
	mAppearTimer = 14;

	const int aCols = SLOT_COLS;
	const int aRows = (static_cast<int>(UltimateType::NUM_ULTIMATE_TYPES) + aCols - 1) / aCols;
	mWidth = PANEL_PAD_X * 2 + aCols * SEED_PACKET_WIDTH + (aCols - 1) * SLOT_GAP_X;
	mHeight = PANEL_PAD_Y * 2 + aRows * SEED_PACKET_HEIGHT + (aRows - 1) * SLOT_GAP_Y;
}

UltimateChooserDialog::~UltimateChooserDialog() = default;

SeedType UltimateChooserDialog::GetSeedIcon(UltimateType theType)
{
	switch (theType)
	{
	case UltimateType::ULTIMATE_SUN_GEYSER: return SeedType::SEED_SUNFLOWER;
	case UltimateType::ULTIMATE_PEA_BARRAGE: return SeedType::SEED_PEASHOOTER;
	case UltimateType::ULTIMATE_JALAPENO_ALL: return SeedType::SEED_JALAPENO;
	case UltimateType::ULTIMATE_HYPNO_ALLIES: return SeedType::SEED_HYPNOSHROOM;
	case UltimateType::ULTIMATE_STAR_STORM: return SeedType::SEED_STARFRUIT;
	case UltimateType::ULTIMATE_COB_BARRAGE: return SeedType::SEED_COBCANNON;
	case UltimateType::ULTIMATE_CHERRY_BOMBS: return SeedType::SEED_CHERRYBOMB;
	case UltimateType::ULTIMATE_PUMPKIN_SHIELD: return SeedType::SEED_PUMPKINSHELL;
	case UltimateType::ULTIMATE_PLANT_UPGRADE: return SeedType::SEED_GATLINGPEA;
	default: return SeedType::SEED_PEASHOOTER;
	}
}

void UltimateChooserDialog::GetSlotPosition(int theIndex, int& x, int& y) const
{
	const int aGridW = SLOT_COLS * SEED_PACKET_WIDTH + (SLOT_COLS - 1) * SLOT_GAP_X;
	const int aLeft = (mWidth - aGridW) / 2;
	x = aLeft + (theIndex % SLOT_COLS) * (SEED_PACKET_WIDTH + SLOT_GAP_X);
	y = PANEL_PAD_Y + (theIndex / SLOT_COLS) * (SEED_PACKET_HEIGHT + SLOT_GAP_Y);
}

UltimateType UltimateChooserDialog::HitTest(int x, int y) const
{
	const int aCount = static_cast<int>(UltimateType::NUM_ULTIMATE_TYPES);
	for (int i = 0; i < aCount; ++i)
	{
		int aX, aY;
		GetSlotPosition(i, aX, aY);
		if (Rect(aX, aY, SEED_PACKET_WIDTH, SEED_PACKET_HEIGHT).Contains(x, y))
			return static_cast<UltimateType>(i);
	}
	return UltimateType::NUM_ULTIMATE_TYPES;
}

void UltimateChooserDialog::UpdateCursor()
{
	const UltimateType aType = HitTest(mApp->mWidgetManager->mLastMouseX - mX, mApp->mWidgetManager->mLastMouseY - mY);
	mHoverType = aType;
	mApp->SetCursor(aType != UltimateType::NUM_ULTIMATE_TYPES ? CURSOR_HAND : CURSOR_POINTER);
}

void UltimateChooserDialog::Update()
{
	LawnDialog::Update();
	if (mAppearTimer > 0)
		--mAppearTimer;
	UpdateCursor();
	MarkDirty();
}

void UltimateChooserDialog::Draw(Graphics* g)
{
	const int aPop = mAppearTimer > 0 ? (mAppearTimer * 2) : 0;

	FillRoundRect(g, 8, 10, mWidth, mHeight, 18, Color(40, 70, 55, 55));
	FillRoundRect(g, 0, aPop, mWidth, mHeight - aPop, 18, Color(255, 250, 235));

	g->SetColor(Color(110, 195, 145));
	g->FillRect(0, aPop, mWidth, 10);
	g->SetColor(Color(255, 255, 255, 70));
	g->FillRect(0, aPop, mWidth, 4);

	g->SetLinearBlend(true);
	const int aCount = static_cast<int>(UltimateType::NUM_ULTIMATE_TYPES);
	for (int i = 0; i < aCount; ++i)
	{
		int aX, aY;
		GetSlotPosition(i, aX, aY);
		aY += aPop;
		const UltimateType aType = static_cast<UltimateType>(i);
		DrawSeedPacket(g, static_cast<float>(aX), static_cast<float>(aY), GetSeedIcon(aType), SeedType::SEED_NONE, 0.0f, 255, false, false);

		if (aType == mHoverType)
		{
			g->SetColor(Color(255, 255, 180, 80));
			g->FillRect(aX, aY, SEED_PACKET_WIDTH, SEED_PACKET_HEIGHT);
			g->SetColor(Color(255, 210, 80, 220));
			g->DrawRect(aX - 1, aY - 1, SEED_PACKET_WIDTH + 1, SEED_PACKET_HEIGHT + 1);
		}
	}
}

void UltimateChooserDialog::MouseDown(int x, int y, int theClickCount)
{
	(void)theClickCount;
	const UltimateType aType = HitTest(x, y);
	if (aType != UltimateType::NUM_ULTIMATE_TYPES)
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
		Board* aBoard = mApp->mBoard;
		mApp->KillDialog(Dialogs::DIALOG_ULTIMATE_CHOOSER);
		if (aBoard)
			aBoard->StartLearningQuizUltimate(aType);
		return;
	}

	// Click empty panel area to dismiss.
	mApp->KillDialog(Dialogs::DIALOG_ULTIMATE_CHOOSER);
}

void UltimateChooserDialog::KeyDown(KeyCode theKey)
{
	if (theKey == KEYCODE_ESCAPE)
	{
		mApp->KillDialog(Dialogs::DIALOG_ULTIMATE_CHOOSER);
		return;
	}
	LawnDialog::KeyDown(theKey);
}
