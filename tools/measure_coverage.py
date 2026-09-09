#!/usr/bin/env python3
# =============================================================================
# The LQ Digital Mode Family — Reference Implementation (lq_lib)
#
# Author:  Luis Quesada (HB9IPH)
# Web:     https://luisquesada.com
# Portal:  https://lquesada.github.io/lq_lib/
# GitHub:  https://github.com/lquesada/lq_lib
# App:     qFT8 — Portable Amateur Radio for Android (https://qft8.com)
#
# License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)
#
# Copyright (c) 2026 Luis Quesada (HB9IPH)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# =============================================================================

import os
import sys
import subprocess
import glob
import re

def run_cmd(cmd, cwd=None):
    res = subprocess.run(cmd, shell=True, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        print(f"Error executing: {cmd}")
        if res.stdout:
            print(res.stdout)
        if res.stderr:
            print(res.stderr)
        sys.exit(res.returncode)
    return res.stdout

def main():
    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    build_cov_dir = os.path.join(root_dir, "build_coverage")

    print("==================================================================")
    print("        LQ Reference Library — Automated Code Coverage Tool       ")
    print("==================================================================")
    print(f" -> Configuring CMake with coverage instrumentation...")
    os.makedirs(build_cov_dir, exist_ok=True)
    gtest_local = os.path.join(root_dir, "build", "_deps", "googletest-src")
    extra_cmake = ""
    if os.path.isdir(gtest_local):
        extra_cmake = f" -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST='{gtest_local}'"
    run_cmd(f"cmake -S '{root_dir}' -B '{build_cov_dir}' -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON{extra_cmake}")

    print(f" -> Compiling library and 23 test suites...")
    run_cmd(f"cmake --build '{build_cov_dir}' -j$(nproc)")

    print(f" -> Cleaning stale coverage profile data...")
    for old_f in glob.glob(os.path.join(build_cov_dir, "**", "*.gcda"), recursive=True) + glob.glob(os.path.join(build_cov_dir, "**", "*.gcov"), recursive=True):
        try:
            os.remove(old_f)
        except OSError:
            pass

    print(f" -> Running complete automated test suite...")
    run_cmd("ctest --output-on-failure", cwd=build_cov_dir)


    print(f" -> Generating and analyzing gcov data for src/*.cpp...")
    gcda_files = glob.glob(os.path.join(build_cov_dir, "**", "*.gcda"), recursive=True)
    if not gcda_files:
        print("Error: No .gcda coverage profile files found.")
        sys.exit(1)

    # Run gcov on all source files
    src_dir = os.path.join(root_dir, "src")
    src_files = sorted(glob.glob(os.path.join(src_dir, "*.cpp")))

    coverage_results = []
    total_exec_lines = 0
    total_hit_lines = 0

    for sf in src_files:
        basename = os.path.basename(sf)
        obj_path = os.path.join(build_cov_dir, "CMakeFiles", "lq.dir", "src", f"{basename}.o")
        # Run gcov on this file
        run_cmd(f"gcov -o '{obj_path}' '{sf}'", cwd=build_cov_dir)
        gcov_filename = os.path.join(build_cov_dir, f"{basename}.gcov")
        if not os.path.exists(gcov_filename):
            continue

        exec_lines = 0
        hit_lines = 0
        uncovered_line_nums = []

        with open(gcov_filename, "r", encoding="utf-8", errors="ignore") as gf:
            for line in gf:
                parts = line.split(":", 2)
                if len(parts) < 3:
                    continue
                count_str = parts[0].strip()
                line_num_str = parts[1].strip()
                if count_str == "-" or count_str == "===" or not line_num_str.isdigit():
                    continue
                line_num = int(line_num_str)
                exec_lines += 1
                if count_str == "#####":
                    uncovered_line_nums.append(line_num)
                else:
                    hit_lines += 1

        pct = (hit_lines / exec_lines * 100.0) if exec_lines > 0 else 100.0
        coverage_results.append({
            "file": basename,
            "exec": exec_lines,
            "hit": hit_lines,
            "pct": pct,
            "uncovered": uncovered_line_nums
        })
        total_exec_lines += exec_lines
        total_hit_lines += hit_lines

    # Print table
    print("\n" + "=" * 78)
    print(f"{'Source File':<24} | {'Exec Lines':<10} | {'Hit Lines':<10} | {'Coverage %':<10} | {'Status'}")
    print("-" * 78)
    for r in coverage_results:
        status = "✓ 100%" if r['pct'] == 100.0 else f"⚠ {r['pct']:.1f}%"
        print(f"{r['file']:<24} | {r['exec']:<10} | {r['hit']:<10} | {r['pct']:>9.2f}% | {status}")

    overall_pct = (total_hit_lines / total_exec_lines * 100.0) if total_exec_lines > 0 else 100.0
    print("=" * 78)
    print(f"{'TOTAL (src/*.cpp)':<24} | {total_exec_lines:<10} | {total_hit_lines:<10} | {overall_pct:>9.2f}% | {'✓ EXCELLENT (>90%)' if overall_pct >= 90.0 else 'INCOMPLETE (<90%)'}")
    print("=" * 78)

    # If any uncovered lines, list them
    uncovered_files = [r for r in coverage_results if r['uncovered']]
    if uncovered_files:
        print("\nUncovered Line Breakdown:")
        for r in uncovered_files:
            lines_str = ", ".join(str(n) for n in r['uncovered'])
            print(f"  - {r['file']}: lines {lines_str}")
    else:
        print("\n✓ 100% of all executable statements across the entire reference codebase are exercised!")

    # Cleanup temporary coverage build folder if needed or keep it
    print(f"\nReport completed successfully. Total line coverage: {overall_pct:.2f}%\n")

if __name__ == "__main__":
    main()
