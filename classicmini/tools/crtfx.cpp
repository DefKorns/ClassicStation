/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define CRTFX_EXPORT extern "C" __attribute__((visibility("default")))

namespace {

constexpr const char* kDefaultSession = "/var/lib/hakchi/emulationstation/crtfx.session";
constexpr unsigned long kFbPanDisplay = 0x4606;
constexpr std::array<const char*, 2> kTargets{{"canoe-shvc", "kachikachi"}};

struct Journal {
    int fd;
    pthread_mutex_t lock;
    std::chrono::steady_clock::time_point origin;
};

Journal gJournal{-1, PTHREAD_MUTEX_INITIALIZER, std::chrono::steady_clock::time_point()};
bool gActive = false;
std::array<char, 160> gSessionPath;
std::atomic<std::uint64_t> gFrames{0};
std::atomic<std::uint64_t> gClaims{0};
std::uint64_t gFilterCapUs = 1500000;
int gSlowLimit = 2;
int gFilterLevel = 0;
std::atomic<std::uint64_t> gFilterNs{0};
std::atomic<bool> gFilterOff{false};
bool gMaskOn = false;
std::atomic<bool> gMaskOff{false};
std::atomic<std::uint64_t> gMaskNs{0};
std::atomic<unsigned> gLoggedGains{0};
std::atomic<bool> gStop{false};

void makeDirectory(const char* file)
{
    std::array<char, 160> dir;
    std::snprintf(dir.data(), dir.size(), "%s", file);
    char* slash = std::strrchr(dir.data(), '/');
    if (slash == nullptr || slash == dir.data()) {
        return;
    }
    *slash = '\0';
    ::mkdir(dir.data(), 0755);
}

void journalOpen(const char* path)
{
    makeDirectory(path);
    gJournal.fd = ::open(path, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
}

void journalSync()
{
    if (gJournal.fd >= 0) {
        ::fdatasync(gJournal.fd);
    }
}

void journal(const char* format, ...) __attribute__((format(printf, 1, 2)));

void journal(const char* format, ...)
{
    if (gJournal.fd < 0) {
        return;
    }
    std::array<char, 384> line;
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - gJournal.origin).count();
    int used = std::snprintf(line.data(), line.size(), "[%8.3f] ", seconds);
    va_list args;
    va_start(args, format);
    used += std::vsnprintf(line.data() + used, line.size() - static_cast<std::size_t>(used) - 1, format, args);
    va_end(args);
    used = std::min(used, static_cast<int>(line.size()) - 2);
    line[static_cast<std::size_t>(used)] = '\n';

    pthread_mutex_lock(&gJournal.lock);
    const ssize_t written = ::write(gJournal.fd, line.data(), static_cast<std::size_t>(used) + 1);
    pthread_mutex_unlock(&gJournal.lock);
    static_cast<void>(written);
}

bool isTarget(const char* exe)
{
    const char* slash = std::strrchr(exe, '/');
    const char* name = slash != nullptr ? slash + 1 : exe;
    return std::any_of(kTargets.begin(), kTargets.end(), [name](const char* target) { return std::strcmp(name, target) == 0; });
}

constexpr std::size_t kMaxSweep = 32;
constexpr const char* kSweepTrigger = "/tmp/crtfx.go";

struct Sweep {
    int period;
    int delay;
    std::size_t count;
    std::array<std::array<float, 4>, kMaxSweep> gains;
};

Sweep gSweep;
std::atomic<int> gSweepIndex{-1};
std::atomic<long long> gSweepStart{-1};

void sweepTick()
{
    const long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - gJournal.origin).count();
    long long start = gSweepStart.load();
    if (start < 0) {
        if (::access(kSweepTrigger, F_OK) != 0) {
            return;
        }
        start = now;
        gSweepStart.store(start);
        journal("sweep start: %zu profiles, %d s each after %d s", gSweep.count, gSweep.period, gSweep.delay);
    }
    const long long since = (now - start) / 1000 - gSweep.delay;
    int index = -1;
    if (since >= 0) {
        index = static_cast<int>((since / gSweep.period) % static_cast<long long>(gSweep.count));
    }
    if (index == gSweepIndex.load()) {
        return;
    }
    gSweepIndex.store(index);
    if (index >= 0) {
        const std::array<float, 4>& g = gSweep.gains[static_cast<std::size_t>(index)];
        journal("sweep profile %d: %.0f,%.0f,%.0f,%.0f", index, static_cast<double>(g[0]), static_cast<double>(g[1]), static_cast<double>(g[2]), static_cast<double>(g[3]));
    }
}

long readNumber(const char* text, const char* key)
{
    const char* found = std::strstr(text, key);
    return found != nullptr ? std::strtol(found + std::strlen(key), nullptr, 10) : -1;
}

bool readSmall(const char* path, std::array<char, 1024>& out)
{
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    const ssize_t count = ::read(fd, out.data(), out.size() - 1);
    ::close(fd);
    out[static_cast<std::size_t>(std::max<ssize_t>(count, 0))] = '\0';
    return count > 0;
}

