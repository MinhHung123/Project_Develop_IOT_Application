#include "task_tinyml.h"

// ---- File-scope TFLite Micro state (hidden from other translation units) ----
namespace {
    constexpr int kTensorArenaSize = 8 * 1024;            // 8 KB, plenty for 2-16-8-3
    alignas(16) uint8_t s_tensor_arena[kTensorArenaSize];

    tflite::MicroErrorReporter   s_error_reporter;
    const tflite::Model*         s_model       = nullptr;
    tflite::MicroInterpreter*    s_interpreter = nullptr;
    TfLiteTensor*                s_input       = nullptr;
    TfLiteTensor*                s_output      = nullptr;

    // AllOpsResolver carries every TFLite Micro op -- generous on binary size
    // but bullet-proof against missing-op errors. Tanaka v1.0.0 lib exposes it
    // as a plain class with default ctor.
    tflite::AllOpsResolver s_op_resolver;
}

/**
 * @brief One-time TFLite Micro setup: load model, allocate tensors, cache I/O pointers.
 * @return true on success, false on any fatal error (caller must abort task).
 */
static bool tflite_setup() {
    s_model = tflite::GetModel(dht_classifier_model_tflite);
    if (s_model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.printf("[tinyml] model schema mismatch: %lu vs %d\n",
                      (unsigned long) s_model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    // Use static storage so the interpreter survives beyond this function's scope.
    static tflite::MicroInterpreter interp(s_model, s_op_resolver,
                                           s_tensor_arena, kTensorArenaSize,
                                           &s_error_reporter);
    s_interpreter = &interp;

    if (s_interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("[tinyml] AllocateTensors() failed");
        return false;
    }

    s_input  = s_interpreter->input(0);
    s_output = s_interpreter->output(0);
    if (s_input == nullptr || s_output == nullptr) {
        Serial.println("[tinyml] input/output tensor null");
        return false;
    }

    Serial.printf("[tinyml] arena used = %lu bytes\n",
                  (unsigned long) s_interpreter->arena_used_bytes());
    return true;
}

/**
 * @brief Run one inference pass. Normalization is embedded in the model -- feed raw values.
 * @param temp      Raw temperature from DHT20 (°C).
 * @param humi      Raw humidity from DHT20 (%RH).
 * @param out_label Output class index: 0=NORMAL, 1=WARNING, 2=CRITICAL.
 * @param out_conf  Output softmax probability of the winning class [0..1].
 * @return true on success, false if interpreter not ready or Invoke() failed.
 */
static bool tflite_predict(float temp, float humi, int* out_label, float* out_conf) {
    if (s_input == nullptr || s_output == nullptr) return false;

    // Model input shape: (1, 2) float32. No manual normalization needed.
    s_input->data.f[0] = temp;
    s_input->data.f[1] = humi;

    if (s_interpreter->Invoke() != kTfLiteOk) return false;

    // Argmax over 3 softmax outputs.
    int   best_idx  = 0;
    float best_prob = s_output->data.f[0];
    for (int i = 1; i < 3; i++) {
        if (s_output->data.f[i] > best_prob) {
            best_prob = s_output->data.f[i];
            best_idx  = i;
        }
    }
    *out_label = best_idx;
    *out_conf  = best_prob;
    return true;
}

static const char* label_name(int label) {
    switch (label) {
        case 0:  return "NORMAL";
        case 1:  return "WARNING";
        case 2:  return "CRITICAL";
        default: return "UNKNOWN";
    }
}

/**
 * @brief FreeRTOS task: waits for xBinarySemaphoreTinyML, reads sensor, runs inference.
 *
 * Semaphore is given by task_temp_humid() after each successful DHT20 read (every 2 s).
 * On setup failure the task deletes itself cleanly -- no reboot, no silent hang.
 */
void task_tinyml_inference(void *pvParameters) {
    (void) pvParameters;

    // ---- One-time setup ----
    if (!tflite_setup()) {
        Serial.println("[tinyml] setup failed -- task suspending");
        vTaskDelete(NULL);
        return; // unreachable, defensive
    }
    Serial.println("[tinyml] setup OK, entering inference loop");

    // ---- Main loop ----
    while (1) {
        xSemaphoreTake(xBinarySemaphoreTinyML, portMAX_DELAY);

        SensorData sample;
        if (!sensor_read(&sample)) {
            // Sample not valid yet -- skip this round, wait for next.
            continue;
        }

        uint32_t t_start_us = micros();
        int   label;
        float confidence;
        if (!tflite_predict(sample.temp, sample.humi, &label, &confidence)) {
            Serial.println("[tinyml] inference invoke failed");
            continue;
        }
        uint32_t lat_us = micros() - t_start_us;

        // Publish to globals for task_coreIOT to pick up.
        glob_ml_label      = label;
        glob_ml_confidence = confidence;

        Serial.printf("[tinyml] label=%s conf=%.3f lat=%lums temp=%.1f humi=%.1f\n",
                      label_name(label), confidence, lat_us / 1000UL,
                      sample.temp, sample.humi);
    }
}
