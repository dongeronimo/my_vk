#pragma once
#include <array>
#include <vector>
/// <summary>
/// Formalizes the definition of the ring buffers. The ring buffers exist because of
/// the concept of frames in flight. Each frame has it's own buffer, that means descriptor sets,
/// uniform buffers, depth buffers, color buffers, etc...
/// </summary>
/// <typeparam name="T"></typeparam>
#ifdef RINGBUFFER_IS_ARRAY

template <typename T>
using ring_buffer_t = std::array<T, MAX_FRAMES_IN_FLIGHT>;

#endif
