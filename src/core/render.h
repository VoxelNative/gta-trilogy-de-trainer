#pragma once

// Draws the menu inside the game by hooking DXGI Present (works for DirectX 11 and 12).
namespace render
{
// Installs the Present / ResizeBuffers / ExecuteCommandLists hooks. MinHook must be initialised.
bool InstallHooks();

// Called inside every ImGui frame we render (set by the game module).
void SetDrawCallback(void (*fn)());
}
