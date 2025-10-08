#ifndef _F_SPHERE_H_
#define _F_SPHERE_H_

template <class T>
struct _sphere {
	_vector<T>	P;
	T			R;
public:
	IC void		set(const _vector<T> &_P, T _R)		{ P.set(_P); R = _R; }
	IC void		set(const _sphere<T> &S)			{ P.set(S.P); R=S.R; }
	IC void		identity()							{ P.set(0,0,0); R=1; }

	// Ray-sphere intersection
	IC BOOL		intersect(const _vector<T>& S, const _vector<T>& D)	
	{
		_vector<T> Q;	Q.sub(P,S);
	
		T c = Q.magnitude	();
		T v = Q.dotproduct	(D);
		T d = R*R - (c*c - v*v);
		return (d > 0);
	}
	IC BOOL		intersect(const _sphere<T>& S) const
	{	
		T SumR = R+S.R;
		return P.distance_to_sqr(S.P) < SumR*SumR;
	}
	IC BOOL		contains(const _vector<T>& PT) const 
	{
		return P.distance_to_sqr(PT) <= (R*R+EPS_S);
	}
	
	// returns true if this wholly contains the argument sphere
	IC BOOL		contains(const _sphere<T>& S) const	
	{
		// can't contain a sphere that's bigger than me !
		const T RDiff		= R - S.R;
		if ( RDiff < 0 )	return false;

		return ( P.distance_to_sqr(S.P) <= RDiff*RDiff );
	}

	// return's volume of sphere
	IC T		volume	() const
	{
		return T( PI_MUL_4 / 3 ) * (R*R*R);
	}
	void		compute			(const _vector<T> *verts, int count);
	void		compute_fast	(const _vector<T> *verts, int count);
};

class Miniball;
class Basis;

// Basis
// -----
class Basis
{
private:
	enum { d = 3 }		eDimensions;

	// data members
	int					m, s;			// size and number of support vectors

	Fvector				q0;

	float				z[d + 1];
	float				f[d + 1];
	Fvector				v[d + 1];
	Fvector				a[d + 1];
	Fvector				c[d + 1];
	float				sqr_r[d + 1];

	Fvector* current_c;      // vectors to some c[j]
	float				current_sqr_r;
public:
	Basis();

	// access
	const Fvector* center() const;
	float				squared_radius() const;
	int                 size() const;
	int                 support_size() const;
	float				excess(const Fvector& p) const;

	// modification
	void                reset(); // generates empty sphere with m=s=0
	bool                push(const Fvector& p);
	void                pop();
};

// Miniball
// --------
class Miniball
{
public:
	// types
	typedef std::list<Fvector>			VectorList;
	typedef VectorList::iterator        It;
	typedef VectorList::const_iterator  Cit;

private:
	// data members
	VectorList	L;						// STL list keeping the gVectors
	Basis		B;						// basis keeping the current ball
	It          support_end;			// past-the-end iterator of support set

	// private methods
	void		mtf_mb(It k);
	void		pivot_mb(It k);
	void		move_to_front(It j);
	float		max_excess(It t, It i, It& pivot) const;
	float		abs(float r) const { return (r > 0) ? r : (-r); }
	float		sqr(float r) const { return r * r; }
public:
	// construction
	Miniball() {}
	void        check_in(const Fvector& p);
	void        build();

	// access
	Fvector     center() const;
	float		squared_radius() const;
	int         num_points() const;
	Cit         points_begin() const;
	Cit         points_end() const;
	int         nr_support_gVectors() const;
	Cit         support_points_begin() const;
	Cit         support_points_end() const;
};

// Miniball
// --------

void Miniball::check_in(const Fvector& p)
{
	L.push_back(p);
}

void Miniball::build()
{
	B.reset();
	support_end = L.begin();

	// @@ pivotting or not ?
	if (1)
		pivot_mb(L.end());
	else
		mtf_mb(L.end());
}

void Miniball::mtf_mb(It i)
{
	support_end = L.begin();

	if ((B.size()) == 4)
		return;

	for (It k = L.begin(); k != i;)
	{
		It j = k++;
		if (B.excess(*j) > 0)
		{
			if (B.push(*j))
			{
				mtf_mb(j);
				B.pop();
				move_to_front(j);
			}
		}
	}
}

void Miniball::move_to_front(It j)
{
	if (support_end == j)
		support_end++;
	L.splice(L.begin(), L, j);
}


void Miniball::pivot_mb(It i)
{
	It t = ++L.begin();
	mtf_mb(t);
	float max_e, old_sqr_r = 0;
	do
	{
		It pivot = L.begin();
		max_e = max_excess(t, i, pivot);
		if (max_e > 0)
		{
			t = support_end;
			if (t == pivot) ++t;
			old_sqr_r = B.squared_radius();
			B.push(*pivot);
			mtf_mb(support_end);
			B.pop();
			move_to_front(pivot);
		}
	} while ((max_e > 0) && (B.squared_radius() > old_sqr_r));
}

float Miniball::max_excess(It t, It i, It& pivot) const
{
	const	Fvector* pCenter = B.center();
	float				sqr_r = B.squared_radius();

	float e, max_e = 0;

	for (It k = t; k != i; ++k)
	{
		const Fvector& point = (*k);
		e = -sqr_r;

		e += point.distance_to_sqr(*pCenter);

		if (e > max_e)
		{
			max_e = e;
			pivot = k;
		}
	}

	return max_e;
}

Fvector Miniball::center() const
{
	return *((Fvector*)B.center());
}

float Miniball::squared_radius() const
{
	return B.squared_radius();
}

int Miniball::num_points() const
{
	return L.size();
}

Miniball::Cit Miniball::points_begin() const
{
	return L.begin();
}

Miniball::Cit Miniball::points_end() const
{
	return L.end();
}

int Miniball::nr_support_gVectors() const
{
	return B.support_size();
}

Miniball::Cit Miniball::support_points_begin() const
{
	return L.begin();
}

Miniball::Cit Miniball::support_points_end() const
{
	return support_end;
}


template <class T>
void _sphere<T>::compute(const _vector<T> *verts, int count)
{
		Miniball<T> mb;
		
		for(int i=0;i<count;i++)
			mb.check_in(verts[i]);
		
		mb.build	();
		
		P.set		(mb.center());
		R =			( sqrtf( mb.squared_radius() ));
}

typedef _sphere<float>	Fsphere;
typedef _sphere<double> Dsphere;


#endif