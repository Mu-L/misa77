A lot of constants will be referenced ahead. Refer to [`src/format.h`](../src/format.h) for more information about them.

Assumptions:

- The size of the raw file is `<= 1e17` bytes.
- Everything is little-endian.

The format is as follows:

- The first 8 bytes contain an unsigned 64-bit integer `src_size`, which is equal to the size of the uncompressed data in bytes.
- If `src_size <= small_lim` bytes, we simply write the raw source.
- Otherwise:
    - The next 8 bytes contain another unsigned 64-bit integer `literal_suffix_cnt`, which is equal to the length of the suffix of source (in bytes) that we're *not* going to compress (ie. we will append these raw bytes to the end of the compressed format). It's guaranteed that `literal_suffix_cnt >= literal_suffix` (constant from [`src/format.h`](../src/format.h)).
    - The bytes that follow correspond to a sequence of disjoint "blocks".
    - Each block contains four pieces of data: 
        - Literal length.
        - Raw literals.
        - Match length.
        - Offset to the position behind us from where the match begins.
    - The decompressor will read these disjoint blocks from left to right, and perform two operations for each block:
        - Append the sequence of raw literals to the bytes already written in the decompression buffer.
        - Append the specified match to the bytes already written in the decompression buffer.
    - Each block is stored in the following format:
        - A "control sub-block", which contains the literal length, match length, and match offset for the block. The byte-level decomposition is `[1 token byte][2 match offset bytes][variable number of extra literal length bytes]`.
            - The token byte is divided into two parts : `[top 3 bits = l'][bottom 5 bits = m']`.
            - The match length is defined as `m = m' + min_match_len - 1`. It's guaranteed that `min_match_len <= m <= max_match_len`.
            - The 2 match offset bytes contain a 16-bit unsigned integer `d'`, with the actual distance to the match being defined as `d' + hashtab_lag + 1`. 
            - The literal length `l` is derived through the following algorithm:
                - If `l' < 7`, then `l = l'`.
                - Otherwise, we read extra literal length bytes until we encounter the first byte with value `< 255`. We define `l = l' + 255 * (number of 255-value bytes) + (value of final byte)` (the value of a byte is just its interpretation as an unsigned 8 bit integer here).
            - Note that each "control sub-block" always contains the token byte and 2 match offset bytes.
        - A "raw literal sub-block", which just contains the raw literal bytes to be copied by the decompressor when processing this block. This sub-block contains literal bytes in the order in which they will be copied by the decompressor.
    - Now, the two sub-blocks for a block aren't stored contiguously. Instead, we have two separate streams for control sub-blocks and literal sub-blocks. To be precise:
        - The control sub-block stream grows from left to right, and the literal sub-block stream grows from right to left.
        - The complete block stream takes the following form: `[control sub-block for block 1][control sub-block for block 2]...[control sub-block for block n][literal sub-block for block n]...[literal sub-block for block 2][literal sub-block for block 1]`. 
    - At the end, we append the previously mentioned suffix of raw bytes. It is guaranteed that the number of literal bytes in this suffix are >= `literal_suffix`.
- Therefore, when `src_size > small_lim`, the complete bitstream has the following form:

```text
[src_size (8 bytes)]
[literal_suffix_cnt (8 bytes)]
[control sub-block for block 1 (>= 3 bytes)]
[control sub-block for block 2 (>= 3 bytes)]
.
.
.
[control sub-block for block n ( >= 3 bytes)]
[literal sub-block for block n (>= 0 bytes)]
.
.
.
[literal sub-block for block 2 (>= 0 bytes)]
[literal sub-block for block 1 (>= 0 bytes)]
[raw literal suffix bytes (>= literal_suffix bytes)]
```

Some constraints:

- If copying a match involves copying range `[x, x + n)` to `[y, y + n)`, then we must have `hashtab_lag + 1 <= y - x <= hashtab_lag + dis_lim`.