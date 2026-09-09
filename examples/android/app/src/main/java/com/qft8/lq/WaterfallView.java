/*
 * =============================================================================
 * The LQ Digital Mode Family — Reference Implementation (lq_lib)
 *
 * Author:  Luis Quesada (HB9IPH)
 * Web:     https://luisquesada.com
 * Portal:  https://lquesada.github.io/lq_lib/
 * GitHub:  https://github.com/lquesada/lq_lib
 * App:     qFT8 — Portable Amateur Radio for Android (https://qft8.com)
 *
 * License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)
 *
 * Copyright (c) 2026 Luis Quesada (HB9IPH)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * =============================================================================
 */

package com.qft8.lq;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

import java.util.Arrays;
import java.util.Locale;

public class WaterfallView extends View {

    public interface OnFrequencySelectedListener {
        void onFrequencySelected(float freqHz);
    }

    public static final float MIN_FREQ = 300.0f;
    public static final float MAX_FREQ = 2700.0f;
    // High-sensitivity dynamic range for microphone & SDR audio (-75 dB floor to -15 dB ceiling)
    private static final float MIN_DB = -75.0f;
    private static final float MAX_DB = -15.0f;

    private final Object lock = new Object();
    private Bitmap waterfallBitmap;
    private int[] pixelBuffer;
    private int bufferWidth = 0;
    private int bufferHeight = 0;
    private int headRow = 0;

    private final Paint gridPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint txMarkerPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    private float txFrequencyHz = 1500.0f;
    private OnFrequencySelectedListener freqListener;

    public WaterfallView(Context context) {
        super(context);
        init();
    }

    public WaterfallView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        gridPaint.setColor(Color.argb(50, 255, 255, 255));
        gridPaint.setStyle(Paint.Style.STROKE);
        gridPaint.setStrokeWidth(1.0f);

        textPaint.setColor(Color.argb(160, 220, 220, 220));
        textPaint.setTextSize(20.0f);
        textPaint.setAntiAlias(true);

