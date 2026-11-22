#include "ring_buffer.h"
#include <string.h>

// ─── Init ──────────────────────────────────────────────
void ring_buffer_init(ring_buffer_t *rb)
{
    memset(rb->samples, 0, sizeof(rb->samples));
    rb->head  = 0;
    rb->tail  = 0;
    rb->count = 0;
    rb->mutex = xSemaphoreCreateMutex();
}

// ─── Write single sample ───────────────────────────────
bool ring_buffer_write(ring_buffer_t *rb, float sample)
{
    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(5)) != pdTRUE) {
        return false;
    }

    rb->samples[rb->head] = sample;
    rb->head = (rb->head + 1) & (RING_BUFFER_SIZE - 1);

    if (rb->count < RING_BUFFER_SIZE) {
        rb->count++;
    } else {
        // Overwrite oldest — advance tail
        rb->tail = (rb->tail + 1) & (RING_BUFFER_SIZE - 1);
    }

    xSemaphoreGive(rb->mutex);
    return true;
}

// ─── Read single sample ────────────────────────────────
bool ring_buffer_read(ring_buffer_t *rb, float *sample)
{
    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(5)) != pdTRUE) {
        return false;
    }

    if (rb->count == 0) {
        xSemaphoreGive(rb->mutex);
        return false;
    }

    *sample  = rb->samples[rb->tail];
    rb->tail = (rb->tail + 1) & (RING_BUFFER_SIZE - 1);
    rb->count--;

    xSemaphoreGive(rb->mutex);
    return true;
}

// ─── Read batch of samples ─────────────────────────────
bool ring_buffer_read_batch(ring_buffer_t *rb, float *out, uint32_t count)
{
    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return false;
    }

    if (rb->count < count) {
        xSemaphoreGive(rb->mutex);
        return false;  // Not enough samples yet
    }

    for (uint32_t i = 0; i < count; i++) {
        out[i]   = rb->samples[rb->tail];
        rb->tail = (rb->tail + 1) & (RING_BUFFER_SIZE - 1);
    }
    rb->count -= count;

    xSemaphoreGive(rb->mutex);
    return true;
}

// ─── Get current count ─────────────────────────────────
uint32_t ring_buffer_count(ring_buffer_t *rb)
{
    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(5)) != pdTRUE) {
        return 0;
    }
    uint32_t c = rb->count;
    xSemaphoreGive(rb->mutex);
    return c;
}

// ─── Clear buffer ──────────────────────────────────────
void ring_buffer_clear(ring_buffer_t *rb)
{
    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }
    rb->head  = 0;
    rb->tail  = 0;
    rb->count = 0;
    xSemaphoreGive(rb->mutex);
}