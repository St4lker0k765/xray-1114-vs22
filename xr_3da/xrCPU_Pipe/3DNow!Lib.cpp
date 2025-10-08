//****************************************************************************** 
#include "stdafx.h"
//******************************************************************************
#pragma warning (disable:4799) // inline asm (MMX/3DNow!)
//******************************************************************************
// x86 MMX/3DNow! константы и фабрики
#include <cstdint>
#include <mmintrin.h> // __m64, _mm_set_pi32

// ‘абрика дл€ сборки __m64 из двух float (lo, hi)
inline __m64 m64_from_floats(float lo, float hi) {
    uint32_t lo_u, hi_u;
    static_assert(sizeof(float) == sizeof(uint32_t), "float size");
    std::memcpy(&lo_u, &lo, sizeof(float));
    std::memcpy(&hi_u, &hi, sizeof(float));
    return _mm_set_pi32(static_cast<int32_t>(hi_u), static_cast<int32_t>(lo_u));
}

// ‘абрика дл€ сборки __m64 из двух int32 (lo, hi)
inline __m64 m64_from_ints(int32_t lo, int32_t hi) {
    return _mm_set_pi32(hi, lo); // формирует (hi|lo)
}

//******************************************************************************
// Vector (3DNow!) data 
//******************************************************************************
static const __m64  PMOne      = m64_from_floats( 1.0f,        -1.0f       );
static const __m64  HalfVal    = m64_from_floats( 0.5f,         0.5f       );
static const __m64  HalfMin    = m64_from_floats( 0.5f,        -0.5f       );
static const __m64  ones       = m64_from_floats( 1.0f,         1.0f       );
static const __m64  twos       = m64_from_floats( 2.0f,         2.0f       );
static const __m64  izeros     = m64_from_ints  ( 0,            0          );
static const __m64  pinfs      = m64_from_ints  ( 0x7f800000,   0x7f800000 ); // PINH, PINH
static const __m64  smh_masks  = m64_from_ints  ( 0x807fffff,   0x807fffff );
static const __m64  sign_mask  = m64_from_ints  ( 0x7fffffff,   0x7fffffff );
static const __m64  sh_masks   = m64_from_ints  ( 0x80000000,   0x80000000 );
static const __m64  two_126s   = m64_from_ints  ( 126,          126        );
// SINCOS specific values
static const __m64  mo2s       = m64_from_floats(-0.5f,        -0.5f       );
static const __m64  mo12_6     = m64_from_floats(-0.0833333333f,-0.166666667f);
static const __m64  mo30_20    = m64_from_floats(-0.0333333333f,-0.05f     );
static const __m64  mo56_42    = m64_from_floats(-0.0178571f,  -0.0238095f );
static const __m64  pio4ht     = m64_from_ints  ( 0xbf490000,   0xb97daa22  ); // approx -0.000241913 | -0.785156
static const __m64  pio4s      = m64_from_ints  ( 0x3f490fdb,   0x3f490fdb  ); // 0.785398 | 0.785398
//******************************************************************************
// Scalar (single float) data (как и было)
static const __int32 fouropi    =       0x3fa2f983;             // 1.27324f
static const __int32 xmax       =       0x46c90fdb;             // 25735.9
static const __int32 sgn        =       0x80000000;
static const __int32 mabs       =       0x7FFFFFFF;
static const __int32 mant       =       0x007FFFFF;
static const __int32 expo       =       0x7F800000;
static const __int32 one        =       0x3F800000;
static const __int32 half       =       0x3F000000;
static const __int32 two        =       0x40000000;
static const __int32 oob        =       0x00000000;
static const __int32 huynan     =       0x7fffffff;
static const __int32 pnan       =       0x7fc00000;
static const __int32 n0         =       0x40A008EF;
static const __int32 n1         =       0x3DAA7B3D;
static const __int32 d0         =       0x412008EF;
static const __int32 qq0        =       0x419D92C8;
static const __int32 qq1        =       0x41E6BD60;
static const __int32 qq2        =       0x41355DC0;
static const __int32 pp0        =       0xC0D21907;
static const __int32 pp1        =       0xC0B59883;
static const __int32 pp2        =       0xBF52C7EA;
static const __int32 bnd        =       0x3F133333;
static const __int32 asp0       =       0x3F6A4AA5;
static const __int32 asp1       =       0xBF004C2C;
static const __int32 asq0       =       0x40AFB829;
static const __int32 asq1       =       0xC0AF5123;
static const __int32 pio2       =       0x3FC90FDB;
static const __int32 npio2      =       0xBFC90FDB;
static const __int32 ooln2      =       0x3FB8AA3B;
static const __int32 upper      =       0x42B17218;
static const __int32 lower      =       0xC2AEAC50;
static const __int32 ln2hi      =       0x3F317200;
static const __int32 ln2lo      =       0x35BFBE8E;
static const __int32 rt2        =       0x3FB504F3;
static const __int32 edec       =       0x00800000;
static const __int32 bias       =       0x0000007F;
static const __int32 c2         =       0x3E18EFE2;
static const __int32 c1         =       0x3E4CAF6F;
static const __int32 c0         =       0x3EAAAABD;
static const __int32 tl2e       =       0x4038AA3B;
static const __int32 maxn       =       0xFF7FFFFF;
static const __int32 q1         =       0x43BC00B5;
static const __int32 p1         =       0x41E77545;
static const __int32 q0         =       0x45E451C5;
static const __int32 p0         =       0x451E424B;
static const __int32 mine       =       0xC2FC0000;
static const __int32 maxe       =       0x43000000;
static const __int32 max        =       0x7F7FFFFF;
static const __int32 rle10      =       0x3ede5bdb;
// For alt_acos
static const __m64  a_c7        = m64_from_floats( 2.838933f, 0.f );
static const __m64  a_c5        = m64_from_floats(-3.853735f, 0.f );
static const __m64  a_c3        = m64_from_floats( 1.693204f, 0.f );
static const __m64  a_c1        = m64_from_floats( 0.892399f, 0.f );
static const __m64  a_pi_div_2  = m64_from_floats( PI_DIV_2 , 0.f );
static const __int64 _msgn_     = 0x8000000080000000ll;
//******************************************************************************
// Routine:  alt_acos
// Input:    mm0.lo
// Result:   mm0.lo
// Uses:     mm0-mm3
// Comment:
//   Compute acos(x) using MMX and 3DNow! instructions. Scalar version.
//   © by OlesЩ, © by ManOwaRЩ
//******************************************************************************
__declspec(naked) void alt_acos(void)
{
    __asm {
        // ------------------------------------------------------------------
        movq        mm3, QWORD PTR [a_c7]         // mm3 = 0.0 | c7
        movq        mm2, QWORD PTR [a_pi_div_2]   // mm2 = 0.0 | PI_DIV_2
        movq        mm1, mm0                      // mm1 = ?.? | x
        pfmul       mm1, mm1                      // x2
        pfmul       mm3, mm1                      // c7*x2
        pfadd       mm3, QWORD PTR [a_c5]
        pfmul       mm3, mm1
        pfadd       mm3, QWORD PTR [a_c3]
        pfmul       mm3, mm1
        pfadd       mm3, QWORD PTR [a_c1]
        pfmul       mm0, mm3
        pfsub       mm2, mm0
        movq        mm0, mm2
        ret
        // ------------------------------------------------------------------
    }
}

