#include "detection_engine.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "DETECTION";

// ─── Isolation Forest node ─────────────────────────────
typedef struct {
    int     feature_idx;
    float   split_value;
    int     left;
    int     right;
    int     depth;
    int     size;
} if_node_t;

typedef struct {
    if_node_t nodes[1 << IF_TREE_DEPTH];
    int       node_count;
} if_tree_t;

static if_tree_t s_forest[IF_NUM_TREES];
static bool      s_forest_built = false;

static calibration_state_t s_calib_state     = CALIB_IDLE;
static uint8_t             s_calib_progress  = 0;
static uint32_t            s_calib_collected = 0;

// ─── Training data — real 200°C baseline ───────────────
static float s_train_data[IF_SAMPLE_SIZE][IF_FEATURE_COUNT] = {
    {6.59f, 1.41f, 8.99f}, {6.65f, 1.41f, 9.84f},
    {7.19f, 1.41f, 8.59f}, {6.96f, 1.41f, 9.17f},
    {6.62f, 1.41f, 8.64f}, {6.69f, 1.41f, 9.51f},
    {6.62f, 1.41f, 9.25f}, {6.69f, 1.41f, 11.45f},
    {6.29f, 1.41f, 6.61f}, {6.65f, 1.41f, 6.40f},
    {6.37f, 1.41f, 5.62f}, {6.43f, 1.41f, 6.66f},
    {6.27f, 1.41f, 7.45f}, {6.60f, 1.41f, 7.05f},
    {6.59f, 1.41f, 6.76f}, {6.96f, 1.41f, 8.14f},
    {6.74f, 1.41f, 9.74f}, {6.88f, 1.41f, 9.28f},
    {6.73f, 1.41f, 9.96f}, {6.51f, 1.41f, 8.38f},
    {6.43f, 1.41f, 10.54f},{6.84f, 1.41f, 10.02f},
    {6.55f, 1.41f, 8.73f}, {6.37f, 1.41f, 9.16f},
    {6.82f, 1.41f, 9.02f}, {6.62f, 1.41f, 10.09f},
    {6.69f, 1.41f, 8.87f}, {6.68f, 1.41f, 10.10f},
    {6.69f, 1.41f, 10.53f},{6.74f, 1.41f, 9.71f},
    {6.59f, 1.41f, 9.48f}, {6.74f, 1.41f, 8.90f},
};

// ─── Build one isolation tree ──────────────────────────
static void build_tree(if_tree_t *tree, int node_idx,
                        int *indices, int count, int depth)
{
    if_node_t *node = &tree->nodes[node_idx];
    node->depth     = depth;
    node->size      = count;
    node->left      = -1;
    node->right     = -1;

    if (count <= 1 || depth >= IF_TREE_DEPTH) {
        node->feature_idx = -1;
        return;
    }

    node->feature_idx = rand() % IF_FEATURE_COUNT;
    int feat = node->feature_idx;

    float min_val = s_train_data[indices[0]][feat];
    float max_val = min_val;
    for (int i = 1; i < count; i++) {
        float v = s_train_data[indices[i]][feat];
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;
    }

    if (max_val - min_val < 1e-6f) {
        node->feature_idx = -1;
        return;
    }

    node->split_value = min_val +
                        ((float)rand() / RAND_MAX)
                        * (max_val - min_val);

    int left_idx[IF_SAMPLE_SIZE];
    int right_idx[IF_SAMPLE_SIZE];
    int left_count  = 0;
    int right_count = 0;

    for (int i = 0; i < count; i++) {
        if (s_train_data[indices[i]][feat]
                < node->split_value) {
            left_idx[left_count++]   = indices[i];
        } else {
            right_idx[right_count++] = indices[i];
        }
    }

    if (left_count > 0 && right_count > 0) {
        node->left  = tree->node_count++;
        node->right = tree->node_count++;
        build_tree(tree, node->left,
                   left_idx,  left_count,  depth + 1);
        build_tree(tree, node->right,
                   right_idx, right_count, depth + 1);
    }
}

// ─── Path length ───────────────────────────────────────
static float path_length(if_tree_t *tree,
                          float *features, int node_idx)
{
    if_node_t *node = &tree->nodes[node_idx];
    if (node->feature_idx < 0 || node->left < 0) {
        return (float)node->depth;
    }
    if (features[node->feature_idx] < node->split_value) {
        return path_length(tree, features, node->left);
    } else {
        return path_length(tree, features, node->right);
    }
}

