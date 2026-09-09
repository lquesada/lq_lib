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
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Shader;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

import java.util.Arrays;
import java.util.Locale;

public class SpectrumPeaksView extends View {

    public interface OnFrequencySelectedListener {
        void onFrequencySelected(float freqHz);
    }

    public static final float MIN_FREQ = 300.0f;
    public static final float MAX_FREQ = 2700.0f;
    private static final float MIN_DB = -75.0f;
    private static final float MAX_DB = -15.0f;

    private final Paint linePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint fillPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint peakPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint gridPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint txMarkerPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path curvePath = new Path();
    private final Path fillPath = new Path();

    private float[] currentSpectrum = new float[0];
    private float[] smoothedSpectrum = new float[0];
    private float[] peakHold = new float[0];
    private float peakFreq = 0.0f;
    private float peakDb = -99.0f;
    private float txFrequencyHz = 1500.0f;
    private OnFrequencySelectedListener freqListener;

    public SpectrumPeaksView(Context context) {
        super(context);
        init();
    }

    public SpectrumPeaksView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        linePaint.setColor(Color.rgb(0, 230, 255)); // Neon Cyan
        linePaint.setStyle(Paint.Style.STROKE);
        linePaint.setStrokeWidth(2.5f);

        fillPaint.setStyle(Paint.Style.FILL);

        peakPaint.setColor(Color.rgb(255, 100, 50)); // Orange Peak Hold
        peakPaint.setStyle(Paint.Style.STROKE);
        peakPaint.setStrokeWidth(1.8f);

        gridPaint.setColor(Color.argb(50, 255, 255, 255));
        gridPaint.setStyle(Paint.Style.STROKE);
        gridPaint.setStrokeWidth(1.0f);

        textPaint.setColor(Color.rgb(180, 200, 220));
        textPaint.setTextSize(22.0f);
        textPaint.setAntiAlias(true);

        txMarkerPaint.setColor(Color.rgb(255, 60, 60)); // Red TX Marker
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

    public void updateSpectrum(float[] powerDb, float binWidthHz) {
        if (powerDb == null || powerDb.length == 0) return;

        int minBin = Math.max(0, (int) Math.floor(MIN_FREQ / binWidthHz));
        int maxBin = Math.min(powerDb.length - 1, (int) Math.ceil(MAX_FREQ / binWidthHz));
        int count = maxBin - minBin + 1;
        if (count <= 1) return;

        if (currentSpectrum.length != count) {
            currentSpectrum = new float[count];
            smoothedSpectrum = new float[count];
            peakHold = new float[count];
            Arrays.fill(smoothedSpectrum, MIN_DB);
            Arrays.fill(peakHold, MIN_DB);
        }

        float maxVal = -999.0f;
        int maxIdx = 0;

        for (int i = 0; i < count; i++) {
            float val = powerDb[minBin + i];
            currentSpectrum[i] = val;

            // Exponential smoothing for silky spectrum decay
            smoothedSpectrum[i] = smoothedSpectrum[i] * 0.45f + val * 0.55f;

            // Peak hold with slow decay
            if (val > peakHold[i]) {
                peakHold[i] = val;
            } else {
                peakHold[i] -= 0.6f;
            }

            if (val > maxVal) {
                maxVal = val;
                maxIdx = i;
            }
        }

        peakFreq = MIN_FREQ + maxIdx * binWidthHz;
        peakDb = maxVal;

        postInvalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        int w = getWidth();
        int h = getHeight();
        if (w <= 0 || h <= 0) return;

        // Dark background
        canvas.drawColor(Color.rgb(12, 16, 24));

        // Draw dB Grid Lines (-60, -45, -30, -15 dB)
        for (int db = -60; db <= -15; db += 15) {
            float y = (1.0f - (db - MIN_DB) / (MAX_DB - MIN_DB)) * (h - 20.0f);
            canvas.drawLine(0, y, w, y, gridPaint);
            canvas.drawText(db + " dB", 8, y - 4, textPaint);
        }

        // Draw Frequency Grid Lines (500, 1000, 1500, 2000, 2500 Hz)
        int[] freqs = {500, 1000, 1500, 2000, 2500};
        for (int f : freqs) {
            float x = ((f - MIN_FREQ) / (MAX_FREQ - MIN_FREQ)) * w;
            canvas.drawLine(x, 0, x, h - 20, gridPaint);
            String label = f >= 1000 ? String.format(Locale.ROOT, "%.1fk", f / 1000.0f) : f + "";
            canvas.drawText(label, x - 18, h - 4, textPaint);
        }

        int numPoints = smoothedSpectrum.length;
        if (numPoints <= 1) return;

        // Build Spectrum Curve & Filled Area
        curvePath.reset();
        fillPath.reset();

        float baselineY = h - 20.0f;
        fillPath.moveTo(0, baselineY);

        for (int i = 0; i < numPoints; i++) {
            float x = (i / (float) (numPoints - 1)) * w;
            float norm = (smoothedSpectrum[i] - MIN_DB) / (MAX_DB - MIN_DB);
            norm = Math.max(0.0f, Math.min(1.0f, norm));
            norm = (float) Math.pow(norm, 0.80);
            float y = baselineY - norm * (baselineY - 10.0f);

            if (i == 0) {
                curvePath.moveTo(x, y);
                fillPath.lineTo(x, y);
            } else {
                curvePath.lineTo(x, y);
                fillPath.lineTo(x, y);
            }
        }

        fillPath.lineTo(w, baselineY);
        fillPath.close();

        // Shaded gradient under curve
        fillPaint.setShader(new LinearGradient(
                0, 0, 0, baselineY,
                Color.argb(120, 0, 200, 255),
                Color.argb(10, 0, 40, 80),
                Shader.TileMode.CLAMP
        ));
        canvas.drawPath(fillPath, fillPaint);
        canvas.drawPath(curvePath, linePaint);

        // Draw Peak Hold Line
        curvePath.reset();
        for (int i = 0; i < numPoints; i++) {
            float x = (i / (float) (numPoints - 1)) * w;
            float norm = (peakHold[i] - MIN_DB) / (MAX_DB - MIN_DB);
            norm = Math.max(0.0f, Math.min(1.0f, norm));
            norm = (float) Math.pow(norm, 0.80);
            float y = baselineY - norm * (baselineY - 10.0f);
            if (i == 0) curvePath.moveTo(x, y);
            else curvePath.lineTo(x, y);
        }
        canvas.drawPath(curvePath, peakPaint);

        // Draw TX Marker Line & Cursor
        float txX = ((txFrequencyHz - MIN_FREQ) / (MAX_FREQ - MIN_FREQ)) * w;
        canvas.drawLine(txX, 0, txX, baselineY, txMarkerPaint);
        textPaint.setColor(Color.rgb(255, 80, 80));
        canvas.drawText("▼", txX - 6, 16, textPaint);
        textPaint.setColor(Color.rgb(180, 200, 220));

        // Peak frequency text overlay
        if (peakDb > -70.0f) {
            String peakInfo = String.format(Locale.ROOT, "Peak: %.0f Hz (%.1f dB)", peakFreq, peakDb);
            textPaint.setColor(Color.rgb(0, 255, 180));
            canvas.drawText(peakInfo, w - 230, 24, textPaint);
            textPaint.setColor(Color.rgb(180, 200, 220));
        }
    }
}
