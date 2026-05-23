"""
Synthetic dataset generator for Task 5 (TinyML DHT anomaly classifier).

Generates ~3000 (temperature, humidity) samples labeled into 3 classes:
    0 = NORMAL, 1 = WARNING, 2 = CRITICAL

Class definition (ADR-09):
    NORMAL   : 22 <= T <= 30  AND  40 <= H <= 75
    WARNING  : 18 <= T < 22  OR  30 < T <= 35
               OR  25 <= H < 40  OR  75 < H <= 85
               (and NOT CRITICAL)
    CRITICAL : T < 18 OR T > 35
               OR  H < 25 OR H > 85

Priority: CRITICAL > WARNING > NORMAL.

Output:
    dataset/dht_anomaly_dataset.csv   columns: temperature,humidity,label
    notebooks/plots/dataset_scatter.png  (eye-check the 3 regions)

Reproducible: numpy seed fixed.
"""

from __future__ import annotations

import os
from dataclasses import dataclass
from typing import List, Tuple

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")  # headless
import matplotlib.pyplot as plt


# ============================================================================
# Configuration
# ============================================================================

SEED = 42

# Target counts per class (ADR-07: mild imbalance ~50/27/23).
TARGET_NORMAL   = 1500
TARGET_WARNING  = 800
TARGET_CRITICAL = 700

# Physical sensor range (DHT20 datasheet: -40..80 C, 0..100 %RH).
# Restrict to plausible household range for tractable training.
T_MIN, T_MAX = 0.0, 50.0
H_MIN, H_MAX = 0.0, 100.0

# Threshold boundaries (ADR-09).
T_NORMAL_LO,   T_NORMAL_HI   = 22.0, 30.0
H_NORMAL_LO,   H_NORMAL_HI   = 40.0, 75.0
T_WARNING_LO,  T_WARNING_HI  = 18.0, 35.0
H_WARNING_LO,  H_WARNING_HI  = 25.0, 85.0
# Anything outside [T_WARNING_LO, T_WARNING_HI] for T (or similar for H) is CRITICAL.

# Noise sigma (added after uniform region sampling to soften boundaries).
NOISE_SIGMA_T = 0.5
NOISE_SIGMA_H = 2.0

# Edge-case samples: tight band straddling each boundary.
EDGE_SAMPLES_PER_BOUNDARY = 20  # x 8 boundaries (T_lo/hi normal+warning, H_lo/hi normal+warning)
EDGE_BAND_T = 0.8  # +/- C around the boundary line
EDGE_BAND_H = 2.0  # +/- %RH around the boundary line

OUTPUT_CSV = "dataset/dht_anomaly_dataset.csv"
OUTPUT_SCATTER = "notebooks/plots/dataset_scatter.png"


# ============================================================================
# Class labeling (priority: CRITICAL > WARNING > NORMAL)
# ============================================================================

LABEL_NORMAL   = 0
LABEL_WARNING  = 1
LABEL_CRITICAL = 2


def label_of(t: float, h: float) -> int:
    """Return class label for one (temp, humid) sample per ADR-09."""
    if t < T_WARNING_LO or t > T_WARNING_HI:
        return LABEL_CRITICAL
    if h < H_WARNING_LO or h > H_WARNING_HI:
        return LABEL_CRITICAL
    if t < T_NORMAL_LO or t > T_NORMAL_HI:
        return LABEL_WARNING
    if h < H_NORMAL_LO or h > H_NORMAL_HI:
        return LABEL_WARNING
    return LABEL_NORMAL


def label_vec(temps: np.ndarray, hums: np.ndarray) -> np.ndarray:
    """Vectorized labeling for arrays."""
    t = temps
    h = hums
    # CRITICAL mask
    crit = (t < T_WARNING_LO) | (t > T_WARNING_HI) | (h < H_WARNING_LO) | (h > H_WARNING_HI)
    # WARNING mask (not critical, but outside normal box)
    warn_t = (t < T_NORMAL_LO) | (t > T_NORMAL_HI)
    warn_h = (h < H_NORMAL_LO) | (h > H_NORMAL_HI)
    warn = (~crit) & (warn_t | warn_h)
    labels = np.zeros_like(t, dtype=np.int32)  # default NORMAL
    labels[warn] = LABEL_WARNING
    labels[crit] = LABEL_CRITICAL
    return labels


