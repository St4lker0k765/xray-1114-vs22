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

    BOOL bWindowed = !(psDeviceFlags & rsFullscreen);

    DWORD dwWindowStyle;
    if (bWindowed) SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle = (WS_BORDER | WS_DLGFRAME | WS_VISIBLE));
    else           SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle = (WS_POPUP | WS_VISIBLE));

    D3DADAPTER_IDENTIFIER8 adapterID;
    R_CHK(pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, D3DENUM_NO_WHQL_LEVEL, &adapterID));
    Msg("* Video board: %s", adapterID.Description);

    D3DDISPLAYMODE mWindowed;
    R_CHK(pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mWindowed));

    {
        u32 w = 0, h = 0;
        selectResolution(w, h, bWindowed);
        dwWidth = w;
        dwHeight = h;
    }

    D3DFORMAT& fTarget = Caps.fTarget;
    D3DFORMAT& fDepth  = Caps.fDepth;

    if (bWindowed)
    {
        fTarget = mWindowed.Format;
        R_CHK(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, TRUE));
        fDepth = selectDepthStencil(fTarget);
    }
    else
    {
        switch (psCurrentBPP) {
        case 32:
            fTarget = D3DFMT_X8R8G8B8;
            if (SUCCEEDED(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_R8G8B8;
            if (SUCCEEDED(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_UNKNOWN;
            break;
        case 16:
        default:
            fTarget = D3DFMT_R5G6B5;
            if (SUCCEEDED(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_X1R5G5B5;
            if (SUCCEEDED(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_X4R4G4B4;
            if (SUCCEEDED(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_UNKNOWN;
            break;
        }
        fDepth = selectDepthStencil(fTarget);
    }

    R_ASSERT(fTarget != D3DFMT_UNKNOWN);
    R_ASSERT(fDepth  != D3DFMT_UNKNOWN);

    D3DPRESENT_PARAMETERS P;
    ZeroMemory(&P, sizeof(P));

    P.BackBufferWidth  = dwWidth;
    P.BackBufferHeight = dwHeight;
    P.BackBufferFormat = fTarget;
    P.BackBufferCount  = bWindowed ? 1 : ((psDeviceFlags & rsTriplebuffer) ? 2 : 1);

    if ((!bWindowed) && (psDeviceFlags & rsAntialias))
        P.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;
    else
        P.MultiSampleType = D3DMULTISAMPLE_NONE;

    P.SwapEffect     = D3DSWAPEFFECT_DISCARD;
    P.hDeviceWindow  = m_hWnd;
    P.Windowed       = bWindowed;

    P.EnableAutoDepthStencil = TRUE;
    P.AutoDepthStencilFormat = fDepth;

    if (!bWindowed)
        P.FullScreen_RefreshRateInHz = selectRefresh(dwWidth, dwHeight);
    else
        P.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

    DWORD GPU = selectGPU();
    R_CHK(HW.pD3D->CreateDevice(D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        m_hWnd,
        GPU,
        &P,
        &pDevice));

    CurrBBWidth  = P.BackBufferWidth;
    CurrBBHeight = P.BackBufferHeight;

    switch (GPU)
    {
    case D3DCREATE_SOFTWARE_VERTEXPROCESSING:      Log("* Geometry Processor: SOFTWARE"); break;
    case D3DCREATE_MIXED_VERTEXPROCESSING:         Log("* Geometry Processor: MIXED");    break;
    case D3DCREATE_HARDWARE_VERTEXPROCESSING:      Log("* Geometry Processor: HARDWARE"); break;
    case D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE:
        Log("* Geometry Processor: PURE HARDWARE"); break;
    }

#ifndef _EDITOR
    fill_vid_mode_list(this);
#endif

    return dwWindowStyle;
}

void CHW::updateWindowProps(HWND m_hWnd)
{
#ifndef DEDICATED_SERVER
    BOOL bWindowed = !(psDeviceFlags & rsFullscreen);
#else
    BOOL bWindowed = TRUE;
#endif

    u32 dwWindowStyle = 0;

    const int W = (int)CurrBBWidth;
    const int H = (int)CurrBBHeight;

    if (bWindowed)
    {
        SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle = (WS_BORDER | WS_DLGFRAME | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX));

        RECT m_rcWindowBounds;
        BOOL bCenter = FALSE;

        if (strstr(GetCommandLineA(), "-center_screen")) bCenter = TRUE;
#ifdef DEDICATED_SERVER
        bCenter = TRUE;
#endif

        if (bCenter)
        {
            RECT DesktopRect;
            GetClientRect(GetDesktopWindow(), &DesktopRect);

            SetRect(&m_rcWindowBounds,
                (DesktopRect.right - W) / 2,
                (DesktopRect.bottom - H) / 2,
                (DesktopRect.right + W) / 2,
                (DesktopRect.bottom + H) / 2);
        }
        else
        {
            SetRect(&m_rcWindowBounds, 0, 0, W, H);
        }

        AdjustWindowRect(&m_rcWindowBounds, dwWindowStyle, FALSE);

        SetWindowPos(m_hWnd,
            HWND_TOP,
            m_rcWindowBounds.left,
            m_rcWindowBounds.top,
            (m_rcWindowBounds.right - m_rcWindowBounds.left),
            (m_rcWindowBounds.bottom - m_rcWindowBounds.top),
            SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_DRAWFRAME);
    }
    else
    {
        SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle = (WS_POPUP | WS_VISIBLE));
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
#ifndef DEDICATED_SERVER
    BOOL bWindowed = !(psDeviceFlags & rsFullscreen);
#else
    BOOL bWindowed = TRUE;
#endif

    u32 w = 0, h = 0;
    selectResolution(w, h, bWindowed);

    D3DDISPLAYMODE mWindowed;
    R_CHK(pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mWindowed));

    D3DFORMAT fTarget = Caps.fTarget;
    D3DFORMAT fDepth  = Caps.fDepth;
    if (bWindowed)
    {
        fTarget = mWindowed.Format;
        R_CHK(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, TRUE));
        fDepth = selectDepthStencil(fTarget);
    }
    else
    {
        switch (psCurrentBPP) {
        case 32:
            fTarget = D3DFMT_X8R8G8B8;
            if (SUCCEEDED(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, FALSE))) break;
            fTarget = D3DFMT_R8G8B8;
            if (SUCCEEDED(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, FALSE))) break;
            fTarget = D3DFMT_UNKNOWN;
            break;
        case 16:
        default:
            fTarget = D3DFMT_R5G6B5;
            if (SUCCEEDED(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, FALSE))) break;
            fTarget = D3DFMT_X1R5G5B5;
            if (SUCCEEDED(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, FALSE))) break;
            fTarget = D3DFMT_X4R4G4B4;
            if (SUCCEEDED(pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, fTarget, fTarget, FALSE))) break;
            fTarget = D3DFMT_UNKNOWN;
            break;
        }
        fDepth = selectDepthStencil(fTarget);
    }
    R_ASSERT(fTarget != D3DFMT_UNKNOWN);
    R_ASSERT(fDepth  != D3DFMT_UNKNOWN);

    D3DPRESENT_PARAMETERS P;
    ZeroMemory(&P, sizeof(P));
    P.BackBufferWidth  = w;
    P.BackBufferHeight = h;
    P.BackBufferFormat = fTarget;
    P.BackBufferCount  = bWindowed ? 1 : ((psDeviceFlags & rsTriplebuffer) ? 2 : 1);
    P.MultiSampleType  = ((!bWindowed) && (psDeviceFlags & rsAntialias)) ? D3DMULTISAMPLE_2_SAMPLES : D3DMULTISAMPLE_NONE;
    P.SwapEffect       = bWindowed ? D3DSWAPEFFECT_COPY : D3DSWAPEFFECT_DISCARD;
    P.hDeviceWindow    = hwnd;
    P.Windowed         = bWindowed;
    P.EnableAutoDepthStencil = TRUE;
    P.AutoDepthStencilFormat = fDepth;
    if (!bWindowed)
        P.FullScreen_RefreshRateInHz = selectRefresh(w, h);
    else
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
