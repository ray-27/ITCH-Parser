#pragma once
#include "parser.hpp"
#include <cstdint>

namespace itch {

    class SoupBinReader {
        private:
            const uint8_t* cursor_;
            const uint8_t* end_;

        public:
            SoupBinReader(const uint8_t* buf, size_t size) : cursor_(buf), end_(buf + size) {}

            template<typename Handler>
            size_t run(Handler&& handler) {
                size_t count = 0;

                while(cursor_ + 2 <= end_) {
                    // Prefetch ~512 bytes ahead into L2 cache. Memory latency
                    // is ~80-100 ns; at target throughput that covers ~15 msgs.
                    __builtin_prefetch(cursor_ + 512, 0, 1);

                    uint16_t msg_len = (static_cast<uint16_t>(cursor_[0]) << 8) | static_cast<uint16_t>(cursor_[1]);
                    cursor_ += 2;

                    if(cursor_ + msg_len > end_) break;

                    dispatch(cursor_, msg_len, handler);

                    cursor_ += msg_len;
                    ++count;
                }
                return count;
            }
    };
}