//******************************************************************************
// SINCOSMAC - sin/cos simultaneous computation
// Input:    mm0 - angle in radians
// Output:   mm0 - (sin|cos)
// Uses:     mm0-mm7, eax, ebx, ecx, edx, esi
//******************************************************************************
__declspec (naked) void SINCOSMAC ()
{
    __asm {
        push        ebx
        movd        eax,mm0
        movq        mm1,mm0
        movd        mm3,[mabs]
        mov         ebx,eax
        mov         edx,eax
        pand        mm0,mm3                 // m0 = fabs(x)
        and         ebx,0x80000000          // get sign bit
        shr         edx,01fh                // edx = (r0 < 0) ? 1: 0
        xor         eax,ebx                 // eax = fabs(eax)
        cmp         eax,[xmax]
        movd        mm2,[fouropi]
        jl          short x2
        movd        mm0,[one]
        jmp         ending
        ALIGN       16
x2:
        movq        mm1,mm0
        pfmul       mm0,mm2                 // fabs(x) * 4 / PI
        movq        mm3,[pio4ht]
        pf2id       mm0,mm0
        movq        mm7,[mo56_42]
        movd        ecx,mm0
        pi2fd       mm0,mm0
        mov         esi,ecx
        movq        mm6,[mo30_20]
        punpckldq   mm0,mm0
        movq        mm5,[ones]
        pfmul       mm0,mm3
        pfadd       mm1,mm0
        shr         esi,2
        punpckhdq   mm0,mm0
        xor         edx,esi
        pfadd       mm1,mm0
        test        ecx,1
        punpckldq   mm1,mm1
        jz          short x5
        pfsubr      mm1,[pio4s]
x5:     movq        mm2,mm5
        shl         edx,01fh
        punpckldq   mm2,mm1
        pfmul       mm1,mm1
        mov         esi,ecx
        movq        mm4,[mo12_6]
        shr         esi,1
        pfmul       mm7,mm1
        xor         ecx,esi
        pfmul       mm6,mm1
        shl         esi,01fh
        pfadd       mm7,mm5
        xor         ebx,esi
        pfmul       mm4,mm1
        pfmul       mm7,mm6
        movq        mm6,[mo2s]
        pfadd       mm7,mm5
        pfmul       mm6,mm1
        pfmul       mm4,mm7
        movd        mm0,edx
        pfadd       mm4,mm5
        punpckldq   mm6,mm5
        psrlq       mm5,32
        pfmul       mm4,mm6
        punpckldq   mm0,mm0
        movd        mm1,ebx
        pfadd       mm4,mm5
        test        ecx,1
        pfmul       mm4,mm2
        jz          short x7
        punpckldq   mm5,mm4
        punpckhdq   mm4,mm5
x7:     pxor        mm4,mm1
        pxor        mm0,mm4

ending:
        pop         ebx
        nop
        ret
    }
}

