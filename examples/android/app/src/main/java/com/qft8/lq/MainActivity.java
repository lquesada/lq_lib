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

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Random;

public class MainActivity extends AppCompatActivity implements AudioEngine.AudioListener {

    private static final int PERMISSION_REQ_RECORD_AUDIO = 101;

    private SpectrumPeaksView spectrumPeaksView;
    private WaterfallView waterfallView;
    private Button btnModeLq8, btnModeLq4, btnModeLq2, btnModeLq16;
    private EditText editMyCall, editMyGrid, editTargetCall;
    private Button btnCallCq, btnCallTarget, btnToggleAuto;
    private TextView txtStatus, txtLog;
    private ScrollView scrollViewLog;

    private volatile String cachedMyCall = "";
    private volatile String cachedMyGrid = "JN47";
    private volatile String cachedTargetCall = "";

    private final AudioEngine audioEngine = new AudioEngine();
    private LqProtocol.Mode currentMode = LqProtocol.Mode.LQ8;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final SimpleDateFormat timeFormat = new SimpleDateFormat("HH:mm:ss", Locale.ROOT);

    // Auto-Responder state (Enabled by default)
    private boolean autoEnabled = true;
    private long lastAutoTxTimeMs = 0;
    private String lastAutoHandledSig = "";

    // Fast FFT & Spectrum processing
    private static final int FFT_SIZE = 1024;
    private final FastFft fastFft = new FastFft(FFT_SIZE);
    private final float[] fftInput = new float[FFT_SIZE];
    private final float[] fftReal = new float[FFT_SIZE];
    private final float[] fftImag = new float[FFT_SIZE];
    private final float[] fftPowerDb = new float[FFT_SIZE / 2];
    private int fftInputPos = 0;
    private final float binWidthHz = (float) AudioEngine.SAMPLE_RATE / (float) FFT_SIZE;

    private String lastDecodedText = "";
    private long lastDecodedTimeMs = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Apply system window insets so content avoids status bar and navigation bar
        View rootLayout = findViewById(R.id.rootLayout);
        if (rootLayout != null) {
            final int padLeft = rootLayout.getPaddingLeft();
            final int padTop = rootLayout.getPaddingTop();
            final int padRight = rootLayout.getPaddingRight();
            final int padBottom = rootLayout.getPaddingBottom();

            ViewCompat.setOnApplyWindowInsetsListener(rootLayout, (v, insets) -> {
                Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
                v.setPadding(
                    padLeft + systemBars.left,
                    padTop + systemBars.top,
                    padRight + systemBars.right,
                    padBottom + systemBars.bottom
                );
                return insets;
            });
        }

        spectrumPeaksView = findViewById(R.id.spectrumPeaksView);
        waterfallView = findViewById(R.id.waterfallView);
        if (spectrumPeaksView != null) {
            spectrumPeaksView.setTxFrequency(LqEngine.DEFAULT_BASE_FREQ);
            spectrumPeaksView.setOnFrequencySelectedListener(this::onFrequencyChanged);
        }
        if (waterfallView != null) {
            waterfallView.setTxFrequency(LqEngine.DEFAULT_BASE_FREQ);
            waterfallView.setOnFrequencySelectedListener(this::onFrequencyChanged);
        }

        btnModeLq8 = findViewById(R.id.btnModeLq8);
        btnModeLq4 = findViewById(R.id.btnModeLq4);
        btnModeLq2 = findViewById(R.id.btnModeLq2);
        btnModeLq16 = findViewById(R.id.btnModeLq16);


        editMyCall = findViewById(R.id.editMyCall);
        editMyGrid = findViewById(R.id.editMyGrid);
        editTargetCall = findViewById(R.id.editTargetCall);

        btnCallCq = findViewById(R.id.btnCallCq);
        btnCallTarget = findViewById(R.id.btnCallTarget);
        btnToggleAuto = findViewById(R.id.btnToggleAuto);

        txtStatus = findViewById(R.id.txtStatus);
        txtLog = findViewById(R.id.txtLog);
        scrollViewLog = findViewById(R.id.scrollViewLog);

        setupModeButtons();
        setupActionButtons();
        setupStationDataWatchers();

        generateRandomStationData();

