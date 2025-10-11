// HW.cpp: implementation of the CHW class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "HW.h"
#include "xr_IOconsole.h"
#include "xr_trims.h"
#include "xr_tokens.h"
#include <vector>
#include <string>
#include <algorithm>
#include <windows.h>

ENGINE_API CHW HW;

static std::vector<std::string> g_vid_modes_storage;
static std::vector<xr_token>    g_vid_tokens;

xr_token* vid_mode_token = NULL;

struct _uniq_mode
{
    _uniq_mode(LPCSTR v): _val(v) {}
    LPCSTR _val;
    bool operator()(const std::string& other) const { return 0 == stricmp(_val, other.c_str()); }
};

#ifndef _EDITOR
void free_vid_mode_list()
{
    if (!vid_mode_token) return;
    g_vid_tokens.clear();
    g_vid_modes_storage.clear();
    vid_mode_token = NULL;
}

void fill_vid_mode_list(CHW* _hw)
{
    if (vid_mode_token) return;

    g_vid_modes_storage.clear();
    g_vid_tokens.clear();

    UINT cnt = _hw->pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);

    for (UINT i = 0; i < cnt; ++i)
    {
        D3DDISPLAYMODE Mode;
        if (FAILED(_hw->pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, i, &Mode)))
            continue;

        if (Mode.Width < 800) continue;

        char s[32];
        sprintf(s, "%dx%d", Mode.Width, Mode.Height);

        if (std::find_if(g_vid_modes_storage.begin(), g_vid_modes_storage.end(), _uniq_mode(s)) != g_vid_modes_storage.end())
            continue;

        g_vid_modes_storage.emplace_back(s);
    }

    g_vid_tokens.reserve(g_vid_modes_storage.size() + 1);
    for (size_t i = 0; i < g_vid_modes_storage.size(); ++i)
    {
        xr_token t;
        t.id   = static_cast<int>(i);
        t.name = g_vid_modes_storage[i].c_str();
        g_vid_tokens.push_back(t);
    }
    xr_token term; term.id = -1; term.name = NULL;
    g_vid_tokens.push_back(term);

    vid_mode_token = g_vid_tokens.data();

    Msg("Available video modes[%d]:", (int)g_vid_modes_storage.size());
    for (auto& s : g_vid_modes_storage)
        Msg("[%s]", s.c_str());
}
#else
void free_vid_mode_list() {}
void fill_vid_mode_list(CHW* _hw) {}
#endif

static void GetMonitorRectForWindow(HWND hWnd, RECT& out)
{
    HMONITOR mon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {}; mi.cbSize = sizeof(mi);
    if (GetMonitorInfo(mon, &mi)) out = mi.rcMonitor;
    else GetClientRect(GetDesktopWindow(), &out);
}

static void MakeBorderlessFullscreenWindow(HWND hWnd, u32& outW, u32& outH) // было DWORD outW, outH по значению
{
    RECT r; GetMonitorRectForWindow(hWnd, r);
    outW = (u32)(r.right - r.left);
    outH = (u32)(r.bottom - r.top);

    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW | WS_BORDER | WS_DLGFRAME | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    style |= WS_POPUP | WS_VISIBLE;
    SetWindowLong(hWnd, GWL_STYLE, style);

    SetWindowPos(hWnd, HWND_TOP, r.left, r.top, (int)outW, (int)outH, SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
}


// ------------------------------------------------------------

void CHW::CreateD3D()
{
    HW.pD3D = Direct3DCreate8(D3D_SDK_VERSION);
    R_ASSERT(HW.pD3D);
}

void CHW::DestroyD3D()
{
    _RELEASE(HW.pD3D);
}

//////////////////////////////////////////////////////////////////////
// Depth-stencil
//////////////////////////////////////////////////////////////////////
D3DFORMAT CHW::selectDepthStencil(D3DFORMAT fTarget)
{
    static D3DFORMAT fDS_Try1[6] =
    { D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D32, D3DFMT_D16, D3DFMT_D15S1 };

    static D3DFORMAT fDS_Try2[2] =
    { D3DFMT_D24S8, D3DFMT_D24X4S4 };

    D3DFORMAT* fDS_Try = fDS_Try1;
    int fDS_Cnt = 6;
    if (HW.Caps.bShowOverdraw) { fDS_Try = fDS_Try2; fDS_Cnt = 2; }

    for (int it = 0; it < fDS_Cnt; it++) {
        if (SUCCEEDED(pD3D->CheckDeviceFormat(
            D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget,
            D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, fDS_Try[it])))
        {
            if (SUCCEEDED(pD3D->CheckDepthStencilMatch(
                D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                fTarget, fTarget, fDS_Try[it])))
            {
                return fDS_Try[it];
            }
        }
    }
    return D3DFMT_UNKNOWN;
}

void CHW::DestroyDevice()
{
    _SHOW_REF("DeviceREF:", HW.pDevice);
    _RELEASE(HW.pDevice);
    DestroyD3D();

#ifndef _EDITOR
    free_vid_mode_list();
#endif
}

DWORD CHW::selectRefresh(DWORD dwWidth, DWORD dwHeight)
{
    DWORD count = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
    DWORD selected = D3DPRESENT_RATE_DEFAULT;
    for (DWORD I = 0; I < count; I++)
    {
        D3DDISPLAYMODE Mode;
        if (FAILED(pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, I, &Mode)))
            continue;
        if (Mode.Width == dwWidth && Mode.Height == dwHeight)
        {
            if (Mode.RefreshRate > selected) selected = Mode.RefreshRate;
        }
    }
    return selected;
}

