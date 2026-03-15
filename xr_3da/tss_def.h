#ifndef TSS_DEF_H
#define TSS_DEF_H

#pragma once

class	ENGINE_API SimulatorStates
{
private:
	struct State
	{
		u32	type;		// 0=RS, 1=TSS
		u32	v1,v2,v3;
		
		IC void	set_RS	(u32 a, u32 b)
		{
			type	= 0;
			v1		= a;
			v2		= b;
			v3		= 0;
		}
		IC void	set_TSS	(u32 a, u32 b, u32 c)
		{
			type	= 1;
			v1		= a;
			v2		= b;
			v3		= c;
		}
		IC void set_SAMP(u32 a, u32 b, u32 c)
		{
			type	= 2;
			v1		= a;
			v2		= b;
			v3		= c;
		}
	};
private:
	vector<State>	States;
public:
	IC void			set_RS	(u32 a, u32 b)
	{
		State		st;
		st.set_RS	(a,b);
		States.push_back(st);
	}
	IC void			set_TSS	(u32 a, u32 b, u32 c)
	{
		State		st;
		st.set_TSS	(a,b,c);
		States.push_back(st);
	}
	void					set_SAMP(u32 a, u32 b, u32 c);
	IC BOOL			equal	(SimulatorStates& S)
	{
		if (States.size()!=S.States.size())	return FALSE;
		if (0!=memcmp(&*States.begin(),&*S.States.begin(),States.size()*sizeof(State))) return FALSE;
		return TRUE;
	}
	IC void			clear	()
	{
		States.clear();
	}

	IDirect3DStateBlock9*			record	();
};
#endif