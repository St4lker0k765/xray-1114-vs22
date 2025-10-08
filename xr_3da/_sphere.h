#ifndef _F_SPHERE_H_
#define _F_SPHERE_H_
#pragma once

#include <list>      // std::list
#include <cmath>     // std::sqrt
#include <cstddef>   // std::size_t
#include <algorithm> // std::min

// Предполагается, что _vector<T> определён в другом заголовке движка и имеет методы:
// set, sub, dotproduct, magnitude, square_magnitude, distance_to_sqr, direct [заглушки не добавляем здесь] [web:254].

// Константы движка, должны быть определены где-то в проекте.
// Здесь лишь используем их: EPS_S, PI_MUL_4 [web:254].

template <class T>
struct _sphere {
    _vector<T> P;
    T          R;
public:
    IC void set(const _vector<T>& _P, T _R) { P.set(_P); R = _R; }  // [web:254]
    IC void set(const _sphere<T>& S)        { P.set(S.P); R = S.R; } // [web:254]
    IC void identity()                      { P.set(T(0), T(0), T(0)); R = T(1); } // [web:254]

    // Ray-sphere intersection
    IC BOOL intersect(const _vector<T>& S, const _vector<T>& D) {
        _vector<T> Q; Q.sub(P, S);                      // this = P - S [web:254]
        T c = Q.magnitude();                            // [web:254]
        T v = Q.dotproduct(D);                          // [web:254]
        T d = R*R - (c*c - v*v);                        // [web:254]
        return d > 0;                                   // [web:254]
    }

    IC BOOL intersect(const _sphere<T>& S) const {
        T SumR = R + S.R;                               // [web:254]
        return P.distance_to_sqr(S.P) < SumR*SumR;      // [web:254]
    }

    IC BOOL contains(const _vector<T>& PT) const {
        return P.distance_to_sqr(PT) <= (R*R + EPS_S);  // [web:254]
    }

    // wholly contains
    IC BOOL contains(const _sphere<T>& S) const {
        const T RDiff = R - S.R;            // [web:254]
        if (RDiff < 0) return FALSE;        // [web:254]
        return P.distance_to_sqr(S.P) <= RDiff*RDiff; // [web:254]
    }

    IC T volume() const {
        return T(PI_MUL_4 / 3) * (R*R*R);   // [web:254]
    }

    void compute(const _vector<T>* verts, int count);       // [web:246]
    void compute_fast(const _vector<T>* verts, int count);  // опционально, тело не изменяем [web:246]
};

// ------------------------ Basis<T> ------------------------

template <class T>
class Basis
{
    using vec3D = _vector<T>;
private:
    enum { d = 3 };

    int    m = 0, s = 0;     // size and number of support vectors [web:246]
    vec3D  q0;

    T      z[d+1]{};         // [web:246]
    T      f[d+1]{};         // [web:246]
    vec3D  v[d+1];           // [web:246]
    vec3D  a[d+1];           // [web:246]
    vec3D  c[d+1];           // [web:246]
    T      sqr_r[d+1]{};     // [web:246]

    vec3D* current_c = nullptr;   // [web:246]
    T      current_sqr_r = T(-1); // [web:246]

    static T sqr(T r) { return r*r; } // [web:246]
public:
    Basis() { reset(); } // [web:246]

    const vec3D* center() const { return current_c; }              // [web:246]
    T            squared_radius() const { return current_sqr_r; }  // [web:246]
    int          size() const { return m; }                        // [web:246]
    int          support_size() const { return s; }                // [web:246]

    T excess(const vec3D& p) const {
        T e = -current_sqr_r;                 // [web:246]
        e += p.distance_to_sqr(*current_c);   // [web:246]
        return e;                             // [web:246]
    }

    void reset() {
        m = s = 0;                 // [web:246]
        c[0].set(T(0),T(0),T(0));  // [web:246]
        current_c     = c;         // [web:246]
        current_sqr_r = T(-1);     // [web:246]
    }

    bool push(const vec3D& p) {
        if (m == 0) {
            q0 = p;                // [web:246]
            c[0] = q0;             // [web:246]
            sqr_r[0] = T(0);       // [web:246]
        } else {
            const T eps = T(1e-16);         // [web:246]
            v[m].sub(p, q0);                // v_m = Q_m [web:246]
            for (int i = 1; i < m; ++i) {   // compute a_{m,i} [web:246]
                a[m][i] = v[i].dotproduct(v[m]);      // [web:246]
                a[m][i] *= (T(2) / z[i]);             // [web:246]
            }
            for (int i = 1; i < m; ++i) {             // update v_m [web:246]
                v[m].direct(v[m], v[i], -a[m][i]);    // [web:246]
            }
            z[m] = T(0);
            z[m] += v[m].square_magnitude();          // [web:246]
            z[m] *= T(2);                             // [web:246]
            if (z[m] < eps * current_sqr_r) {         // reject if too small [web:246]
                return false;                         // [web:246]
            }
            T e = -sqr_r[m-1];                        // [web:246]
            e += p.distance_to_sqr(c[m-1]);           // [web:246]
            f[m]   = e / z[m];                        // [web:246]
            c[m].direct(c[m-1], v[m], f[m]);          // [web:246]
            sqr_r[m] = sqr_r[m-1] + e * f[m] / T(2);  // [web:246]
        }
        current_c     = c + m;        // [web:246]
        current_sqr_r = sqr_r[m];     // [web:246]
        s = ++m;                      // [web:246]
        return true;                  // [web:246]
    }