# ============================================================================
# Sub-region samplers
# ============================================================================

@dataclass
class Region:
    """Uniform sampling rectangle in (T, H) plane."""
    t_lo: float
    t_hi: float
    h_lo: float
    h_hi: float
    weight: float = 1.0  # relative sampling weight within a class


def sample_region(region: Region, n: int, rng: np.random.Generator) -> Tuple[np.ndarray, np.ndarray]:
    """Uniform sample inside the rectangle, then add Gaussian noise."""
    t = rng.uniform(region.t_lo, region.t_hi, n)
    h = rng.uniform(region.h_lo, region.h_hi, n)
    t += rng.normal(0.0, NOISE_SIGMA_T, n)
    h += rng.normal(0.0, NOISE_SIGMA_H, n)
    # Clip to physical sensor envelope.
    t = np.clip(t, T_MIN, T_MAX)
    h = np.clip(h, H_MIN, H_MAX)
    return t, h


def sample_class(regions: List[Region], target: int, rng: np.random.Generator,
                 want_label: int) -> Tuple[np.ndarray, np.ndarray]:
    """Sample `target` points that all have `want_label` after labeling.

    Oversamples then rejection-filters, because Gaussian noise can push a
    point into a neighboring class. Keeps drawing in batches until target met.
    """
    # Weighted allocation across sub-regions.
    weights = np.array([r.weight for r in regions], dtype=np.float64)
    weights /= weights.sum()

    collected_t: List[np.ndarray] = []
    collected_h: List[np.ndarray] = []
    collected = 0
    max_iters = 50
    batch_multiplier = 2.0  # oversample factor per draw

    for _ in range(max_iters):
        remaining = target - collected
        if remaining <= 0:
            break
        # Draw oversample batch.
        batch_total = int(np.ceil(remaining * batch_multiplier))
        per_region = (weights * batch_total).astype(int)
        # Ensure at least 1 from each.
        per_region = np.maximum(per_region, 1)

        for region, n in zip(regions, per_region):
            t, h = sample_region(region, int(n), rng)
            lab = label_vec(t, h)
            keep = lab == want_label
            collected_t.append(t[keep])
            collected_h.append(h[keep])
            collected += int(keep.sum())
            if collected >= target:
                break

    t_all = np.concatenate(collected_t)
    h_all = np.concatenate(collected_h)
    # Trim to exact target.
    t_all = t_all[:target]
    h_all = h_all[:target]
    return t_all, h_all


# ============================================================================
# Region definitions per class
# ============================================================================

def normal_regions() -> List[Region]:
    """One rectangle covering the safe living envelope."""
    # Pull boundaries slightly inward so noise rarely escapes.
    inset_t = 0.5
    inset_h = 1.5
    return [
        Region(
            t_lo=T_NORMAL_LO + inset_t,
            t_hi=T_NORMAL_HI - inset_t,
            h_lo=H_NORMAL_LO + inset_h,
            h_hi=H_NORMAL_HI - inset_h,
            weight=1.0,
        ),
    ]


def warning_regions() -> List[Region]:
    """Four bands flanking the NORMAL box on each axis, still within WARNING envelope."""
    # We bias H sub-bands' temp range to NORMAL T range so the only "out" coord
    # is humidity (keeps the sample comfortably WARNING, not CRITICAL).
    # Similarly for T sub-bands.
    return [
        # T low side (T in [WARNING_LO, NORMAL_LO), H in NORMAL band)
        Region(t_lo=T_WARNING_LO, t_hi=T_NORMAL_LO,
               h_lo=H_NORMAL_LO + 1.0, h_hi=H_NORMAL_HI - 1.0, weight=1.0),
        # T high side
        Region(t_lo=T_NORMAL_HI, t_hi=T_WARNING_HI,
               h_lo=H_NORMAL_LO + 1.0, h_hi=H_NORMAL_HI - 1.0, weight=1.0),
        # H low side
        Region(t_lo=T_NORMAL_LO + 0.5, t_hi=T_NORMAL_HI - 0.5,
               h_lo=H_WARNING_LO, h_hi=H_NORMAL_LO, weight=1.0),
        # H high side
        Region(t_lo=T_NORMAL_LO + 0.5, t_hi=T_NORMAL_HI - 0.5,
               h_lo=H_NORMAL_HI, h_hi=H_WARNING_HI, weight=1.0),
    ]


