// tools/dump.cpp
#include "itch/reader.hpp"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: dump <file.itch>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    struct stat st;
    fstat(fd, &st);

    auto* data = static_cast<uint8_t*>(std::malloc(st.st_size));
    if (!data) { perror("malloc"); return 1; }

    fprintf(stderr, "Loading %.2f GB into RAM...\n", (double)st.st_size / 1e9);
    {
        constexpr size_t CHUNK = 1ULL << 30;
        size_t remaining = st.st_size;
        uint8_t* dst = data;
        while (remaining > 0) {
            ssize_t n = read(fd, dst, remaining < CHUNK ? remaining : CHUNK);
            if (n <= 0) { perror("read"); return 1; }
            dst       += n;
            remaining -= (size_t)n;
        }
    }
    if (mlock(data, st.st_size) != 0)
        fprintf(stderr, "  warn: mlock failed (try sudo) — results may vary\n");
    else
        fprintf(stderr, "  Pages pinned in RAM (mlock ok).\n");
    fprintf(stderr, "Parsing...\n\n");

    itch::SoupBinReader reader(data, st.st_size);

    // Accumulate one field per message so the compiler cannot dead-code-eliminate
    // the entire dispatch chain. Uses timestamp_ns (present on every message type).
    uint64_t checksum = 0;

    auto t_start = std::chrono::high_resolution_clock::now();

    size_t total = reader.run([&](auto&& m) {
        checksum += m.timestamp_ns.value;
    });

    auto t_end = std::chrono::high_resolution_clock::now();
    double elapsed_s = std::chrono::duration<double>(t_end - t_start).count();

    fprintf(stderr, "═══════════════════════════════════════════\n");
    fprintf(stderr, "  dump  — pure parse throughput\n");
    fprintf(stderr, "═══════════════════════════════════════════\n");
    fprintf(stderr, "  total frames  : %zu\n",   total);
    fprintf(stderr, "  elapsed       : %.1f ms\n", elapsed_s * 1e3);
    fprintf(stderr, "  throughput    : %.2f M msg/s  (%.1f ns/msg)\n",
            total / elapsed_s / 1e6,
            elapsed_s * 1e9 / total);
    fprintf(stderr, "  checksum      : %llu\n", (unsigned long long)checksum);
    fprintf(stderr, "═══════════════════════════════════════════\n\n");

    std::free(data);
    close(fd);
    return 0;
}
