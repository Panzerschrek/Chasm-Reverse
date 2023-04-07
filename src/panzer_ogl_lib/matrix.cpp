#include "matrix.hpp"

/*
MAT3
*/

m_Mat3::m_Mat3( const m_Mat4& m )
{
	value[0]= m.value[0];
	value[1]= m.value[1];
	value[2]= m.value[2];

	value[3]= m.value[4];
	value[4]= m.value[5];
	value[5]= m.value[6];

	value[6]= m.value[8];
	value[7]= m.value[9];
	value[8]= m.value[10];
}

m_Mat3 m_Mat3::operator*( const m_Mat3& m ) const
{
	m_Mat3 r;

	r.value[0]= value[0] * m.value[0] + value[1] * m.value[3] + value[2] * m.value[6];
	r.value[1]= value[0] * m.value[1] + value[1] * m.value[4] + value[2] * m.value[7];
	r.value[2]= value[0] * m.value[2] + value[1] * m.value[5] + value[2] * m.value[8];

	r.value[3]= value[3] * m.value[0] + value[4] * m.value[3] + value[5] * m.value[6];
	r.value[4]= value[3] * m.value[1] + value[4] * m.value[4] + value[5] * m.value[7];
	r.value[5]= value[3] * m.value[2] + value[4] * m.value[5] + value[5] * m.value[8];

	r.value[6]= value[6] * m.value[0] + value[7] * m.value[3] + value[8] * m.value[6];
	r.value[7]= value[6] * m.value[1] + value[7] * m.value[4] + value[8] * m.value[7];
	r.value[8]= value[6] * m.value[2] + value[7] * m.value[5] + value[8] * m.value[8];

	return r;
}

m_Mat3& m_Mat3::operator*=( const m_Mat3& m )
{
	m_Mat3 r;

	r.value[0]= value[0] * m.value[0] + value[1] * m.value[3] + value[2] * m.value[6];
	r.value[1]= value[0] * m.value[1] + value[1] * m.value[4] + value[2] * m.value[7];
	r.value[2]= value[0] * m.value[2] + value[1] * m.value[5] + value[2] * m.value[8];

	r.value[3]= value[3] * m.value[0] + value[4] * m.value[3] + value[5] * m.value[6];
	r.value[4]= value[3] * m.value[1] + value[4] * m.value[4] + value[5] * m.value[7];
	r.value[5]= value[3] * m.value[2] + value[4] * m.value[5] + value[5] * m.value[8];

	r.value[6]= value[6] * m.value[0] + value[7] * m.value[3] + value[8] * m.value[6];
	r.value[7]= value[6] * m.value[1] + value[7] * m.value[4] + value[8] * m.value[7];
	r.value[8]= value[6] * m.value[2] + value[7] * m.value[5] + value[8] * m.value[8];

	*this= r;
	return *this;
}

m_Vec3 m_Mat3::operator*( const m_Vec3& v ) const
{
	m_Vec3 r;
	r.x= value[0] * v.x + value[1] * v.y + value[2] * v.z;
	r.y= value[3] * v.x + value[4] * v.y + value[5] * v.z;
	r.z= value[6] * v.x + value[7] * v.y + value[8] * v.z;
	return r;
}

void m_Mat3::Identity()
{
	value[0]= value[4]= value[8]= 1.0f;

	value[1] = value[2] = value[3] = value[5] = value[6] = value[7] = 0.0f;
}

void m_Mat3::Transpose()
{
	float tmp;

	tmp= value[3];
	value[3] = value[1];
	value[1]= tmp;

	tmp= value[6];
	value[6] = value[2];
	value[2]= tmp;

	tmp= value[7];
	value[7] = value[5];
	value[5]= tmp;
}

void m_Mat3::Inverse()
{
	m_Mat3 r;

	float det= Determinant();
	if( det == 0.0f )
		return;

	det= 1.0f / det;

	r.value[0]= ( value[4] * value[8] - value[7] * value[5] ) * det;
	r.value[1]= ( value[2] * value[7] - value[1] * value[8] ) * det;
	r.value[2]= ( value[1] * value[5] - value[2] * value[4] ) * det;

	r.value[3]= ( value[5] * value[6] - value[3] * value[8] ) * det;
	r.value[4]= ( value[0] * value[8] - value[2] * value[6] ) * det;
	r.value[5]= ( value[2] * value[3] - value[0] * value[5] ) * det;

	r.value[6]= ( value[3] * value[7] - value[4] * value[6] ) * det;
	r.value[7]= ( value[1] * value[6] - value[0] * value[7] ) * det;
	r.value[8]= ( value[0] * value[4] - value[1] * value[3] ) * det;

	*this= r;
}