DWORD CHW::selectPresentInterval() //костыль, на dx8 он не нужен
{
    D3DCAPS8 caps;
    pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

    if (psDeviceFlags & rsNoVSync) {
        if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE)
            return D3DPRESENT_INTERVAL_IMMEDIATE;
        if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE)
            return D3DPRESENT_INTERVAL_ONE;
    }
    return D3DPRESENT_INTERVAL_DEFAULT;
}

DWORD CHW::selectGPU()
{
    if (Caps.bForceGPU_SW) return D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    D3DCAPS8 caps;
    pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

    if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
    {
        if (Caps.bForceGPU_NonPure) return D3DCREATE_HARDWARE_VERTEXPROCESSING;
        else {
            if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE) return D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
            else return D3DCREATE_HARDWARE_VERTEXPROCESSING;
        }
    }
    else return D3DCREATE_SOFTWARE_VERTEXPROCESSING;
}

void CHW::selectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed)
{
    fill_vid_mode_list(this);
#ifdef DEDICATED_SERVER
    dwWidth = 640;
    dwHeight = 480;
#else
    if (bWindowed)
    {
        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
    }
    else
    {
#ifndef _EDITOR
        string64 buff;
        sprintf(buff, "%dx%d", psCurrentVidMode[0], psCurrentVidMode[1]);

        if (_ParseItem(buff, vid_mode_token) == u32(-1)) //not found
        { //select safe
            sprintf_s(buff, sizeof(buff), "vid_mode %s", vid_mode_token[0].name);
            Console.Execute(buff);
        }

        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
#endif
    }
#endif
}

DWORD CHW::CreateDevice(HWND m_hWnd, DWORD& dwWidth, DWORD& dwHeight)
{
    CreateD3D();

    const BOOL wantFullscreenLook = (psDeviceFlags & rsFullscreen);
    const BOOL bWindowedSwap = TRUE;

    if (wantFullscreenLook) {
        SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        MakeBorderlessFullscreenWindow(m_hWnd, (u32&)dwWidth, (u32&)dwHeight);
    } else {
        SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX);
        u32 w = 0, h = 0; selectResolution(w, h, TRUE);
        dwWidth = w; dwHeight = h;
        RECT rc = { 0,0,(LONG)dwWidth,(LONG)dwHeight };
        AdjustWindowRect(&rc, GetWindowLong(m_hWnd, GWL_STYLE), FALSE);
        SetWindowPos(m_hWnd, HWND_TOP, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW | SWP_NOCOPYBITS);
    }

    D3DADAPTER_IDENTIFIER8 adapterID;
    R_CHK(pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, D3DENUM_NO_WHQL_LEVEL, &adapterID));
    Msg("* Video board: %s", adapterID.Description);

    D3DDISPLAYMODE desk;
    R_CHK(pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &desk));
    D3DFORMAT& fTarget = Caps.fTarget;
    D3DFORMAT& fDepth  = Caps.fDepth;

    fTarget = desk.Format;
    R_CHK(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, TRUE));
    fDepth = selectDepthStencil(fTarget);
    R_ASSERT(fTarget != D3DFMT_UNKNOWN);
    R_ASSERT(fDepth  != D3DFMT_UNKNOWN);

    D3DPRESENT_PARAMETERS P = {};
    P.BackBufferWidth  = dwWidth;
    P.BackBufferHeight = dwHeight;
    P.BackBufferFormat = fTarget;
    P.BackBufferCount  = 1;
    P.MultiSampleType  = D3DMULTISAMPLE_NONE;
    P.SwapEffect       = D3DSWAPEFFECT_COPY;
    P.hDeviceWindow    = m_hWnd;
    P.Windowed         = TRUE;
    P.EnableAutoDepthStencil = TRUE;
    P.AutoDepthStencilFormat = fDepth;
    P.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

    DWORD GPU = selectGPU();
    R_CHK(HW.pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, GPU, &P, &pDevice));

    CurrBBWidth  = P.BackBufferWidth;
    CurrBBHeight = P.BackBufferHeight;

