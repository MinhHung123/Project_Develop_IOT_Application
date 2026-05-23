"""
Train a TinyML classifier for DHT anomaly detection and export to ESP32.

Pipeline:
  1. Load synthetic dataset (Phase A output).
  2. EDA: class distribution + scatter.
  3. Stratified 80/20 train/test split.
  4. Build Keras MLP per ADR-08:
       Normalization (adapted on X_train)
         -> Dense(16, ReLU) -> Dense(8, ReLU) -> Dense(3, softmax)
  5. Train with EarlyStopping (val_loss, patience=5, restore_best_weights).
  6. Evaluate: accuracy, classification report, confusion matrix.
  7. Plot training curves + confusion matrix heatmap.
  8. Convert to TFLite with dynamic range quantization.
  9. Generate C header (`dht_classifier_model.h`) for ESP32 deployment.
 10. Sanity-check the .tflite by running it through tf.lite.Interpreter and
     comparing predictions against the float Keras model.

Run from project root:
    python3 notebooks/train_and_export.py
"""

from __future__ import annotations

import os
import random
import textwrap
from typing import List, Tuple

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# TF imports kept after matplotlib so TF log noise stays after our prints.
os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")  # hide INFO/WARNING from TF C++
import tensorflow as tf  # noqa: E402
from tensorflow import keras  # noqa: E402
from tensorflow.keras import layers  # noqa: E402

from sklearn.model_selection import train_test_split  # noqa: E402
from sklearn.metrics import (  # noqa: E402
    accuracy_score,
    classification_report,
    confusion_matrix,
    f1_score,
)


# ============================================================================
# Configuration
# ============================================================================

SEED = 42

DATASET_PATH = "dataset/dht_anomaly_dataset.csv"

OUT_DIR_PLOTS    = "notebooks/plots"
OUT_DIR_MODEL_C  = "include/ml_model"
OUT_TRAINING_PNG = os.path.join(OUT_DIR_PLOTS, "training_curves.png")
OUT_CM_PNG       = os.path.join(OUT_DIR_PLOTS, "confusion_matrix.png")
OUT_METRICS_TXT  = "notebooks/metrics.txt"
OUT_TFLITE       = "notebooks/dht_classifier_model.tflite"
OUT_HEADER       = os.path.join(OUT_DIR_MODEL_C, "dht_classifier_model.h")

CLASS_NAMES = ["NORMAL", "WARNING", "CRITICAL"]
NUM_CLASSES = len(CLASS_NAMES)

TEST_SIZE   = 0.20
BATCH_SIZE  = 32
MAX_EPOCHS  = 100
PATIENCE    = 5

# Model topology (ADR-08).
HIDDEN_1 = 16
HIDDEN_2 = 8

# C header symbol names.
C_ARRAY_NAME = "dht_classifier_model_tflite"
C_LEN_NAME   = "dht_classifier_model_tflite_len"
C_GUARD      = "DHT_CLASSIFIER_MODEL_H"


# ============================================================================
# Reproducibility
# ============================================================================

def fix_seeds(seed: int) -> None:
    """Fix every RNG source TF/Keras/Python/NumPy/sklearn might consult."""
    os.environ["PYTHONHASHSEED"] = str(seed)
    random.seed(seed)
    np.random.seed(seed)
    tf.keras.utils.set_random_seed(seed)
    # Optional: deterministic ops (slower but fully reproducible).
    try:
        tf.config.experimental.enable_op_determinism()
    except Exception:  # noqa: BLE001
        pass


# ============================================================================
# Pretty logging
# ============================================================================

def log(msg: str) -> None:
    print(f"[train] {msg}")


def section(title: str) -> None:
    bar = "=" * 70
    print(f"\n{bar}\n[train] {title}\n{bar}")


# ============================================================================
# Data loading & EDA
# ============================================================================

def load_dataset(path: str) -> Tuple[np.ndarray, np.ndarray]:
    """Load CSV produced by Phase A, return (X, y) arrays."""
    if not os.path.exists(path):
        raise FileNotFoundError(
            f"Dataset not found at {path}. Run scripts/generate_dataset.py first."
        )
    df = pd.read_csv(path)
    expected_cols = {"temperature", "humidity", "label"}
    if set(df.columns) != expected_cols:
        raise ValueError(f"Expected columns {expected_cols}, got {set(df.columns)}")
    X = df[["temperature", "humidity"]].to_numpy(dtype=np.float32)
    y = df["label"].to_numpy(dtype=np.int32)
    return X, y


