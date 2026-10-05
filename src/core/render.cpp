#include "render.h"
#include "menu.h"
#include "log.h"
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <atomic>
#include <vector>
#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_dx12.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace render
{
namespace
{
using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
using ExecuteCommandListsFn = void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);

// DXGI may use different Present implementations for D3D11 and D3D12 swap chains; hook both.
PresentFn g_origPresent[2] = {};
ResizeBuffersFn g_origResize[2] = {};
ExecuteCommandListsFn g_origExecute = nullptr;

enum class Api { None, D3D11, D3D12 };

void (*g_draw)() = nullptr;
Api g_api = Api::None;
bool g_ready = false;
bool g_imguiCreated = false;
bool g_win32Ready = false;
IDXGISwapChain* g_swapChain = nullptr; // not ref-counted; identity only
HWND g_hwnd = nullptr;
WNDPROC g_origWndProc = nullptr;

// D3D11
ID3D11Device* g_dev11 = nullptr;
ID3D11DeviceContext* g_ctx11 = nullptr;
ID3D11RenderTargetView* g_rtv11 = nullptr;

// D3D12
struct Frame12
{
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12Resource* backBuffer = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    UINT64 fenceValue = 0;
};
ID3D12Device* g_dev12 = nullptr;
std::atomic<ID3D12CommandQueue*> g_queue{nullptr};
ID3D12DescriptorHeap* g_rtvHeap = nullptr;
ID3D12DescriptorHeap* g_srvHeap = nullptr;
ID3D12GraphicsCommandList* g_cmdList = nullptr;
ID3D12Fence* g_fence = nullptr;
HANDLE g_fenceEvent = nullptr;
UINT64 g_fenceCounter = 0;
std::vector<Frame12> g_frames;
IDXGISwapChain3* g_sc3 = nullptr;

template <class T> void SafeRelease(T*& p)
{
    if (p)
    {
        p->Release();
        p = nullptr;
    }
}

// ---------- input ----------

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (menu::OnKey((int)wp, true)) return 0;
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (menu::OnKey((int)wp, false)) return 0;
        break;
    case WM_CHAR:
        if (menu::IsOpen() && (wp == '\r' || wp == '\b')) return 0;
        break;
    }
    return CallWindowProcW(g_origWndProc, hwnd, msg, wp, lp);
}

// ---------- ImGui setup shared by both APIs ----------

bool SetupImGuiCommon(HWND hwnd)
{
    if (!g_imguiCreated)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NoMouse;

        RECT rc;
        GetClientRect(hwnd, &rc);
        float scale = (rc.bottom - rc.top) / 1080.0f;
        if (scale < 0.6f) scale = 0.6f;
        // Bake the font big enough for the title; smaller text is drawn scaled down.
        float size = 40.0f * scale;
        wchar_t fonts[MAX_PATH];
        GetWindowsDirectoryW(fonts, MAX_PATH);
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%ls\\Fonts\\segoeuib.ttf", fonts);
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES ||
            !io.Fonts->AddFontFromFileTTF(path, size))
        {
            ImFontConfig cfg;
            cfg.SizePixels = size;
            io.Fonts->AddFontDefault(&cfg);
        }
        g_imguiCreated = true;
    }
    if (!g_win32Ready)
    {
        ImGui_ImplWin32_Init(hwnd);
        g_win32Ready = true;
    }
    if (g_hwnd != hwnd)
    {
        if (g_hwnd && g_origWndProc) SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_origWndProc);
        g_hwnd = hwnd;
        g_origWndProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
    }
    return true;
}

// ---------- D3D11 ----------

void CreateRtv11()
{
    ID3D11Texture2D* back = nullptr;
    if (SUCCEEDED(g_swapChain->GetBuffer(0, IID_PPV_ARGS(&back))))
    {
        g_dev11->CreateRenderTargetView(back, nullptr, &g_rtv11);
        back->Release();
    }
}