void sessionOpen()
{
    const char* path = std::getenv("CRTFX_SESSION");
    std::snprintf(gSessionPath.data(), gSessionPath.size(), "%s", path != nullptr ? path : kDefaultSession);
    std::array<char, 1024> previous;
    if (readSmall(gSessionPath.data(), previous)) {
        journal("previous session did not end cleanly: %.80s", previous.data());
    }
    const int fd = ::open(gSessionPath.data(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) {
        journal("cannot write the session marker");
        return;
    }
    std::array<char, 64> text;
    const int length = std::snprintf(text.data(), text.size(), "pid %d\n", static_cast<int>(::getpid()));
    const ssize_t written = ::write(fd, text.data(), static_cast<std::size_t>(length));
    static_cast<void>(written);
    ::fdatasync(fd);
    ::close(fd);
}

void* heartbeat(void*)
{
    std::uint64_t last_frames = 0;
    std::uint64_t last_claims = 0;
    std::uint64_t last_filter_ns = 0;
    std::uint64_t last_mask_ns = 0;
    int slow_seconds = 0;
    while (!gStop.load()) {
        const timespec second{1, 0};
        ::nanosleep(&second, nullptr);
        std::array<char, 1024> memory;
        std::array<char, 1024> load;
        const bool logging = gJournal.fd >= 0;
        const bool have_memory = logging && readSmall("/proc/meminfo", memory);
        const bool have_load = logging && readSmall("/proc/loadavg", load);
        const std::uint64_t frames = gFrames.load();
        if (gSweep.count > 0) {
            sweepTick();
        }
        journal("beat frames %llu (+%llu) free %ld kB cached %ld kB load %.4s", static_cast<unsigned long long>(frames),
                static_cast<unsigned long long>(frames - last_frames), have_memory ? readNumber(memory.data(), "MemFree:") : -1L,
                have_memory ? readNumber(memory.data(), "Cached:") : -1L, have_load ? load.data() : "?");
        const std::uint64_t claims = gClaims.load();
        journal("claims %llu (+%llu)", static_cast<unsigned long long>(claims), static_cast<unsigned long long>(claims - last_claims));
        last_claims = claims;
        const std::uint64_t filter_ns = gFilterNs.load();
        std::uint64_t spent_us = (filter_ns - last_filter_ns) / 1000;
        last_filter_ns = filter_ns;
        const std::uint64_t mask_ns = gMaskNs.load();
        const std::uint64_t mask_us = (mask_ns - last_mask_ns) / 1000;
        last_mask_ns = mask_ns;
        journal("mask %llu us in the last second", static_cast<unsigned long long>(mask_us));
        spent_us += mask_us;
        journal("filter %llu us in the last second", static_cast<unsigned long long>(spent_us));
        const std::uint64_t frame_step = frames - last_frames;
        const bool mask_running = gMaskOn && !gMaskOff.load();
        const bool filter_running = gFilterLevel > 0 && !gFilterOff.load();
        if (mask_running || filter_running) {
            slow_seconds = (frame_step > 0 && frame_step < 50) ? slow_seconds + 1 : 0;
            if (slow_seconds >= gSlowLimit || spent_us > gFilterCapUs) {
                slow_seconds = 0;
                if (mask_running) {
                    gMaskOff.store(true);
                    journal("mask off: %llu frames in the last second, %llu us of filtering", static_cast<unsigned long long>(frame_step),
                            static_cast<unsigned long long>(spent_us));
                } else {
                    gFilterOff.store(true);
                    journal("filter off: %llu frames in the last second, %llu us of filtering", static_cast<unsigned long long>(frame_step),
                            static_cast<unsigned long long>(spent_us));
                }
            }
        }
        journalSync();
        last_frames = frames;
    }
    return nullptr;
}

void heartbeatStart()
{
    pthread_t thread;
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread, &attributes, heartbeat, nullptr) != 0) {
        journal("cannot start the heartbeat thread");
    }
    pthread_attr_destroy(&attributes);
}

struct CanoeBuild {
    std::uint64_t fingerprint;
    std::array<std::uintptr_t, 4> returns;
};

constexpr std::array<CanoeBuild, 3> kCanoeBuilds{{
    {0x3dbdb051edd4da6aULL, {{0x1632f8, 0x163324, 0x16334c, 0x163374}}},
    {0xb1a356324c106d56ULL, {{0x169f20, 0x169f44, 0x169f68, 0x169f8c}}},
    {0xb51ef72901909b9aULL, {{0x16a600, 0x16a62c, 0x16a654, 0x16a67c}}},
}};

constexpr std::array<float, 4> kGainScale{{16384.0F, 64.0F, 16384.0F, 64.0F}};

const CanoeBuild* gBuild = nullptr;
bool gHasGains = false;
std::array<float, 4> gGains{{0.0F, 0.0F, 0.0F, 0.0F}};