#ifndef _EDITOR
    fill_vid_mode_list(this);
#endif
    return GetWindowLong(m_hWnd, GWL_STYLE);
}


void CHW::updateWindowProps(HWND m_hWnd)
{
#ifndef DEDICATED_SERVER
    const bool wantFullscreenLook = (psDeviceFlags & rsFullscreen);
#else
    const bool wantFullscreenLook = false;
#endif

    if (wantFullscreenLook)
    {
        u32 w=0, h=0;
        SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        MakeBorderlessFullscreenWindow(m_hWnd, w, h);
    }
    else
    {
        const DWORD style = (WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX);
        SetWindowLong(m_hWnd, GWL_STYLE, style);

        RECT rc = { 0,0,(LONG)CurrBBWidth,(LONG)CurrBBHeight };
        AdjustWindowRect(&rc, style, FALSE);

        BOOL bCenter = FALSE;
        if (strstr(GetCommandLineA(), "-center_screen")) bCenter = TRUE;
        int x = 0, y = 0, w = rc.right - rc.left, h = rc.bottom - rc.top;
        if (bCenter)
        {
            RECT desk; GetClientRect(GetDesktopWindow(), &desk);
            x = (desk.right - w) / 2;
            y = (desk.bottom - h) / 2;
        }
        SetWindowPos(m_hWnd, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_DRAWFRAME);
    }

#ifndef DEDICATED_SERVER
    ShowCursor(FALSE);
    SetForegroundWindow(m_hWnd);
#endif
}


void CHW::Reset(HWND hwnd)
{
#ifdef DEBUG
    _RELEASE(dwDebugSB);
#endif
    _RELEASE(pBaseZB);
    _RELEASE(pBaseRT);

#ifndef _EDITOR
    const bool wantFullscreenLook = (psDeviceFlags & rsFullscreen);

    u32 w = 0, h = 0;
    if (wantFullscreenLook)
    {
        MakeBorderlessFullscreenWindow(hwnd, w, h);
    }
    else
    {
        w = CurrBBWidth; h = CurrBBHeight;
        if (!w || !h) { u32 rw=0, rh=0; selectResolution(rw, rh, TRUE); w = rw; h = rh; }

        const DWORD style = (WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX);
        SetWindowLong(hwnd, GWL_STYLE, style);
        RECT rc = { 0,0,(LONG)w,(LONG)h };
        AdjustWindowRect(&rc, style, FALSE);
        SetWindowPos(hwnd, HWND_TOP, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW | SWP_NOCOPYBITS);
    }

    D3DDISPLAYMODE desk;
    R_CHK(pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &desk));

    D3DFORMAT fTarget = desk.Format;
    R_CHK(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, TRUE));
    D3DFORMAT fDepth  = selectDepthStencil(fTarget);
    R_ASSERT(fTarget != D3DFMT_UNKNOWN);
    R_ASSERT(fDepth  != D3DFMT_UNKNOWN);

    D3DPRESENT_PARAMETERS P = {};
    P.BackBufferWidth  = w;
    P.BackBufferHeight = h;
    P.BackBufferFormat = fTarget;
    P.BackBufferCount  = 1;
    P.MultiSampleType  = D3DMULTISAMPLE_NONE;
    P.SwapEffect       = D3DSWAPEFFECT_COPY;
    P.hDeviceWindow    = hwnd;
    P.Windowed         = TRUE; // критично: без эксклюзива
    P.EnableAutoDepthStencil = TRUE;
    P.AutoDepthStencilFormat = fDepth;
    P.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

    for (;;)
    {
        HRESULT hr = pDevice->Reset(&P);
        if (SUCCEEDED(hr)) break;
        Sleep(100);
    }

    CurrBBWidth  = P.BackBufferWidth;
    CurrBBHeight = P.BackBufferHeight;

#ifndef _EDITOR
    updateWindowProps(hwnd);
#endif
#endif // !_EDITOR
}

