#include "Utils.hpp"

#include <cassert>

using namespace std;

/* min_chunk_size actually does not guarantee every (chunk_size >= min_chunk_size), because imagine divideIntoChunks(1, 10, 2, 6) - we can divide that into {{1,6}, {7,10}}
 * but if we enforce min_chunk_size, that outputs {{1,10}}. Now image corner case like divideIntoChunks(0, INT_MAX, 2, (INT_MAX / 2 + 2)) - we could have 2 chunks of almost half INT_MAX,
 * but instead we get 1 chunk {0, INT_MAX}. Since this function is used to divide range into subranges for paralellism, first approach is much preferred. */
QVector<std::tuple<int, int>> divideIntoChunks(int x_from, int x_to, int max_chunk_count, int min_chunk_size)
{
    QVector<std::tuple<int, int>> chunks;
    if (x_from > x_to || max_chunk_count <= 0 || min_chunk_size < 1)
        return {};
    if (max_chunk_count == 1)
    {
        chunks.push_back(make_tuple(x_from, x_to));
        return chunks;
    }

    // (x_from == INT_MAX) causes overflow -> need to use int64_t for just this one special case...
    // beyond that, if (max_chunk_count > 1) -> (chunk_size < INT_MAX); thus only case of (max_chunk_count == 1) needs special handling, done above
    int64_t total = static_cast<int64_t>(x_to) - x_from + 1;
    if (total <= 0)
        return {};

    int chunk_count = total / min_chunk_size;
    int remainder = total % min_chunk_size;
    if (chunk_count + (remainder ? 1 : 0) <= max_chunk_count) // available amount of chunks <= max_chunk_count of min_chunk_size
    {
        int current = x_from;
        for (int i = 0; i < chunk_count; ++i)
        {
            int chunk_start = current;
            int chunk_end = current + min_chunk_size - 1;
            chunks.push_back(make_tuple(chunk_start, chunk_end));
            current = chunk_end + 1;
        }
        if (remainder)
            chunks.push_back(make_tuple(current, current + remainder - 1));
    }
    else // there will be max_chunk_count chunks of size >= min_chunk_size
    {
        remainder = total % max_chunk_count;
        int base_chunk_size = total / max_chunk_count;
        int current = x_from;
        for (int i = 0; i < max_chunk_count; ++i)
        {
            int chunk_size = base_chunk_size + (i < remainder ? 1 : 0);
            int chunk_start = current;
            int chunk_end = current + chunk_size - 1;
            chunks.push_back(make_tuple(chunk_start, chunk_end));
            current = chunk_end + 1;
        }
    }
    return chunks;
}
