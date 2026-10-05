#pragma once
#include <functional>

// Work that touches game objects must run on the game's own thread, between frames.
// The game module hooks a per-frame function and calls gt::Tick() from it.
namespace gt
{
// Queue fn to run on the game thread at the start of the next tick.
void Post(std::function<void()> fn);

// Called by the game hook once per frame: runs queued work, then the per-frame callback.
void Tick();

// Something to run every frame (god mode refills, frozen values, ...).
void SetPerFrame(std::function<void()> fn);

// Runs fn and swallows access violations, so a bad pointer can't take the game down.
bool SafeCall(void (*fn)(void*), void* ctx);
}
