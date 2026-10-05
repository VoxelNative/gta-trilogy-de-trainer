#pragma once
#include <string>

// Keyboard-driven trainer menu in the style of Liberty's Legacy / Native Trainer.
// Submenus are plain functions that declare their options every frame ("immediate mode"):
//
//   void PlayerMenu() {
//       menu::Toggle("God mode", &cfg.god);
//       if (menu::Action("Refill health")) ...;
//       menu::Submenu("Skins", SkinsMenu);
//   }
//
// Controls: open/close key (default F11), Up/Down or Num8/Num2 to move, Enter or Num5 to select,
// Left/Right or Num4/Num6 to change values, Backspace or Num0 to go back.
namespace menu
{
using SubmenuFn = void (*)();

void Init(const char* title, SubmenuFn root);
void SetOpenKey(int vk);
int OpenKey();
bool IsOpen();

// Window-message hook: returns true when the key was used by the menu and should not reach the game.
bool OnKey(int vk, bool down);

// Builds and draws the menu. Call inside an ImGui frame on the render thread.
void Frame();

// Shows a short message on screen.
void Notify(const char* fmt, ...);

// ---- builder API (only valid inside submenu functions) ----
// Each returns true on the frame the user activated / changed it.
bool Action(const char* label, const char* description = nullptr);
bool Toggle(const char* label, bool* value, const char* description = nullptr);
bool ToggleState(const char* label, bool state, const char* description = nullptr); // externally owned state
bool Int(const char* label, int* value, int min, int max, int step = 1, const char* description = nullptr);
bool Float(const char* label, float* value, float min, float max, float step, const char* format = "%.2f",
           const char* description = nullptr);
// Left/Right cycles through items; Enter returns true.
bool Choice(const char* label, int* index, const char* const* items, int count, const char* description = nullptr);
void Submenu(const char* label, SubmenuFn fn, const char* description = nullptr);
// A row that only shows a value.
void Info(const char* label, const std::string& value);
// Overrides the title shown for the current submenu (defaults to the label that opened it).
void Title(const char* title);
}
