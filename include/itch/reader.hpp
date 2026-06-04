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
                    //shifting the first 8 bits to the left to add it with the other 8 bits to find the length
                    uint16_t msg_len = (static_cast<uint16_t>(cursor_[0]) << 8) | static_cast<uint16_t>(cursor_[1]);
                    cursor_ += 2;

                    // Guard against malformed frames
                    if(cursor_ + msg_len > end_) break;

                    auto result = parse(cursor_, msg_len);
                    if(result.has_value()) {
                        handler(result.value());
                    }

                    cursor_ += msg_len;
                    ++count;
                }
                return count;

            }
    };
}
