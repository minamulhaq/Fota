#include "is_ringbuffer.h"

#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) ||           \
	defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_BASE__) || \
	defined(__ARM_ARCH_8M_MAIN__)
#else
#include <stdio.h>
#endif

void ring_buffer_setup(ring_buffer_t *rb, uint8_t *buffer, uint32_t size)
{
	rb->buffer = buffer;
	rb->read_index = 0;
	rb->write_index = 0;

	/* Assumes size is power of 2*/
	rb->mask = size - 1;
}
bool ring_buffer_empty(ring_buffer_t *rb)
{
	return rb->read_index == rb->write_index;
}
bool ring_buffer_read(ring_buffer_t *rb, uint8_t *byte)
{
	uint32_t localreadindex = rb->read_index;
	uint32_t localwriteindex = rb->write_index;
	if (localreadindex == localwriteindex) {
		return false;
	}
	*byte = rb->buffer[localreadindex];
	localreadindex = (localreadindex + 1) & rb->mask;
	rb->read_index = localreadindex;
	return true;
}
bool ring_buffer_write(ring_buffer_t *rb, uint8_t byte)
{
	uint32_t localreadindex = rb->read_index;
	uint32_t localwriteindex = rb->write_index;

	uint32_t next_write_index = (localwriteindex + 1) & rb->mask;

	/* Drop latest data because buffer is full*/
	if (next_write_index == localreadindex) {
		return false;
	}

	rb->buffer[localwriteindex] = byte;
	rb->write_index = next_write_index;
	return true;
}

#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) ||           \
	defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_BASE__) || \
	defined(__ARM_ARCH_8M_MAIN__)
#else
void ring_buffer_print(const ring_buffer_t *rb)
{
	uint32_t r = rb->read_index;
	uint32_t w = rb->write_index;

	while (r != w) {
		uint8_t c = rb->buffer[r & rb->mask];
		printf("%02X ", c);
		r++;
	}
	printf("\n");
}
#endif