float m_Mat3::Determinant() const
{
	return
	value[0] * ( value[4] * value[8] - value[7] * value[5] ) -
	value[1] * ( value[3] * value[8] - value[6] * value[5] ) +
	value[2] * ( value[3] * value[7] - value[6] * value[4] );
}

float& m_Mat3::operator[]( int i )
{
	return value[i];
}

float m_Mat3::operator[]( int i )const
{
	return value[i];
}

void m_Mat3::Translate( const m_Vec2& v )
{
	Identity();
	value[6]= v.x;
	value[7]= v.y;
}

void m_Mat3::Scale( float s )
{
	Identity();
	value[0]= s;
	value[4]= s;
	value[8]= 1.0f;
}

void m_Mat3::Scale( const m_Vec3& v )
{
	Identity();
	value[0]= v.x;
	value[4]= v.y;
	value[8]= v.z;
}

void m_Mat3::RotateX( float a )
{
	float s= std::sin(a), c= std::cos(a);

	value[4]= c;
	value[7]= -s;
	value[5]= s;
	value[8]= c;

	value[0]= 1.0f;
	value[1]= value[2]= value[3]= value[6]= 0.0f;
}

void m_Mat3::RotateY( float a )
{
	float s= std::sin(a), c= std::cos(a);

	value[0]= c;
	value[6]= s;
	value[2]= -s;
	value[8]= c;

	value[4]= 1.0f;
	value[1]= value[3]= value[5]= value[7]= 0.0f;
}

void m_Mat3::RotateZ( float a )
{
	float s= std::sin(a), c= std::cos(a);
	value[0]= c;
	value[3]= -s;
	value[1]= s;
	value[4]= c;

	value[8]= 1.0f;
	value[2]= value[5]= value[6]= value[7]= 0.0f;
}

m_Vec3 operator*( const m_Vec3& v, const m_Mat3& m )
{
	m_Vec3 r;
	r.x= v.x * m.value[0] + v.y * m.value[3] + v.z * m.value[6];
	r.y= v.x * m.value[1] + v.y * m.value[4] + v.z * m.value[7];
	r.z= v.x * m.value[2] + v.y * m.value[5] + v.z * m.value[8];
	return r;
}

m_Vec2 operator*( const m_Vec2& v, const m_Mat3& m )
{
	m_Vec2 r;
	r.x= v.x * m.value[0] + v.y * m.value[3] + m.value[6];
	r.y= v.x * m.value[1] + v.y * m.value[4] + m.value[7];
	return r;
}

/*
MAT4
*/

m_Mat4 m_Mat4::operator*( const m_Mat4& m ) const
{
	m_Mat4 r;

	r.value[0 ]= value[0]*m.value[0] + value[1]*m.value[4] + value[2]*m.value[8]  + value[3]*m.value[12];
	r.value[1 ]= value[0]*m.value[1] + value[1]*m.value[5] + value[2]*m.value[9]  + value[3]*m.value[13];
	r.value[2 ]= value[0]*m.value[2] + value[1]*m.value[6] + value[2]*m.value[10] + value[3]*m.value[14];
	r.value[3 ]= value[0]*m.value[3] + value[1]*m.value[7] + value[2]*m.value[11] + value[3]*m.value[15];

	r.value[4 ]= value[4]*m.value[0] + value[5]*m.value[4] + value[6]*m.value[8]  + value[7]*m.value[12];
	r.value[5 ]= value[4]*m.value[1] + value[5]*m.value[5] + value[6]*m.value[9]  + value[7]*m.value[13];
	r.value[6 ]= value[4]*m.value[2] + value[5]*m.value[6] + value[6]*m.value[10] + value[7]*m.value[14];
	r.value[7 ]= value[4]*m.value[3] + value[5]*m.value[7] + value[6]*m.value[11] + value[7]*m.value[15];

	r.value[8 ]= value[8]*m.value[0] + value[9]*m.value[4] + value[10]*m.value[8]  + value[11]*m.value[12];
	r.value[9 ]= value[8]*m.value[1] + value[9]*m.value[5] + value[10]*m.value[9]  + value[11]*m.value[13];
	r.value[10]= value[8]*m.value[2] + value[9]*m.value[6] + value[10]*m.value[10] + value[11]*m.value[14];
	r.value[11]= value[8]*m.value[3] + value[9]*m.value[7] + value[10]*m.value[11] + value[11]*m.value[15];

	r.value[12]= value[12]*m.value[0] + value[13]*m.value[4] + value[14]*m.value[8]  + value[15]*m.value[12];
	r.value[13]= value[12]*m.value[1] + value[13]*m.value[5] + value[14]*m.value[9]  + value[15]*m.value[13];
	r.value[14]= value[12]*m.value[2] + value[13]*m.value[6] + value[14]*m.value[10] + value[15]*m.value[14];
	r.value[15]= value[12]*m.value[3] + value[13]*m.value[7] + value[14]*m.value[11] + value[15]*m.value[15];
	return r;
}

