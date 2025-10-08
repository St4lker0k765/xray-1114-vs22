// -- cool defines --------------------------------------------------
#define _MANOWAR_SUPER_PROGRAMMER_
#define _TOP_SECRET_
#define _COOL_
#define _AMD_RULEZ_FOREVER_
#define _MS_BUGS_OFF_
// --  includes -----------------------------------------------------
#include "stdafx.h"
#include "..\bodyinstance.h"

#pragma warning (disable:4799) // inline asm in x86

#include <cstdint>
#include <cstring>
#include <mmintrin.h> // __m64, _mm_set_pi32

// Вспомогательные фабрики для инициализации __m64
inline __m64 m64_from_floats(float lo, float hi) {
    uint32_t lo_u, hi_u;
    static_assert(sizeof(float) == sizeof(uint32_t), "float size");
    std::memcpy(&lo_u, &lo, sizeof(float));
    std::memcpy(&hi_u, &hi, sizeof(float));
    return _mm_set_pi32(static_cast<int32_t>(hi_u), static_cast<int32_t>(lo_u));
}
inline __m64 m64_from_ints(int32_t lo, int32_t hi) {
    return _mm_set_pi32(hi, lo); // формирует (hi|lo)
}

// --  externs from 3DNow!Lib ---------------------------------------
extern void a_acos();
extern void alt_acos();
extern void SINCOSMAC();

// -- consts --------------------------------------------------------
__declspec(align(32)) static const __int64 _msgn_     = 0x8000000080000000ll;
__declspec(align(32)) static const float   _QEPSILON_ = 0.00001f;
__declspec(align(32)) static const float   _ONE_      = 1.0f;
__declspec(align(32)) static const __m64   _KEY_Q     = m64_from_floats(1.0f/32767.0f, 1.0f/32767.0f);