//******************************************************************************
// Routine:  a_acos
// Input:    mm0.lo
// Result:   mm0.lo
// Uses:     mm0-mm7
//******************************************************************************
__declspec (naked) void a_acos ()
{
    __asm
    {
        movd        mm6, [sgn]  // mask for sign bit
        movd        mm7, [mabs] // mask for absolute value
        movd        mm4, [half] // 0.5
        pand        mm6, mm0    // extract sign bit
        movd        mm5, [one]  // 1.0
        pand        mm0, mm7    // z = abs(x)
        movq        mm3, mm0    // z
        pcmpgtd     mm3, mm5    // z > 1.0 ? 0xFFFFFFFF : 0
        movq        mm5, mm0    // save z
        pfmul       mm0, mm4    // z*0.5
        movd        mm2, [bnd]  // 0.575
        pfsubr      mm0, mm4    // 0.5 - z * 0.5
        pfrsqrt     mm7, mm0    // 1/sqrt((1-z)/2) approx low
        movq        mm1, mm7
        pfmul       mm7, mm7
        pcmpgtd     mm2, mm5
        pfrsqit1    mm7, mm0
        movd        mm4, [asq1]
        pfrcpit2    mm7, mm1
        pfmul       mm7, mm0
        movq        mm0, mm2
        pand        mm5, mm2
        pandn       mm0, mm7
        movd        mm7, [asp1]
        por         mm0, mm5
        movq        mm1, mm0
        pfmul       mm0, mm0
        movd        mm5, [asp0]
        pfmul       mm7, mm0
        pfadd       mm4, mm0
        pfadd       mm7, mm5
        movd        mm5, [asq0]
        pfmul       mm7, mm0
        pfmul       mm0, mm4
        pfadd       mm0, mm5
        pfmul       mm7, mm1
        pfrcp       mm4, mm0
        pfrcpit1    mm0, mm4
        movd        mm5, [npio2]
        pfrcpit2    mm0, mm4
        pfmul       mm0, mm7
        movq        mm4, mm2
        pandn       mm2, mm5
        movd        mm5, [pio2]
        movq        mm7, mm4
        pxor        mm2, mm6
        pfadd       mm1, mm0
        movd        mm0, [oob]
        pfadd       mm2, mm5
        pslld       mm7, 31
        pandn       mm4, mm1
        pxor        mm7, mm6
        pfadd       mm1, mm4
        pand        mm0, mm3
        por         mm1, mm7
        pfadd       mm2, mm1
        pandn       mm3, mm2
        por         mm0, mm3
        ret
    }
}

