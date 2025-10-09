// HW.h: interface for the CHW class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_)
#define AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_

#pragma once

class ENGINE_API CHW {
public:
	IDirect3D8* 			pD3D;		// direct 3d
	IDirect3DDevice8*       pDevice;	// render device

	IDirect3DSurface8*		pBaseRT;
	IDirect3DSurface8*		pBaseZB;

	u32                     CurrBBWidth;
	u32                     CurrBBHeight;

	CHWCaps					Caps;

	CHW()
	{ ZeroMemory(this, sizeof(CHW)); };

	void					CreateD3D				();
	void					DestroyD3D				();
	DWORD					CreateDevice			(HWND hw,DWORD &dwWidth,DWORD &dwHeight);
	void					DestroyDevice			();

	void					Reset(HWND hw);

    // Selection helpers
    D3DFORMAT               selectDepthStencil      (D3DFORMAT);
    DWORD                   selectPresentInterval   ();
    DWORD                   selectGPU               ();
    DWORD                   selectRefresh           (DWORD dwWidth, DWORD dwHeight);
    void                    selectResolution        (u32 &dwWidth, u32 &dwHeight, BOOL bWindowed);

	void					updateWindowProps		(HWND hw);

#ifdef DEBUG
	void	Validate(void)	{	VERIFY(pDevice); VERIFY(pD3D); };
#else
	void	Validate(void)	{};
#endif
};

extern ENGINE_API CHW HW;

#ifndef _EDITOR
void fill_vid_mode_list      (CHW* _hw); 
void free_vid_mode_list      ();
#else
inline void fill_vid_mode_list(CHW* ) {}
inline void free_vid_mode_list() {}
#endif

#endif // !defined(AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_)
