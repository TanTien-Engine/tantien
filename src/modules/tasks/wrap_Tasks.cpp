#include "modules/tasks/wrap_Tasks.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <filesystem>
#include <chrono>
#include <thread>

// JSON for the adapter views (header-only, vendored inside the cax submodule;
// the editor target adds thirdparty/cax/thirdparty/nlohmann/single_include).
#include <nlohmann/json.hpp>

#ifndef _WIN32
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace
{

// ---------------------------------------------------------------- spawn ---

// pid -> exit code, recorded when poll() first observes the exit so later
// polls keep answering instead of failing on an already-reaped pid.
std::map<int, int> g_task_exits;

void push_int_list(const std::vector<int>& v)
{
    ves_newlist(static_cast<int>(v.size()));
    for (int i = 0; i < static_cast<int>(v.size()); ++i) {
        ves_pushnumber(static_cast<double>(v[i]));
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Tasks_spawn()
{
    // args (list of strings), log_path (string|nil), cwd (string|nil)
    if (ves_type(1) != VES_TYPE_LIST) { ves_set_number(0, 0); return; }

    std::vector<std::string> storage;
    const int argc = ves_len(1);
    storage.reserve(argc > 0 ? argc : 1);
    std::vector<char*> argv;
    for (int i = 0; i < argc; ++i) {
        ves_geti(1, i);
        const char* s = ves_tostring(-1);
        storage.push_back(s ? s : "");
        ves_pop(1);
    }
    if (storage.empty() || storage[0].empty()) { ves_set_number(0, 0); return; }
    for (auto& s : storage) argv.push_back(s.data());
    argv.push_back(nullptr);

    const char* log_path = ves_type(2) == VES_TYPE_STRING ? ves_tostring(2) : nullptr;
    const char* cwd      = ves_type(3) == VES_TYPE_STRING ? ves_tostring(3) : nullptr;

#ifdef _WIN32
    // Windows: not wired in this delivery; the UI adapter reports the module
    // as unavailable there until a job-object backend lands.
    (void)log_path; (void)cwd;
    ves_set_number(0, 0);
#else
    fflush(stdout);
    fflush(stderr);
    pid_t pid = fork();
    if (pid < 0) { ves_set_number(0, 0); return; }
    if (pid == 0)
    {
        // Child: own process group so kill_tree reaches every descendant.
        setpgid(0, 0);
        if (cwd && cwd[0] != '\0' && chdir(cwd) != 0) _exit(127);
        // stdout + stderr into the per-run log (append) so the adapter's
        // attempts view shows the worker's real output; /dev/null when the
        // caller passed nil.
        if (log_path && log_path[0] != '\0') {
            int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd >= 0) { dup2(fd, 1); dup2(fd, 2); if (fd > 2) close(fd); }
        } else {
            int fd = open("/dev/null", O_WRONLY);
            if (fd >= 0) { dup2(fd, 1); dup2(fd, 2); if (fd > 2) close(fd); }
        }
        execvp(argv[0], argv.data());
        _exit(127);
    }
    // Parent: put the child in its new group from this side too, so there is
    // no window where a signal to the group misses it.
    setpgid(pid, pid);
    ves_set_number(0, static_cast<double>(pid));
#endif
}

void w_Tasks_poll()
{
    // Read args BEFORE popping the api frame (receiver + args): after the pop
    // slot 0 is caller junk, not the pid.
    const int pid = static_cast<int>(ves_tonumber(1));
    ves_pop(ves_argnum());
    int code = -1;
    bool alive = false;
    // pid <= 0 would make waitpid reap an arbitrary child -- refuse it.
    if (pid > 0)
    {

    auto it = g_task_exits.find(pid);
    if (it != g_task_exits.end())
    {
        alive = false;
        code = it->second;
    }
    else
    {
#ifdef _WIN32
        alive = false; code = -1;
#else
        int status = 0;
        pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == 0)
        {
            alive = true; // still running
        }
        else if (r == pid)
        {
            alive = false;
            if (WIFEXITED(status)) code = WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) code = 128 + WTERMSIG(status);
            g_task_exits[pid] = code;
        }
        else
        {
            // Not our child (never spawned here or already reaped elsewhere).
            alive = false; code = -1;
        }
#endif
    }
    }
    // [alive(bool), exit_code]; code only meaningful once alive is false.
    ves_newlist(2);
    ves_pushboolean(alive ? 1 : 0);
    ves_seti(-2, 0);
    ves_pop(1);
    ves_pushnumber(static_cast<double>(code));
    ves_seti(-2, 1);
    ves_pop(1);
}

void w_Tasks_kill_tree()
{
    const int pid = static_cast<int>(ves_tonumber(1));
    const int sig = static_cast<int>(ves_tonumber(2));
#ifdef _WIN32
    (void)pid; (void)sig;
    ves_set_boolean(0, false);
#else
    // pid 0 would signal the CALLER'S OWN process group (kill(0, sig)); only
    // the negative-group form created by spawn() is ever legal here.
    if (pid <= 0) { ves_set_boolean(0, false); return; }
    // Negative pid = the whole process group created by spawn().
    ves_set_boolean(0, kill(-pid, sig) == 0);
#endif
}

// ---------------------------------------------------------------- files ---

void w_Tasks_mkdir_p()
{
    const char* path = ves_tostring(1);
    if (!path || path[0] == '\0') { ves_set_boolean(0, false); return; }
    std::error_code ec;
    ves_set_boolean(0, std::filesystem::create_directories(
        std::filesystem::path(path), ec) || (ec == std::error_code()));
}