std::uint64_t fingerprintFile(const char* path)
{
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return 0;
    }
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    std::array<unsigned char, 4096> buffer;
    for (ssize_t count = ::read(fd, buffer.data(), buffer.size()); count > 0; count = ::read(fd, buffer.data(), buffer.size())) {
        for (ssize_t i = 0; i < count; ++i) {
            hash ^= buffer[static_cast<std::size_t>(i)];
            hash *= 0x100000001b3ULL;
        }
    }
    ::close(fd);
    return hash;
}

std::uint64_t gPrint = 0;

void identifyBuild(const char* exe)
{
    const char* slash = std::strrchr(exe, '/');
    if (std::strcmp(slash != nullptr ? slash + 1 : exe, "canoe-shvc") == 0) {
        gPrint = fingerprintFile(exe);
    }
}

void selectBuild()
{
    if (gPrint == 0) {
        return;
    }
#ifdef CRTFX_PROBE_ANY_SITE
    gBuild = &kCanoeBuilds[0];
    journal("canoe build: test build, the first one is assumed");
#else
    const std::uint64_t print = gPrint;
    const auto build = std::find_if(kCanoeBuilds.begin(), kCanoeBuilds.end(), [print](const CanoeBuild& b) { return b.fingerprint == print; });
    if (build == kCanoeBuilds.end()) {
        journal("canoe build not recognised (%016llx): the floorf hook stays out of the way", static_cast<unsigned long long>(print));
        return;
    }
    gBuild = &*build;
    journal("canoe build recognised (%016llx)", static_cast<unsigned long long>(print));
#endif
}

void gainsSelect()
{
    const char* gains = std::getenv("CRTFX_GAINS");
    float v[4];
    if (gains == nullptr || std::sscanf(gains, "%f,%f,%f,%f", &v[0], &v[1], &v[2], &v[3]) != 4) {
        journal("no gains requested");
        return;
    }
    if (gBuild == nullptr) {
        return;
    }
    std::copy(v, v + 4, gGains.begin());
    gHasGains = true;
    journal("gains %.0f,%.0f,%.0f,%.0f", static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]), static_cast<double>(v[3]));
}

void sweepSelect()
{
    const char* spec = std::getenv("CRTFX_SWEEP");
    if (spec == nullptr || gBuild == nullptr) {
        return;
    }
    int period = 0;
    int delay = 0;
    int used = 0;
    if (std::sscanf(spec, "%d:%d:%n", &period, &delay, &used) < 2 || used == 0) {
        journal("sweep: cannot read the setting");
        return;
    }
    const char* cursor = spec + used;
    while (*cursor != '\0' && gSweep.count < kMaxSweep) {
        float v[4];
        int consumed = 0;
        if (std::sscanf(cursor, "%f,%f,%f,%f%n", &v[0], &v[1], &v[2], &v[3], &consumed) != 4) {
            break;
        }
        std::copy(v, v + 4, gSweep.gains[gSweep.count].begin());
        ++gSweep.count;
        cursor += consumed;
        if (*cursor != '/') {
            break;
        }
        ++cursor;
    }
    gSweep.period = std::max(period, 1);
    gSweep.delay = std::max(delay, 0);
    journal("sweep: %zu profiles, %d s each, waiting for %s", gSweep.count, gSweep.period, kSweepTrigger);
}

struct ProbeSites {
    std::uint64_t fingerprint;
    std::uintptr_t first_claim;
    std::uintptr_t loop_claim;
};

constexpr std::array<ProbeSites, 1> kProbeSites{{{0x3dbdb051edd4da6aULL, 0x16312c, 0x163210}}};

constexpr std::ptrdiff_t kTaskCounter = 0x2c;

const ProbeSites* gProbe = nullptr;
std::atomic<std::uint64_t> gLastTask{0};
pthread_mutex_t gProbeLock = PTHREAD_MUTEX_INITIALIZER;
std::array<std::uint64_t, 8> gSeenTasks{{0, 0, 0, 0, 0, 0, 0, 0}};
std::size_t gSeenCount = 0;

void probeSelect()
{
#ifdef CRTFX_PROBE_ANY_SITE
    gProbe = &kProbeSites[0];
    journal("probe: test build, every SDL_AtomicAdd counts as a claim");
#else
    const std::uint64_t print = gPrint;
    const auto site = std::find_if(kProbeSites.begin(), kProbeSites.end(), [print](const ProbeSites& s) { return s.fingerprint == print; });
    if (site == kProbeSites.end()) {
        journal("probe: no claim sites known for this canoe build, the hook stays out of the way");
        return;
    }
    gProbe = &*site;
    journal("probe: claim sites %#lx and %#lx", static_cast<unsigned long>(site->first_claim), static_cast<unsigned long>(site->loop_claim));
#endif
}

std::atomic<unsigned> gPixelDumps{0};

