/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 */

#ifndef __ULTIMATECHOOSERDIALOG_H__
#define __ULTIMATECHOOSERDIALOG_H__

#include "LawnDialog.h"
#include "../../ConstEnums.h"
#include <memory>

class ToolTipWidget;

class UltimateChooserDialog : public LawnDialog
{
public:
	std::unique_ptr<ToolTipWidget> mToolTip;
	UltimateType mToolTipType = UltimateType::NUM_ULTIMATE_TYPES;

public:
	UltimateChooserDialog();
	~UltimateChooserDialog() override;

	UltimateType HitTest(int x, int y) const;
	void GetSlotPosition(int theIndex, int& x, int& y) const;
	static SeedType GetSeedIcon(UltimateType theType);
	void UpdateCursor();
	void Update() override;
	void Draw(Graphics* g) override;
	void ShowToolTip();
	void RemoveToolTip();
	void MouseDown(int x, int y, int theClickCount) override;
	void MouseUp(int, int, int) override {}
	void ButtonDepress(int theId) override;
};

#endif
