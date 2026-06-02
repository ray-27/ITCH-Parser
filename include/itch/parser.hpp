#pragma once
#include "fields.hpp"
#include "messages.hpp"
#include "types.hpp"
#include <cstdint>
#include <cstring> // memcpy
#include <optional>

namespace itch {
    //ITCH is a Big-endian. x86 is little endian. We must byte swap every multi byte integer field we read off the wire

    // inline uint16_t read_u16(const uint8_t* p) {
    //     uint16_t v;
    //     memcpy(&v, p, 2);
    //     return __builtin_bswap16(v);
    // }

    // inline uint16_t read_u32(const uint8_t* p) {
    //     uint16_t v;
    //     memcpy(&v, p, 2);
    //     return __builtin_bswap16(v);
    // }
    // These functions can be compressed into a template

    template<typename T>
    struct underlying_int {
        using type = T;
    };
    template<typename T, typename Tag>
    struct underlying_int<StrongType<T, Tag>> {
        using type = T;
    };
    template<typename T>
    using underlying_int_t = typename underlying_int<T>::type;

    template<typename T>
    inline T read_be(const uint8_t* p) {
        using Raw = underlying_int_t<T>;

        Raw v;
        memcpy(&v, p, sizeof(Raw));

        Raw swapped;
        if constexpr (sizeof(Raw) == 1) swapped = v;
        if constexpr (sizeof(Raw) == 2) swapped = __builtin_bswap16(v);
        if constexpr (sizeof(Raw) == 4) swapped = __builtin_bswap32(v);
        if constexpr (sizeof(Raw) == 8) swapped = __builtin_bswap64(v);

        return T{swapped};
    }
    // template<typename T>
    // inline T read_be(const uint8_t* p) {
    //     T v;
    //     memcpy(&v, p, sizeof(T));

    //     if constexpr (sizeof(T) == 1) return v;
    //     if constexpr (sizeof(T) == 2) return static_cast<T>(__builtin_bswap16(v));
    //     if constexpr (sizeof(T) == 4) return static_cast<T>(__builtin_bswap32(v));
    //     if constexpr (sizeof(T) == 8) return static_cast<T>(__builtin_bswap64(v));
    // }

    // Stays as-is — 6-byte timestamp has no generic solution
    // There's no standard integer type that's 6 bytes, so the template can't handle it cleanly. Keeping it explicit
    inline uint64_t read_u48(const uint8_t* p) {
        uint64_t v = 0;
        memcpy(reinterpret_cast<uint8_t*>(&v) + 2, p, 6);
        return __builtin_bswap64(v);
    }

    template<size_t N>
    inline Alpha<N> read_alpha(const uint8_t* p) {
        Alpha<N> a;
        memcpy(a.data(), p, N);
        return a;
    }

    // Per message parse functions
    inline SystemEvent parse_system_event(const uint8_t* buf) {
        return SystemEvent {
            // .stock_locate = Locate{read_be<uint16_t>(buf + 1)} //also works
            .stock_locate = read_be<Locate>(buf + 1),
            .tracking_number = read_be<TrackingNumber>(buf + 3),
            .timestamp_ns = Timestamp{read_u48(buf + 5)},
            .event_code = static_cast<EventCode>(buf[11]),
        };
    }

    inline AddOrder parse_add_order(const uint8_t* buf) {
        return AddOrder {
            .stock_locate = read_be<Locate>(buf + 1),
            .tracking_number = read_be<TrackingNumber>(buf + 3),
            .timestamp_ns = Timestamp{read_u48(buf + 5)},
            .order_ref = read_be<OrderRef>(buf + 11),
            .side = static_cast<Side>(buf[19]),
            .shares = read_be<Shares>(buf + 20),
            .stock = read_alpha<8>(buf + 24),
            .price = read_be<Price4>(buf + 32),
        };
    }

    inline std::optional<ITCHMessage> parse(const uint8_t* buf, uint16_t len) {
        const MessageType msg_type = static_cast<MessageType>(buf[0]);

        switch(msg_type) {
            case MessageType::SystemEvent:
                return parse_system_event(buf);
            case MessageType::AddOrder:
                return parse_add_order(buf);
            default:
                return std::nullopt;
        }
    }
}