        audioEngine.setListener(this);
        checkAudioPermission();
    }

    private void setupStationDataWatchers() {
        android.text.TextWatcher watcher = new android.text.TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                cachedMyCall = editMyCall.getText().toString().trim().toUpperCase(Locale.ROOT);
                cachedMyGrid = editMyGrid.getText().toString().trim().toUpperCase(Locale.ROOT);
                cachedTargetCall = editTargetCall.getText().toString().trim().toUpperCase(Locale.ROOT);
            }
            @Override
            public void afterTextChanged(android.text.Editable s) {}
        };
        editMyCall.addTextChangedListener(watcher);
        editMyGrid.addTextChangedListener(watcher);
        editTargetCall.addTextChangedListener(watcher);
    }

    private static final String[] CALL_PREFIXES = {
        "K", "W", "N", "AA", "AI", "DL", "DJ", "DK", "EA", "EB", "EC", "F", "G", "GM", "GW",
        "HB", "HA", "I", "IK", "JA", "JH", "JR", "LA", "LU", "OE", "OH", "OK", "OM", "ON",
        "OZ", "PA", "PD", "PY", "S5", "SM", "SP", "SQ", "SV", "UR", "US", "VE", "VA", "VK",
        "VR", "VU", "XE", "YO", "YU", "ZL", "ZS"
    };

    public static String generateRandomCallsign(Random random) {
        String prefix = CALL_PREFIXES[random.nextInt(CALL_PREFIXES.length)];
        int digit = random.nextInt(10);
        int suffixLen = 1 + random.nextInt(3); // 1, 2, or 3 letters
        StringBuilder suffix = new StringBuilder();
        for (int i = 0; i < suffixLen; i++) {
            suffix.append((char) ('A' + random.nextInt(26)));
        }
        return prefix + digit + suffix;
    }

    public static String generateRandomGrid(Random random) {
        char f1 = (char) ('A' + random.nextInt(18)); // A..R
        char f2 = (char) ('A' + random.nextInt(18)); // A..R
        int s1 = random.nextInt(10); // 0..9
        int s2 = random.nextInt(10); // 0..9
        return "" + f1 + f2 + s1 + s2;
    }

    private void generateRandomStationData() {
        Random random = new Random();
        String randomCall = generateRandomCallsign(random);
        String randomGrid = generateRandomGrid(random);

        cachedMyCall = randomCall;
        cachedMyGrid = randomGrid;
        cachedTargetCall = "";

        editMyCall.setText(randomCall);
        editMyGrid.setText(randomGrid);
        editTargetCall.setText(""); // Empty by default; listening to a CQ fills it in
    }

    private void setupModeButtons() {
        btnModeLq8.setOnClickListener(v -> selectStandardMode(LqProtocol.Mode.LQ8));
        btnModeLq4.setOnClickListener(v -> selectStandardMode(LqProtocol.Mode.LQ4));
        btnModeLq2.setOnClickListener(v -> selectStandardMode(LqProtocol.Mode.LQ2));
        if (btnModeLq16 != null) {
            btnModeLq16.setOnClickListener(v -> selectStandardMode(LqProtocol.Mode.LQ16));
        }
        updateModeButtonUi();
    }

    private void selectStandardMode(LqProtocol.Mode mode) {
        currentMode = mode;
        updateModeButtonUi();
        logMessage("Mode switched to: " + mode.name + " (" + mode.slotDuration + "s slot, " + mode.numTones + "-GFSK, " + String.format(Locale.ROOT, "%.1fms/sym", mode.symbolPeriod * 1000.0f) + ")");
    }

    private void updateModeButtonUi() {
        int primaryColor = ContextCompat.getColor(this, R.color.primary);
        int surfaceColor = ContextCompat.getColor(this, R.color.surface_dark);

        btnModeLq8.setBackgroundColor(currentMode == LqProtocol.Mode.LQ8 ? primaryColor : surfaceColor);
        btnModeLq4.setBackgroundColor(currentMode == LqProtocol.Mode.LQ4 ? primaryColor : surfaceColor);
        btnModeLq2.setBackgroundColor(currentMode == LqProtocol.Mode.LQ2 ? primaryColor : surfaceColor);
        if (btnModeLq16 != null) {
            btnModeLq16.setBackgroundColor(currentMode == LqProtocol.Mode.LQ16 ? primaryColor : surfaceColor);
            btnModeLq16.setTextColor(Color.WHITE);
        }

        btnModeLq8.setTextColor(Color.WHITE);
        btnModeLq4.setTextColor(Color.WHITE);
        btnModeLq2.setTextColor(Color.WHITE);

        updateStatusText();
    }

    public float getSelectedTxFrequency() {
        if (waterfallView != null) {
            return waterfallView.getTxFrequency();
        }
        if (spectrumPeaksView != null) {
            return spectrumPeaksView.getTxFrequency();
        }
        return LqEngine.DEFAULT_BASE_FREQ;
    }

    private void onFrequencyChanged(float freqHz) {
        if (waterfallView != null && Math.abs(waterfallView.getTxFrequency() - freqHz) > 0.5f) {
            waterfallView.setTxFrequency(freqHz);
        }
        if (spectrumPeaksView != null && Math.abs(spectrumPeaksView.getTxFrequency() - freqHz) > 0.5f) {
            spectrumPeaksView.setTxFrequency(freqHz);
        }
        updateStatusText();
    }

    private float getDecodeWindowSeconds() {
        switch (currentMode) {
            case LQ8:  return 24.0f;
            case LQ4:  return 12.0f;
            case LQ2:  return 8.0f;
            case LQ16: return 32.0f;
            default:   return currentMode.decodeWindowSeconds;
        }
    }

    private void updateStatusText() {
        if (txtStatus != null) {
            float windowSec = getDecodeWindowSeconds();
            txtStatus.setText(String.format(Locale.ROOT, "Listening (%s, %d Hz, %.0fs window)...",
                    currentMode.name, Math.round(getSelectedTxFrequency()), windowSec));
        }
    }


    private void hideKeyboard() {
        View view = getCurrentFocus();
        if (view == null) {
            view = findViewById(R.id.rootLayout);
        }
        if (view != null) {
            android.view.inputmethod.InputMethodManager imm =
                (android.view.inputmethod.InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
            if (imm != null) {
                imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
            }
            view.clearFocus();
        }
    }

    private void setupActionButtons() {
        btnCallCq.setOnClickListener(v -> {
            hideKeyboard();
            transmitCq();
        });
        btnCallTarget.setOnClickListener(v -> {
            hideKeyboard();
            transmitCallTarget();
        });
        btnToggleAuto.setOnClickListener(v -> {
            hideKeyboard();
            toggleAutoMode();
        });
        updateAutoButtonUi();
    }


    private void toggleAutoMode() {
        autoEnabled = !autoEnabled;
        updateAutoButtonUi();
        logMessage("AUTO Mode: " + (autoEnabled ? "ENABLED (Automatic QSO responder active)" : "DISABLED"));
    }

    private void updateAutoButtonUi() {
        int rxColor = ContextCompat.getColor(this, R.color.rx_color);
        int surfaceColor = ContextCompat.getColor(this, R.color.surface_dark);

        btnToggleAuto.setText(autoEnabled ? "AUTO: ON" : "AUTO: OFF");
        androidx.core.view.ViewCompat.setBackgroundTintList(btnToggleAuto, android.content.res.ColorStateList.valueOf(autoEnabled ? rxColor : surfaceColor));
        btnToggleAuto.setTextColor(Color.WHITE);
    }

    private void transmitCq() {
        hideKeyboard();
        if (audioEngine.isTransmitting()) {
            audioEngine.stopPlayback();
            logMessage("Tx Aborted.", Color.parseColor("#FFAB40"));
            return;
        }

        String call = cachedMyCall;
        String grid = cachedMyGrid;
        if (call.isEmpty()) {
            Toast.makeText(this, "Enter your callsign", Toast.LENGTH_SHORT).show();
            return;
        }

        LqMessage msg = new LqMessage();
        msg.type = LqMessage.Type.CQ_STD;
        msg.call1 = call;
        msg.locator = grid;

        transmitMessage(msg);
    }

    private void transmitCallTarget() {
        hideKeyboard();
        if (audioEngine.isTransmitting()) {
            audioEngine.stopPlayback();
            logMessage("Tx Aborted.", Color.parseColor("#FFAB40"));
            return;
        }

        String myCall = cachedMyCall;
        String myGrid = cachedMyGrid;
        String targetCall = cachedTargetCall;

        if (myCall.isEmpty()) {
            Toast.makeText(this, "Enter your callsign", Toast.LENGTH_SHORT).show();
            return;
        }
        if (targetCall.isEmpty()) {
            Toast.makeText(this, "Target callsign is empty (listen for CQ or enter one)", Toast.LENGTH_SHORT).show();
            return;
        }
        if (myGrid.isEmpty()) myGrid = "JN47";

        LqMessage msg = new LqMessage();
        if (LqCodec.isStandardCallsign(targetCall) && LqCodec.isStandardCallsign(myCall)) {
            msg.type = LqMessage.Type.CALL_STD_NOSUF;
            msg.call1 = targetCall;
            msg.call2 = myCall;
            msg.locator = myGrid;
            msg.rstDb = -3;
        } else {
            msg.type = LqMessage.Type.CALL_NONSTD;
            msg.hash1 = LqCodec.hashCallsign(targetCall);
            msg.call2 = myCall;
            msg.locator = myGrid;
            msg.rstDb = -3;
        }

        transmitMessage(msg);
    }

    private String lastTransmittedMsgFormatted = "";
    private long lastTransmittedTimeMs = 0;

    private void transmitMessage(LqMessage msg) {
        int[] tones = LqEngine.encodeMessageToTones(msg, currentMode);
        if (tones == null) {
            logMessage("Error: Failed to encode message.", Color.RED);
            return;
        }

        float txFreq = getSelectedTxFrequency();
        float[] audio = LqEngine.generateAudio(tones, currentMode, txFreq, AudioEngine.SAMPLE_RATE);
        String formatted = msg.format();
        lastTransmittedMsgFormatted = formatted;
        lastTransmittedTimeMs = System.currentTimeMillis();
        lastAutoTxTimeMs = lastTransmittedTimeMs;

        logMessage("Tx: " + formatted + " (" + Math.round(txFreq) + " Hz)", Color.parseColor("#FFD54F"));
        audioEngine.playAudio(audio);
    }

    private void logMessage(String msg) {
        logMessage(msg, Color.WHITE);
    }

    private void logMessage(String msg, int color) {
        mainHandler.post(() -> {
            String time = timeFormat.format(new Date());
            String fullLine = "[" + time + "] " + msg + "\n";
            SpannableString span = new SpannableString(fullLine);
            span.setSpan(new ForegroundColorSpan(color), 0, fullLine.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            if (color == Color.RED || color == Color.parseColor("#FF5252")) {
                span.setSpan(new StyleSpan(Typeface.BOLD), 0, fullLine.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }
            txtLog.append(span);
            scrollViewLog.post(() -> scrollViewLog.fullScroll(ScrollView.FOCUS_DOWN));
        });
    }

    // -----------------------------------------------------------------------
    // Automated QSO Protocol State Machine Handler
    // -----------------------------------------------------------------------

    private void handleAutoResponder(LqMessage decoded) {
        if (decoded == null) return;

        String myCall = cachedMyCall;
        String myGrid = cachedMyGrid.isEmpty() ? "JN47" : cachedMyGrid;
        if (myCall.isEmpty()) return;

        int myHash = LqCodec.hashCallsign(myCall) & 0xFFFFFF;
        long now = System.currentTimeMillis();

        switch (decoded.type) {
            case CQ_STD:
            case CQ_NONSTD_1:
            case CQ_NONSTD_2:
            case CQ_NONSTD_3: {
                // Listening to a CQ fills in Target Callsign
                String cqStation = decoded.call1 != null ? decoded.call1.trim().toUpperCase(Locale.ROOT) : "";
                if (cqStation.isEmpty() || cqStation.equalsIgnoreCase(myCall)) {
                    return; // Don't target our own CQ
                }

                cachedTargetCall = cqStation;
                mainHandler.post(() -> editTargetCall.setText(cqStation));

                if (!autoEnabled) return;

                // Prevent fast re-triggering during transmission duration
                if (now - lastAutoTxTimeMs < Math.round(currentMode.txDuration * 1000) + 600) {
                    return;
                }

                String msgSig = decoded.type.name() + ":" + cqStation;
                if (msgSig.equals(lastAutoHandledSig) && (now - lastAutoTxTimeMs < 8000)) {
                    return;
                }
                lastAutoHandledSig = msgSig;

                // Step 1: Auto-reply with CALL
                LqMessage replyCall = new LqMessage();
                if (LqCodec.isStandardCallsign(cqStation) && LqCodec.isStandardCallsign(myCall)) {
                    replyCall.type = LqMessage.Type.CALL_STD_NOSUF;
                    replyCall.call1 = cqStation;
                    replyCall.call2 = myCall;
                    replyCall.locator = myGrid;
                    replyCall.rstDb = -3;
                } else {
                    replyCall.type = LqMessage.Type.CALL_NONSTD;
                    replyCall.hash1 = LqCodec.hashCallsign(cqStation);
                    replyCall.call2 = myCall;
                    replyCall.locator = myGrid;
                    replyCall.rstDb = -3;
                }

                transmitMessage(replyCall);
                break;
            }

            case CALL_STD_NOSUF:
            case CALL_STD_SUF:
            case CALL_NONSTD: {
                // Step 2: Station answered our CQ with CALL -> Reply with REPLY73
                boolean addressedToMe = false;
                if (decoded.call1 != null && decoded.call1.equalsIgnoreCase(myCall)) {
                    addressedToMe = true;
                } else if ((decoded.hash1 & 0xFFFFFF) == myHash && myHash != 0) {
                    boolean myStd = LqCodec.isStandardCallsign(myCall);
                    boolean senderStd = decoded.call2 != null && LqCodec.isStandardCallsign(decoded.call2);
                    if (!(myStd && senderStd)) {
                        addressedToMe = true;
                    }
                }

                if (!addressedToMe) return;

                String callerStation = decoded.call2 != null ? decoded.call2.trim().toUpperCase(Locale.ROOT) : "";
                if (callerStation.isEmpty() || callerStation.equalsIgnoreCase(myCall)) {
                    return;
                }

                cachedTargetCall = callerStation;
                mainHandler.post(() -> editTargetCall.setText(callerStation));

                if (!autoEnabled) return;

                if (now - lastAutoTxTimeMs < Math.round(currentMode.txDuration * 1000) + 600) {
                    return;
                }

                String msgSig = decoded.type.name() + ":" + callerStation;
                if (msgSig.equals(lastAutoHandledSig) && (now - lastAutoTxTimeMs < 8000)) {
                    return;
                }
                lastAutoHandledSig = msgSig;

                LqMessage reply73 = new LqMessage();
                if (LqCodec.isStandardCallsign(callerStation) && LqCodec.isStandardCallsign(myCall)) {
                    reply73.type = LqMessage.Type.REPORT73_STD;
                    reply73.call1 = callerStation;
                    reply73.call2 = myCall;
                    reply73.rstDb = +5;
                } else {
                    reply73.type = LqMessage.Type.MULTI_REPORT73;
                    reply73.hash1 = LqCodec.hashCallsign(callerStation);
                    reply73.call2 = myCall;
                    reply73.rstDb = +5;
                }

                transmitMessage(reply73);

                // For the CQing station, sending REPLY73 concludes the confirmed 3-step QSO!
                logMessage("QSO complete " + callerStation + " " + myCall, Color.parseColor("#FF5252"));
                break;
            }

            case REPORT73_STD:
            case M73_STD:
            case M73_NONSTD:
            case MULTI_REPORT73:
            case MULTI_73: {
                // Step 3: Originating station sent REPLY73 to us -> QSO Complete for answering station!
                boolean addressedToMe = false;
                if (decoded.call1 != null && decoded.call1.equalsIgnoreCase(myCall)) {
                    addressedToMe = true;
                } else if ((decoded.hash1 & 0xFFFFFF) == myHash && myHash != 0) {
                    boolean myStd = LqCodec.isStandardCallsign(myCall);
                    boolean senderStd = decoded.call2 != null && LqCodec.isStandardCallsign(decoded.call2);
                    if (!(myStd && senderStd)) {
                        addressedToMe = true;
                    }
                }

                if (!addressedToMe) return;

                String originatingStation = decoded.call2 != null ? decoded.call2.trim().toUpperCase(Locale.ROOT) : "";
                if (originatingStation.isEmpty()) {
                    originatingStation = cachedTargetCall;
                }

                if (!originatingStation.isEmpty()) {
                    // For the answering station, receiving REPLY73 concludes the confirmed 3-step QSO!
                    logMessage("QSO complete " + originatingStation + " " + myCall, Color.parseColor("#FF5252"));
                }
                break;
            }

            case FREE_TEXT: {
                if (decoded.text != null && !decoded.text.isEmpty()) {
                    logMessage("Rx Text: " + decoded.text, Color.parseColor("#00E676"));
                }
                break;
            }


            default:
                break;
        }
    }

    // -----------------------------------------------------------------------
    // AudioEngine.AudioListener callbacks
    // -----------------------------------------------------------------------
    private final java.util.concurrent.ExecutorService decodeExecutor = java.util.concurrent.Executors.newSingleThreadExecutor();
    private final java.util.concurrent.atomic.AtomicBoolean isDecoding = new java.util.concurrent.atomic.AtomicBoolean(false);

    private boolean isSelfMessage(LqMessage decoded, String myCall) {
        if (decoded == null) return false;
        long now = System.currentTimeMillis();

        // If this device recently transmitted this exact message, treat as self echo
        long txDurMs = Math.round(currentMode.txDuration * 1000L);
        if (now - lastTransmittedTimeMs < txDurMs + 4000L) {
            String formatted = decoded.format();
            if (formatted.equals(lastTransmittedMsgFormatted)) {
                return true;
            }
        }

        // Check if message originates from myCall
        if (myCall != null && !myCall.isEmpty()) {
            if (decoded.call1 != null && decoded.call1.equalsIgnoreCase(myCall)) {
                return true;
            }
            if (decoded.call2 != null && decoded.call2.equalsIgnoreCase(myCall)) {
                return true;
            }
        }

        return false;
    }

    @Override
    public void onAudioChunk(float[] chunk, int length) {
        for (int i = 0; i < length; i++) {
            fftInput[fftInputPos++] = chunk[i];
            if (fftInputPos >= FFT_SIZE) {
                fastFft.computeSpectrum(fftInput, fftReal, fftImag, fftPowerDb);

                if (spectrumPeaksView != null) {
                    spectrumPeaksView.updateSpectrum(fftPowerDb, binWidthHz);
                }
                if (waterfallView != null) {
                    waterfallView.addSpectrumRow(fftPowerDb, binWidthHz);
                }

                // 50% overlap: shift back by 512 samples
                System.arraycopy(fftInput, 512, fftInput, 0, 512);
                fftInputPos = 512;
            }
        }
    }

    @Override
    public void onPeriodicDecodeCheck(float[] ringBuffer, int writePos) {
        // Do not decode while actively transmitting audio
        if (audioEngine.isTransmitting()) return;

        // Only allow one decode task to run at a time to prevent backlog
        if (!isDecoding.compareAndSet(false, true)) return;

        decodeExecutor.execute(() -> {
            try {
                // Generous search window in samples for current mode to capture recent bursts
                float windowSecs = getDecodeWindowSeconds();
                int windowSamples = Math.round(windowSecs * AudioEngine.SAMPLE_RATE);
                float[] samples = audioEngine.getRecentSamples(windowSamples);

                float activeFreq = getSelectedTxFrequency();

                Log.d("LqDecode", "onPeriodicDecodeCheck: mode=" + currentMode.name + ", freq=" + activeFreq + ", window=" + windowSecs + "s, samples=" + samples.length);

                // Priority 1: High-speed targeted decode on active channel frequency (+/- 60 Hz)
                LqMessage decoded = LqEngine.attemptDecodeFromAudio(samples, 0, activeFreq, AudioEngine.SAMPLE_RATE, currentMode);
                if (decoded != null) {
                    Log.d("LqDecode", "Priority 1 decoded: " + decoded.format());
                } else {
                    // Priority 2: Wideband passband search if not detected at activeFreq
                    decoded = LqEngine.attemptDecodeFromAudio(samples, 0, 0.0f, AudioEngine.SAMPLE_RATE, currentMode);
                    if (decoded != null) {
                        Log.d("LqDecode", "Priority 2 decoded: " + decoded.format());
                    }
                }

                if (decoded != null) {
                    String myCall = cachedMyCall;
                    boolean isSelf = isSelfMessage(decoded, myCall);

                    String formatted = decoded.format();
                    long now = System.currentTimeMillis();
                    long dedupeWindow = Math.max(2000L, Math.round(currentMode.txDuration * 1000L) - 1000L);
                    if (!formatted.equals(lastDecodedText) || (now - lastDecodedTimeMs > dedupeWindow)) {
                        lastDecodedText = formatted;
                        lastDecodedTimeMs = now;
                        if (isSelf) {
                            logMessage("Rx (Self): " + formatted, Color.parseColor("#80D8FF"));
                        } else {
                            logMessage("Rx: " + formatted, Color.parseColor("#00E5FF"));
                        }
                    }

                    // Only trigger AUTO responder on incoming external stations
                    if (!isSelf) {
                        handleAutoResponder(decoded);
                    }
                }
            } finally {
                isDecoding.set(false);
            }
        });
    }

    private void checkAudioPermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.RECORD_AUDIO}, PERMISSION_REQ_RECORD_AUDIO);
        } else {
            audioEngine.startRecording();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQ_RECORD_AUDIO) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                audioEngine.startRecording();
            } else {
                Toast.makeText(this, "Microphone permission needed for decoding", Toast.LENGTH_LONG).show();
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        decodeExecutor.shutdownNow();
        audioEngine.stopRecording();
    }
}
