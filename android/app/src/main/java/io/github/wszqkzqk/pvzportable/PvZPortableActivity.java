/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

package io.github.wszqkzqk.pvzportable;

import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.RelativeLayout;
import android.window.OnBackInvokedCallback;
import android.window.OnBackInvokedDispatcher;

import org.libsdl.app.SDLActivity;

import java.io.File;

public class PvZPortableActivity extends SDLActivity {
    private static final String TAG = "PvZPortable";
    private OnBackInvokedCallback mBackCallback;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        File extDir = getExternalFilesDir(null);
        if (extDir != null && !extDir.exists()) extDir.mkdirs();
        Log.i(TAG, "Resource dir: " + (extDir != null ? extDir.getAbsolutePath() : "null"));

        if (!hasGameResources(extDir)) {
            super.onCreate(savedInstanceState);
            startActivity(new Intent(this, ResourceImportActivity.class));
            finish();
            return;
        }

        super.onCreate(savedInstanceState);
        hideSystemUI();
        setupImeLayoutAdjustment();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            mBackCallback = this::dispatchBackToSdl;
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                OnBackInvokedDispatcher.PRIORITY_DEFAULT,
                mBackCallback
            );
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) hideSystemUI();
    }

    @Override
    public void onBackPressed() {
        dispatchBackToSdl();
    }

    @Override
    protected void onDestroy() {
        if (mBackCallback != null) {
            getOnBackInvokedDispatcher().unregisterOnBackInvokedCallback(mBackCallback);
            mBackCallback = null;
        }
        super.onDestroy();
    }

    private void dispatchBackToSdl() {
        SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_BACK);
        SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_BACK);
    }

    private void hideSystemUI() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Window window = getWindow();
            window.setDecorFitsSystemWindows(false);
            WindowInsetsController controller = window.getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
                controller.setSystemBarsBehavior(
                    WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
        }
    }

    private void setupImeLayoutAdjustment() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING); // legacy adjustPan is inert with edge-to-edge on R+
            View content = findViewById(android.R.id.content);
            content.setOnApplyWindowInsetsListener((v, insets) -> {
                int shift = 0;
                if (insets.isVisible(WindowInsets.Type.ime()) && mTextEdit != null) {
                    int imeBottom = insets.getInsets(WindowInsets.Type.ime()).bottom;
                    View dummyEdit = mTextEdit; // DummyEdit is package-private, access via View
                    if (dummyEdit.getLayoutParams() instanceof RelativeLayout.LayoutParams) {
                        RelativeLayout.LayoutParams p = (RelativeLayout.LayoutParams) dummyEdit.getLayoutParams();
                        shift = Math.max(0, p.topMargin + p.height - (v.getHeight() - imeBottom));
                    }
                }
                mLayout.setTranslationY(-shift);
                return v.onApplyWindowInsets(insets);
            });
        } else {
            getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[]{
            "SDL2",
            "main"
        };
    }

    @Override
    public void setRequestedOrientation(int requestedOrientation) {
        if (requestedOrientation == android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE) {
            requestedOrientation = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_USER_LANDSCAPE;
        }
        super.setRequestedOrientation(requestedOrientation);
    }

    private static boolean hasGameResources(File dir) {
        if (dir == null || !dir.isDirectory()) return false;
        File pak = new File(dir, "main.pak");
        File props = new File(dir, "properties");
        return pak.exists() && props.isDirectory();
    }

    /**
     * Native ultimate button label. Returns [width, height, ARGB pixels...] or null.
     * Draws 「大招」 with system CJK fonts (Skia), matching the Windows candy-red style.
     */
    public static int[] createUltimateLabelImage() {
        try {
            final String text = "大招";
            Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.SUBPIXEL_TEXT_FLAG);
            paint.setTypeface(Typeface.create(Typeface.SANS_SERIF, Typeface.BOLD));
            paint.setTextSize(20f);
            paint.setTextAlign(Paint.Align.CENTER);
            paint.setFakeBoldText(true);

            Paint.FontMetrics fm = paint.getFontMetrics();
            final int pad = 6;
            final int width = Math.max(1, (int) Math.ceil(paint.measureText(text)) + pad * 2);
            final int height = Math.max(22, (int) Math.ceil(fm.bottom - fm.top) + pad);
            final float cx = width * 0.5f;
            final float cy = height * 0.5f - (fm.ascent + fm.descent) * 0.5f;

            Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);

            paint.setStyle(Paint.Style.FILL);
            paint.setColor(0xFF5A1414);
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    canvas.drawText(text, cx + dx, cy + dy, paint);
                }
            }
            paint.setColor(0xFFFFFFFF);
            canvas.drawText(text, cx, cy, paint);
            paint.setColor(0xFFFF4646);
            canvas.drawText(text, cx, cy - 1f, paint);

            int[] pixels = new int[width * height];
            bitmap.getPixels(pixels, 0, width, 0, 0, width, height);
            bitmap.recycle();

            for (int i = 0; i < pixels.length; ++i) {
                final int src = pixels[i];
                final int aR = (src >> 16) & 0xFF;
                final int aG = (src >> 8) & 0xFF;
                final int aB = src & 0xFF;
                final int coverage = Math.max(aR, Math.max(aG, aB));
                if (coverage == 0) {
                    pixels[i] = 0;
                    continue;
                }
                final boolean outline = aR < 140 && aG < 80 && aB < 80;
                final int outR = outline ? 70 : 255;
                final int outG = outline ? 18 : 55;
                final int outB = outline ? 18 : 55;
                pixels[i] = (coverage << 24) | (outR << 16) | (outG << 8) | outB;
            }

            int[] out = new int[2 + pixels.length];
            out[0] = width;
            out[1] = height;
            System.arraycopy(pixels, 0, out, 2, pixels.length);
            return out;
        } catch (Throwable t) {
            Log.e(TAG, "createUltimateLabelImage failed", t);
            return null;
        }
    }
}
