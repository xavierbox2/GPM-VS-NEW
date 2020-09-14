
#ifndef DVECTOR2_H_   
#define DVECTOR2_H_ 1

#include <iostream>
 

#ifdef GPACOMMON_EXPORTS
#define GPACOMMON_API __declspec(dllexport)
#else
#define GPACOMMON_API __declspec(dllimport)
#endif

template<typename T>
class 
#ifdef ISDLL
    GPACOMMON_API
#endif
Vector2
{
	friend std::ostream& operator<<(std::ostream &os, const Vector2<T> &v2)
	{
		os << v2.x << " " << v2.y; return os;
	}

	friend std::istream& operator >> (std::istream& is, Vector2<T>& vec)
	{
		is >> vec.x; is >> vec.y; return is;
	}

public:

	T x, y;
	inline Vector2(T a = (T)(0.0)) : x(a), y(a) { ; }
	inline Vector2(T a, T b) : x(a), y(b) { ; }
	inline Vector2(T *a) : x(a[0]), y(a[1]) { ; }
	inline Vector2(const Vector2 &a) : x(a.x), y(a.y) { ; }
	inline T operator*(const Vector2 &v2) const { return x * v2.x + y * v2.y; }
	inline T dot(const Vector2 &v2) const { return x * v2.x + y * v2.y; }
	
    inline T selfDot() const { return x * x + y * y; }

    T length( ) const { return sqrt( selfDot()); }

    Vector2 normalize( )
    {
        float inv_lenght = (float)(1.0 / length( ));
        return Vector2( x * inv_lenght, y * inv_lenght );
    }


	inline Vector2  operator*(const T a) const { return Vector2(x*a, y*a); }
	inline Vector2  operator*=(const T a) { x *= a; y *= a; return *this; }//Vector2d(x*a,y*a);}
	inline Vector2  operator=(const Vector2 &a) { if (&a != this) { x = a.x; y = a.y; }return *this; }
	inline Vector2 operator+(const Vector2 &a) const { return Vector2(a.x + x, a.y + y); };
	inline Vector2 operator-(const Vector2 &a) const { return Vector2(x - a.x, y - a.y); };
	inline Vector2 operator+=(const Vector2 &a) { x += a.x; y += a.y; return *this; }//Vector2d(a.x+x,a.y+y);};
	inline Vector2 operator-=(const Vector2 &a) { x -= a.x; y -= a.y; return *this; }//Vector2d(x-a.x,y-a.y);};
	inline void setZero() { x = (T)(0.0); y = (T)(0.0); }



	static inline Vector2 diff(T *a, T *b)
	{
		return Vector2(a[0] - b[0], a[1] - b[1]);
	}
	static inline Vector2 diff(T *a, Vector2  &b)
	{
		return Vector2(a[0] - b.x, a[1] - b.y);
	}
	static inline Vector2 diff(Vector2 &a, Vector2 &b)
	{
		return Vector2(a.x - b.x, a.y - b.y);
	}
	static inline Vector2 sum(Vector2 &a, Vector2 &b)
	{
		return Vector2(a.x + b.x, a.y + b.y);
	}
	inline  T operator[](int k) const { return (k == 0 ? x : y); }
	inline T& operator[](int k) { return (k == 0 ? x : y); }
	static inline Vector2 diff(Vector2  &b, T *a)
	{
		return Vector2(b.x - a[0], b.y - a[1]);
	}

	T trace() const { return x+y; }

	int size() const { return 2; }

};

template<typename T>
class 
#ifdef ISDLL
	MATHLIB_API