// ─── Build forest ──────────────────────────────────────
static void build_forest(void)
{
    int indices[IF_SAMPLE_SIZE];
    for (int i = 0; i < IF_SAMPLE_SIZE; i++) indices[i] = i;

    for (int t = 0; t < IF_NUM_TREES; t++) {
        memset(&s_forest[t], 0, sizeof(if_tree_t));
        s_forest[t].node_count = 1;
        build_tree(&s_forest[t], 0,
                   indices, IF_SAMPLE_SIZE, 0);
    }
    s_forest_built = true;
    ESP_LOGI(TAG, "Isolation Forest built — %d trees",
             IF_NUM_TREES);
}

// ─── Z-Score ───────────────────────────────────────────
float compute_zscore(float value, float mean, float variance)
{
    if (variance < 1e-6f) return 0.0f;
    return fabsf(value - mean) / sqrtf(variance);
}

// ─── Anomaly Score ─────────────────────────────────────
float compute_anomaly_score(float rms,
                             float crest, float roc)
{
    if (!s_forest_built) return 0.0f;

    float features[IF_FEATURE_COUNT] = {rms, crest, roc};
    float avg_path = 0.0f;

    for (int t = 0; t < IF_NUM_TREES; t++) {
        avg_path += path_length(&s_forest[t], features, 0);
    }
    avg_path /= IF_NUM_TREES;

    float c = 2.0f * (logf(IF_SAMPLE_SIZE - 1.0f) + 0.5772f)
              - (2.0f * (IF_SAMPLE_SIZE - 1.0f)
                 / IF_SAMPLE_SIZE);
    float score = powf(2.0f, -avg_path / c);

    if (score < 0.0f) score = 0.0f;
    if (score > 1.0f) score = 1.0f;
    return score;
}

// ─── Signature Matching ────────────────────────────────
uint8_t match_fault_signature(float rms, float crest_factor)
{
    // Dry run — no load
    if (rms <= SIG_DRY_RUN_RMS_MAX) {
        return FAULT_DRY_RUN;
    }

    // Rotor stall
    if (rms >= SIG_STALL_RMS_MIN) {
        return FAULT_STALL;
    }

    // Mud ingestion
    if (rms >= SIG_MUD_RMS_MIN && rms < SIG_MUD_RMS_MAX) {
        return FAULT_MUD;
    }

    return FAULT_NONE;
}

// ─── Calibration getters ───────────────────────────────
calibration_state_t detection_engine_get_calib_state(void)
{
    return s_calib_state;
}

uint8_t detection_engine_get_calib_progress(void)
{
    return s_calib_progress;
}

// ─── Run calibration ───────────────────────────────────
static void run_calibration(void)
{
    ESP_LOGI(TAG, "Calibration started — hold load bank"
                  " at NORMAL level for 30 seconds");

    s_calib_state     = CALIB_RUNNING;
    s_calib_progress  = 0;
    s_calib_collected = 0;
    s_forest_built    = false;

    feature_vector_t fv;
    uint32_t start_ms = esp_timer_get_time() / 1000;

    while (s_calib_collected < CALIBRATION_SAMPLE_COUNT) {

        uint32_t now_ms  = esp_timer_get_time() / 1000;
        uint32_t elapsed = now_ms - start_ms;

        if (elapsed > CALIBRATION_DURATION_MS) {
            if (s_calib_collected < 8) {
                ESP_LOGW(TAG, "Calibration timeout —"
                              " using real baseline data");
            }
            break;
        }

        if (xQueueReceive(g_state.feature_queue,
                          &fv,
                          pdMS_TO_TICKS(500)) != pdTRUE) {
            continue;
        }

        // Skip dry run during calibration
        if (fv.rms_current <= SIG_DRY_RUN_RMS_MAX) {
            ESP_LOGW(TAG, "Low current during calibration"
                          " — is load bank on?");
            continue;
        }

        s_train_data[s_calib_collected][0] = fv.rms_current;
        s_train_data[s_calib_collected][1] = fv.crest_factor;
        s_train_data[s_calib_collected][2] = fv.rate_of_change;
        s_calib_collected++;

        s_calib_progress = (uint8_t)(
            (s_calib_collected * 100)
            / CALIBRATION_SAMPLE_COUNT);

        ESP_LOGI(TAG,
                 "Calibration %d%% — sample %lu:"
                 " RMS=%.2f CF=%.2f ROC=%.2f",
                 s_calib_progress,
                 s_calib_collected,
                 fv.rms_current,
                 fv.crest_factor,
                 fv.rate_of_change);
    }

    ESP_LOGI(TAG, "Building Isolation Forest on"
                  " %lu samples...", s_calib_collected);
    build_forest();

    s_calib_state    = CALIB_COMPLETE;
    s_calib_progress = 100;

    float sum = 0.0f;
    for (uint32_t i = 0; i < s_calib_collected; i++) {
        sum += s_train_data[i][0];
    }
    float calib_mean = (s_calib_collected > 0)
                       ? sum / s_calib_collected
                       : BASELINE_RMS_MEAN;

    system_state_lock();
    g_state.rolling_mean     = calib_mean;
    g_state.rolling_variance = 0.16f;
    g_state.sample_count     = s_calib_collected;
    system_state_unlock();

    ESP_LOGI(TAG, "Calibration complete —"
                  " baseline RMS=%.2fA", calib_mean);
}

