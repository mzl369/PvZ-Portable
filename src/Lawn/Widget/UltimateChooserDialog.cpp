/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 */

#include "UltimateChooserDialog.h"
#include "../Plant.h"
#include "GameButton.h"
#include "../SeedPacket.h"
#include "../../LawnApp.h"
#include "../Board.h"
#include "../ToolTipWidget.h"
#include "../../GameConstants.h"
#include "../../Resources.h"
#include "widget/WidgetManager.h"

namespace
{
	constexpr int CHOOSER_WIDTH = 360;
	constexpr int CHOOSER_HEIGHT = 420;
	constexpr int SLOT_COLS = 3;
	constexpr int SLOT_GAP_X = 12;
	constexpr int SLOT_GAP_Y = 14;
	constexpr int SLOT_TOP = 100;
}

UltimateChooserDialog::UltimateChooserDialog() :
	LawnDialog(
		gLawnApp,
		Dialogs::DIALOG_ULTIMATE_CHOOSER,
		true,
		"[CHOOSE_YOUR_PLANT]",
		"",
		"[DIALOG_BUTTON_CANCEL]",
		Dialog::BUTTONS_FOOTER)
{
	CalcSize(CHOOSER_WIDTH - mWidth, CHOOSER_HEIGHT - mHeight);
	mToolTip = std::make_unique<ToolTipWidget>();
	mClip = false;
	mToolTipType = UltimateType::NUM_ULTIMATE_TYPES;
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
	y = SLOT_TOP + (theIndex / SLOT_COLS) * (SEED_PACKET_HEIGHT + SLOT_GAP_Y);
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
	mApp->SetCursor(aType != UltimateType::NUM_ULTIMATE_TYPES ? CURSOR_HAND : CURSOR_POINTER);
}

void UltimateChooserDialog::Update()
{
	LawnDialog::Update();
	ShowToolTip();
	UpdateCursor();
}

void UltimateChooserDialog::Draw(Graphics* g)
{
	LawnDialog::Draw(g);
	g->SetLinearBlend(true);

	const int aCount = static_cast<int>(UltimateType::NUM_ULTIMATE_TYPES);
	for (int i = 0; i < aCount; ++i)
	{
		int aX, aY;
		GetSlotPosition(i, aX, aY);
		const UltimateType aType = static_cast<UltimateType>(i);
		DrawSeedPacket(g, static_cast<float>(aX), static_cast<float>(aY), GetSeedIcon(aType), SeedType::SEED_NONE, 0.0f, 255, false, false);

		if (aType == mToolTipType)
		{
			g->SetColor(Color(255, 255, 180, 70));
			g->FillRect(aX, aY, SEED_PACKET_WIDTH, SEED_PACKET_HEIGHT);
		}
	}

	mToolTip->Draw(g);
}

void UltimateChooserDialog::ShowToolTip()
{
	if (!mApp->mWidgetManager->mMouseIn || !mApp->mActive)
	{
		RemoveToolTip();
		return;
	}

	const UltimateType aType = HitTest(mApp->mWidgetManager->mLastMouseX - mX, mApp->mWidgetManager->mLastMouseY - mY);
	if (aType == UltimateType::NUM_ULTIMATE_TYPES)
	{
		RemoveToolTip();
		return;
	}

	if (aType != mToolTipType)
	{
		RemoveToolTip();
		const SeedType aSeed = GetSeedIcon(aType);
		mToolTip->SetTitle(Plant::GetNameString(aSeed, SeedType::SEED_NONE));
		mToolTip->SetLabel("");
		mToolTip->SetWarningText("");

		int aSeedX, aSeedY;
		GetSlotPosition(static_cast<int>(aType), aSeedX, aSeedY);
		mToolTip->mX = (SEED_PACKET_WIDTH - mToolTip->mWidth) / 2 + aSeedX;
		mToolTip->mY = SEED_PACKET_HEIGHT + aSeedY;
		mToolTip->mVisible = true;
		mToolTipType = aType;
	}
}

void UltimateChooserDialog::RemoveToolTip()
{
	mToolTip->mVisible = false;
	mToolTipType = UltimateType::NUM_ULTIMATE_TYPES;
}

void UltimateChooserDialog::MouseDown(int x, int y, int theClickCount)
{
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

	Widget::MouseDown(x, y, theClickCount);
}

void UltimateChooserDialog::ButtonDepress(int theId)
{
	if (theId == Dialog::ID_YES || theId == Dialog::ID_NO || theId == 1000 || theId == 1001)
	{
		mApp->KillDialog(Dialogs::DIALOG_ULTIMATE_CHOOSER);
		return;
	}
	LawnDialog::ButtonDepress(theId);
}