void probePixels(const unsigned char* task)
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t source = 0;
    std::memcpy(&source, task + 8, sizeof source);
    std::memcpy(&width, task + 12, sizeof width);
    std::memcpy(&height, task + 16, sizeof height);
    if ((width != 256 && width != 512) || height < 100 || height > 480 || source < 0x10000) {
        return;
    }
    const std::array<std::array<std::size_t, 2>, 2> spots{{{{height / 2, width / 4}}, {{height / 4, width / 2}}}};
    for (const auto& spot : spots) {
        std::array<std::uint32_t, 8> pixels;
        std::memcpy(pixels.data(), reinterpret_cast<const unsigned char*>(static_cast<std::uintptr_t>(source)) + (spot[0] * width + spot[1]) * sizeof(std::uint32_t), sizeof pixels);
        journal("probe source row %zu from column %zu: %08x %08x %08x %08x %08x %08x %08x %08x", spot[0], spot[1], pixels[0], pixels[1], pixels[2],
                pixels[3], pixels[4], pixels[5], pixels[6], pixels[7]);
    }
}

void probeTask(const unsigned char* task)
{
    std::uint64_t key = 0xcbf29ce484222325ULL;
    for (std::size_t i = 12; i < 25; ++i) {
        key = (key ^ task[i]) * 0x100000001b3ULL;
    }
    if (gLastTask.load(std::memory_order_relaxed) == key) {
        return;
    }
    pthread_mutex_lock(&gProbeLock);
    const bool known = std::find(gSeenTasks.begin(), gSeenTasks.begin() + static_cast<std::ptrdiff_t>(gSeenCount), key) != gSeenTasks.begin() + static_cast<std::ptrdiff_t>(gSeenCount);
    if (!known && gSeenCount < gSeenTasks.size()) {
        gSeenTasks[gSeenCount++] = key;
    }
    pthread_mutex_unlock(&gProbeLock);
    gLastTask.store(key, std::memory_order_relaxed);
    if (known) {
        return;
    }

    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t destination = 0;
    std::uint32_t source = 0;
    std::memcpy(&destination, task + 4, sizeof destination);
    std::memcpy(&source, task + 8, sizeof source);
    std::memcpy(&width, task + 12, sizeof width);
    std::memcpy(&height, task + 16, sizeof height);
    journal("probe task: destination %#x source %#x size %ux%u flags %02x %02x %02x %02x %02x", static_cast<unsigned>(destination),
            static_cast<unsigned>(source), width, height, task[20], task[21], task[22], task[23], task[24]);
    if ((width != 256 && width != 512) || height < 100 || height > 480 || source < 0x10000) {
        journal("probe task: values look unusual, source not read");
        return;
    }
    probePixels(task);
}

struct FilterLevel {
    int luma;
    int chroma_i;
    int chroma_q;
};

constexpr std::array<FilterLevel, 3> kFilterLevels{{{0, 2, 3}, {1, 2, 3}, {1, 3, 5}}};
constexpr int kMaxRadius = 12;
constexpr int kMaxLines = 512;
constexpr int kMaxWidth = 512;
constexpr int kPadded = kMaxWidth + 2 * kMaxRadius;
constexpr std::array<std::uint8_t, 4> kMark{{0x5a, 0xc3, 0x7e, 0x91}};

struct Kernel {
    int radius;
    std::array<int, 2 * kMaxRadius + 1> weights;
};

bool gSwapRb = false;
std::array<std::array<Kernel, 3>, 2> gKernels;
int gLineLock[kMaxLines];

Kernel makeKernel(int radius)
{
    Kernel kernel;
    std::memset(&kernel, 0, sizeof kernel);
    kernel.radius = std::min(std::max(radius, 0), kMaxRadius);
    int total = 0;
    for (int k = -kernel.radius; k <= kernel.radius; ++k) {
        total += kernel.radius + 1 - std::abs(k);
    }
    int sum = 0;
    for (int k = -kernel.radius; k <= kernel.radius; ++k) {
        const int weight = (kernel.radius + 1 - std::abs(k)) * 1024 / total;
        kernel.weights[static_cast<std::size_t>(k + kernel.radius)] = weight;
        sum += weight;
    }
    kernel.weights[static_cast<std::size_t>(kernel.radius)] += 1024 - sum;
    return kernel;
}

void filterSelect()
{
    const char* level = std::getenv("CRTFX_FILTER");
    const int wanted = level != nullptr ? std::atoi(level) : 0;
    if (wanted <= 0) {
        journal("filter: off");
        return;
    }
    if (gProbe == nullptr) {
        journal("filter: the canoe build is not recognised, off");
        return;
    }
    gFilterLevel = std::min(wanted, static_cast<int>(kFilterLevels.size()));
    const char* order = std::getenv("CRTFX_ORDER");
    gSwapRb = order != nullptr && std::strcmp(order, "bgr") == 0;
    const FilterLevel& spec = kFilterLevels[static_cast<std::size_t>(gFilterLevel - 1)];
    for (int scale = 1; scale <= 2; ++scale) {
        gKernels[static_cast<std::size_t>(scale - 1)][0] = makeKernel(spec.luma * scale);
        gKernels[static_cast<std::size_t>(scale - 1)][1] = makeKernel(spec.chroma_i * scale);
        gKernels[static_cast<std::size_t>(scale - 1)][2] = makeKernel(spec.chroma_q * scale);
    }
    journal("filter: level %d (luma %d, I %d, Q %d), colour order %s", gFilterLevel, spec.luma, spec.chroma_i, spec.chroma_q,
            gSwapRb ? "bgr" : "rgb");
}