    void pop() { --m; } // [web:246]
};

// ------------------------ Miniball<T> ------------------------

template <class T>
class Miniball
{
    using vec3D = _vector<T>;
public:
    using VectorList = std::list<vec3D>;
    using It  = typename VectorList::iterator;
    using Cit = typename VectorList::const_iterator;
private:
    VectorList L;  // точки [web:246]
    Basis<T>   B;  // текущая опорная база [web:246]
    It         support_end; // past-the-end итератор множества опоры [web:246]

    void mtf_mb(It i) {
        support_end = L.begin();            // [web:246]
        if (B.size() == 4) return;          // [web:246]
        for (It k = L.begin(); k != i; ) {  // [web:246]
            It j = k++;                     // [web:246]
            if (B.excess(*j) > 0) {         // [web:246]
                if (B.push(*j)) {           // [web:246]
                    mtf_mb(j);              // [web:246]
                    B.pop();                // [web:246]
                    move_to_front(j);       // [web:246]
                }                            // [web:246]
            }                                // [web:246]
        }                                    // [web:246]
    }

    void pivot_mb(It i) {
        It t = L.begin(); if (t != L.end()) ++t; // [web:246]
        mtf_mb(t);                                // [web:246]
        T max_e = T(0), old_sqr_r = T(0);         // [web:246]
        do {
            It pivot = L.begin();                 // [web:246]
            max_e = max_excess(t, i, pivot);      // [web:246]
            if (max_e > 0) {                      // [web:246]
                t = support_end;                  // [web:246]
                if (t == pivot) ++t;              // [web:246]
                old_sqr_r = B.squared_radius();   // [web:246]
                B.push(*pivot);                   // [web:246]
                mtf_mb(support_end);              // [web:246]
                B.pop();                          // [web:246]
                move_to_front(pivot);             // [web:246]
            }                                      // [web:246]
        } while ((max_e > 0) && (B.squared_radius() > old_sqr_r)); // [web:246]
    }

    void move_to_front(It j) {
        if (support_end == j) ++support_end;  // [web:246]
        L.splice(L.begin(), L, j);            // [web:246]
    }

    T max_excess(It t, It i, It& pivot) const {
        const vec3D* pCenter = B.center();    // [web:246]
        T sqr_r = B.squared_radius();         // [web:246]
        T e, max_e = T(0);                    // [web:246]
        for (It k = t; k != i; ++k) {         // [web:246]
            const vec3D& point = *k;          // [web:246]
            e = -sqr_r;                       // [web:246]
            e += point.distance_to_sqr(*pCenter); // [web:246]
            if (e > max_e) {                  // [web:246]
                max_e = e;                    // [web:246]
                pivot = k;                    // [web:246]
            }                                  // [web:246]
        }                                      // [web:246]
        return max_e;                          // [web:246]
    }
public:
    Miniball() = default;           // [web:246]
    void check_in(const vec3D& p) { L.push_back(p); } // [web:246]
    void build() { B.reset(); support_end = L.begin(); pivot_mb(L.end()); } // [web:246]

    vec3D center() const { return *B.center(); }   // [web:246]
    T     squared_radius() const { return B.squared_radius(); } // [web:246]
    int   num_points() const { return static_cast<int>(L.size()); } // [web:246]
    Cit   points_begin() const { return L.begin(); } // [web:246]
    Cit   points_end()   const { return L.end(); }   // [web:246]
    int   nr_support_gVectors() const { return B.support_size(); } // [web:246]
    Cit   support_points_begin() const { return L.begin(); }       // [web:246]
    Cit   support_points_end()   const { return support_end; }     // [web:246]
};

// ------------------------ _sphere<T>::compute ------------------------

template <class T>
void _sphere<T>::compute(const _vector<T>* verts, int count)
{
    Miniball<T> mb;                                 // [web:246]
    for (int i = 0; i < count; ++i) mb.check_in(verts[i]); // [web:246]
    mb.build();                                     // [web:246]
    P.set(mb.center());                             // [web:246]
    R = static_cast<T>(std::sqrt(static_cast<double>(mb.squared_radius()))); // [web:246]
}

// Быстрая версия — оставить декларацию, если не реализуется здесь [web:246]
template <class T>
void _sphere<T>::compute_fast(const _vector<T>* verts, int count)
{
    compute(verts, count); // по умолчанию делаем то же самое [web:246]
}

// Удобные типы
typedef _sphere<float>  Fsphere; // [web:254]
typedef _sphere<double> Dsphere; // [web:254]

#endif // _F_SPHERE_H_