m_Mat4& m_Mat4::operator*=( const m_Mat4& m )
{
	m_Mat4 r;

	r.value[0 ]= value[0]*m.value[0] + value[1]*m.value[4] + value[2]*m.value[8]  + value[3]*m.value[12];
	r.value[1 ]= value[0]*m.value[1] + value[1]*m.value[5] + value[2]*m.value[9]  + value[3]*m.value[13];
	r.value[2 ]= value[0]*m.value[2] + value[1]*m.value[6] + value[2]*m.value[10] + value[3]*m.value[14];
	r.value[3 ]= value[0]*m.value[3] + value[1]*m.value[7] + value[2]*m.value[11] + value[3]*m.value[15];

	r.value[4 ]= value[4]*m.value[0] + value[5]*m.value[4] + value[6]*m.value[8]  + value[7]*m.value[12];
	r.value[5 ]= value[4]*m.value[1] + value[5]*m.value[5] + value[6]*m.value[9]  + value[7]*m.value[13];
	r.value[6 ]= value[4]*m.value[2] + value[5]*m.value[6] + value[6]*m.value[10] + value[7]*m.value[14];
	r.value[7 ]= value[4]*m.value[3] + value[5]*m.value[7] + value[6]*m.value[11] + value[7]*m.value[15];

	r.value[8 ]= value[8]*m.value[0] + value[9]*m.value[4] + value[10]*m.value[8]  + value[11]*m.value[12];
	r.value[9 ]= value[8]*m.value[1] + value[9]*m.value[5] + value[10]*m.value[9]  + value[11]*m.value[13];
	r.value[10]= value[8]*m.value[2] + value[9]*m.value[6] + value[10]*m.value[10] + value[11]*m.value[14];
	r.value[11]= value[8]*m.value[3] + value[9]*m.value[7] + value[10]*m.value[11] + value[11]*m.value[15];

	r.value[12]= value[12]*m.value[0] + value[13]*m.value[4] + value[14]*m.value[8]  + value[15]*m.value[12];
	r.value[13]= value[12]*m.value[1] + value[13]*m.value[5] + value[14]*m.value[9]  + value[15]*m.value[13];
	r.value[14]= value[12]*m.value[2] + value[13]*m.value[6] + value[14]*m.value[10] + value[15]*m.value[14];
	r.value[15]= value[12]*m.value[3] + value[13]*m.value[7] + value[14]*m.value[11] + value[15]*m.value[15];

	*this= r;
	return *this;
}

m_Vec3 m_Mat4::operator*( const m_Vec3& v ) const
{
	return m_Vec3(
		value[0] * v.x + value[1] * v.y + value[2] * v.z + value[3],
		value[4] * v.x + value[5] * v.y + value[6] * v.z + value[7],
		value[8] * v.x + value[9] * v.y + value[10]* v.z + value[11] );
}

void m_Mat4::Identity()
{
	value[0]= value[5]= value[10]= value[15]= 1.0f;

	value[1] = value[2] = value[3] = 0.0f;
	value[4] = value[6] = value[7] = 0.0f;
	value[8] = value[9] = value[11]= 0.0f;
	value[12]= value[13]= value[14]= 0.0f;
}

void m_Mat4::Transpose()
{
	float tmp;

	tmp= value[1];
	value[1]= value[4];
	value[4]= tmp;

	tmp= value[2];
	value[2]= value[8];
	value[8]= tmp;

	tmp= value[3];
	value[3]= value[12];
	value[12]= tmp;

	tmp= value[6];
	value[6]= value[9];
	value[9]= tmp;

	tmp= value[7];
	value[7]= value[13];
	value[13]= tmp;

	tmp= value[11];
	value[11]= value[14];
	value[14]= tmp;
}

static float inline Mat3Det(
	float& m00, float& m10, float& m20,
	float& m01, float& m11, float& m21,
	float& m02, float& m12, float& m22 )
{
	return
		m00 * ( m11 * m22 - m21 * m12 ) -
		m10 * ( m01 * m22 - m21 * m02 ) +
		m20 * ( m01 * m12 - m11 * m02 );
}