#endif
Vector3
{
	friend std::ostream& operator<<(std::ostream &os, const Vector3<T> &v2)
	{
		os << v2[0]<< " " << v2[1] << " " << v2[2]; return os;
	}

	friend std::istream& operator >> (std::istream& is, Vector3<T>& v2)
	{
		is >> v2[0] >> " " >> v2[1] << " " >> v2[2];  return is;
	}

public:

	T r[3];
	inline Vector3(T a = (T)(0.0))  { r[0] = a; r[1] = a; r[2] = a; }
	inline Vector3(T a, T b, T c)  {  r[0] = a; r[1] = b; r[2] = c;  }
	inline Vector3(T *a) { r[0] = a[0]; r[1] = a[1]; r[2] = a[2]; }
	inline Vector3(const Vector3 &a) { r[0] = a[0]; r[1] = a[1]; r[2] = a[2]; }
	inline Vector3(const Vector2<T> &a) { r[0] = a[0]; r[1] = a[1]; r[2] = T(0.0); }

	inline Vector3 operator=(T* a) { r[0] = a[0]; r[1] = a[1]; r[2] = a[2]; return *this;}

	inline T operator*(const Vector3 &v) const { return r[0] * v[0] + r[1] * v[1] + r[2] * v[2]; }
	inline T dot(const Vector3 &v) const { return r[0] * v[0] + r[1] * v[1] + r[2] * v[2]; }

	inline T selfDot() const { return r[0] * r[0] + r[1] * r[1] + r[2] * r[2]; }
    inline T length( ) const { return sqrt( selfDot( ) ); }

	inline Vector3  operator*(const T a) const { return Vector3(r[0]*a, r[1]*a, r[2]*a); }
	inline Vector3  operator*=(const T a) { r[0] *= a; r[1] *= a; r[2] *= a; return *this; }//Vector3d(x*a,y*a);}
	inline Vector3  operator=(const Vector3 &a) { if (&a != this) { r[0] = a.r[0]; r[1] = a.r[1]; r[2] = a.r[2]; }return *this; }
	inline Vector3 operator+(const Vector3 &a) const { return Vector3( r[0]+a[0],r[1]+a[1],r[2]+a[2]); };
	inline Vector3 operator-(const Vector3 &a) const { return Vector3(r[0] - a[0], r[1] - a[1], r[2] - a[2]); };
	inline Vector3 operator+=(const Vector3 &a) { r[0] += a.r[0]; r[1] += a.r[1]; r[2] += a.r[2]; return *this; }//Vector3d(a.x+x,a.y+y);};
	inline Vector3 operator-=(const Vector3 &a) { r[0] -= a.r[0]; r[1] -= a.r[1]; r[2] -= a.r[2]; return *this; }
	inline void setZero() { r[0] = (T)(0.0); r[1] = (T)(0.0); r[2] = (T)(0.0); }

    Vector3 cross( const Vector3 &v2)
    {
    return Vector3( r[1]*v2.r[2]  - r[2]*v2.r[1], -(r[0] * v2.r[2] - r[2] * v2.r[0]), r[0] * v2.r[1] - r[1] * v2.r[0] );
    }


	Vector3 normalize() 
	{
		float inv_lenght = (float)(  1.0/ length() );
		return Vector3( r[0]*inv_lenght, r[1]*inv_lenght, r[2]*inv_lenght );
	}

	//should be avoided, slow but using as data an [3] would kill the inlining and cache
	inline  T operator[](int k) const { return r[k]; }
	inline T& operator[](int k) { return r[k]; }

	T trace() const { return r[0] +  r[1]  +  r[2];  }

    T trace_mult() const { return r[0] * r[1] * r[2]; }

	int size() const { return 3; }
};

typedef Vector3<int>     iVector3;
typedef Vector3<float>   fVector3;
typedef Vector3<float>   fvector3;
typedef Vector3<double>  dVector3;

typedef Vector2<int>     iVector2;
typedef Vector2<float>   fVector2;
typedef Vector2<double>  dVector2;


#ifdef D2 
typedef Vector2<REAL> dvector;
typedef Vector2<int> ivector;
typedef Vector2<float> fvector;
#endif 

#ifdef D3 
typedef Vector3<REAL> dvector;
typedef Vector3<int>  ivector;
typedef Vector3<float> fvector;
#endif 





#endif 