// -- xrBoneLerp_3DNow ----------------------------------------------
void __stdcall xrBoneLerp_3DNow(CKey* D, CKeyQ* K1, CKeyQ* K2, float delta)
{ 
    // -- local variables -------------------------------------------
    __declspec(align(32)) static __int64 Q0_1;      // Q0.y | Q0.x
    __declspec(align(32)) static __int64 Q0_2;      // Q0.w | Q0.z
    __declspec(align(32)) static __int64 Q1_1;      // Q1.y | Q1.x
    __declspec(align(32)) static __int64 Q1_2;      // Q1.w | Q1.z
    __declspec(align(32)) static __int64 omega;     // omega
    __declspec(align(32)) static __int64 rev_sinom; // 1/sinom
    __declspec(align(32)) static __int64 tomega;    // T*omega
    __declspec(align(32)) static __int64 scale0;    // scale0

    __asm{
        // ------------------------------------------------------------------
        femms                                   ; Clear MMX/3DNow! state
        // -- targets --------------------------------------------------------
        mov         esi, DWORD PTR [K1]
        mov         edi, DWORD PTR [K2]
        // -- KEY_QuantI -----------------------------------------------------
        movq        mm3, QWORD PTR [_KEY_Q]     ; mm3 = Quanti | Quanti
        // -- Q0 -------------------------------------------------------------
        movq        mm0, QWORD PTR [esi]K1.x    ; mm0 = K1.w | K1.z | K1.y | K1.x
        movq        mm2, mm0

        punpcklwd   mm0, mm0                    ; K1.y | K1.y | K1.x | K1.x
        punpckhwd   mm2, mm2                    ; K1.w | K1.w | K1.z | K1.z

        pi2fw       mm0, mm0                    ; K1.y(f) | K1.x(f)
        pi2fw       mm2, mm2                    ; K1.w(f) | K1.z(f)

        pfmul       mm0, mm3                    ; Q0.y | Q0.x
        pfmul       mm2, mm3                    ; Q0.w | Q0.z
        // -- Q1 -------------------------------------------------------------
        movq        mm1, QWORD PTR [edi]K2.x    ; mm1 = K2.w | K2.z | K2.y | K2.x
        movq        mm4, mm1

        punpcklwd   mm1, mm1                    ; K2.y | K2.y | K2.x | K2.x
        punpckhwd   mm4, mm4                    ; K2.w | K2.w | K2.z | K2.z

        pi2fw       mm1, mm1                    ; K2.y(f) | K2.x(f)
        pi2fw       mm4, mm4                    ; K2.w(f) | K2.z(f)

        pfmul       mm1, mm3                    ; Q1.y | Q1.x
        pfmul       mm4, mm3                    ; Q1.w | Q1.z
        // -- saving mm0 & mm2 ----------------------------------------------
        movq        mm3, mm0                    ; Q0.y | Q0.x
        movq        mm5, mm2                    ; Q0.w | Q0.z
        // -- cosom ----------------------------------------------------------
        pfmul       mm0, mm1                    ; Q0.y*Q1.y | Q0.x*Q1.x
        pfmul       mm2, mm4                    ; Q0.w*Q1.w | Q0.z*Q1.z
        pfadd       mm0, mm2                    ; sums
        pfacc       mm0, mm0                    ; cosom in both lanes
        // -- (cosom < 0) ----------------------------------------------------
        pxor        mm2, mm2                    ; 0
        pfcmpgt     mm2, mm0                    ; (0 > cosom) ? 0xFFFFFFFF : 0
        movd        eax, mm2
        test        eax, eax
        jz          cont_comp
        // -- change signs ---------------------------------------------------
        movq        mm2, QWORD PTR [_msgn_]     ; 0x80000000 | 0x80000000
        pxor        mm0, mm2                    ; -cosom
        pxor        mm1, mm2                    ; -Q1.y | -Q1.x
        pxor        mm4, mm2                    ; -Q1.w | -Q1.z
cont_comp:
        // -- (1.0f - cosom) > QEPSILON ) -----------------------------------
        movd        mm2, DWORD PTR [_ONE_]      ; 1.0
        pfsub       mm2, mm0                    ; 1.0 - cosom
        movd        mm6, DWORD PTR [_QEPSILON_] ; eps
        pfcmpgt     mm2, mm6
        movd        eax, mm2
        test        eax, eax
        // -- saving registers into locals ----------------------------------
        movq        QWORD PTR [Q0_1], mm3
        movq        QWORD PTR [Q0_2], mm5
        movq        QWORD PTR [Q1_1], mm1
        movq        QWORD PTR [Q1_2], mm4
        // ------------------------------------------------------------------
        jz          linear_interpol
        // -- real path ------------------------------------------------------
        call        alt_acos                    ; omega=acos(cosom) -> mm0
        movq        QWORD PTR [omega], mm0
        call        SINCOSMAC                   ; mm0 = sin|cos
        punpckhdq   mm0, mm0                    ; sin(omega)
        pfrcp       mm1, mm0                    ; ~1/sin(omega)
        movq        QWORD PTR [rev_sinom], mm1

        movd        mm1, DWORD PTR [delta]      ; T
        movq        mm0, QWORD PTR [omega]
        pfmul       mm1, mm0                    ; T*omega
        movq        QWORD PTR [tomega], mm1
        pfsub       mm0, mm1                    ; omega - T*omega
        call        SINCOSMAC                   ; sin(omega - T*omega)
        punpckhdq   mm0, mm0
        pfmul       mm0, QWORD PTR [rev_sinom]  ; scale0 = sin(omega - tomega)/sin(omega)
        movq        QWORD PTR [scale0], mm0
        movq        mm0, QWORD PTR [tomega]
        call        SINCOSMAC                   ; sin(tomega)
        punpckhdq   mm0, mm0                    ; scale1 in mm0 (sin(tomega))
        pfmul       mm0, QWORD PTR [rev_sinom]  ; scale1 = sin(tomega)/sin(omega)
        jmp short   scale_it
        // ------------------------------------------------------------------
        ALIGN       16
linear_interpol:
        movd        mm7, DWORD PTR [_ONE_]      ; 1.0
        movd        mm0, DWORD PTR [delta]      ; T
        pfsub       mm7, mm0                    ; 1.0 - T
        movq        QWORD PTR [scale0], mm7     ; scale0 = 1 - T
        // scale1 уже в mm0 (=T)
        // ------------------------------------------------------------------
scale_it:
        // -- restoring registers --------------------------------------------
        movq        mm7, QWORD PTR [Q0_1]
        movq        mm6, QWORD PTR [Q0_2]
        movq        mm5, QWORD PTR [Q1_1]
        movq        mm4, QWORD PTR [Q1_2]
        movq        mm1, QWORD PTR [scale0]
        // -- unpack low -----------------------------------------------------
        punpckldq   mm0, mm0                    ; scale1 | scale1
        punpckldq   mm1, mm1                    ; scale0 | scale0
        // -- scaling ---------------------------------------------------------
        pfmul       mm7, mm1                    ; Q0.xy * s0
        pfmul       mm5, mm0                    ; Q1.xy * s1
        pfmul       mm6, mm1                    ; Q0.wz * s0
        pfmul       mm4, mm0                    ; Q1.wz * s1
        pfadd       mm7, mm5
        pfadd       mm6, mm4
        // -- storing results ------------------------------------------------
        mov         edx, DWORD PTR [D]D.Q
        movq        QWORD PTR [edx]D.Q.x, mm7   ; D.Q.x, D.Q.y
        movq        QWORD PTR [edx]D.Q.z, mm6   ; D.Q.z, D.Q.w
        // -- lerp position --------------------------------------------------
        mov         esi, DWORD PTR [K1]
        mov         edi, DWORD PTR [K2]
        mov         edx, DWORD PTR [D]
        
        movd        mm7, DWORD PTR [_ONE_]      ; 1.0
        movd        mm6, DWORD PTR [delta]      ; T
        pfsub       mm7, mm6                    ; 1.0 - T

        movq        mm0, QWORD PTR [esi]K1.T.x  ; p1.y | p1.x
        movd        mm1, DWORD PTR [esi]K1.T.z  ; 0.0 | p1.z

        punpckldq   mm6, mm6                    ; T | T
        punpckldq   mm7, mm7                    ; invT | invT

        movq        mm2, QWORD PTR [edi]K2.T.x  ; p2.y | p2.x
        movd        mm3, DWORD PTR [edi]K2.T.z  ; 0.0 | p2.z

        pfmul       mm0, mm7
        pfmul       mm1, mm7
        pfmul       mm2, mm6
        pfmul       mm3, mm6

        pfadd       mm0, mm2
        pfadd       mm1, mm3

        movq        QWORD PTR [edx]D.T.x, mm0   ; D.T.x, D.T.y
        movd        DWORD PTR [edx]D.T.z, mm1   ; D.T.z
        // ------------------------------------------------------------------
        femms                                   ; cleanup
        // ------------------------------------------------------------------
    }
}

#pragma warning (default:4799)