bool Init11(IDXGISwapChain* sc, ID3D11Device* dev, HWND hwnd)
{
    g_dev11 = dev; // keeps the reference from GetDevice
    g_dev11->GetImmediateContext(&g_ctx11);
    g_swapChain = sc;
    CreateRtv11();
    SetupImGuiCommon(hwnd);
    if (!ImGui_ImplDX11_Init(g_dev11, g_ctx11)) return false;
    g_api = Api::D3D11;
    Log("Overlay ready (DirectX 11).");
    return true;
}

void Shutdown11()
{
    ImGui_ImplDX11_Shutdown();
    SafeRelease(g_rtv11);
    SafeRelease(g_ctx11);
    SafeRelease(g_dev11);
}

void Render11()
{
    if (!g_rtv11) CreateRtv11();
    if (!g_rtv11) return;
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    if (g_draw) g_draw();
    ImGui::Render();
    g_ctx11->OMSetRenderTargets(1, &g_rtv11, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

// ---------- D3D12 ----------

void WaitGpu12()
{
    if (!g_fence || !g_queue) return;
    UINT64 v = ++g_fenceCounter;
    if (SUCCEEDED(g_queue.load()->Signal(g_fence, v)) && g_fence->GetCompletedValue() < v)
    {
        g_fence->SetEventOnCompletion(v, g_fenceEvent);
        WaitForSingleObject(g_fenceEvent, 1000);
    }
}

bool Init12(IDXGISwapChain* sc, ID3D12Device* dev, HWND hwnd)
{
    g_dev12 = dev;
    g_swapChain = sc;
    if (FAILED(sc->QueryInterface(IID_PPV_ARGS(&g_sc3)))) return false;

    DXGI_SWAP_CHAIN_DESC desc;
    sc->GetDesc(&desc);
    UINT count = desc.BufferCount;

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV, count, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0};
    if (FAILED(dev->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&g_rtvHeap)))) return false;
    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    if (FAILED(dev->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&g_srvHeap)))) return false;

    UINT rtvSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    g_frames.resize(count);
    for (UINT i = 0; i < count; i++)
    {
        Frame12& f = g_frames[i];
        if (FAILED(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&f.allocator)))) return false;
        if (FAILED(sc->GetBuffer(i, IID_PPV_ARGS(&f.backBuffer)))) return false;
        f.rtv = handle;
        dev->CreateRenderTargetView(f.backBuffer, nullptr, handle);
        handle.ptr += rtvSize;
    }
    if (FAILED(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frames[0].allocator, nullptr,
                                      IID_PPV_ARGS(&g_cmdList))))
        return false;
    g_cmdList->Close();
    if (FAILED(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)))) return false;
    g_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    SetupImGuiCommon(hwnd);
    ImGui_ImplDX12_InitInfo info;
    info.Device = dev;
    info.CommandQueue = g_queue.load();
    info.NumFramesInFlight = (int)count;
    info.RTVFormat = desc.BufferDesc.Format;
    info.SrvDescriptorHeap = g_srvHeap;
    info.LegacySingleSrvCpuDescriptor = g_srvHeap->GetCPUDescriptorHandleForHeapStart();
    info.LegacySingleSrvGpuDescriptor = g_srvHeap->GetGPUDescriptorHandleForHeapStart();
    if (!ImGui_ImplDX12_Init(&info)) return false;
    g_api = Api::D3D12;
    Log("Overlay ready (DirectX 12, %u buffers, format %d).", count, (int)desc.BufferDesc.Format);
    return true;
}

void Shutdown12()
{
    WaitGpu12();
    if (g_api == Api::D3D12) ImGui_ImplDX12_Shutdown();
    for (auto& f : g_frames)
    {
        SafeRelease(f.allocator);
        SafeRelease(f.backBuffer);
    }
    g_frames.clear();
    SafeRelease(g_cmdList);
    SafeRelease(g_rtvHeap);
    SafeRelease(g_srvHeap);
    SafeRelease(g_fence);
    if (g_fenceEvent)
    {
        CloseHandle(g_fenceEvent);
        g_fenceEvent = nullptr;
    }
    SafeRelease(g_sc3);
    SafeRelease(g_dev12);
}

