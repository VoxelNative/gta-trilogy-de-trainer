#include "gamethread.h"
#include "log.h"
#include <mutex>
#include <vector>

namespace gt
{
static std::mutex g_lock;
static std::vector<std::function<void()>> g_queue;
static std::function<void()> g_perFrame;

void Post(std::function<void()> fn)
{
    std::lock_guard<std::mutex> lock(g_lock);
    g_queue.push_back(std::move(fn));
}

void SetPerFrame(std::function<void()> fn)
{
    std::lock_guard<std::mutex> lock(g_lock);
    g_perFrame = std::move(fn);
}

bool SafeCall(void (*fn)(void*), void* ctx)
{
    __try
    {
        fn(ctx);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

static void InvokeFunction(void* p) { (*static_cast<std::function<void()>*>(p))(); }

void Tick()
{
    std::vector<std::function<void()>> work;
    std::function<void()> perFrame;
    {
        std::lock_guard<std::mutex> lock(g_lock);
        work.swap(g_queue);
        perFrame = g_perFrame;
    }
    for (auto& fn : work)
        if (!SafeCall(InvokeFunction, &fn)) Log("A queued action crashed and was skipped.");
    if (perFrame && !SafeCall(InvokeFunction, &perFrame))
    {
        static int reported = 0;
        if (reported++ < 5) Log("Per-frame features hit an exception.");
    }
}
}
