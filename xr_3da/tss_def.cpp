#include "stdafx.h"
#pragma hdrstop

#include "tss_def.h"

u32	SimulatorStates::record	()
{
	CHK_DX(HW.pDevice->BeginStateBlock());
	for (u32 it=0; it<States.size(); it++)
	{
		State& S = States[it];
		switch (S.type) 
		{
		case 0:	CHK_DX(HW.pDevice->SetRenderState((D3DRENDERSTATETYPE)S.v1,S.v2));	break;
		case 1: CHK_DX(HW.pDevice->SetTextureStageState(S.v1,(D3DTEXTURESTAGESTATETYPE)S.v2,S.v3));	break;
		}
	}
	u32 SB = 0;
	CHK_DX(HW.pDevice->EndStateBlock(reinterpret_cast<IDirect3DStateBlock9**>(&SB)));
	return SB;
}

void	SimulatorStates::set_SAMP(u32 a, u32 b, u32 c)
{
	// Search duplicates
	for (int t = 0; t<int(States.size()); t++)
	{
		State& S = States[t];
		if ((2 == S.type) && (a == S.v1) && (b == S.v2)) {
			States.erase(States.begin() + t);
			break;
		}
	}

	// Register
	State		st;
	st.set_SAMP(a, b, c);
	States.push_back(st);
}