// ─── Trigger calibration from outside ─────────────────
void detection_engine_start_calibration(void)
{
    s_calib_state  = CALIB_RUNNING;
    s_forest_built = false;
    ESP_LOGI(TAG, "Calibration triggered");
}

// ─── Init ──────────────────────────────────────────────
void detection_engine_init(void)
{
    srand(42);
    build_forest();
    ESP_LOGI(TAG, "Detection engine init done");
}

// ─── Detection Task ────────────────────────────────────
void detection_engine_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Detection task started on core %d",
             xPortGetCoreID());

    ESP_LOGI(TAG, "Auto-calibration starting"
                  " in 3 seconds...");
    ESP_LOGI(TAG, "Set load bank to NORMAL level now");
    vTaskDelay(pdMS_TO_TICKS(3000));
    run_calibration();

    feature_vector_t   fv;
    detection_result_t result;

    while (1) {
        if (s_calib_state == CALIB_RUNNING &&
            !s_forest_built) {
            run_calibration();
            continue;
        }

        if (xQueueReceive(g_state.feature_queue,
                          &fv,
                          pdMS_TO_TICKS(100)) != pdTRUE) {
            continue;
        }

        if (s_calib_state == CALIB_RUNNING) continue;

        // ── Dry run — skip all detection ──────────────
        if (fv.rms_current <= SIG_DRY_RUN_RMS_MAX) {
            result.zscore          = 0.0f;
            result.anomaly_score   = 0.0f;
            result.layer1_fired    = false;
            result.layer2_fired    = false;
            result.layer3_fired    = true;
            result.signature_match = FAULT_DRY_RUN;

            system_state_lock();
            g_state.zscore        = 0.0f;
            g_state.anomaly_score = 0.0f;
            system_state_unlock();

            xQueueSend(g_state.detection_queue,
                       &result, pdMS_TO_TICKS(10));
            continue;
        }

        // ── Normal range — skip L1 and L2 ─────────────
        if (fv.rms_current >= SIG_NORMAL_RMS_MIN &&
            fv.rms_current <= SIG_NORMAL_RMS_MAX) {
            result.zscore          = 0.0f;
            result.anomaly_score   = 0.0f;
            result.layer1_fired    = false;
            result.layer2_fired    = false;
            result.layer3_fired    = false;
            result.signature_match = FAULT_NONE;

            system_state_lock();
            g_state.zscore        = 0.0f;
            g_state.anomaly_score = 0.0f;
            system_state_unlock();

            xQueueSend(g_state.detection_queue,
                       &result, pdMS_TO_TICKS(10));
            continue;
        }

        // ── Outside normal range — run all layers ──────
        system_state_lock();
        float mean     = g_state.rolling_mean;
        float variance = g_state.rolling_variance /
                         (g_state.sample_count > 1
                             ? g_state.sample_count - 1
                             : 1);
        system_state_unlock();

        // ── Layer 1: Z-Score ──────────────────────────
        result.zscore       = compute_zscore(
                                  fv.rms_current,
                                  mean, variance);
        result.layer1_fired = (result.zscore >
                               g_state.config.zscore_threshold);

        // ── Layer 2: Isolation Forest ─────────────────
        result.anomaly_score = compute_anomaly_score(
                                   fv.rms_current,
                                   fv.crest_factor,
                                   fv.rate_of_change);
        result.layer2_fired  = (result.anomaly_score >
                                g_state.config.anomaly_threshold);

        // ── Layer 3: Signature Match ───────────────────
        result.signature_match = match_fault_signature(
                                     fv.rms_current,
                                     fv.crest_factor);
        result.layer3_fired    = (result.signature_match
                                  != FAULT_NONE);

        // ── Update global state ────────────────────────
        system_state_lock();
        g_state.zscore        = result.zscore;
        g_state.anomaly_score = result.anomaly_score;
        system_state_unlock();

        ESP_LOGD(TAG,
                 "Z=%.2f IF=%.2f SIG=%d"
                 " L1=%d L2=%d L3=%d",
                 result.zscore,
                 result.anomaly_score,
                 result.signature_match,
                 result.layer1_fired,
                 result.layer2_fired,
                 result.layer3_fired);

        xQueueSend(g_state.detection_queue,
                   &result, pdMS_TO_TICKS(10));
    }
}