void m_Mat4::Inverse()
{
	m_Mat4 m;

	m.value[0 ]= +Mat3Det(
	value[5 ], value[6 ], value[7 ],
	value[9 ], value[10], value[11],
	value[13], value[14], value[15] );//
	m.value[1 ]= -Mat3Det(
	value[4 ], value[6 ], value[7 ],
	value[8 ], value[10], value[11],
	value[12], value[14], value[15] );//
	m.value[2 ]= +Mat3Det(
	value[4 ], value[5 ], value[7 ],
	value[8 ], value[9 ], value[11],
	value[12], value[13], value[15] );//
	m.value[3 ]= -Mat3Det(
	value[4 ], value[5 ], value[6 ],
	value[8 ], value[9 ], value[10],
	value[12], value[13], value[14] );//

	m.value[4 ]= -Mat3Det(
	value[1 ], value[2 ], value[3 ],
	value[9 ], value[10], value[11],
	value[13], value[14], value[15] );//
	m.value[5 ]= +Mat3Det(
	value[0 ], value[2 ], value[3 ],
	value[8 ], value[10], value[11],
	value[12], value[14], value[15] );//
	m.value[6 ]= -Mat3Det(
	value[0 ], value[1 ], value[3 ],
	value[8 ], value[9 ], value[11],
	value[12], value[13], value[15] );//
	m.value[7 ]= +Mat3Det(
	value[0 ], value[1 ], value[2 ],
	value[8 ], value[9 ], value[10],
	value[12], value[13], value[14] );//

	m.value[8 ]= +Mat3Det(
	value[1 ], value[2 ], value[3 ],
	value[5 ], value[6 ], value[7 ],
	value[13], value[14], value[15] );//
	m.value[9 ]= -Mat3Det(
	value[0 ], value[2 ], value[3 ],
	value[4 ], value[6 ], value[7 ],
	value[12], value[14], value[15] );//
	m.value[10]= +Mat3Det(
	value[0 ], value[1 ], value[3 ],
	value[4 ], value[5 ], value[7 ],
	value[12], value[13], value[15] );//
	m.value[11]= -Mat3Det(
	value[0 ], value[1 ], value[2 ],
	value[4 ], value[5 ], value[6 ],
	value[12], value[13], value[14] );//

	m.value[12]= -Mat3Det(
	value[1 ], value[2 ], value[3 ],
	value[5 ], value[6 ], value[7 ],
	value[9 ], value[10], value[11] );//
	m.value[13]= +Mat3Det(
	value[0 ], value[2 ], value[3 ],
	value[4 ], value[6 ], value[7 ],
	value[8 ], value[10], value[11] );//
	m.value[14]= -Mat3Det(
	value[0 ], value[1 ], value[3 ],
	value[4 ], value[5 ], value[7 ],
	value[8 ], value[9 ], value[11] );//
	m.value[15]= +Mat3Det(
	value[0 ], value[1 ], value[2 ],
	value[4 ], value[5 ], value[6 ],
	value[8 ], value[9 ], value[10] );//

	float det= m.value[0] * value[0] + m.value[1] * value[1] + m.value[2] * value[2] + m.value[3] * value[3];
	float inv_det= 1.0f / det;

	value[0 ]= m.value[0 ] * inv_det;
	value[5 ]= m.value[5 ] * inv_det;
	value[10]= m.value[10] * inv_det;
	value[15]= m.value[15] * inv_det;

	value[1 ]= m.value[4 ] * inv_det;
	value[2 ]= m.value[8 ] * inv_det;
	value[3 ]= m.value[12] * inv_det;
	value[4 ]= m.value[1 ] * inv_det;
	value[6 ]= m.value[9 ] * inv_det;
	value[7 ]= m.value[13] * inv_det;
	value[8 ]= m.value[2 ] * inv_det;
	value[9 ]= m.value[6 ] * inv_det;
	value[11]= m.value[14] * inv_det;
	value[12]= m.value[3 ] * inv_det;
	value[13]= m.value[7 ] * inv_det;
	value[14]= m.value[11] * inv_det;
}

float& m_Mat4::operator[]( int i )
{
	return value[i];
}
float m_Mat4::operator[]( int i )const
{
	return value[i];
}

void m_Mat4::Translate( const m_Vec3& v )
{
	Identity();
	value[12]= v.x;
	value[13]= v.y;
	value[14]= v.z;
}

void m_Mat4::Scale( float s )
{
	Identity();
	value[0 ]= s;
	value[5 ]= s;
	value[10]= s;
}

void m_Mat4::Scale( const m_Vec3& v )
{
	Identity();
	value[0 ]= v.x;
	value[5 ]= v.y;
	value[10]= v.z;
}

