// misa77 - A codec optimized for decompression throughput
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Shreyas Ghildiyal <nonadhocproblems@gmail.com>

// AVX2 TU.
// Built only on x86 targets and the implementations herein are chosen at dispatch iff the cpu has
// AVX2 support.

#include "compress_dispatch.h"
#include "compress_impl.h"
#include "decompress_dispatch.h"
#include "decompress_impl.h"
#include "isa/lib_avx2.h"

#include <cstdint>

namespace misa77
{
    uint64_t compress_avx2(const uint8_t* __restrict src,
                           uint64_t src_size,
                           uint8_t* __restrict dst,
                           uint64_t dst_cap)
    {
        return compress_impl<lib_avx2>(src, src_size, dst, dst_cap);
    }

    uint64_t decompress_avx2(const uint8_t* __restrict src,
                             uint64_t src_size,
                             uint8_t* __restrict dst,
                             uint64_t dst_cap)
    {
        return decompress_impl<lib_avx2>(src, src_size, dst, dst_cap);
    }
} // namespace misa77
