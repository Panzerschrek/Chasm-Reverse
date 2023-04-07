#pragma once
#include <cmath>
#include <cstddef>

class m_Vec2
{
public:
	static constexpr size_t size= 2;

	float x, y;

	m_Vec2(){}
	m_Vec2( const m_Vec2& v );
	m_Vec2( float a, float b );
	explicit m_Vec2( const float* f ); // construct from array
	~m_Vec2(){}

	float Length() const;
	float SquareLength() const;
	float InvLength() const;
	float InvSquareLength() const;
	float MaxComponent() const;
	m_Vec2& Normalize();
	float* ToArr();
	const float* ToArr() const;

	m_Vec2 operator+() const;
	m_Vec2 operator-() const;

	m_Vec2  operator+ ( const m_Vec2& v ) const;
	m_Vec2& operator+=( const m_Vec2& v );
	m_Vec2  operator- ( const m_Vec2& v ) const;
	m_Vec2& operator-=( const m_Vec2& v );

	m_Vec2  operator* ( float a ) const;
	m_Vec2& operator*=( float a );
	m_Vec2  operator/ ( float a ) const;
	m_Vec2& operator/=( float a );

	float operator*( const m_Vec2& v ) const;
	m_Vec2& operator*=( const m_Vec2& v );
};

bool operator==( const m_Vec2& v1, const m_Vec2& v2 );
bool operator!=( const m_Vec2& v1, const m_Vec2& v2 );

m_Vec2 operator*( float a, const m_Vec2& v );
float mVec2Cross( const m_Vec2& v1, const m_Vec2 v2 );

class m_Vec3
{
public:
	static constexpr size_t size= 3;

	float x, y, z;

	m_Vec3(){}
	m_Vec3( const m_Vec3& v );
	m_Vec3( float a, float b, float c );
	explicit m_Vec3( const float* f ); // construct from array
	m_Vec3( const m_Vec2& vec2, float in_z );
	~m_Vec3(){}

	float Length() const;
	float SquareLength() const;
	float InvLength() const;
	float InvSquareLength() const;
	float MaxComponent() const;
	m_Vec3& Normalize();

	float* ToArr();
	const float* ToArr() const;

	m_Vec3 operator+() const;
	m_Vec3 operator-() const;

	m_Vec3  operator+ ( const m_Vec3& v ) const;
	m_Vec3& operator+=( const m_Vec3& v );
	m_Vec3  operator- ( const m_Vec3& v ) const;
	m_Vec3& operator-=( const m_Vec3& v );

	m_Vec3  operator* ( float a ) const;
	m_Vec3& operator*=( float a );
	m_Vec3  operator/ ( float a ) const;
	m_Vec3& operator/=( float a );

	m_Vec2 xy() const;
	m_Vec2 xz() const;
	m_Vec2 yz() const;

	float operator* ( const m_Vec3& v ) const;
	m_Vec3& operator*=( const m_Vec3& v );
};

bool operator==( const m_Vec3& v1, const m_Vec3& v2 );
bool operator!=( const m_Vec3& v1, const m_Vec3& v2 );

m_Vec3 operator*( float a, const m_Vec3& v );
m_Vec3 mVec3Cross( const m_Vec3& v1, const m_Vec3& v2 );

/*
VEC2
*/

inline m_Vec2::m_Vec2( const m_Vec2& v ) : x(v.x), y(v.y) {}

inline m_Vec2::m_Vec2( float a, float b ) : x(a), y(b) {}

inline m_Vec2::m_Vec2( const float* f ) : x(f[0]), y(f[1]) {}

inline float m_Vec2::Length() const
{
	return std::sqrt( x * x + y * y );

}
inline float m_Vec2::SquareLength() const
{
	return  x * x + y * y;
}

inline float m_Vec2::InvLength() const
{
	return 1.0f / std::sqrt( x * x + y * y );
}

inline float m_Vec2::InvSquareLength() const
{
	return 1.0f / ( x * x + y * y );
}

inline float m_Vec2::MaxComponent() const
{
	return x > y ? x : y;
}

inline m_Vec2& m_Vec2::Normalize()
{
	float r= std::sqrt( x * x + y * y );
	if( r != 0.0f )
		r= 1.0f / r;
	x*= r;
	y*= r;
	return *this;
}

inline float* m_Vec2::ToArr()
{
	return &x;
}

inline const float* m_Vec2::ToArr() const
{
	return &x;
}

inline m_Vec2 m_Vec2::operator+() const
{
	return *this;
}

inline m_Vec2 m_Vec2::operator-() const
{
	return m_Vec2( -x, -y );
}

inline m_Vec2 m_Vec2::operator+( const m_Vec2& v ) const
{
	return m_Vec2( x + v.x, y + v.y );
}

inline m_Vec2& m_Vec2::operator+=( const m_Vec2& v )
{
	x+= v.x;
	y+= v.y;
	return *this;
}

inline m_Vec2 m_Vec2::operator-( const m_Vec2& v ) const
{
	return m_Vec2( x - v.x, y - v.y );
}

inline m_Vec2& m_Vec2::operator-=( const m_Vec2& v )
{
	x-= v.x;
	y-= v.y;
	return *this;
}

inline m_Vec2  m_Vec2::operator*( float a ) const
{
	return m_Vec2( x * a, y * a );
}

inline m_Vec2& m_Vec2::operator*=( float a )
{
	x*= a;
	y*= a;
	return *this;
}