        txMarkerPaint.setColor(Color.argb(200, 255, 60, 60)); // Red TX Marker
        txMarkerPaint.setStyle(Paint.Style.STROKE);
        txMarkerPaint.setStrokeWidth(2.0f);
    }

    public void setOnFrequencySelectedListener(OnFrequencySelectedListener listener) {
        this.freqListener = listener;
    }

    public void setTxFrequency(float freqHz) {
        this.txFrequencyHz = Math.max(MIN_FREQ, Math.min(MAX_FREQ, freqHz));
        postInvalidate();
    }

    public float getTxFrequency() {
        return txFrequencyHz;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN || event.getAction() == MotionEvent.ACTION_MOVE) {
            float x = event.getX();
            int w = getWidth();
            if (w > 0) {
                float freq = MIN_FREQ + (x / (float) w) * (MAX_FREQ - MIN_FREQ);
                freq = Math.round(freq / 10.0f) * 10.0f; // Round to nearest 10 Hz
                setTxFrequency(freq);
                if (freqListener != null) {
                    freqListener.onFrequencySelected(txFrequencyHz);
                }
                return true;
            }
        }
        return super.onTouchEvent(event);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        if (w > 0 && h > 0) {
            synchronized (lock) {
                bufferWidth = w;
                bufferHeight = h;
                waterfallBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
                pixelBuffer = new int[w * h];
                Arrays.fill(pixelBuffer, Color.rgb(6, 10, 18));
                headRow = 0;
            }
        }
    }

    /**
     * Appends a new instantaneous power spectrum row to the waterfall.
     * The new row enters at the top (y = 0) and older rows drip down continuously.
     */
    public void addSpectrumRow(float[] powerDb, float binWidthHz) {
        if (powerDb == null || powerDb.length == 0 || binWidthHz <= 0.0f) {
            return;
        }

        synchronized (lock) {
            if (pixelBuffer == null || bufferWidth <= 0 || bufferHeight <= 0) {
                return;
            }

            // Move circular head pointer up by 1 row (newest row at top)
            headRow = (headRow - 1 + bufferHeight) % bufferHeight;

            int rowOffset = headRow * bufferWidth;
            float freqRange = MAX_FREQ - MIN_FREQ;
            int maxBin = powerDb.length - 1;

            for (int x = 0; x < bufferWidth; x++) {
                float freq = MIN_FREQ + (x / (float) bufferWidth) * freqRange;
                int bin = Math.min(maxBin, Math.max(0, Math.round(freq / binWidthHz)));
                float db = powerDb[bin];
                pixelBuffer[rowOffset + x] = colorMap(db);
            }
        }

        postInvalidate();
    }

    /**
     * Maps dB power levels (-75 dB to -15 dB) to ultra high-contrast, vivid SDR colors.
     */
    private static int colorMap(float db) {
        // Normalize [-75 dB, -15 dB] -> [0.0, 1.0]
        float norm = (db - MIN_DB) / (MAX_DB - MIN_DB);
        norm = Math.max(0.0f, Math.min(1.0f, norm));

        // Mild gamma curve (0.80) to dramatically increase contrast and visibility of weak/mid audio signals
        norm = (float) Math.pow(norm, 0.80);

        // Ultra High-Contrast Palette:
        // Deep Navy -> Vivid Royal Blue -> Electric Cyan -> Emerald Green -> Bright Yellow -> Fiery Orange -> Crimson Red -> Incandescent White
        if (norm < 0.12f) {
            // Dark Navy to Blue floor
            float t = norm / 0.12f;
            return Color.rgb((int) (6 * (1.0f - t)), (int) (10 * (1.0f - t)), (int) (18 + t * 110));
        } else if (norm < 0.28f) {
            // Royal Blue to Electric Cyan
            float t = (norm - 0.12f) / 0.16f;
            return Color.rgb(0, (int) (t * 220), (int) (128 + t * 127));
        } else if (norm < 0.48f) {
            // Electric Cyan to Emerald Green
            float t = (norm - 0.28f) / 0.20f;
            return Color.rgb(0, (int) (220 + t * 35), (int) (255 * (1.0f - t)));
        } else if (norm < 0.68f) {
            // Emerald Green to Vivid Yellow
            float t = (norm - 0.48f) / 0.20f;
            return Color.rgb((int) (t * 255), 255, 0);
        } else if (norm < 0.85f) {
            // Vivid Yellow to Fiery Orange
            float t = (norm - 0.68f) / 0.17f;
            return Color.rgb(255, (int) (255 - t * 145), 0);
        } else if (norm < 0.95f) {
            // Fiery Orange to Crimson Red
            float t = (norm - 0.85f) / 0.10f;
            return Color.rgb(255, (int) (110 * (1.0f - t)), (int) (20 * t));
        } else {
            // Crimson Red to Incandescent White
            float t = (norm - 0.95f) / 0.05f;
            return Color.rgb(255, (int) (t * 255), (int) (20 + t * 235));
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        int w = getWidth();
        int h = getHeight();
        if (w <= 0 || h <= 0) return;

        synchronized (lock) {
            if (waterfallBitmap != null && pixelBuffer != null && bufferWidth == w && bufferHeight == h) {
                // Transfer circular buffer into the bitmap with newest rows at the top (y = 0)
                int topCount = bufferHeight - headRow;
                waterfallBitmap.setPixels(pixelBuffer, headRow * bufferWidth, bufferWidth, 0, 0, bufferWidth, topCount);
                if (headRow > 0) {
                    waterfallBitmap.setPixels(pixelBuffer, 0, bufferWidth, 0, topCount, bufferWidth, headRow);
                }
                canvas.drawBitmap(waterfallBitmap, 0, 0, null);
            }
        }

        // Frequency Grid Overlay
        int[] freqs = {500, 1000, 1500, 2000, 2500};
        for (int f : freqs) {
            float x = ((f - MIN_FREQ) / (MAX_FREQ - MIN_FREQ)) * w;
            canvas.drawLine(x, 0, x, h, gridPaint);
            String label = f >= 1000 ? String.format(Locale.ROOT, "%.1fk", f / 1000.0f) : f + "";
            canvas.drawText(label, x - 18, h - 6, textPaint);
        }

        // Draw TX Marker Line
        float txX = ((txFrequencyHz - MIN_FREQ) / (MAX_FREQ - MIN_FREQ)) * w;
        canvas.drawLine(txX, 0, txX, h, txMarkerPaint);
    }
}