bool isMarked(const std::array<std::uint32_t, 4>& head)
{
    for (std::size_t k = 0; k < kMark.size(); ++k) {
        if ((head[k] >> 24) != kMark[k]) {
            return false;
        }
    }
    return true;
}

void markLine(std::uint32_t* line)
{
    for (std::size_t k = 0; k < kMark.size(); ++k) {
        line[k] = (line[k] & 0x00ffffffU) | (static_cast<std::uint32_t>(kMark[k]) << 24);
    }
}

int clampByte(int value)
{
    return std::min(std::max(value, 0), 255);
}

template <int R>
void blurFixed(const int* padded, int* out, int width, const int* weights)
{
    for (int x = 0; x < width; ++x) {
        int sum = 0;
        for (int k = -R; k <= R; ++k) {
            sum += weights[k + R] * padded[x + k];
        }
        out[x] = sum >> 10;
    }
}

void blur(const int* padded, int* out, int width, const Kernel& kernel)
{
    const int* const weights = kernel.weights.data();
    switch (kernel.radius) {
    case 0:
        for (int x = 0; x < width; ++x) {
            out[x] = padded[x];
        }
        return;
    case 1:
        blurFixed<1>(padded, out, width, weights);
        return;
    case 2:
        blurFixed<2>(padded, out, width, weights);
        return;
    case 3:
        blurFixed<3>(padded, out, width, weights);
        return;
    case 4:
        blurFixed<4>(padded, out, width, weights);
        return;
    case 5:
        blurFixed<5>(padded, out, width, weights);
        return;
    case 6:
        blurFixed<6>(padded, out, width, weights);
        return;
    default:
        break;
    }
    const int radius = kernel.radius;
    for (int x = 0; x < width; ++x) {
        int sum = 0;
        for (int k = -radius; k <= radius; ++k) {
            sum += weights[k + radius] * padded[x + k];
        }
        out[x] = sum >> 10;
    }
}

void filterLine(std::uint32_t* video, int width)
{
    const std::array<Kernel, 3>& kernels = gKernels[width > 256 ? 1 : 0];
    std::array<std::uint32_t, kMaxWidth> pixels;
    std::memcpy(pixels.data(), video, static_cast<std::size_t>(width) * sizeof(std::uint32_t));
    std::uint32_t* const line = pixels.data();
    std::array<int, kPadded> luma;
    std::array<int, kPadded> in_phase;
    std::array<int, kPadded> quadrature;
    std::array<int, kMaxWidth> luma_out;
    std::array<int, kMaxWidth> in_phase_out;
    std::array<int, kMaxWidth> quadrature_out;
    int* const y = luma.data() + kMaxRadius;
    int* const i = in_phase.data() + kMaxRadius;
    int* const q = quadrature.data() + kMaxRadius;

    for (int x = 0; x < width; ++x) {
        const std::uint32_t pixel = line[x];
        int r = static_cast<int>((pixel >> 16) & 0xff);
        int g = static_cast<int>((pixel >> 8) & 0xff);
        int b = static_cast<int>(pixel & 0xff);
        if (gSwapRb) {
            std::swap(r, b);
        }
        y[x] = 306 * r + 601 * g + 117 * b;
        i[x] = 610 * r - 281 * g - 330 * b;
        q[x] = 216 * r - 536 * g + 320 * b;
    }
    for (int k = 1; k <= kMaxRadius; ++k) {
        y[-k] = y[0];
        i[-k] = i[0];
        q[-k] = q[0];
        y[width - 1 + k] = y[width - 1];
        i[width - 1 + k] = i[width - 1];
        q[width - 1 + k] = q[width - 1];
    }
    blur(y, luma_out.data(), width, kernels[0]);
    blur(i, in_phase_out.data(), width, kernels[1]);
    blur(q, quadrature_out.data(), width, kernels[2]);

    for (int x = 0; x < width; ++x) {
        const int yy = luma_out[static_cast<std::size_t>(x)];
        const int ii = in_phase_out[static_cast<std::size_t>(x)];
        const int qq = quadrature_out[static_cast<std::size_t>(x)];
        int r = clampByte((yy + ((979 * ii) >> 10) + ((636 * qq) >> 10) + 512) >> 10);
        const int g = clampByte((yy - ((279 * ii) >> 10) - ((662 * qq) >> 10) + 512) >> 10);
        int b = clampByte((yy - ((1133 * ii) >> 10) + ((1744 * qq) >> 10) + 512) >> 10);
        if (gSwapRb) {
            std::swap(r, b);
        }
        line[x] = (line[x] & 0xff000000U) | (static_cast<std::uint32_t>(r) << 16) | (static_cast<std::uint32_t>(g) << 8) | static_cast<std::uint32_t>(b);
    }
    markLine(line);
    std::memcpy(video, pixels.data(), static_cast<std::size_t>(width) * sizeof(std::uint32_t));
}

