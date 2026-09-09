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

public class LqMessage {

    public enum Type {
        CQ_STD(1),
        CQ_NONSTD_1(2),
        CQ_NONSTD_2(3),
        CQ_NONSTD_3(4),
        CALL_STD_NOSUF(5),
        CALL_STD_SUF(6),
        CALL_NONSTD(7),
        REPORT73_STD(8),
        M73_STD(9),
        M73_NONSTD(10),
        MULTI_REPORT73(11),
        MULTI_73(12),
        FREE_TEXT(13),
        RESERVED_A(14),
        RESERVED_B(15),
        RESERVED_C(16);

        // Aliases
        public static final Type CALL_STD = CALL_STD_NOSUF;
        public static final Type CALL_STD_2SUF = CALL_STD_SUF;
        public static final Type CALL_NONSTD_1 = CALL_NONSTD;
        public static final Type REPLY73_STD = REPORT73_STD;
        public static final Type REPORT73_NONSTD = MULTI_REPORT73;
        public static final Type REPLY73_NONSTD = MULTI_REPORT73;
        public static final Type MULTI_REPLY73 = MULTI_REPORT73;
        public static final Type RESERVED = RESERVED_A;

        public final int id;
        Type(int id) { this.id = id; }

        public static Type fromId(int id) {
            for (Type t : values()) {
                if (t.id == id) return t;
            }
            return CQ_STD;
        }
    }

    public static class MultiTarget {
        public String call = "";
        public int hash = 0;
        public int rstDb = 0;

        public MultiTarget() {}
        public MultiTarget(String call, int hash, int rstDb) {
            this.call = call;
            this.hash = hash;
            this.rstDb = rstDb;
        }
    }

    public Type type = Type.CQ_STD;
    public String call1 = "";
    public String call2 = "";
    public int suffix1 = 0;
    public int suffix2 = 0;
    public int hash1 = 0;
    public int hash2 = 0;
    public String modifier = "";
    public String locator = "";
    public int rstDb = 0;
    public String text = "";
    public byte[] rawPayload = null;
    public java.util.List<MultiTarget> multiTargets = new java.util.ArrayList<>();

    public static String formatRst(int rstDb) {
        if (rstDb >= 0) {
            return String.format(java.util.Locale.ROOT, "+%02d", rstDb);
        } else {
            return String.format(java.util.Locale.ROOT, "-%02d", Math.abs(rstDb));
        }
    }