//******************************************************************************
// Routine:  a_asin
// Input:    mm0.lo
// Result:   mm0.lo
// Uses:     mm0-mm7
//******************************************************************************
__declspec (naked) void a_asin ()
{
    __asm
    {
        movd        mm6, [sgn]
        movd        mm7, [mabs]
        movd        mm4, [half]
        pand        mm6, mm0
        movd        mm5, [one]
        pand        mm0, mm7
        movq        mm3, mm0
        pcmpgtd     mm3, mm5
        movq        mm5, mm0
        pfmul       mm0, mm4
        movd        mm2, [bnd]
        pfsubr      mm0, mm4
        pfrsqrt     mm7, mm0
        movq        mm1, mm7
        pfmul       mm7, mm7
        pcmpgtd     mm2, mm5
        pfrsqit1    mm7, mm0
        movd        mm4, [asq1]
        pfrcpit2    mm7, mm1
        pfmul       mm7, mm0
        movq        mm0, mm2
        pand        mm5, mm2
        pandn       mm0, mm7
        movd        mm7, [asp1]
        por         mm0, mm5
        movq        mm1, mm0
        pfmul       mm0, mm0
        movd        mm5, [asp0]
        pfmul       mm7, mm0
        pfadd       mm4, mm0
        pfadd       mm7, mm5
        movd        mm5, [asq0]
        pfmul       mm7, mm0
        pfmul       mm0, mm4
        pfadd       mm0, mm5
        pfmul       mm7, mm1
        pfrcp       mm4, mm0
        pfrcpit1    mm0, mm4
        pfrcpit2    mm0, mm4
        movd        mm4, [pio2]
        pfmul       mm7, mm0
        movd        mm0, [oob]
        pfadd       mm1, mm7
        movq        mm5, mm1
        pfadd       mm1, mm1
        pfsubr      mm1, mm4
        pand        mm5, mm2
        pandn       mm2, mm1
        pand        mm0, mm3
        por         mm2, mm5
        por         mm2, mm6
        pandn       mm3, mm2
        por         mm0, mm3
        ret
    }
}

//******************************************************************************
// Routine:  a_sin
// Input:    mm0.lo
// Result:   mm0 (sin|sin)
// Uses:     mm0-mm7, eax, ebx, ecx, edx, esi
//******************************************************************************
__declspec (naked) void a_sin ()
{
    __asm
    {
        call        SINCOSMAC          // mm0 = cos(x) | sin(x)
        punpckhdq   mm0,mm0            // select sin value
        ret
    }
}

//******************************************************************************
#pragma warning (default:4799)
//******************************************************************************
