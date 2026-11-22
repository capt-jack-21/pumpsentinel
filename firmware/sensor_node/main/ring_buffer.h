#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ─── Ring Buffer Config ────────────────────────────────
#define RING_BUFFER_SIZE 16384   // must be power of 2

// ─── Ring Buffer Struct ────────────────────────────────
typedef struct {
    float             samples[RING_BUFFER_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
    SemaphoreHandle_t mutex;
} ring_buffer_t;

// ─── API ───────────────────────────────────────────────
void ring_buffer_init(ring_buffer_t *rb);
bool ring_buffer_write(ring_buffer_t *rb, float sample);
bool ring_buffer_read(ring_buffer_t *rb, float *sample);
bool ring_buffer_read_batch(ring_buffer_t *rb, float *out, uint32_t count);
uint32_t ring_buffer_count(ring_buffer_t *rb);
void ring_buffer_clear(ring_buffer_t *rb);