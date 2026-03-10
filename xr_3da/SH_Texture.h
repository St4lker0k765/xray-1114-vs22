#ifndef SH_TEXTURE_H
#define SH_TEXTURE_H
#pragma once

class ENGINE_API CAviPlayerCustom;

class ENGINE_API CTexture
{
public:
	DWORD						dwReference;
	IDirect3DTexture9*			pSurface;
	CAviPlayerCustom*			pAVI;
	DWORD						dwMemoryUsage;
	
	// Sequence data
	DWORD						seqMSPF;	// milliseconds per frame
	vector<IDirect3DTexture9*>	seqDATA;
	BOOL						seqCycles;

	DWORD						Calculate_MemUsage(IDirect3DTexture9* T);
public:
	void						Load		(LPCSTR Name);
	void						Unload		(void);
	void						Apply		(DWORD dwStage);
	
	void						surface_set	(IDirect3DTexture9* surf);
	IDirect3DTexture9*			surface_get ();

	CTexture					();
	virtual ~CTexture			();
};
#endif