    public String format() {
        switch (type) {
            case CQ_STD: {
                StringBuilder sb = new StringBuilder("CQ");
                if (modifier != null && !modifier.isEmpty()) sb.append(" ").append(modifier);
                if (call1 != null && !call1.isEmpty()) sb.append(" ").append(call1);
                if (locator != null && !locator.isEmpty()) sb.append(" ").append(locator);
                return sb.toString();
            }
            case CQ_NONSTD_1: {
                StringBuilder sb = new StringBuilder("CQ");
                if (call1 != null && !call1.isEmpty()) sb.append(" ").append(call1);
                if (locator != null && !locator.isEmpty()) sb.append(" ").append(locator);
                return sb.toString();
            }
            case CQ_NONSTD_2: {
                StringBuilder sb = new StringBuilder("CQ");
                if (modifier != null && !modifier.isEmpty()) sb.append(" ").append(modifier);
                if (call1 != null && !call1.isEmpty()) sb.append(" ").append(call1);
                return sb.toString();
            }
            case CQ_NONSTD_3: {
                return "CQ " + (call1 != null ? call1 : "");
            }
            case CALL_STD_NOSUF: {
                String rst = formatRst(rstDb);
                StringBuilder sb = new StringBuilder();
                if (call1 != null && !call1.isEmpty()) sb.append(call1);
                if (call2 != null && !call2.isEmpty()) {
                    if (sb.length() > 0) sb.append(" ");
                    sb.append(call2);
                }
                if (locator != null && !locator.isEmpty()) {
                    if (sb.length() > 0) sb.append(" ");
                    sb.append(locator);
                }
                if (sb.length() > 0) sb.append(" ");
                sb.append(rst);
                return sb.toString();
            }
            case CALL_STD_SUF: {
                String rst = formatRst(rstDb);
                StringBuilder sb = new StringBuilder();
                if (call1 != null && !call1.isEmpty()) {
                    if (call1.startsWith("<")) sb.append(call1);
                    else sb.append("<").append(call1).append(">");
                } else if (hash1 != 0) {
                    sb.append(String.format(java.util.Locale.ROOT, "<%06X>", hash1 & 0xFFFFFF));
                } else {
                    sb.append("<...>");
                }
                if (call2 != null && !call2.isEmpty()) {
                    sb.append(" ").append(call2);
                }
                if (locator != null && !locator.isEmpty()) {
                    sb.append(" ").append(locator);
                }
                sb.append(" ").append(rst);
                return sb.toString();
            }
            case CALL_NONSTD: {
                String h = String.format(java.util.Locale.ROOT, "<%05X>", hash1 & 0xFFFFF);
                String rst = formatRst(rstDb);
                StringBuilder sb = new StringBuilder();
                sb.append(call1 != null && !call1.isEmpty() ? (call1.startsWith("<") ? call1 : "<" + call1 + ">") : h);
                if (call2 != null && !call2.isEmpty()) sb.append(" ").append(call2);
                sb.append(" ").append(rst);
                return sb.toString();
            }
            case REPORT73_STD: {
                String rst = formatRst(rstDb);
                return (call1 != null ? call1 : "") + " " + (call2 != null ? call2 : "") + " R" + rst;
            }
            case M73_STD: {
                return (call1 != null ? call1 : "") + " " + (call2 != null ? call2 : "") + " 73";
            }
            case M73_NONSTD: {
                String h = String.format(java.util.Locale.ROOT, "<%06X>", hash1 & 0xFFFFFF);
                return (call1 != null && !call1.isEmpty() ? (call1.startsWith("<") ? call1 : "<" + call1 + ">") : h) + " " + (call2 != null ? call2 : "") + " 73";
            }
            case MULTI_REPORT73: {
                StringBuilder sb = new StringBuilder();
                boolean first = true;
                for (MultiTarget t : multiTargets) {
                    if (!first) sb.append(" ");
                    first = false;
                    if (t.call != null && !t.call.isEmpty()) {
                        if (t.call.startsWith("<")) sb.append(t.call);
                        else sb.append("<").append(t.call).append(">");
                    } else {
                        sb.append(String.format(java.util.Locale.ROOT, "<%06X>", t.hash & 0xFFFFFF));
                    }
                    sb.append(" R").append(formatRst(t.rstDb));
                }
                if (!first) sb.append(" ");
                if (call1 != null && !call1.isEmpty()) {
                    if (call1.startsWith("<")) sb.append(call1);
                    else sb.append("<").append(call1).append(">");
                } else {
                    sb.append(String.format(java.util.Locale.ROOT, "<%04X>", hash1 & 0xFFFF));
                }
                return sb.toString();
            }
            case MULTI_73: {
                StringBuilder sb = new StringBuilder();
                boolean first = true;
                for (MultiTarget t : multiTargets) {
                    if (!first) sb.append(" ");
                    first = false;
                    if (t.call != null && !t.call.isEmpty()) {
                        if (t.call.startsWith("<")) sb.append(t.call);
                        else sb.append("<").append(t.call).append(">");
                    } else {
                        sb.append(String.format(java.util.Locale.ROOT, "<%06X>", t.hash & 0xFFFFFF));
                    }
                }
                if (!first) sb.append(" ");
                if (call1 != null && !call1.isEmpty()) {
                    if (call1.startsWith("<")) sb.append(call1);
                    else sb.append("<").append(call1).append(">");
                } else {
                    sb.append(String.format(java.util.Locale.ROOT, "<%04X>", hash1 & 0xFFFF));
                }
                sb.append(" 73");
                return sb.toString();
            }
            case FREE_TEXT: {
                return text != null ? text : "";
            }
            case RESERVED_A:
            case RESERVED_B:
            case RESERVED_C:
                return "[RESERVED]";
            default:
                return "<UNKNOWN>";
        }
    }
}
