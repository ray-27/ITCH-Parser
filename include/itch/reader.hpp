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
                // Local variables let the compiler keep both pointers in
                // registers for the entire loop without aliasing concerns
                // from the [&] lambda capture of `this`.
                const uint8_t* __restrict__ cur = cursor_;
                const uint8_t* const        end = end_;
                size_t count = 0;

                while (cur + 2 <= end) {
                    __builtin_prefetch(cur + 512, 0, 1);

                    uint16_t msg_len = (static_cast<uint16_t>(cur[0]) << 8)
                                     |  static_cast<uint16_t>(cur[1]);
                    cur += 2;

                    // Only fires once at EOF — never taken in the hot loop.
                    if (__builtin_expect(cur + msg_len > end, 0)) break;

                    dispatch(cur, msg_len, handler);

                    cur += msg_len;
                    ++count;
                }

                cursor_ = cur;
                return count;
            }
    };
}
