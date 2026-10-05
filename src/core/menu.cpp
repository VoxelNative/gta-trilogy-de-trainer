#include "menu.h"
#include "imgui.h"
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <vector>
#include <algorithm>

namespace menu
{
namespace
{
enum class Input { Up, Down, Left, Right, Select, Back };

struct Level
{
    SubmenuFn fn;
    std::string title;
    int selected = 0;
    int scroll = 0;
};

enum class RowKind { Action, Toggle, Value, Submenu, Info };

struct Row
{
    std::string label;
    std::string value;
    RowKind kind;
    bool on = false;
    std::string description;
};

struct Note
{
    std::string text;
    DWORD until;
};

std::mutex g_inputLock;
std::vector<Input> g_inputs;
bool g_open = false;
int g_openKey = VK_F11;
std::string g_title;
std::vector<Level> g_stack;

// Per-frame state while building.
std::vector<Row> g_rows;
int g_index = 0;          // index of the row being declared
bool g_select = false;    // input for the selected row this frame
int g_horizontal = 0;     // -1 left, +1 right
std::string g_titleOverride;

std::mutex g_noteLock;
std::vector<Note> g_notes;

constexpr int kVisibleRows = 14;

ImU32 Rgba(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }

// Theme: Liberty's Legacy default (grey text, orange highlight).
const ImU32 kHeader = Rgba(16, 16, 16, 235);
const ImU32 kAccent = Rgba(240, 160, 0);
const ImU32 kBack = Rgba(0, 0, 0, 185);
const ImU32 kSub = Rgba(30, 30, 30, 235);
const ImU32 kText = Rgba(225, 225, 225);
const ImU32 kDim = Rgba(150, 150, 150);
const ImU32 kSelText = Rgba(10, 10, 10);
const ImU32 kOn = Rgba(90, 220, 110);
const ImU32 kOff = Rgba(200, 80, 80);

bool IsMine(int vk)
{
    switch (vk)
    {
    case VK_UP: case VK_DOWN: case VK_LEFT: case VK_RIGHT:
    case VK_NUMPAD8: case VK_NUMPAD2: case VK_NUMPAD4: case VK_NUMPAD6:
    case VK_RETURN: case VK_NUMPAD5: case VK_BACK: case VK_NUMPAD0:
        return true;
    }
    return false;
}

void Push(Input in)
{
    std::lock_guard<std::mutex> lock(g_inputLock);
    g_inputs.push_back(in);
}

bool IsSelected() { return g_index == g_stack.back().selected; }

Row& AddRow(const char* label, RowKind kind, const char* description)
{
    g_rows.push_back(Row{label, "", kind, false, description ? description : ""});
    return g_rows.back();
}
} // namespace

void Init(const char* title, SubmenuFn root)
{
    g_title = title;
    g_stack.clear();
    g_stack.push_back(Level{root, "MAIN MENU"});
}

void SetOpenKey(int vk) { g_openKey = vk; }
int OpenKey() { return g_openKey; }
bool IsOpen() { return g_open; }

bool OnKey(int vk, bool down)
{
    if (vk == g_openKey)
    {
        if (down) g_open = !g_open;
        return true;
    }
    if (!g_open || !IsMine(vk)) return false;
    if (!down) return true; // swallow the key-up too, so the game never sees half a press

    switch (vk)
    {
    case VK_UP: case VK_NUMPAD8: Push(Input::Up); break;
    case VK_DOWN: case VK_NUMPAD2: Push(Input::Down); break;
    case VK_LEFT: case VK_NUMPAD4: Push(Input::Left); break;
    case VK_RIGHT: case VK_NUMPAD6: Push(Input::Right); break;
    case VK_RETURN: case VK_NUMPAD5: Push(Input::Select); break;
    case VK_BACK: case VK_NUMPAD0: Push(Input::Back); break;
    }
    return true;
}

void Notify(const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    std::lock_guard<std::mutex> lock(g_noteLock);
    g_notes.push_back(Note{buf, GetTickCount() + 3000});
    if (g_notes.size() > 5) g_notes.erase(g_notes.begin());
}

// ---------- builder ----------

bool Action(const char* label, const char* description)
{
    AddRow(label, RowKind::Action, description);
    bool hit = IsSelected() && g_select;
    g_index++;
    return hit;
}

bool Toggle(const char* label, bool* value, const char* description)
{
    Row& r = AddRow(label, RowKind::Toggle, description);
    bool hit = IsSelected() && g_select;
    if (hit) *value = !*value;
    r.on = *value;
    g_index++;
    return hit;
}

bool ToggleState(const char* label, bool state, const char* description)
{
    Row& r = AddRow(label, RowKind::Toggle, description);
    r.on = state;
    bool hit = IsSelected() && g_select;
    g_index++;
    return hit;
}

bool Int(const char* label, int* value, int min, int max, int step, const char* description)
{
    Row& r = AddRow(label, RowKind::Value, description);
    bool changed = false;
    if (IsSelected() && g_horizontal != 0)
    {
        int v = *value + g_horizontal * step;
        if (v < min) v = max;
        if (v > max) v = min;
        changed = v != *value;
        *value = v;
    }
    if (IsSelected() && g_select) changed = true;
    char buf[32];
    snprintf(buf, sizeof(buf), "< %d >", *value);
    r.value = buf;
    g_index++;
    return changed;
}

bool Float(const char* label, float* value, float min, float max, float step, const char* format,
           const char* description)
{
    Row& r = AddRow(label, RowKind::Value, description);
    bool changed = false;
    if (IsSelected() && g_horizontal != 0)
    {
        float v = *value + g_horizontal * step;
        v = std::clamp(v, min, max);
        changed = v != *value;
        *value = v;
    }
    if (IsSelected() && g_select) changed = true;
    char num[32], buf[48];
    snprintf(num, sizeof(num), format, *value);
    snprintf(buf, sizeof(buf), "< %s >", num);
    r.value = buf;
    g_index++;
    return changed;
}

bool Choice(const char* label, int* index, const char* const* items, int count, const char* description)
{
    Row& r = AddRow(label, RowKind::Value, description);
    if (IsSelected() && g_horizontal != 0)
        *index = (*index + g_horizontal + count) % count;
    bool hit = IsSelected() && g_select;
    r.value = std::string("< ") + items[std::clamp(*index, 0, count - 1)] + " >";
    g_index++;
    return hit;
}

void Submenu(const char* label, SubmenuFn fn, const char* description)
{
    AddRow(label, RowKind::Submenu, description);
    if (IsSelected() && g_select)
    {
        g_stack.push_back(Level{fn, label});
        g_select = false; // don't let the press leak into the new submenu
    }
    g_index++;
}

void Info(const char* label, const std::string& value)
{
    Row& r = AddRow(label, RowKind::Info, nullptr);
    r.value = value;
    g_index++;
}

void Title(const char* title) { g_titleOverride = title; }

// ---------- frame ----------

static void DrawNotes(ImDrawList* dl, float scale, ImFont* font, float fontSize)
{
    std::lock_guard<std::mutex> lock(g_noteLock);
    DWORD now = GetTickCount();
    g_notes.erase(std::remove_if(g_notes.begin(), g_notes.end(), [&](const Note& n) { return (LONG)(n.until - now) <= 0; }),
                  g_notes.end());
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    float y = screen.y - 140 * scale;
    for (auto it = g_notes.rbegin(); it != g_notes.rend(); ++it)
    {
        ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0, it->text.c_str());
        float x = screen.x - size.x - 60 * scale;
        dl->AddRectFilled(ImVec2(x - 14 * scale, y - 8 * scale), ImVec2(x + size.x + 14 * scale, y + size.y + 8 * scale), kBack, 4 * scale);
        dl->AddRectFilled(ImVec2(x - 14 * scale, y - 8 * scale), ImVec2(x - 10 * scale, y + size.y + 8 * scale), kAccent);
        dl->AddText(font, fontSize, ImVec2(x, y), kText, it->text.c_str());
        y -= size.y + 22 * scale;
    }
}

