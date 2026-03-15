#include "stdafx.h"
#include "xr_trims.h"
#include "xr_tokens.h"

LPSTR _TrimLeft( LPSTR str )
{
	LPSTR p 	= str;
	while( *p && (u8(*p)<=u8(' ')) ) p++;
    if (p!=str){
		LPSTR t = str;
        for (; *p; t++,p++) *t=*p;
        *t = 0;
    }
	return str;
}

LPSTR _TrimRight( LPSTR str )
{
	LPSTR p 	= str+strlen(str);
	while( (p!=str) && (u8(*p)<=u8(' ')) ) p--;
    *(++p) 		= 0;
	return str;
}

LPSTR _Trim( LPSTR str )
{
	_TrimLeft( str );
	_TrimRight( str );
	return str;
}

char* _GetFileExt ( char* name )
{
	char *point = strchr(name,'.');
	if (point) return point+1;
	return NULL;
}

const char* _SetPos (const char* src, DWORD pos )
{
	const char*	res			= src;
	DWORD		p			= 0;
	while( p<pos && (res=strchr(res,',')) )
	{
		res		++;
		p		++;
	}
	return		res;
}

char* _CopyVal ( const char* src, char* dst )
{
	const char*	p;
	DWORD		n;
	p			= strchr	( src, ',' );
	n			= (p>0) ? (p-src) : strlen(src);
	strncpy		( dst, src, n );
	dst[n]		= 0;
	return		dst;
}

int				_GetItemCount ( const char* src )
{
	const char*	res			= src;
	DWORD		p			= 0;
	while( res=strchr(res,',') )
	{
		res		++;
		p		++;
	}
	return		++p;
}

char* _GetItem ( const char* src, int index, char* dst, char* def )
{
	const char*	ptr;
	ptr			= _SetPos	( src, index );
	if( ptr )	_CopyVal	( ptr, dst );
		else	strcpy		( dst, def );
	_Trim( dst );
	return		dst;
}

DWORD _ParseItem ( char* src, xr_token* token_list )
{
	for( int i=0; token_list[i].name; i++ )
		if( !_stricmp(src,token_list[i].name) )
			return token_list[i].id;
	return -1;
}

DWORD _ParseItem ( char* src, int ind, xr_token* token_list )
{
	char dst[128];
	_GetItem(src, ind, dst);
	return _ParseItem(dst, token_list);
}

char* _ChangeSymbol ( char* name, char src, char dest )
{
    char						*sTmpName = name;
    while(sTmpName[0] ){
		if (sTmpName[0] == src) sTmpName[0] = dest;
		sTmpName ++;
	}
	return						name;
}