void ensureFiltered(std::uint32_t* base, int width, int line)
{
    int* const lock = &gLineLock[line];
    while (__sync_lock_test_and_set(lock, 1) != 0) {
        sched_yield();
    }
    std::uint32_t* const pixels = base + static_cast<std::size_t>(line) * static_cast<std::size_t>(width);
    std::array<std::uint32_t, 4> head;
    std::memcpy(head.data(), pixels, sizeof head);
    if (!isMarked(head)) {
        const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        filterLine(pixels, width);
        gFilterNs.fetch_add(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count()),
                            std::memory_order_relaxed);
    }
    __sync_lock_release(lock);
}

void filterAround(const unsigned char* task, int claimed)
{
    std::uint32_t source = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::memcpy(&source, task + 8, sizeof source);
    std::memcpy(&width, task + 12, sizeof width);
    std::memcpy(&height, task + 16, sizeof height);
    if ((width != 256 && width != 512) || height < 100 || height > static_cast<std::uint32_t>(kMaxLines) || source < 0x10000 || (source & 3U) != 0 ||
        claimed < 0 || claimed >= static_cast<int>(height)) {
        return;
    }
    std::uint32_t* const base = reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(source));
    const int first = std::max(claimed - 1, 0);
    const int last = std::min(claimed + 1, static_cast<int>(height) - 1);
    for (int line = first; line <= last; ++line) {
        ensureFiltered(base, static_cast<int>(width), line);
    }
}

constexpr int kMaskMax = 1280;
int gMaskOffPct = 60;
int gMaskGainPct = 125;
int gMaskYoff = 0;
std::array<std::array<std::array<std::uint8_t, 256>, 3>, 3> gMaskLut;
std::array<std::uint16_t, kMaskMax> gMapIndex;
int gMaskCol0 = 0;
int gMaskWidth = 0;
int gMaskStride = 5120;
std::uint32_t gMaskEnd = 0;
std::uint32_t gMaskBegin = 0;
std::atomic<bool> gMaskReady{false};
std::atomic<bool> gGeometryDone{false};
std::array<int, 3> gMaskMul{{0, 0, 0}};
thread_local int tMaskLine = -1;
thread_local std::uint32_t tMaskDestination = 0;
thread_local std::uint32_t tMaskSource = 0;
thread_local std::uint32_t tMaskWidth = 0;

void maskSelect()
{
    const char* pct = std::getenv("CRTFX_MASK");
    if (pct == nullptr || std::atoi(pct) <= 0) {
        journal("mask: off");
        return;
    }
    gMaskOffPct = std::min(std::max(std::atoi(pct), 10), 100);
    const char* gain = std::getenv("CRTFX_MASK_GAIN");
    if (gain != nullptr && std::atoi(gain) > 0) {
        gMaskGainPct = std::min(std::max(std::atoi(gain), 100), 300);
    }
    const char* slow = std::getenv("CRTFX_SLOW");
    if (slow != nullptr && std::atoi(slow) > 0) {
        gSlowLimit = std::atoi(slow);
    }
    const char* cap = std::getenv("CRTFX_CAP");
    if (cap != nullptr && std::atoi(cap) > 0) {
        gFilterCapUs = static_cast<std::uint64_t>(std::atoi(cap)) * 1000U;
    }
    gMaskOn = true;
    journal("mask: off-channel %d%%, gain %d%%, waiting for the geometry probe", gMaskOffPct, gMaskGainPct);
}