def critical_regions() -> List[Region]:
    """Outer bands and four corners well beyond the WARNING envelope."""
    return [
        # T very low (any H within plausible)
        Region(t_lo=T_MIN + 2.0, t_hi=T_WARNING_LO - 1.0,
               h_lo=H_MIN + 5.0, h_hi=H_MAX - 5.0, weight=1.0),
        # T very high
        Region(t_lo=T_WARNING_HI + 1.0, t_hi=T_MAX - 2.0,
               h_lo=H_MIN + 5.0, h_hi=H_MAX - 5.0, weight=1.0),
        # H very low
        Region(t_lo=T_WARNING_LO + 0.5, t_hi=T_WARNING_HI - 0.5,
               h_lo=H_MIN + 2.0, h_hi=H_WARNING_LO - 1.0, weight=1.0),
        # H very high
        Region(t_lo=T_WARNING_LO + 0.5, t_hi=T_WARNING_HI - 0.5,
               h_lo=H_WARNING_HI + 1.0, h_hi=H_MAX - 2.0, weight=1.0),
        # Extra weight on four corners (most "extreme" anomalies)
        Region(t_lo=T_MIN + 1.0, t_hi=T_WARNING_LO - 2.0,
               h_lo=H_MIN + 1.0, h_hi=H_WARNING_LO - 2.0, weight=0.5),
        Region(t_lo=T_WARNING_HI + 2.0, t_hi=T_MAX - 1.0,
               h_lo=H_WARNING_HI + 2.0, h_hi=H_MAX - 1.0, weight=0.5),
    ]


# ============================================================================
# Edge-case sampling near boundaries (to sharpen learned decision surface)
# ============================================================================

