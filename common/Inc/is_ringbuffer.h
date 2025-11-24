#ifndef __RING_BUFFER_IS_RINGBUFFER_H_
#define __RING_BUFFER_IS_RINGBUFFER_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct ring_buffer {
	uint8_t *buffer;
	uint32_t mask;
	uint32_t read_index;
	uint32_t write_index;
} ring_buffer_t;

void ring_buffer_setup(ring_buffer_t *rb, uint8_t *buffer, uint32_t size);
bool ring_buffer_empty(ring_buffer_t *rb);
bool ring_buffer_write(ring_buffer_t *rb, uint8_t byte);
bool ring_buffer_read(ring_buffer_t *rb, uint8_t *byte);

#if defined(__ARM_ARCH_6M__) && defined(__ARM_ARCH_7M__) &&           \
	defined(__ARM_ARCH_7EM__) && defined(__ARM_ARCH_8M_BASE__) && \
	defined(__ARM_ARCH_8M_MAIN__)
#else
void ring_buffer_print(const ring_buffer_t *rb);
#endif

#endif // __RING_BUFFER_IS_RINGBUFFER_H_