void maskGeometry(const unsigned char* task)
{
    if (gGeometryDone.exchange(true)) {
        return;
    }
    std::uint32_t destination = 0;
    std::memcpy(&destination, task + 4, sizeof destination);
    journal("geometry: destination %#x", static_cast<unsigned>(destination));
    std::FILE* maps = std::fopen("/proc/self/maps", "r");
    unsigned long begin_saved = 0;
    if (maps != nullptr) {
        std::array<char, 256> text;
        unsigned long end = 0;
        while (std::fgets(text.data(), static_cast<int>(text.size()), maps) != nullptr) {
            unsigned long begin = 0;
            if (std::sscanf(text.data(), "%lx-%lx", &begin, &end) == 2 && destination >= begin && destination < end) {
                begin_saved = begin;
                gMaskBegin = static_cast<std::uint32_t>(begin);
                journal("geometry: mapping %lx-%lx (%lu bytes) %.60s", begin, end, end - begin, text.data() + 20);
                gMaskEnd = static_cast<std::uint32_t>(end);
                break;
            }
        }
        std::fclose(maps);
    }
    if (gMaskEnd != 0) {
        const std::uint32_t mapping_begin = static_cast<std::uint32_t>(begin_saved);
        const std::uint32_t page_bytes = static_cast<std::uint32_t>(gMaskStride) * 720U;
        const std::uint32_t page = (destination - mapping_begin) / page_bytes;
        const std::uint32_t page_start = mapping_begin + page * page_bytes;
        const std::uint32_t inside = destination - page_start;
        journal("geometry: page %u starts %#x, destination is %u rows and %u bytes in (column %u)", page, static_cast<unsigned>(page_start),
                inside / static_cast<std::uint32_t>(gMaskStride), inside % static_cast<std::uint32_t>(gMaskStride),
                (inside % static_cast<std::uint32_t>(gMaskStride)) / 4);
        gMaskCol0 = static_cast<int>((inside % static_cast<std::uint32_t>(gMaskStride)) / 4);
        gMaskWidth = kMaskMax - 2 * gMaskCol0;
    }
    const std::uint64_t last_row_end = static_cast<std::uint64_t>(destination) + static_cast<std::uint64_t>(gMaskYoff + 3 * 224 + 3) * static_cast<std::uint64_t>(gMaskStride);
    const bool fits = gMaskEnd != 0 && last_row_end <= gMaskEnd && gMaskStride == kMaskMax * 4 && gMaskWidth >= 256 && gMaskWidth <= kMaskMax;
    gMaskMul = {{256 * gMaskGainPct / 100, 256 * gMaskOffPct / 100, 256 * gMaskOffPct / 100}};
    const float linear = gHasGains ? gGains[0] : 195.0F;
    const float square = gHasGains ? gGains[1] : -95.0F;
    for (std::size_t phase = 0; phase < 3; ++phase) {
        for (std::size_t channel = 0; channel < 3; ++channel) {
            const int multiplier = gMaskMul[(channel + 3 - phase) % 3];
            for (int value = 0; value < 256; ++value) {
                const float c = static_cast<float>(value) / 255.0F;
                const float lit = std::min(std::max((linear * c + square * c * c) / 100.0F, 0.0F), 1.0F);
                gMaskLut[phase][channel][static_cast<std::size_t>(value)] = static_cast<std::uint8_t>(std::min(lit * 255.0F * static_cast<float>(multiplier) / 256.0F + 0.5F, 255.0F));
            }
        }
    }
    if (gMaskWidth >= 256 && gMaskWidth <= kMaskMax) {
        for (int x = 0; x < gMaskWidth; ++x) {
            const int source = std::min(std::max(static_cast<int>((static_cast<float>(x) + 0.5F) * 256.0F / static_cast<float>(gMaskWidth)), 0), 255);
            const int phase = (x + gMaskCol0) % 3;
            gMapIndex[static_cast<std::size_t>(x)] = static_cast<std::uint16_t>(source * 3 + phase);
        }
    }
    gMaskReady.store(gMaskOn && fits);
    journal("geometry: mask %s (last row ends %#llx, mapping ends %#x)", gMaskReady.load() ? "armed" : "not armed, the rows would leave the mapping",
            static_cast<unsigned long long>(last_row_end), static_cast<unsigned>(gMaskEnd));
}

void maskLine(std::uint32_t destination, std::uint32_t source, std::uint32_t width, int line)
{
    const std::uint64_t row_end = static_cast<std::uint64_t>(destination) + static_cast<std::uint64_t>(gMaskYoff + 3 * line + 1) * static_cast<std::uint64_t>(gMaskStride);
    if (width != 256 || source < 0x10000 || (source & 3U) != 0 || destination < gMaskBegin || row_end > gMaskEnd) {
        return;
    }
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::array<std::uint32_t, 256> in;
    std::memcpy(in.data(), reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(source)) + static_cast<std::size_t>(line) * 256, sizeof in);
    std::array<std::uint32_t, 256 * 3> lit;
    for (std::size_t i = 0; i < 256; ++i) {
        const std::uint32_t pixel = in[i];
        const std::uint32_t r = pixel & 0xffU;
        const std::uint32_t g = (pixel >> 8) & 0xffU;
        const std::uint32_t b = (pixel >> 16) & 0xffU;
        for (std::size_t phase = 0; phase < 3; ++phase) {
            const auto& lut = gMaskLut[phase];
            lit[i * 3 + phase] = (static_cast<std::uint32_t>(lut[0][r]) << 16) | (static_cast<std::uint32_t>(lut[1][g]) << 8) | lut[2][b];
        }
    }
    std::array<std::uint32_t, kMaskMax> out;
    for (int x = 0; x < gMaskWidth; ++x) {
        out[static_cast<std::size_t>(x)] = lit[gMapIndex[static_cast<std::size_t>(x)]];
    }
    unsigned char* const base = reinterpret_cast<unsigned char*>(static_cast<std::uintptr_t>(destination));
    std::memcpy(base + static_cast<std::size_t>(gMaskYoff + 3 * line) * static_cast<std::size_t>(gMaskStride), out.data(), static_cast<std::size_t>(gMaskWidth) * sizeof(std::uint32_t));
    gMaskNs.fetch_add(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count()),
                      std::memory_order_relaxed);
}