void Frame()
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    float scale = io.DisplaySize.y / 1080.0f;
    ImFont* font = ImGui::GetFont();
    float fontSize = 21.0f * scale;
    float titleSize = 38.0f * scale;

    std::vector<Input> inputs;
    {
        std::lock_guard<std::mutex> lock(g_inputLock);
        inputs.swap(g_inputs);
    }

    if (!g_open || g_stack.empty())
    {
        DrawNotes(dl, scale, font, fontSize);
        return;
    }

    // Apply navigation from the previous frame's row count.
    Level& lvl = g_stack.back();
    static int lastCount = 0;
    g_select = false;
    g_horizontal = 0;
    for (Input in : inputs)
    {
        switch (in)
        {
        case Input::Up: lvl.selected = lastCount ? (lvl.selected - 1 + lastCount) % lastCount : 0; break;
        case Input::Down: lvl.selected = lastCount ? (lvl.selected + 1) % lastCount : 0; break;
        case Input::Left: g_horizontal = -1; break;
        case Input::Right: g_horizontal = 1; break;
        case Input::Select: g_select = true; break;
        case Input::Back:
            if (g_stack.size() > 1) g_stack.pop_back();
            else g_open = false;
            lastCount = 0;
            return;
        }
    }

    // Build the current submenu.
    size_t depth = g_stack.size();
    g_rows.clear();
    g_index = 0;
    g_titleOverride.clear();
    SubmenuFn fn = g_stack.back().fn;
    fn();
    if (g_stack.size() != depth)
    {
        lastCount = 0; // entered a submenu this frame; draw it next frame
        return;
    }
    Level& cur = g_stack.back();
    lastCount = (int)g_rows.size();
    if (lastCount == 0) return;
    cur.selected = std::clamp(cur.selected, 0, lastCount - 1);
    if (cur.selected < cur.scroll) cur.scroll = cur.selected;
    if (cur.selected >= cur.scroll + kVisibleRows) cur.scroll = cur.selected - kVisibleRows + 1;

    // ---- draw ----
    float x = 50 * scale, y = 90 * scale, w = 460 * scale;
    float headerH = 78 * scale, subH = 34 * scale, rowH = 36 * scale, pad = 14 * scale;

    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + headerH), kHeader, 6 * scale, ImDrawFlags_RoundCornersTop);
    dl->AddRectFilled(ImVec2(x, y + headerH - 4 * scale), ImVec2(x + w, y + headerH), kAccent);
    ImVec2 ts = font->CalcTextSizeA(titleSize, FLT_MAX, 0, g_title.c_str());
    dl->AddText(font, titleSize, ImVec2(x + (w - ts.x) / 2, y + (headerH - ts.y) / 2 - 2 * scale), kAccent, g_title.c_str());
    y += headerH;

    std::string sub = g_titleOverride.empty() ? cur.title : g_titleOverride;
    for (auto& c : sub) c = (char)toupper((unsigned char)c);
    char count[32];
    snprintf(count, sizeof(count), "%d / %d", cur.selected + 1, lastCount);
    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + subH), kSub);
    dl->AddText(font, fontSize * 0.9f, ImVec2(x + pad, y + (subH - fontSize * 0.9f) / 2), kDim, sub.c_str());
    ImVec2 cs = font->CalcTextSizeA(fontSize * 0.9f, FLT_MAX, 0, count);
    dl->AddText(font, fontSize * 0.9f, ImVec2(x + w - pad - cs.x, y + (subH - fontSize * 0.9f) / 2), kDim, count);
    y += subH;

    int end = std::min(lastCount, cur.scroll + kVisibleRows);
    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + rowH * (end - cur.scroll)), kBack);
    for (int i = cur.scroll; i < end; i++)
    {
        const Row& r = g_rows[i];
        bool sel = i == cur.selected;
        if (sel) dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + rowH), kAccent);
        float ty = y + (rowH - fontSize) / 2;
        dl->AddText(font, fontSize, ImVec2(x + pad, ty), sel ? kSelText : (r.kind == RowKind::Info ? kDim : kText), r.label.c_str());

        std::string right;
        ImU32 rc = sel ? kSelText : kText;
        switch (r.kind)
        {
        case RowKind::Toggle:
            right = r.on ? "ON" : "OFF";
            rc = sel ? kSelText : (r.on ? kOn : kOff);
            break;
        case RowKind::Submenu: right = ">>"; break;
        case RowKind::Value:
        case RowKind::Info: right = r.value; break;
        default: break;
        }
        if (!right.empty())
        {
            ImVec2 rs = font->CalcTextSizeA(fontSize, FLT_MAX, 0, right.c_str());
            dl->AddText(font, fontSize, ImVec2(x + w - pad - rs.x, ty), rc, right.c_str());
        }
        y += rowH;
    }

    // Footer: scroll hint + description of the selected row.
    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + 6 * scale), kAccent);
    y += 10 * scale;
    const std::string& desc = g_rows[cur.selected].description;
    if (!desc.empty())
    {
        float wrap = w - 2 * pad;
        ImVec2 ds = font->CalcTextSizeA(fontSize * 0.85f, FLT_MAX, wrap, desc.c_str());
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + ds.y + 2 * pad), kBack, 4 * scale);
        dl->AddText(font, fontSize * 0.85f, ImVec2(x + pad, y + pad), kText, desc.c_str(), nullptr, wrap);
    }

    DrawNotes(dl, scale, font, fontSize);
}
}
