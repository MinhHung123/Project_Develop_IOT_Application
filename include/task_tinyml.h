#ifndef TASK_TINYML_H
#define TASK_TINYML_H

#include "global.h"

#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "ml_model/dht_classifier_model.h"

/**
 * @brief FreeRTOS task entry: runs TFLite Micro inference on each new DHT20 sample.
 *
 * Consumer of xBinarySemaphoreTinyML (given by task_temp_humid after each write).
 * Reads via sensor_read() to obey ADR-03 (no direct global access).
 * Publishes (glob_ml_label, glob_ml_confidence) for task_coreIOT to telemeter.
 *
 * Recommended FreeRTOS params:
 *   - stack size: 8192 bytes (TFLite Micro is stack-heavy)
 *   - priority:   1 (lower than DHT20 producer)
 */
void task_tinyml_inference(void *pvParameters);

#endif // TASK_TINYML_H
