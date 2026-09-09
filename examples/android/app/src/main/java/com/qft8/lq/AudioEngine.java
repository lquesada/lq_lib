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

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.util.Log;

public class AudioEngine {

    private static final String TAG = "AudioEngine";
    public static final int SAMPLE_RATE = 12000;
    public static final int BUFFER_SECONDS = 32;
    public static final int RING_BUFFER_SIZE = SAMPLE_RATE * BUFFER_SECONDS;

    public interface AudioListener {
        void onAudioChunk(float[] chunk, int length);
        void onPeriodicDecodeCheck(float[] ringBuffer, int writePos);
    }

    private final float[] ringBuffer = new float[RING_BUFFER_SIZE];
    private int ringWritePos = 0;

    private AudioRecord audioRecord;
    private AudioTrack audioTrack;
    private Thread recordingThread;
    private volatile boolean isRecording = false;
    private AudioListener listener;

    private long lastDecodeCheckTimeMs = 0;

    public void setListener(AudioListener listener) {
        this.listener = listener;
    }

    public synchronized boolean startRecording() {
        if (isRecording) return true;

        int minBufferSize = AudioRecord.getMinBufferSize(
                SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT
        );
        int bufferSize = Math.max(minBufferSize, SAMPLE_RATE * 2);

        try {
            int[] audioSources = new int[]{
                    MediaRecorder.AudioSource.VOICE_RECOGNITION,
                    MediaRecorder.AudioSource.UNPROCESSED,
                    MediaRecorder.AudioSource.MIC,
                    MediaRecorder.AudioSource.DEFAULT
            };

            for (int source : audioSources) {
                try {
                    audioRecord = new AudioRecord(
                            source,
                            SAMPLE_RATE,
                            AudioFormat.CHANNEL_IN_MONO,
                            AudioFormat.ENCODING_PCM_16BIT,
                            bufferSize
                    );
                    if (audioRecord.getState() == AudioRecord.STATE_INITIALIZED) {
                        Log.i(TAG, "AudioRecord successfully initialized with source: " + source);
                        break;
                    } else {
                        audioRecord.release();
                        audioRecord = null;
                    }
                } catch (Exception ignored) {
                    audioRecord = null;
                }
            }

            if (audioRecord == null || audioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
                Log.e(TAG, "AudioRecord initialization failed across all audio sources.");
                return false;
            }

            audioRecord.startRecording();
            isRecording = true;

            recordingThread = new Thread(this::recordLoop, "LQ-AudioRecordThread");
            recordingThread.start();
            return true;
        } catch (SecurityException e) {
            Log.e(TAG, "Permission denied for AudioRecord: " + e.getMessage());
            return false;
        } catch (Exception e) {
            Log.e(TAG, "Exception starting AudioRecord: " + e.getMessage());
            return false;
        }
    }

    public synchronized void stopRecording() {
        isRecording = false;
        if (recordingThread != null) {
            try {
                recordingThread.join(500);
            } catch (InterruptedException ignored) {}
            recordingThread = null;
        }
        if (audioRecord != null) {
            try {
                audioRecord.stop();
                audioRecord.release();
            } catch (Exception ignored) {}
            audioRecord = null;
        }
    }

    private volatile boolean isTransmitting = false;

    public boolean isTransmitting() {
        return isTransmitting;
    }

    private void recordLoop() {
        short[] shortBuffer = new short[512];
        float[] floatChunk = new float[512];

        while (isRecording) {
            int read = 0;
            try {
                if (audioRecord != null && audioRecord.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING) {
                    read = audioRecord.read(shortBuffer, 0, shortBuffer.length);
                }
            } catch (Exception e) {
                Log.e(TAG, "Error in audioRecord.read: " + e.getMessage());
                break;
            }

            if (read > 0) {
                if (!isTransmitting) {
                    synchronized (ringBuffer) {
                        for (int i = 0; i < read; ++i) {
                            float sample = shortBuffer[i] / 32768.0f;
                            floatChunk[i] = sample;
                            ringBuffer[ringWritePos] = sample;
                            ringWritePos = (ringWritePos + 1) % RING_BUFFER_SIZE;
                        }
                    }

                    if (listener != null) {
                        listener.onAudioChunk(floatChunk, read);

                        long now = System.currentTimeMillis();
                        if (now - lastDecodeCheckTimeMs >= 200) { // Check continuously every 200ms
                            lastDecodeCheckTimeMs = now;
                            listener.onPeriodicDecodeCheck(ringBuffer, ringWritePos);
                        }
                    }
                }
            }
        }
    }

