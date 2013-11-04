/*!
 * File containing vector calculations, taken from net no author described
 * 
 * 
 */
#ifndef Vector3D_h
#define Vector3D_h
#include <math.h>

class Vector3D
{
public:
	float x,y,z;
	// Ctors
	Vector3D( float InX, float InY, float InZ ) : x( InX ), y( InY ), z( InZ )
		{
		}
	Vector3D( ) : x(0), y(0), z(0)
		{
		}

	// Operator Overloads
	inline bool operator== (const Vector3D& V2) const 
		{
		return (x == V2.x && y == V2.y && z == V2.z);
		}

	inline Vector3D operator+ (const Vector3D& V2) const 
		{
		return Vector3D( x + V2.x,  y + V2.y,  z + V2.z);
		}
	inline Vector3D operator- (const Vector3D& V2) const
		{
		return Vector3D( x - V2.x,  y - V2.y,  z - V2.z);
		}
	inline Vector3D operator- ( ) const
		{
		return Vector3D(-x, -y, -z);
		}

	inline Vector3D operator/ (float S ) const
		{
		float fInv = 1.0f / S;
		return Vector3D (x * fInv , y * fInv, z * fInv);
		}
	inline Vector3D operator/ (const Vector3D& V2) const
		{
		return Vector3D (x / V2.x,  y / V2.y,  z / V2.z);
		}
	inline Vector3D operator* (const Vector3D& V2) const
		{
		return Vector3D (x * V2.x,  y * V2.y,  z * V2.z);
		}
	inline Vector3D operator* (float S) const
		{
		return Vector3D (x * S,  y * S,  z * S);
		}

	inline void operator+= ( const Vector3D& V2 )
		{
		x += V2.x;
		y += V2.y;
		z += V2.z;
		}
	inline void operator-= ( const Vector3D& V2 )
		{
		x -= V2.x;
		y -= V2.y;
		z -= V2.z;
		}

	inline float operator[] ( int i )
		{
		if ( i == 0 ) return x;
		else if ( i == 1 ) return y;
		else return z;
		}

	// Functions
	inline float Dot( const Vector3D &V1 ) const
		{
		return V1.x*x + V1.y*y + V1.z*z;
		}

	inline Vector3D CrossProduct( const Vector3D &V2 ) const
		{
		return Vector3D(
			y * V2.z  -  z * V2.y,
			z * V2.x  -  x * V2.z,
			x * V2.y  -  y * V2.x 	);
		}

	// Return vector rotated by the 3x3 portion of matrix m
	// (provided because it's used by bbox.cpp in article 21)
	Vector3D RotByMatrix( const float m[16] ) const
	{
	return Vector3D( 
		x*m[0] + y*m[4] + z*m[8],
		x*m[1] + y*m[5] + z*m[9],
		x*m[2] + y*m[6] + z*m[10] );
 	}

	// These require math.h for the sqrtf function
	float Magnitude( ) const
		{
		return sqrtf( x*x + y*y + z*z );
		}

	float Distance( const Vector3D &V1 ) const
		{
		return ( *this - V1 ).Magnitude();	
		}

	inline void Normalize()
		{
		float fMag = ( x*x + y*y + z*z );
		if (fMag == 0) {return;}

		float fMult = 1.0f/sqrtf(fMag);            
		x *= fMult;
		y *= fMult;
		z *= fMult;
		return;
		}
	inline void SetZero()
		{
		         
		x = 0;
		y = 0;
		z = 0;
		return;
		}
	inline void Set(float x1, float y1, float z1)
		{
		         
		x = x1;
		y = y1;
		z = z1;
		return;
		}
};
#endif