def describe_split(name: str, y: np.ndarray) -> str:
    """Return a short text summary of class counts for logging/reports."""
    lines = [f"{name} size: {len(y)}"]
    for k, cname in enumerate(CLASS_NAMES):
        n = int((y == k).sum())
        pct = 100.0 * n / max(len(y), 1)
        lines.append(f"  {k} {cname:8s}: {n:4d} ({pct:5.1f}%)")
    return "\n".join(lines)


# ============================================================================
# Model
# ============================================================================

def build_model(X_train: np.ndarray) -> keras.Model:
    """Build the Keras model with in-graph normalization.

    The Normalization layer is adapted on X_train only -- never on the test
    set, so mean/std don't leak test statistics into the model.
    """
    norm = layers.Normalization(axis=-1, name="feature_normalization")
    norm.adapt(X_train)

    model = keras.Sequential(
        [
            keras.Input(shape=(2,), name="input_features"),
            norm,
            layers.Dense(HIDDEN_1, activation="relu", name="dense_1"),
            layers.Dense(HIDDEN_2, activation="relu", name="dense_2"),
            layers.Dense(NUM_CLASSES, activation="softmax", name="output"),
        ],
        name="dht_anomaly_classifier",
    )
    model.compile(
        optimizer=keras.optimizers.Adam(learning_rate=1e-3),
        loss="sparse_categorical_crossentropy",
        metrics=["accuracy"],
    )
    return model


# ============================================================================
# Plots
# ============================================================================

def plot_training_curves(history: keras.callbacks.History, out_path: str) -> None:
    """Save train/val accuracy + loss curves side-by-side."""
    h = history.history
    epochs = range(1, len(h["loss"]) + 1)

    fig, axes = plt.subplots(1, 2, figsize=(11, 4.2))

    axes[0].plot(epochs, h["accuracy"],     label="train", lw=1.8)
    axes[0].plot(epochs, h["val_accuracy"], label="val",   lw=1.8)
    axes[0].set_xlabel("Epoch")
    axes[0].set_ylabel("Accuracy")
    axes[0].set_title("Accuracy")
    axes[0].grid(True, alpha=0.3)
    axes[0].legend()

    axes[1].plot(epochs, h["loss"],     label="train", lw=1.8)
    axes[1].plot(epochs, h["val_loss"], label="val",   lw=1.8)
    axes[1].set_xlabel("Epoch")
    axes[1].set_ylabel("Loss")
    axes[1].set_title("Loss (sparse categorical CE)")
    axes[1].grid(True, alpha=0.3)
    axes[1].legend()

    fig.suptitle("Training curves -- DHT anomaly MLP (2-16-8-3)")
    fig.tight_layout()
    fig.savefig(out_path, dpi=130)
    plt.close(fig)


def plot_confusion(cm: np.ndarray, out_path: str) -> None:
    """3x3 annotated heatmap with raw counts and per-row percentages."""
    fig, ax = plt.subplots(figsize=(6.2, 5.2))
    im = ax.imshow(cm, cmap="Blues")
    fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)

    ax.set_xticks(range(NUM_CLASSES))
    ax.set_yticks(range(NUM_CLASSES))
    ax.set_xticklabels(CLASS_NAMES)
    ax.set_yticklabels(CLASS_NAMES)
    ax.set_xlabel("Predicted label")
    ax.set_ylabel("True label")
    ax.set_title("Confusion matrix (test set)")

    row_sums = cm.sum(axis=1, keepdims=True)
    row_pct = np.where(row_sums == 0, 0, cm / np.maximum(row_sums, 1))
    threshold = cm.max() / 2.0
    for i in range(NUM_CLASSES):
        for j in range(NUM_CLASSES):
            txt = f"{cm[i, j]}\n({100 * row_pct[i, j]:.1f}%)"
            color = "white" if cm[i, j] > threshold else "black"
            ax.text(j, i, txt, ha="center", va="center", color=color, fontsize=11)

    fig.tight_layout()
    fig.savefig(out_path, dpi=130)
    plt.close(fig)


# ============================================================================
# TFLite conversion + sanity check
# ============================================================================

def convert_to_tflite(model: keras.Model, out_path: str) -> bytes:
    """Convert the Keras model to TFLite with dynamic range quantization.

    Dynamic range quant: weights -> int8, activations -> float32 at inference.
    ~4x smaller than float32 weights, no representative dataset needed.
    """
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    tflite_bytes = converter.convert()
    with open(out_path, "wb") as f:
        f.write(tflite_bytes)
    return tflite_bytes