// --------------------------------------------------------------- sha256 ---
// FIPS 180-4, compact free-standing implementation (no external deps).
struct Sha256
{
    uint32_t h[8];
    uint64_t len = 0;
    uint8_t  buf[64];
    size_t   buf_len = 0;

    static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

    void init()
    {
        static const uint32_t iv[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
        for (int i = 0; i < 8; ++i) h[i] = iv[i];
        len = 0; buf_len = 0;
    }

    static uint32_t load32(const uint8_t* p)
    {
        return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
               (uint32_t(p[2]) << 8) | uint32_t(p[3]);
    }

    void block(const uint8_t* p)
    {
        static const uint32_t k[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,
            0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
            0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,
            0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,
            0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
            0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,
            0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,
            0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
            0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) w[i] = load32(p + i * 4);
        for (int i = 16; i < 64; ++i)
        {
            uint32_t s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
            uint32_t s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i)
        {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + k[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + maj;
            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }

    void update(const uint8_t* data, size_t n)
    {
        len += n;
        while (n > 0)
        {
            size_t take = 64 - buf_len;
            if (take > n) take = n;
            memcpy(buf + buf_len, data, take);
            buf_len += take; data += take; n -= take;
            if (buf_len == 64) { block(buf); buf_len = 0; }
        }
    }

    std::string finish()
    {
        uint64_t bits = len * 8;
        uint8_t pad = 0x80;
        update(&pad, 1);
        uint8_t zero = 0;
        while (buf_len != 56) update(&zero, 1);
        uint8_t lb[8];
        for (int i = 0; i < 8; ++i) lb[i] = uint8_t(bits >> (56 - i * 8));
        update(lb, 8);
        static const char* hex = "0123456789abcdef";
        std::string out;
        out.reserve(64);
        for (int i = 0; i < 8; ++i)
            for (int b = 7; b >= 0; --b)
                out += hex[(h[i] >> (b * 4)) & 0xf];
        return out;
    }
};

void w_Tasks_sha256()
{
    const char* path = ves_tostring(1);
    if (!path || path[0] == '\0') { ves_set_nil(0); return; }
    FILE* f = fopen(path, "rb");
    if (!f) { ves_set_nil(0); return; }
    Sha256 ctx;
    ctx.init();
    uint8_t buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) ctx.update(buf, n);
    fclose(f);
    std::string hex = ctx.finish();
    ves_set_lstring(0, hex.data(), hex.size());
}

void w_Tasks_now_ms()
{
    using namespace std::chrono;
    const double ms = double(duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count());
    ves_set_number(0, ms);
}

// Frame-cadence wait for headless probes (ves_test): parks the caller
// WITHOUT spinning, so a polling probe behaves like the editor's frame
// loop instead of a busy loop. Editor code never needs this (frames are
// paced by the event loop).
void w_Tasks_wait_ms()
{
    const int ms = static_cast<int>(ves_tonumber(1));
    if (ms <= 0 || ms > 5000) { ves_set_number(0, 0); return; }
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    ves_set_number(0, ms);
}

// ----------------------------------------------------------------- json ---

void push_json(const nlohmann::json& j)
{
    if (j.is_null())                        ves_pushnil();
    else if (j.is_boolean())                ves_pushboolean(j.get<bool>() ? 1 : 0);
    else if (j.is_number())                 ves_pushnumber(j.get<double>());
    else if (j.is_string())
    {
        const std::string s = j.get<std::string>();
        ves_pushlstring(s.data(), s.size());
    }
    else if (j.is_array())
    {
        const int n = static_cast<int>(j.size());
        ves_newlist(n);
        for (int i = 0; i < n; ++i)
        {
            push_json(j[i]);
            ves_seti(-2, i);
            ves_pop(1);
        }
    }
    else // object
    {
        ves_newmap();
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            push_json(it.value());
            ves_setfield(-2, it.key().c_str());
            ves_pop(1);
        }
    }
}

void w_Json_parse()
{
    const char* text = ves_tostring(1);
    ves_pop(ves_argnum());
    if (!text) { ves_pushnil(); return; }
    try
    {
        nlohmann::json j = nlohmann::json::parse(text);
        push_json(j);
    }
    catch (...)
    {
        // Unparseable text reads as nil -- never as an empty result.
        ves_pushnil();
    }
}

}

namespace tt
{

VesselForeignMethodFn TasksBindMethod(const char* signature)
{
    if (strcmp(signature, "static Tasks.spawn(_,_,_)") == 0) return w_Tasks_spawn;
    if (strcmp(signature, "static Tasks.poll(_)") == 0) return w_Tasks_poll;
    if (strcmp(signature, "static Tasks.kill_tree(_,_)") == 0) return w_Tasks_kill_tree;
    if (strcmp(signature, "static Tasks.mkdir_p(_)") == 0) return w_Tasks_mkdir_p;
    if (strcmp(signature, "static Tasks.sha256(_)") == 0) return w_Tasks_sha256;
    if (strcmp(signature, "static Tasks.now_ms()") == 0) return w_Tasks_now_ms;
    if (strcmp(signature, "static Tasks.wait_ms(_)") == 0) return w_Tasks_wait_ms;
    if (strcmp(signature, "static Json.parse(_)") == 0) return w_Json_parse;
    return NULL;
}

void TasksBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    (void)class_name;
    (void)methods;
}

}
