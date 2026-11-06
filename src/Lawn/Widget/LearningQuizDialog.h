/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 */

#ifndef __LEARNINGQUIZDIALOG_H__
#define __LEARNINGQUIZDIALOG_H__

#include "LawnDialog.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

class NewLawnButton;
namespace Sexy
{
	class MemoryImage;
	class Image;
}

enum class LearningQuizKind : int
{
	MathMixed,
	HanziToPinyin,
	PinyinToHanzi
};

enum class LearningQuizPurpose : int
{
	Plant,
	Ultimate
};

struct LearningQuizQuestion
{
	LearningQuizKind mKind = LearningQuizKind::MathMixed;
	std::string mTitle;
	std::string mPrompt;
	std::string mBigText;
	std::array<std::string, 4> mOptions{};
	int mCorrectIndex = 0;
};

class LearningQuizDialog : public LawnDialog
{
public:
	enum
	{
		Quiz_Option0 = 3000,
		Quiz_Option1 = 3001,
		Quiz_Option2 = 3002,
		Quiz_Option3 = 3003
	};

	std::array<std::unique_ptr<NewLawnButton>, 4> mOptionButtons;
	LearningQuizQuestion mQuestion;
	LearningQuizPurpose mPurpose = LearningQuizPurpose::Plant;
	std::string mFeedback;
	Color mFeedbackColor;
	int mFeedbackTimer = 0;
	bool mAnsweredCorrectly = false;
	bool mAnswerResolved = false;
	int mShakeWrongTimer = 0;
	int mAppearTimer = 0;
	int mResultFxTimer = 0;
	int mCloseDelay = 0;

	struct FxSpark
	{
		float x = 0.0f;
		float y = 0.0f;
		float vx = 0.0f;
		float vy = 0.0f;
		float life = 0.0f;
		float maxLife = 1.0f;
		int r = 255;
		int g = 200;
		int b = 80;
		float size = 6.0f;
	};
	std::vector<FxSpark> mFxSparks;

	std::unique_ptr<Sexy::MemoryImage> mTitleImage;
	std::unique_ptr<Sexy::MemoryImage> mPromptImage;
	std::unique_ptr<Sexy::MemoryImage> mBigTextImage;
	std::unique_ptr<Sexy::MemoryImage> mFeedbackImage;
	std::array<std::unique_ptr<Sexy::MemoryImage>, 4> mOptionLabelImages;
	Sexy::Image* mFxEmojiImage = nullptr;

public:
	LearningQuizDialog(LawnApp* theApp, LearningQuizPurpose thePurpose);
	~LearningQuizDialog() override;

	void					GenerateQuestion();
	void					RebuildTextImages();
	void					LayoutButtons();
	void					BeginResultFx(bool theCorrect);
	void					UpdateResultFx();
	void					DrawResultFx(Graphics* g);
	void					FinishAndClose();
	void					AddedToManager(WidgetManager* theWidgetManager) override;
	void					RemovedFromManager(WidgetManager* theWidgetManager) override;
	void					Resize(int theX, int theY, int theWidth, int theHeight) override;
	void					Update() override;
	void					Draw(Graphics* g) override;
	void					ButtonPress(int theId) override;
	void					ButtonDepress(int theId) override;
	void					KeyDown(KeyCode theKey) override;
};

LearningQuizQuestion GenerateRandomLearningQuiz();

#endif