def tflite_predict(tflite_bytes: bytes, X: np.ndarray) -> np.ndarray:
    """Run inference on each row of X via tf.lite.Interpreter; returns class indices."""
    interp = tf.lite.Interpreter(model_content=tflite_bytes)
    interp.allocate_tensors()
    in_det  = interp.get_input_details()[0]
    out_det = interp.get_output_details()[0]

    preds: List[int] = []
    for row in X:
        x = row.reshape(1, -1).astype(in_det["dtype"])
        interp.set_tensor(in_det["index"], x)
        interp.invoke()
        out = interp.get_tensor(out_det["index"])
        preds.append(int(np.argmax(out)))
    return np.array(preds, dtype=np.int32)


# ============================================================================
# C header generation
# ============================================================================

def bytes_to_c_header(tflite_bytes: bytes, header_path: str) -> None:
    """Write tflite bytes as a C array with 16-byte alignment and length symbol.

    Format matches `xxd -i` style: 12 hex bytes per line, lowercase, comma-separated.
    Includes `alignas(16)` so TFLite Micro is happy on ESP32 ARM/Xtensa.
    """
    os.makedirs(os.path.dirname(header_path), exist_ok=True)

    arr_lines: List[str] = []
    bytes_per_line = 12
    for i in range(0, len(tflite_bytes), bytes_per_line):
        chunk = tflite_bytes[i : i + bytes_per_line]
        hex_part = ", ".join(f"0x{b:02x}" for b in chunk)
        # Trailing comma on every chunk except the last (cosmetic, both compile).
        if i + bytes_per_line < len(tflite_bytes):
            arr_lines.append(f"    {hex_part},")
        else:
            arr_lines.append(f"    {hex_part}")
    body = "\n".join(arr_lines)

    header_text = (
        f"/*\n"
        f" * dht_classifier_model.h\n"
        f" *\n"
        f" * Auto-generated by notebooks/train_and_export.py -- do not edit by hand.\n"
        f" *\n"
        f" * Model:    MLP 2 -> 16 -> 8 -> 3 (softmax), in-graph Normalization layer.\n"
        f" * Source:   Keras (TensorFlow {tf.__version__})\n"
        f" * Quant:    TFLite dynamic range (weights int8, activations float32)\n"
        f" * Size:     {len(tflite_bytes)} bytes\n"
        f" *\n"
        f" * The C array is `alignas(16)` so TFLite Micro can mmap it without\n"
        f" * unaligned-access traps on ESP32.\n"
        f" */\n"
        f"#ifndef {C_GUARD}\n"
        f"#define {C_GUARD}\n"
        f"\n"
        f"#ifdef __cplusplus\n"
        f'extern "C" {{\n'
        f"#endif\n"
        f"\n"
        f"#if defined(__has_include)\n"
        f"#  if __has_include(<cstdalign>)\n"
        f"#    include <cstdalign>\n"
        f"#  endif\n"
        f"#endif\n"
        f"\n"
        f"alignas(16) const unsigned char {C_ARRAY_NAME}[] = {{\n"
        f"{body}\n"
        f"}};\n"
        f"const unsigned int {C_LEN_NAME} = {len(tflite_bytes)};\n"
        f"\n"
        f"#ifdef __cplusplus\n"
        f"}}\n"
        f"#endif\n"
        f"\n"
        f"#endif  /* {C_GUARD} */\n"
    )
    with open(header_path, "w") as f:
        f.write(header_text)


# ============================================================================
# Main
# ============================================================================