void Render12()
{
    ID3D12CommandQueue* queue = g_queue.load();
    if (!queue || g_frames.empty()) return;
    UINT i = g_sc3->GetCurrentBackBufferIndex();
    if (i >= g_frames.size()) return;
    Frame12& f = g_frames[i];

    if (g_fence->GetCompletedValue() < f.fenceValue)
    {
        g_fence->SetEventOnCompletion(f.fenceValue, g_fenceEvent);
        WaitForSingleObject(g_fenceEvent, 100);
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    if (g_draw) g_draw();
    ImGui::Render();

    f.allocator->Reset();
    g_cmdList->Reset(f.allocator, nullptr);
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = f.backBuffer;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    g_cmdList->ResourceBarrier(1, &b);
    g_cmdList->OMSetRenderTargets(1, &f.rtv, FALSE, nullptr);
    g_cmdList->SetDescriptorHeaps(1, &g_srvHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_cmdList);
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    g_cmdList->ResourceBarrier(1, &b);
    g_cmdList->Close();
    ID3D12CommandList* lists[] = {g_cmdList};
    queue->ExecuteCommandLists(1, lists);
    f.fenceValue = ++g_fenceCounter;
    queue->Signal(g_fence, f.fenceValue);
}

// ---------- lifecycle ----------

void Shutdown()
{
    if (g_api == Api::D3D11) Shutdown11();
    else if (g_api == Api::D3D12 || g_dev12) Shutdown12();
    g_api = Api::None;
    g_ready = false;
    g_swapChain = nullptr;
}

bool IsGameWindow(HWND hwnd)
{
    char cls[64] = {};
    GetClassNameA(hwnd, cls, sizeof(cls));
    return strcmp(cls, "UnrealWindow") == 0;
}

void TryInit(IDXGISwapChain* sc)
{
    DXGI_SWAP_CHAIN_DESC desc;
    if (FAILED(sc->GetDesc(&desc)) || !IsGameWindow(desc.OutputWindow)) return;
    if (g_ready) Shutdown(); // the game made a new swap chain

    ID3D11Device* d11 = nullptr;
    ID3D12Device* d12 = nullptr;
    bool ok = false;
    if (SUCCEEDED(sc->GetDevice(IID_PPV_ARGS(&d11))))
    {
        ok = Init11(sc, d11, desc.OutputWindow);
        if (!ok) Shutdown11();
    }
    else if (SUCCEEDED(sc->GetDevice(IID_PPV_ARGS(&d12))))
    {
        if (!g_queue)
        {
            d12->Release(); // wait until we've seen the game's command queue
            return;
        }
        ok = Init12(sc, d12, desc.OutputWindow);
        if (!ok)
        {
            Log("DirectX 12 overlay setup failed.");
            Shutdown12();
        }
    }
    g_ready = ok;
}

void OnPresent(IDXGISwapChain* sc)
{
    static bool failedOnce = false;
    if (!g_ready || sc != g_swapChain)
    {
        if (g_ready && sc != g_swapChain)
        {
            // A different swap chain: only switch if it belongs to the game window.
            DXGI_SWAP_CHAIN_DESC desc;
            if (FAILED(sc->GetDesc(&desc)) || !IsGameWindow(desc.OutputWindow)) return;
        }
        TryInit(sc);
        if (!g_ready)
        {
            if (!failedOnce && g_api == Api::None && g_queue) failedOnce = true;
            return;
        }
    }
    if (g_api == Api::D3D11) Render11();
    else if (g_api == Api::D3D12) Render12();
}

template <int N> HRESULT STDMETHODCALLTYPE HookPresent(IDXGISwapChain* sc, UINT sync, UINT flags)
{
    if (!(flags & DXGI_PRESENT_TEST)) OnPresent(sc);
    return g_origPresent[N](sc, sync, flags);
}

template <int N>
HRESULT STDMETHODCALLTYPE HookResize(IDXGISwapChain* sc, UINT count, UINT w, UINT h, DXGI_FORMAT fmt, UINT flags)
{
    if (g_ready && sc == g_swapChain)
    {
        // Our references to the back buffers must go before the game resizes them.
        if (g_api == Api::D3D11) SafeRelease(g_rtv11);
        else Shutdown();
    }
    return g_origResize[N](sc, count, w, h, fmt, flags);
}

void STDMETHODCALLTYPE HookExecute(ID3D12CommandQueue* queue, UINT n, ID3D12CommandList* const* lists)
{
    if (!g_queue.load() && queue->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
        g_queue = queue;
    g_origExecute(queue, n, lists);
}

// ---------- finding the vtable entries with throwaway devices ----------

struct Targets
{
    void* present[2] = {};
    void* resize[2] = {};
    void* execute = nullptr;
};

bool FindTargets(Targets& t)
{
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"TrilogyTrainerDummy";
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr,
                                wc.hInstance, nullptr);
    if (!hwnd) return false;

    // D3D11
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Width = 64;
    sd.BufferDesc.Height = 64;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    IDXGISwapChain* sc11 = nullptr;
    ID3D11Device* dev11 = nullptr;
    ID3D11DeviceContext* ctx11 = nullptr;
    if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                                                D3D11_SDK_VERSION, &sd, &sc11, &dev11, nullptr, &ctx11)))
    {
        void** vt = *(void***)sc11;
        t.present[0] = vt[8];
        t.resize[0] = vt[13];
    }
    SafeRelease(ctx11);
    SafeRelease(dev11);
    SafeRelease(sc11);

    // D3D12
    ID3D12Device* dev12 = nullptr;
    if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&dev12))))
    {
        D3D12_COMMAND_QUEUE_DESC qd{D3D12_COMMAND_LIST_TYPE_DIRECT};
        ID3D12CommandQueue* queue = nullptr;
        IDXGIFactory4* factory = nullptr;
        if (SUCCEEDED(dev12->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue))))
        {
            t.execute = (*(void***)queue)[10];
            if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
            {
                DXGI_SWAP_CHAIN_DESC1 d1{};
                d1.Width = 64;
                d1.Height = 64;
                d1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                d1.SampleDesc.Count = 1;
                d1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                d1.BufferCount = 2;
                d1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                IDXGISwapChain1* sc12 = nullptr;
                if (SUCCEEDED(factory->CreateSwapChainForHwnd(queue, hwnd, &d1, nullptr, nullptr, &sc12)))
                {
                    void** vt = *(void***)sc12;
                    if (vt[8] != t.present[0]) t.present[1] = vt[8];
                    if (vt[13] != t.resize[0]) t.resize[1] = vt[13];
                    sc12->Release();
                }
                factory->Release();
            }
            queue->Release();
        }
        dev12->Release();
    }

    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return t.present[0] || t.present[1];
}
} // namespace

void SetDrawCallback(void (*fn)()) { g_draw = fn; }

bool InstallHooks()
{
    Targets t;
    if (!FindTargets(t))
    {
        Log("Couldn't create a test device to find DXGI Present.");
        return false;
    }
    bool ok = false;
    if (t.present[0] && MH_CreateHook(t.present[0], (void*)&HookPresent<0>, (void**)&g_origPresent[0]) == MH_OK) ok = true;
    if (t.present[1] && MH_CreateHook(t.present[1], (void*)&HookPresent<1>, (void**)&g_origPresent[1]) == MH_OK) ok = true;
    if (t.resize[0]) MH_CreateHook(t.resize[0], (void*)&HookResize<0>, (void**)&g_origResize[0]);
    if (t.resize[1]) MH_CreateHook(t.resize[1], (void*)&HookResize<1>, (void**)&g_origResize[1]);
    if (t.execute) MH_CreateHook(t.execute, (void*)&HookExecute, (void**)&g_origExecute);
    MH_EnableHook(MH_ALL_HOOKS);
    Log("Render hooks: present=%p/%p resize=%p/%p execute=%p", t.present[0], t.present[1], t.resize[0], t.resize[1],
        t.execute);
    return ok;
}
}