def edge_case_samples(rng: np.random.Generator) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Generate tight samples straddling each axis-aligned boundary line.

    Returns (t, h, label) arrays. Labels assigned via label_vec to be self-consistent.
    """
    t_chunks: List[np.ndarray] = []
    h_chunks: List[np.ndarray] = []

    t_boundaries = [T_NORMAL_LO, T_NORMAL_HI, T_WARNING_LO, T_WARNING_HI]
    h_boundaries = [H_NORMAL_LO, H_NORMAL_HI, H_WARNING_LO, H_WARNING_HI]

    n = EDGE_SAMPLES_PER_BOUNDARY

    # For each T boundary, vary H across a wide-but-plausible range so we cover
    # multiple class transitions along that vertical line.
    for tb in t_boundaries:
        t = rng.uniform(tb - EDGE_BAND_T, tb + EDGE_BAND_T, n)
        h = rng.uniform(H_MIN + 5.0, H_MAX - 5.0, n)
        t_chunks.append(t)
        h_chunks.append(h)

    # For each H boundary, vary T similarly.
    for hb in h_boundaries:
        t = rng.uniform(T_MIN + 2.0, T_MAX - 2.0, n)
        h = rng.uniform(hb - EDGE_BAND_H, hb + EDGE_BAND_H, n)
        t_chunks.append(t)
        h_chunks.append(h)

    t_all = np.concatenate(t_chunks)
    h_all = np.concatenate(h_chunks)
    t_all = np.clip(t_all, T_MIN, T_MAX)
    h_all = np.clip(h_all, H_MIN, H_MAX)
    lab = label_vec(t_all, h_all)
    return t_all, h_all, lab


# ============================================================================
# Main pipeline
# ============================================================================

def log(msg: str) -> None:
    print(f"[generate_dataset] {msg}")


def main() -> None:
    rng = np.random.default_rng(SEED)

    log(f"seed={SEED}, targets: NORMAL={TARGET_NORMAL}, WARNING={TARGET_WARNING}, CRITICAL={TARGET_CRITICAL}")

    # --- Per-class sampling ---
    log("sampling NORMAL ...")
    t_n, h_n = sample_class(normal_regions(), TARGET_NORMAL, rng, LABEL_NORMAL)
    log(f"  got {len(t_n)} NORMAL samples")

    log("sampling WARNING ...")
    t_w, h_w = sample_class(warning_regions(), TARGET_WARNING, rng, LABEL_WARNING)
    log(f"  got {len(t_w)} WARNING samples")

    log("sampling CRITICAL ...")
    t_c, h_c = sample_class(critical_regions(), TARGET_CRITICAL, rng, LABEL_CRITICAL)
    log(f"  got {len(t_c)} CRITICAL samples")

    # --- Edge cases (extra) ---
    log("generating edge-case samples near boundaries ...")
    t_e, h_e, lab_e = edge_case_samples(rng)
    log(f"  got {len(t_e)} edge-case samples (labels auto-assigned)")

    # --- Combine ---
    t_all = np.concatenate([t_n, t_w, t_c, t_e])
    h_all = np.concatenate([h_n, h_w, h_c, h_e])
    lab_all = np.concatenate([
        np.full(len(t_n), LABEL_NORMAL, dtype=np.int32),
        np.full(len(t_w), LABEL_WARNING, dtype=np.int32),
        np.full(len(t_c), LABEL_CRITICAL, dtype=np.int32),
        lab_e,
    ])

    # Defensive re-label (some noise points may have crossed boundary).
    relabel = label_vec(t_all, h_all)
    mismatch = (relabel != lab_all).sum()
    if mismatch > 0:
        log(f"  WARNING: {mismatch} samples had noisy labels; correcting to deterministic label")
        lab_all = relabel

    # Shuffle.
    perm = rng.permutation(len(t_all))
    t_all = t_all[perm]
    h_all = h_all[perm]
    lab_all = lab_all[perm]

    # Round to typical DHT sensor precision (0.1).
    t_all = np.round(t_all, 1)
    h_all = np.round(h_all, 1)

    # --- Stats ---
    df = pd.DataFrame({"temperature": t_all, "humidity": h_all, "label": lab_all})
    log("=== class distribution ===")
    counts = df["label"].value_counts().sort_index()
    names = {0: "NORMAL", 1: "WARNING", 2: "CRITICAL"}
    total = len(df)
    for k, v in counts.items():
        log(f"  {k} ({names[int(k)]}): {v}  ({100.0 * v / total:.1f}%)")
    log(f"  TOTAL: {total}")

    log("=== per-class feature stats ===")
    for k in sorted(df["label"].unique()):
        sub = df[df["label"] == k]
        log(f"  {names[int(k)]:8s}  T: min={sub.temperature.min():5.1f}  max={sub.temperature.max():5.1f}  "
            f"mean={sub.temperature.mean():5.2f}    H: min={sub.humidity.min():5.1f}  "
            f"max={sub.humidity.max():5.1f}  mean={sub.humidity.mean():5.2f}")

    # --- Save CSV ---
    os.makedirs(os.path.dirname(OUTPUT_CSV), exist_ok=True)
    df.to_csv(OUTPUT_CSV, index=False)
    log(f"saved CSV: {OUTPUT_CSV}  ({os.path.getsize(OUTPUT_CSV)} bytes)")

    # --- Scatter plot for eye-check ---
    os.makedirs(os.path.dirname(OUTPUT_SCATTER), exist_ok=True)
    plt.figure(figsize=(9, 7))
    colors = {0: "#2ecc71", 1: "#f39c12", 2: "#e74c3c"}
    for k in [0, 1, 2]:
        sub = df[df["label"] == k]
        plt.scatter(sub.temperature, sub.humidity, s=8, alpha=0.55,
                    c=colors[k], label=f"{k} = {names[k]} (n={len(sub)})")
    # Boundary lines.
    for x in [T_NORMAL_LO, T_NORMAL_HI]:
        plt.axvline(x, color="green",  ls=":", lw=0.8)
    for x in [T_WARNING_LO, T_WARNING_HI]:
        plt.axvline(x, color="red",    ls=":", lw=0.8)
    for y in [H_NORMAL_LO, H_NORMAL_HI]:
        plt.axhline(y, color="green",  ls=":", lw=0.8)
    for y in [H_WARNING_LO, H_WARNING_HI]:
        plt.axhline(y, color="red",    ls=":", lw=0.8)
    plt.xlabel("Temperature (°C)")
    plt.ylabel("Humidity (%RH)")
    plt.title(f"DHT anomaly dataset — synthetic ({total} samples, seed={SEED})")
    plt.legend(loc="upper right", fontsize=9, framealpha=0.9)
    plt.grid(True, alpha=0.25)
    plt.tight_layout()
    plt.savefig(OUTPUT_SCATTER, dpi=120)
    plt.close()
    log(f"saved scatter plot: {OUTPUT_SCATTER}")

    log("done.")


if __name__ == "__main__":
    main()