def main() -> None:
    fix_seeds(SEED)

    section("0. Environment")
    log(f"TensorFlow: {tf.__version__}")
    log(f"Keras:      {tf.keras.__version__}")
    log(f"NumPy:      {np.__version__}")
    log(f"seed:       {SEED}")

    # ---- 1. Load data ----
    section("1. Load dataset")
    X, y = load_dataset(DATASET_PATH)
    log(f"X shape: {X.shape}, y shape: {y.shape}")
    log(describe_split("full dataset", y))

    # ---- 2. Stratified split ----
    section("2. Train/test split (stratified 80/20)")
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=TEST_SIZE, random_state=SEED, stratify=y
    )
    log(describe_split("train", y_train))
    log(describe_split("test",  y_test))

    # ---- 3. Build model ----
    section("3. Build model (Keras MLP 2-16-8-3)")
    model = build_model(X_train)
    model.summary(print_fn=lambda s: print(f"[train] {s}"))

    # ---- 4. Train ----
    section("4. Training")
    early_stop = keras.callbacks.EarlyStopping(
        monitor="val_loss",
        patience=PATIENCE,
        restore_best_weights=True,
        verbose=1,
    )
    history = model.fit(
        X_train, y_train,
        validation_split=0.15,           # carve val out of train (stratify happens via shuffle)
        epochs=MAX_EPOCHS,
        batch_size=BATCH_SIZE,
        callbacks=[early_stop],
        verbose=2,
    )
    epochs_run = len(history.history["loss"])
    final_train_acc = float(history.history["accuracy"][-1])
    final_val_acc   = float(history.history["val_accuracy"][-1])
    log(f"epochs run:      {epochs_run}")
    log(f"final train acc: {final_train_acc:.4f}")
    log(f"final val acc:   {final_val_acc:.4f}")

    # ---- 5. Evaluate on test ----
    section("5. Evaluate on test set (float Keras model)")
    test_loss, test_acc = model.evaluate(X_test, y_test, verbose=0)
    log(f"test loss:      {test_loss:.4f}")
    log(f"test accuracy:  {test_acc:.4f}")

    y_pred_proba = model.predict(X_test, verbose=0)
    y_pred = np.argmax(y_pred_proba, axis=1)
    cm = confusion_matrix(y_test, y_pred, labels=list(range(NUM_CLASSES)))
    macro_f1 = f1_score(y_test, y_pred, average="macro")
    class_report = classification_report(
        y_test, y_pred,
        target_names=CLASS_NAMES,
        digits=4,
        zero_division=0,
    )
    log(f"macro F1: {macro_f1:.4f}")
    print(f"\n[train] classification report (test set):\n{class_report}")
    print(f"[train] confusion matrix (rows=true, cols=pred):\n{cm}")

    # ---- 6. Plots ----
    section("6. Save plots")
    os.makedirs(OUT_DIR_PLOTS, exist_ok=True)
    plot_training_curves(history, OUT_TRAINING_PNG)
    log(f"saved: {OUT_TRAINING_PNG}")
    plot_confusion(cm, OUT_CM_PNG)
    log(f"saved: {OUT_CM_PNG}")

    # ---- 7. Convert to TFLite ----
    section("7. Convert to TFLite (dynamic range quantization)")
    tflite_bytes = convert_to_tflite(model, OUT_TFLITE)
    tflite_size = len(tflite_bytes)
    log(f"tflite size: {tflite_size} bytes ({tflite_size/1024:.2f} KB)")

    # ---- 8. Sanity check tflite predictions ----
    section("8. Sanity-check TFLite predictions vs Keras float model")
    y_pred_tflite = tflite_predict(tflite_bytes, X_test)
    tflite_acc = accuracy_score(y_test, y_pred_tflite)
    agreement = float((y_pred_tflite == y_pred).mean())
    log(f"tflite test accuracy:        {tflite_acc:.4f}")
    log(f"tflite vs keras agreement:   {agreement:.4f}")
    if abs(tflite_acc - test_acc) > 0.01:
        log("WARNING: tflite accuracy diverges from keras float by >1%. Quant may be too aggressive.")

    # ---- 9. Emit C header ----
    section("9. Emit C header for ESP32")
    bytes_to_c_header(tflite_bytes, OUT_HEADER)
    header_size = os.path.getsize(OUT_HEADER)
    log(f"saved: {OUT_HEADER}  ({header_size} bytes, {header_size/1024:.2f} KB)")

    # ---- 10. Write metrics.txt ----
    section("10. Write metrics.txt")
    cm_lines = ["    " + " ".join(f"{v:5d}" for v in row) for row in cm]
    cm_text = (
        "Confusion matrix (rows = true, columns = predicted):\n"
        + "          " + " ".join(f"{c:>5s}" for c in CLASS_NAMES) + "\n"
        + "\n".join(
            f"  {CLASS_NAMES[i]:8s}" + " ".join(f"{cm[i, j]:5d}" for j in range(NUM_CLASSES))
            for i in range(NUM_CLASSES)
        )
    )
    metrics_text = (
        f"DHT Anomaly Classifier -- Phase B metrics\n"
        f"=========================================\n"
        f"\n"
        f"Environment\n"
        f"-----------\n"
        f"TensorFlow:           {tf.__version__}\n"
        f"Keras:                {tf.keras.__version__}\n"
        f"Random seed:          {SEED}\n"
        f"\n"
        f"Dataset\n"
        f"-------\n"
        f"Source CSV:           {DATASET_PATH}\n"
        f"Total samples:        {len(y)}\n"
        f"Train / Test split:   {1 - TEST_SIZE:.0%} / {TEST_SIZE:.0%}, stratified\n"
        f"Train size:           {len(y_train)}\n"
        f"Test  size:           {len(y_test)}\n"
        f"\n"
        f"Class distribution (test set)\n"
        f"-----------------------------\n"
        f"{describe_split('test', y_test)}\n"
        f"\n"
        f"Model\n"
        f"-----\n"
        f"Topology:             Input(2) -> Normalization -> Dense(16, ReLU)\n"
        f"                      -> Dense(8, ReLU) -> Dense(3, softmax)\n"
        f"Total params:         {model.count_params()}\n"
        f"Optimizer:            Adam(lr=1e-3)\n"
        f"Loss:                 sparse_categorical_crossentropy\n"
        f"Batch size:           {BATCH_SIZE}\n"
        f"Max epochs:           {MAX_EPOCHS}\n"
        f"EarlyStopping:        monitor=val_loss, patience={PATIENCE}, restore_best_weights=True\n"
        f"Epochs actually run:  {epochs_run}\n"
        f"\n"
        f"Training results\n"
        f"----------------\n"
        f"Final train accuracy: {final_train_acc:.4f}\n"
        f"Final val   accuracy: {final_val_acc:.4f}\n"
        f"\n"
        f"Test results (Keras float model)\n"
        f"--------------------------------\n"
        f"Test loss:            {test_loss:.4f}\n"
        f"Test accuracy:        {test_acc:.4f}\n"
        f"Macro F1:             {macro_f1:.4f}\n"
        f"\n"
        f"Classification report (Keras float model)\n"
        f"-----------------------------------------\n"
        f"{class_report}\n"
        f"{cm_text}\n"
        f"\n"
        f"TFLite model\n"
        f"------------\n"
        f"Quantization:         Dynamic range (weights int8, activations float32)\n"
        f"Size:                 {tflite_size} bytes ({tflite_size/1024:.2f} KB)\n"
        f"Test accuracy:        {tflite_acc:.4f}\n"
        f"Agreement vs Keras:   {agreement:.4f}\n"
        f"\n"
        f"C header\n"
        f"--------\n"
        f"Path:                 {OUT_HEADER}\n"
        f"File size:            {header_size} bytes ({header_size/1024:.2f} KB)\n"
        f"Array symbol:         {C_ARRAY_NAME}\n"
        f"Length symbol:        {C_LEN_NAME}\n"
        f"Alignment:            16-byte (alignas(16))\n"
    )
    with open(OUT_METRICS_TXT, "w") as f:
        f.write(metrics_text)
    log(f"saved: {OUT_METRICS_TXT}")

    # ---- 11. Final summary ----
    section("DONE -- Phase B summary")
    log(f"epochs run:              {epochs_run}")
    log(f"keras test accuracy:     {test_acc:.4f}")
    log(f"tflite test accuracy:    {tflite_acc:.4f}")
    log(f"macro F1:                {macro_f1:.4f}")
    log(f"tflite size:             {tflite_size} bytes")
    log(f"header size:             {header_size} bytes")
    # Arena size estimate -- TFLite Micro typically needs roughly
    # model size + (sum of activation tensor sizes). For this tiny model,
    # 4-8 KB is plenty; we recommend 8 KB on ESP32-S3 for safety.
    log("arena size estimate:     8 KB recommended for ESP32-S3")

    # Acceptance gates
    section("Acceptance criteria check")
    gates = [
        ("test accuracy >= 0.93",         test_acc >= 0.93),
        ("macro F1 >= 0.92",              macro_f1 >= 0.92),
        ("tflite size < 10 KB",           tflite_size < 10 * 1024),
        ("header size < 100 KB",          header_size < 100 * 1024),
        ("tflite/keras agreement >= 0.98", agreement >= 0.98),
    ]
    all_pass = True
    for desc, passed in gates:
        mark = "OK " if passed else "FAIL"
        log(f"  [{mark}] {desc}")
        if not passed:
            all_pass = False
    log("ALL GATES PASS" if all_pass else "SOME GATES FAILED -- review above")


if __name__ == "__main__":
    main()