void maskStep(const unsigned char* task, int claimed, std::uint64_t claims)
{
    if (!gGeometryDone.load(std::memory_order_relaxed) && claims >= 1000) {
        maskGeometry(task);
    }
    if (tMaskLine >= 0 && gMaskReady.load(std::memory_order_relaxed) && !gMaskOff.load(std::memory_order_relaxed)) {
        maskLine(tMaskDestination, tMaskSource, tMaskWidth, tMaskLine);
    }
    std::uint32_t height = 0;
    std::memcpy(&tMaskDestination, task + 4, sizeof tMaskDestination);
    std::memcpy(&tMaskSource, task + 8, sizeof tMaskSource);
    std::memcpy(&tMaskWidth, task + 12, sizeof tMaskWidth);
    std::memcpy(&height, task + 16, sizeof height);
    tMaskLine = (claimed >= 0 && claimed < static_cast<int>(height) && height <= 224) ? claimed : -1;
}

}  // namespace

__attribute__((constructor)) static void onLoad()
{
    std::array<char, 160> exe;
    exe.fill('\0');
    const ssize_t length = ::readlink("/proc/self/exe", exe.data(), exe.size() - 1);
    if (length <= 0 || !isTarget(exe.data())) {
        return;
    }
    gJournal.origin = std::chrono::steady_clock::now();
    const char* path = std::getenv("CRTFX_JOURNAL");
    if (path != nullptr && path[0] != 0) {
        journalOpen(path);
    }
    gActive = true;
    journal("loaded in %s pid %d", exe.data(), static_cast<int>(::getpid()));
    if (gJournal.fd >= 0) {
        sessionOpen();
    }
    identifyBuild(exe.data());
    selectBuild();
    gainsSelect();
    sweepSelect();
    probeSelect();
    filterSelect();
    maskSelect();
    heartbeatStart();
    journalSync();
}

__attribute__((destructor)) static void onUnload()
{
    if (!gActive) {
        return;
    }
    gStop.store(true);
    journal("exit, clean");
    journalSync();
    ::unlink(gSessionPath.data());
}

CRTFX_EXPORT float floorf(float value) noexcept
{
    using Fn = float (*)(float);
    static const Fn real = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, "floorf"));
    const int sweep = gSweepIndex.load(std::memory_order_relaxed);
    if (gBuild == nullptr || (!gHasGains && sweep < 0)) {
        return real(value);
    }
#ifdef CRTFX_PROBE_ANY_SITE
    const std::size_t index = 0;
#else
    const auto caller = reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
    const auto site = std::find(gBuild->returns.begin(), gBuild->returns.end(), caller);
    if (site == gBuild->returns.end()) {
        return real(value);
    }
    const std::size_t index = static_cast<std::size_t>(site - gBuild->returns.begin());
#endif
    const float original = value;
    const float gain = sweep >= 0 ? gSweep.gains[static_cast<std::size_t>(sweep)][index] : gGains[index];
    value = gain / 100.0F * kGainScale[index] + 0.5F;
    const unsigned bit = 1U << index;
    if ((gLoggedGains.fetch_or(bit) & bit) == 0) {
        journal("gain %zu: stock %.2f%%, now %.2f%%", index, static_cast<double>((original - 0.5F) / kGainScale[index] * 100.0F), static_cast<double>(gain));
    }
    return real(value);
}

CRTFX_EXPORT int ioctl(int fd, unsigned long request, ...)
{
    using Fn = int (*)(int, unsigned long, void*);
    static const Fn real = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, "ioctl"));
    va_list args;
    va_start(args, request);
    void* const argument = va_arg(args, void*);
    va_end(args);
    if (gActive && request == kFbPanDisplay) {
        gFrames.fetch_add(1, std::memory_order_relaxed);
    }
    return real(fd, request, argument);
}

CRTFX_EXPORT int SDL_AtomicAdd(void* atomic, int value)
{
    using Fn = int (*)(void*, int);
    static const Fn real = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, "SDL_AtomicAdd"));
    const int previous = real(atomic, value);
    const ProbeSites* const probe = gProbe;
    if (probe == nullptr) {
        return previous;
    }
#ifndef CRTFX_PROBE_ANY_SITE
    const auto caller = reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
    if (caller != probe->first_claim && caller != probe->loop_claim) {
        return previous;
    }
#endif
    const std::uint64_t claims = gClaims.fetch_add(1, std::memory_order_relaxed) + 1;
    const unsigned char* const task = static_cast<const unsigned char*>(atomic) - kTaskCounter;
    probeTask(task);
    if ((claims & 0x3fff) == 0 && gPixelDumps.fetch_add(1) < 6) {
        probePixels(task);
    }
    if (gFilterLevel > 0 && !gFilterOff.load(std::memory_order_relaxed)) {
        filterAround(task, previous);
    }
    if (gMaskOn) {
        maskStep(task, previous, claims);
    }
    return previous;
}