void m_Mat4::RotateX( float a )
{
	float s= std::sin(a), c= std::cos(a);

	value[5 ]= c;
	value[9 ]= -s;
	value[6 ]= s;
	value[10]= c;

	value[0]= value[15]= 1.0f;
	value[1]= value[2]= value[3]= 0.0f;
	value[4]= value[7]= 0.0f;
	value[8]= value[11]= 0.0f;
	value[12]= value[13]= value[14]= 0.0f;
}

void m_Mat4::RotateY( float a )
{
	float s= std::sin(a), c= std::cos(a);

	value[0]= c;
	value[8]= s;
	value[2]= -s;
	value[10]= c;

	value[5]= value[15]= 1.0f;
	value[1]= value[3]= 0.0f;
	value[4]= value[6]= value[7]= 0.0f;
	value[9]= value[11]= 0.0f;
	value[12]= value[13]= value[14]= 0.0f;
}

void m_Mat4::RotateZ( float a )
{
	float s= std::sin(a), c= std::cos(a);
	value[0]= c;
	value[4]= -s;
	value[1]= s;
	value[5]= c;

	value[10]= value[15]= 1.0f;
	value[2]= value[3]= 0.0f;
	value[6]= value[7]= 0.0f;
	value[8]= value[9]= value[11]= 0.0f;
	value[12]= value[13]= value[14]= 0.0f;
}

void m_Mat4::Rotate( const m_Vec3& axis, float angle )
{
	m_Vec3 normalized_vec= axis;
	normalized_vec.Normalize();

	float cos_a= std::cos( +angle );
	float sin_a= std::sin( -angle );
	float one_minus_cos_a= 1.0f - cos_a;

	value[ 0]= cos_a + one_minus_cos_a * normalized_vec.x * normalized_vec.x;
	value[ 1]= one_minus_cos_a * normalized_vec.x * normalized_vec.y - sin_a * normalized_vec.z;
	value[ 2]= one_minus_cos_a * normalized_vec.x * normalized_vec.z + sin_a * normalized_vec.y;
	value[ 3]= 0.0f;
	value[ 4]= one_minus_cos_a * normalized_vec.x * normalized_vec.y + sin_a * normalized_vec.z;
	value[ 5]= cos_a + one_minus_cos_a * normalized_vec.y * normalized_vec.y;
	value[ 6]= one_minus_cos_a * normalized_vec.y * normalized_vec.z - sin_a * normalized_vec.x;
	value[ 7]= 0.0f;
	value[ 8]= one_minus_cos_a * normalized_vec.x * normalized_vec.z - sin_a * normalized_vec.y;
	value[ 9]= one_minus_cos_a * normalized_vec.y * normalized_vec.z + sin_a * normalized_vec.x;
	value[10]= cos_a + one_minus_cos_a * normalized_vec.z * normalized_vec.z;
	value[11]= 0.0f;
	value[12]= 0.0f;
	value[13]= 0.0f;
	value[14]= 0.0f;
	value[15]= 1.0f;
}

void m_Mat4::PerspectiveProjection( float aspect, float fov_y, float z_near, float z_far)
{
	float f= 1.0f / std::tan( fov_y * 0.5f );

	value[0]= f / aspect;
	value[5]= f;

	f= 1.0f / ( z_far - z_near );
	value[14]= -2.0f * f * z_near * z_far;
	value[10]= ( z_near + z_far ) * f;
	value[11]= 1.0f;

	value[1]= value[2]= value[3]= 0.0f;
	value[4]= value[6]= value[7]= 0.0f;
	value[8]= value[9]= 0.0f;
	value[12]= value[13]= value[15]= 0.0f;
}

void m_Mat4::AxonometricProjection( float scale_x, float scale_y, float z_near, float z_far )
{
	value[0]= scale_x;
	value[5]= scale_y;
	value[10]= 2.0f / ( z_far - z_near );
	value[14]= 1.0f - value[10] * z_far;

	value[1 ]= value[2 ]= value[3 ]= 0.0f;
	value[4 ]= value[6 ]= value[7 ]= 0.0f;
	value[8 ]= value[9 ]= value[11]= 0.0f;
	value[12]= value[13]= value[15]= 0.0f;
	value[15]= 1.0f;
}

m_Vec3 operator*( const m_Vec3& v, const m_Mat4& m )
{
	return m_Vec3(
		v.x * m.value[0] + v.y * m.value[4] + v.z * m.value[8] + m.value[12],
		v.x * m.value[1] + v.y * m.value[5] + v.z * m.value[9] + m.value[13],
		v.x * m.value[2] + v.y * m.value[6] + v.z * m.value[10]+ m.value[14] );
}