    private Thread playbackThread;
    private AudioTrack activeTrack;

    public synchronized void stopPlayback() {
        if (playbackThread != null) {
            playbackThread.interrupt();
            playbackThread = null;
        }
        if (activeTrack != null) {
            try {
                activeTrack.pause();
                activeTrack.flush();
                activeTrack.stop();
                activeTrack.release();
            } catch (Exception ignored) {}
            activeTrack = null;
        }
        isTransmitting = false;
    }

    public synchronized void playAudio(float[] samples) {
        if (samples == null || samples.length == 0) return;
        stopPlayback();

        isTransmitting = true;
        playbackThread = new Thread(() -> {
            short[] pcm16 = new short[samples.length];
            for (int i = 0; i < samples.length; ++i) {
                float clamped = Math.max(-1.0f, Math.min(1.0f, samples[i]));
                pcm16[i] = (short) Math.round(clamped * 32767.0f);
            }

            int minBufferSize = AudioTrack.getMinBufferSize(
                    SAMPLE_RATE,
                    AudioFormat.CHANNEL_OUT_MONO,
                    AudioFormat.ENCODING_PCM_16BIT
            );

            AudioTrack track;
            synchronized (AudioEngine.this) {
                track = new AudioTrack(
                        AudioManager.STREAM_MUSIC,
                        SAMPLE_RATE,
                        AudioFormat.CHANNEL_OUT_MONO,
                        AudioFormat.ENCODING_PCM_16BIT,
                        Math.max(minBufferSize, pcm16.length * 2),
                        AudioTrack.MODE_STATIC
                );
                activeTrack = track;
            }

            try {
                track.write(pcm16, 0, pcm16.length);
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
                    track.setVolume(1.0f);
                } else {
                    track.setStereoVolume(1.0f, 1.0f);
                }
                track.play();

                // Stream transmitted audio progressively into ringBuffer and notify waterfall listener
                int chunkSize = 512;
                float[] chunk = new float[chunkSize];
                int totalSamples = samples.length;
                int offset = 0;
                long startTime = System.currentTimeMillis();

                while (offset < totalSamples && isTransmitting) {
                    int len = Math.min(chunkSize, totalSamples - offset);
                    System.arraycopy(samples, offset, chunk, 0, len);

                    synchronized (ringBuffer) {
                        for (int i = 0; i < len; ++i) {
                            ringBuffer[ringWritePos] = chunk[i];
                            ringWritePos = (ringWritePos + 1) % RING_BUFFER_SIZE;
                        }
                    }

                    if (listener != null) {
                        listener.onAudioChunk(chunk, len);
                    }

                    offset += len;
                    long targetElapsed = Math.round((offset / (float) SAMPLE_RATE) * 1000.0f);
                    long actualElapsed = System.currentTimeMillis() - startTime;
                    long sleepMs = targetElapsed - actualElapsed;
                    if (sleepMs > 0) {
                        Thread.sleep(sleepMs);
                    }
                }
            } catch (InterruptedException ignored) {
            } finally {
                synchronized (AudioEngine.this) {
                    if (activeTrack == track) {
                        try {
                            track.stop();
                            track.release();
                        } catch (Exception ignored) {}
                        activeTrack = null;
                    }
                }
                isTransmitting = false;

                // Immediately notify listener to run decode check on completed transmission
                if (listener != null) {
                    listener.onPeriodicDecodeCheck(ringBuffer, ringWritePos);
                }
            }
        }, "LQ-AudioTrackThread");
        playbackThread.start();
    }

    public float[] getRecentSamples(int count) {
        float[] out = new float[count];
        synchronized (ringBuffer) {
            int start = (ringWritePos - count + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
            for (int i = 0; i < count; ++i) {
                out[i] = ringBuffer[(start + i) % RING_BUFFER_SIZE];
            }
        }
        return out;
    }
}