inline m_Vec2 m_Vec2::operator/( float a ) const
{
	float r= 1.0f / a;
	return m_Vec2( x * r, y * r );
}

inline m_Vec2& m_Vec2::operator/=( float a )
{
	float r= 1.0f / a;
	x*= r;
	y*= r;
	return *this;
}

inline float m_Vec2::operator*( const m_Vec2& v ) const
{
	return x * v.x + y * v.y;
}

inline m_Vec2& m_Vec2::operator*=( const m_Vec2& v )
{
	x*= v.x;
	y*= v.y;
	return *this;
}

inline bool operator==( const m_Vec2& v1, const m_Vec2& v2 )
{
	return v1.x == v2.x && v1.y == v2.y;
}

inline bool operator!=( const m_Vec2& v1, const m_Vec2& v2 )
{
	return v1.x != v2.x || v1.y != v2.y;
}

inline m_Vec2 operator*( float a, const m_Vec2& v )
{
	return m_Vec2( a * v.x, a * v.y );
}

inline float mVec2Cross( const m_Vec2& v1, const m_Vec2 v2 )
{
	return v1.x * v2.y - v1.y * v2.x;
}

/*
VEC3
*/

inline m_Vec3::m_Vec3( const m_Vec3& v ) : x(v.x), y(v.y), z(v.z) {}

inline m_Vec3::m_Vec3( float a, float b, float c) : x(a), y(b), z(c) {}

inline m_Vec3::m_Vec3( const float* f ) : x(f[0]), y(f[1]), z(f[2]) {}
inline m_Vec3::m_Vec3( const m_Vec2& vec2, float in_z ) : x(vec2.x), y(vec2.y), z(in_z) {}

inline float m_Vec3::Length() const
{
	return std::sqrt( x * x + y * y + z * z );
}

inline float m_Vec3::SquareLength() const
{
	return x * x + y * y + z * z;
}

inline float m_Vec3::InvLength() const
{
	return 1.0f / std::sqrt( x * x + y * y + z * z );
}

inline float m_Vec3::InvSquareLength() const
{
	return 1.0f / ( x * x + y * y + z * z );
}

inline m_Vec3& m_Vec3::Normalize()
{
	float r= std::sqrt( x * x + y * y + z * z );
	r= 1.0f / r; // if r is zero, r= inf, 0*inf= NaN
	x*= r;
	y*= r;
	z*= r;
	return *this;
}

inline float m_Vec3::MaxComponent() const
{
	float m= x > y ? x : y;
	return m > z ? m : z;
}

inline float* m_Vec3::ToArr()
{
	return &x;
}

inline const float* m_Vec3::ToArr() const
{
	return &x;
}

inline m_Vec3 m_Vec3::operator+() const
{
	return *this;
}

inline m_Vec3 m_Vec3::operator-() const
{
	return m_Vec3( -x, -y, -z );
}

inline m_Vec3 m_Vec3::operator+( const m_Vec3& v ) const
{
	return m_Vec3( x + v.x, y + v.y, z + v.z );
}

inline m_Vec3& m_Vec3::operator+=( const m_Vec3& v )
{
	x+= v.x;
	y+= v.y;
	z+= v.z;
	return *this;
}

inline m_Vec3 m_Vec3::operator-(const m_Vec3& v ) const
{
	return m_Vec3( x - v.x, y - v.y, z - v.z );
}

inline m_Vec3& m_Vec3::operator-=( const m_Vec3& v )
{
	x-= v.x;
	y-= v.y;
	z-= v.z;
	return *this;
}

inline m_Vec3  m_Vec3::operator*( float a ) const
{
	return m_Vec3( x * a, y * a, z * a );
}

inline m_Vec3& m_Vec3::operator*=( float a )
{
	x*= a;
	y*= a;
	z*= a;
	return *this;
}

inline m_Vec3 m_Vec3::operator/( float a ) const
{
	float inv_a= 1.0f / a;
	return m_Vec3( x * inv_a, y * inv_a, z * inv_a );
}

inline m_Vec3& m_Vec3::operator/=( float a )
{
	float r= 1.0f / a;
	x*= r;
	y*= r;
	z*= r;
	return *this;
}

inline float m_Vec3::operator*( const m_Vec3& v ) const
{
 return x * v.x + y * v.y + z * v.z;
}

inline m_Vec3& m_Vec3::operator*=( const m_Vec3& v )
{
	x*= v.x;
	y*= v.y;
	z*= v.z;
	return *this;
}

inline m_Vec2 m_Vec3::xy() const
{
	return m_Vec2( x, y );
}

inline m_Vec2 m_Vec3::xz() const
{
	return m_Vec2( x, z );
}

inline m_Vec2 m_Vec3::yz() const
{
	return m_Vec2( y, z );
}

inline bool operator==( const m_Vec3& v1, const m_Vec3& v2 )
{
	return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}

inline bool operator!=( const m_Vec3& v1, const m_Vec3& v2 )
{
	return v1.x != v2.x || v1.y != v2.y || v1.z != v2.z;
}

inline m_Vec3 operator*( float a, const m_Vec3& v )
{
	return m_Vec3( a * v.x, a * v.y, a * v.z );
}

inline m_Vec3 mVec3Cross( const m_Vec3& v1, const m_Vec3& v2 )
{
	return m_Vec3(
		v1.y * v2.z - v1.z * v2.y,
		v1.z * v2.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x );
}
