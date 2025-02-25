#pragma once

#include <cmath>

#include <Core/Config.hpp>

// This header file defines all math types, same as Telltale API, which is used in rendering, etc.

// HDR Colour
struct ColorHDR
{
    float r, g, b, intensity;
};

/// Four part RGBA Colour.
struct alignas(4) Colour {
    
    float r, g, b, a;
    
    inline Colour()
    {
        r = g = b = a = 0.f;
    }
    
    inline Colour(float _R, float _G, float _B, float _A = 1.0f)
    {
        r = _R;
        b = _B;
        g = _G;
        a = _A;
    }
    
    // less than
    inline bool operator<(const Colour& rhs) const {
        return (r < rhs.r) || (r == rhs.r && (g < rhs.g || (g == rhs.g && (b < rhs.b || (b == rhs.b && a <= rhs.a)))));
    }
    
    // convert colour format
    Colour RGBToRGBM(float ex, float scale);
    
    // convert colour format
    inline Colour RGBMToRGB(float ex, float scale) {
        return Colour(powf(a * r * scale, ex), powf(a * g * scale, ex), powf(a * b * scale, ex), 1.0f);
    }
    
    // convert colour format
    inline Colour GammaToLinear() {
        return Colour(powf(r, 2.2f), powf(g, 2.2f), powf(b, 2.2f), a);
    }
    
    // convert colour format
    inline Colour LinearToGamma() {
        return Colour(powf(r, 1.0f / 2.2f), powf(g, 1.0f / 2.2f), powf(b, 1.0f / 2.2f), a);
    }
    
    inline Colour operator*=(float mult) {
        return Colour(r * mult, g * mult, b * mult, a * mult);
    }
    
    inline Colour operator/=(float mult) {
        return operator*=(1.0f / mult);
    }
    
    inline Colour operator+=(float mult) {
        return Colour(r + mult, g + mult, b + mult, a + mult);
    }
    
    inline Colour operator-=(float mult) {
        return operator+=(-mult);
    }
    
    // STATIC COLOURS
    
    static Colour Black;
    static Colour White;
    static Colour Red;
    static Colour Green;
    static Colour Blue;
    static Colour Cyan;
    static Colour Magenta;
    static Colour Yellow;
    static Colour DarkRed;
    static Colour DarkGreen;
    static Colour DarkBlue;
    static Colour DarkCyan;
    static Colour DarkMagenta;
    static Colour DarkYellow;
    static Colour Gray;
    static Colour LightGray;
    static Colour NonMetal;
    
};

// 2D VECTOR
struct alignas(4) Vector2 {
    
    static Vector2 Zero;
    
    // Access by vec2.x,u,array[0] and vec2.y,v,array[1]
    union {
        struct {
            union {
                float x;
                float u;
            };
            union {
                float y;
                float v;
            };
        };
        float array[2];
    };
    
    inline Vector2()
    {
        x = y = 0.f;
    }
    
    inline Vector2(float _x, float _y)
    {
        x = _x;
        y = _y;
    }
    
    inline Vector2(float _all) : x(_all), y(_all) {}
    
    // Returns the length squared, no square root invovled, so is faster
    inline float MagnitudeSquared() const
    {
        float mag = x * x + y * y;
        return mag;
    }
    
    // Returns the length of the vector
    inline float Magnitude() const
    {
        float mag = MagnitudeSquared();
        if (mag < 9.9999997e-21f)
            mag = 0.f;
        else
            mag = sqrtf(mag);
        return mag;
    }
    
    // Normalises this vector to length 1
    inline void Normalize()
    {
        float mag = Magnitude();
        if (mag > 0.f)
            mag = 1.0f / mag;
        x *= mag;
        y *= mag;
    }
    
    // Sets the values of this vector
    inline void Set(float x, float y)
    {
        this->x = x;
        this->y = y;
    }
    
    inline Vector2& operator+=(const Vector2& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    
    inline Vector2& operator-=(const Vector2& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    
    inline Vector2& operator*=(const Vector2& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }
    
    inline Vector2& operator/=(const Vector2& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }
    
    // Access by index 0 or 1 to x or y.
    inline float operator[](int index) const
    {
        return index == 0 ? x : y;
    }
    
    // Returns negated vector
    inline Vector2 operator-() const
    {
        return Vector2(-x, -y);
    }
    
};

struct Vector4;

// 3D VECTOR
struct alignas(4) Vector3 {
    
    // Returns 2D vector of the first two components
    inline operator Vector2() const
    {
        return Vector2(x, y);
    }
    
    inline Vector3() { x = y = z = 0.f; }
    
    inline Vector3(float _all) : x(_all), y(_all), z(_all) {}
    
    inline Vector3(float _x, float _y, float _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }
    
    // Returns component by index
    inline float operator[](int index) const
    {
        return index == 0 ? x : index == 1 ? y : z;
    }
    
    inline Vector3(Vector2 xy, float z) : Vector3(xy.x, xy.y, z) {}
    
    inline Vector3(float x, Vector2 yz) : Vector3(x, yz.x, yz.y) {}
    
    float x;
    float y;
    float z;
    
    // Sets the components
    inline void Set(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    
    // Gets the magnitude of this vector, the length.
    inline float Magnitude() const
    {
        float mag = x * x + y * y + z * z;
        if (mag < 9.9999997e-21f)
            mag = 0.f;
        else
            mag = sqrtf(mag);
        return mag;
    }
    
    // Gets the squared length of this vector, no square root.
    inline float MagnitudeSquared() const
    {
        float mag = x * x + y * y + z * z;
        if (mag < 9.9999997e-21f)
            mag = 0.f;
        return mag;
    }
    
    // Normalises to length 1
    inline void Normalize()
    {
        float mag = Magnitude();
        if (mag > 0.f)
            mag = 1.0f / mag;
        x *= mag;
        y *= mag;
        z *= mag;
    }
    
    inline Vector3& operator+=(const Vector3& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    
    inline Vector3& operator-=(const Vector3& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }
    
    inline Vector3& operator*=(const Vector3& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }
    
    inline Vector3& operator/=(const Vector3& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        return *this;
    }
    
    // Construct from vector 2 setting z to zero.
    inline Vector3(Vector2 xy) : Vector3(xy,0.f) {}
    
    inline Vector3(Vector4 xyz);
    
    // Cross product of two vectors, determinant of {xhat,yhat,zhat, v1, v2} 3x3 matrix
    inline static Vector3 XProduct(const Vector3& v1, const Vector3& v2)
    {
        return Vector3(v1[1] * v2[2] - v1[2] * v2[1],
                       v1[2] * v2[0] - v1[0] * v2[2],
                       v1[0] * v2[1] - v1[1] * v2[0]);
    }
    
    // Returns the normalised version of the given vector
    static inline Vector3 Normalize(Vector3 vec)
    {
        vec.Normalize();
        return vec;
    }
    
    // Calculate dot product between two vectors
    static inline float Dot(const Vector3& lhs, const Vector3& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }
    
    // Calculate the cross product between two vectors
    static inline Vector3 Cross(const Vector3& v1, const Vector3& v2)
    {
        return XProduct(v1, v2);
    }
    
    // Calculates, using the a dot b = 2ab cos theta, the angle between two vectors
    static inline float AngleBetween(const Vector3& lhs, const Vector3& rhs)
    {
        return acosf(Dot(lhs, rhs) / sqrtf(lhs.MagnitudeSquared() * rhs.MagnitudeSquared()));
    }
    
    // Completes the (normalised) orthonormal basis, given the normal vector. Outputs into the last two arguments.
    static inline void CompleteOrthonormalBasis(const Vector3& n, Vector3& b1, Vector3& b2) {
        if (n.z < -0.9999999f)
        {
            b1 = Vector3(0.0f, -1.0f, 0.0f);
            b2 = Vector3(-1.0f, 0.0f, 0.0f);
            return;
        }
        const float a = 1.0f / (1.0f + n.z);
        const float b = -n.x * n.y * a;
        b1 = Vector3(1.0f - n.x * n.x * a, b, -n.x);
        b2 = Vector3(b, 1.0f - n.y * n.y * a, -n.y);
    }
    
    // Negated vector
    inline Vector3 operator-() const
    {
        return Vector3(-x, -y, -z);
    }
    
    // Vector from color, ignoring alpha.
    inline Vector3(Colour c) : x(c.r), y(c.g), z(c.b) {}
    
    static Vector3 Up;
    static Vector3 Down;
    static Vector3 Left;
    static Vector3 Right;
    static Vector3 Forward;
    static Vector3 Backward;
    static Vector3 VeryFar;
    static Vector3 Zero;
    static Vector3 Identity;
    
};

struct alignas(4) Vector4 {
    
    // Convert to color, rgba to xyzw
    inline operator Colour() {
        return Colour(x, y, z, w);
    }
    
    static Vector4 Zero;
    
    inline Vector4(Colour c) : x(c.r), y(c.g), z(c.b), w(c.a) {}
    
    // Comparison
    inline bool operator==(const Vector4& rhs)
    {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }
    
    // Negated this vector
    inline Vector4 operator-() const
    {
        return Vector4(-x, -y, -z, -w);
    }
    
    // Ignore W, return x,y,z as a vector 3.
    inline operator Vector3() const
    {
        return Vector3(x, y, z);
    }
    
    // Ignore Z, W. Return X,Y as a vector 2.
    inline operator Vector2() const
    {
        return Vector2(x, y);
    }
    
    inline Vector4(Vector3 xyz) : Vector4(xyz, 0.f) {}
    
    inline Vector4(Vector2 xy) : Vector4(xy, 0.f, 0.f) {}
    
    inline Vector4() { x = y = z = 0.f; w = 0.0f; }
    
    inline Vector4(float _all) : x(_all), y(_all), z(_all), w(_all) {}
    
    inline Vector4(float _x, float _y, float _z, float _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }
    
    inline Vector4(float _x, Vector3 yzw) : x(_x), y(yzw.x), z(yzw.y), w(yzw.z) {}
    
    inline Vector4(Vector3 xyz, float _w) : x(xyz.x), y(xyz.y), z(xyz.z), w(_w) {}
    
    inline Vector4(Vector2 xy, Vector2 zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}
    
    inline Vector4(float _x, float _y, Vector2 zw) : x(_x), y(_y), z(zw.x), w(zw.y) {}
    
    inline Vector4(Vector2 xy, float _z, float _w) : x(xy.x), y(xy.y), z(_z), w(_w) {}
    
    inline Vector4(float _x, Vector2 yz, float _w) : x(_x), y(yz.x), z(yz.y), w(_w) {}
    
    float x;
    float y;
    float z;
    float w;
    
    // Access by index
    inline float operator[](int index) const
    {
        return index == 0 ? x : index == 1 ? y : index == 2 ? z : w;
    }

    // Sets components
    inline void Set(float x, float y, float z, float w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }
    
    inline Vector4& operator+=(const Vector4& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }
    
    inline Vector4& operator-=(const Vector4& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }
    
    inline Vector4& operator*=(const Vector4& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        z *= rhs.w;
        return *this;
    }
    
    inline Vector4& operator/=(const Vector4& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        w /= rhs.w;
        return *this;
    }
    
    // Returns the length of this vector
    inline float Magnitude() const
    {
        float mag = MagnitudeSquared();
        return sqrtf(mag);
    }
    
    // Returns the squared length of this vector with no square root.
    inline float MagnitudeSquared() const
    {
        float mag = x * x + y * y + z * z + w * w;
        if (mag < 9.9999997e-21f)
            mag = 0.f;
        return mag;
    }
    
};

inline Vector3::Vector3(Vector4 rhs)
{
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
}

inline Vector2 operator*(const Vector2& lhs, const Vector2& rhs)
{
    Vector2 res = lhs;
    res *= rhs;
    return res;
}

inline Vector3 operator*(const Vector3& lhs, const Vector3& rhs)
{
    Vector3 res = lhs;
    res *= rhs;
    return res;
}

inline Vector4 operator*(const Vector4& lhs, const Vector4& rhs)
{
    Vector4 res = lhs;
    res *= rhs;
    return res;
}

inline Vector2 operator/(const Vector2& lhs, const Vector2& rhs)
{
    Vector2 res = lhs;
    res /= rhs;
    return res;
}

inline Vector3 operator/(const Vector3& lhs, const Vector3& rhs)
{
    Vector3 res = lhs;
    res /= rhs;
    return res;
}

inline Vector4 operator/(const Vector4& lhs, const Vector4& rhs)
{
    Vector4 res = lhs;
    res /= rhs;
    return res;
}

inline Vector2 operator-(const Vector2& lhs, const Vector2& rhs)
{
    Vector2 res = lhs;
    res -= rhs;
    return res;
}

inline Vector3 operator-(const Vector3& lhs, const Vector3& rhs)
{
    Vector3 res = lhs;
    res -= rhs;
    return res;
}

inline Vector4 operator-(const Vector4& lhs, const Vector4& rhs)
{
    Vector4 res = lhs;
    res -= rhs;
    return res;
}

inline Vector2 operator+(const Vector2& lhs, const Vector2& rhs)
{
    Vector2 res = lhs;
    res += rhs;
    return res;
}

inline Vector3 operator+(const Vector3& lhs, const Vector3& rhs)
{
    Vector3 res = lhs;
    res += rhs;
    return res;
}

inline Vector4 operator+(const Vector4& lhs, const Vector4& rhs)
{
    Vector4 res = lhs;
    res += rhs;
    return res;
}

struct Sphere;

// Bounding box
struct BoundingBox {
    
    // Face enum
    enum eFace
    {
        kFace_None   = 0x0,
        kFace_Top    = 0x1,
        kFace_Bottom = 0x2,
        kFace_Left   = 0x4,
        kFace_Right  = 0x8,
        kFace_Front  = 0x10,
        kFace_Back   = 0x20,
        kFaces_All = 0x3F,
    };
    
    Vector3 _Min, _Max;
    
    // First vector when computing bounding box. Keep using AddPoint for all, then finalise after. Then this BB will be the box for the vecs.
    inline void Start(const Vector3& v)
    {
        _Max = _Min = v;
    }
    
    // See Start(vector3). Sets min and max to very large values.
    inline void Start()
    {
        _Min.x = 3.4028235e38;
        _Min.y = 3.4028235e38;
        _Min.z = 3.4028235e38;
        _Max.x = -3.4028235e38;
        _Max.y = -3.4028235e38;
        _Max.z = -3.4028235e38;
    }
    
    // Add a point in a mesh or whatever's bounding box needs to be calculated.
    inline void AddPoint(const Vector3& v)
    {
        if (v.x < _Min.x) _Min.x = v.x;
        if (v.y < _Min.y) _Min.y = v.y;
        if (v.z < _Min.z) _Min.z = v.z;
        
        if (v.x > _Max.x) _Max.x = v.x;
        if (v.y > _Max.y) _Max.y = v.y;
        if (v.z > _Max.z) _Max.z = v.z;
    }
    
    // After all calls to AddPoint, finalise it here.
    inline void Finalize(){
        if (_Max.x < _Min.x)
        {
            _Max.x = 0.f;
            _Min.x = 0.f;
        }
        if (_Max.y < _Min.y)
        {
            _Max.y = 0.f;
            _Min.y = 0.f;
        }
        if (_Max.z < _Min.z)
        {
            _Max.z = 0.f;
            _Min.z = 0.f;
        }
    }
    
    // Returns the nearest point on or within the bounding box defined by the minimum corner and the maximum corner, which lies inside the BB.
    Vector3 PointOnBox(const Vector3& pt);
    
    // Tests a line intersection with this bounding box. Outputs into intersection point and also t, how much (0-1) along the line p1 to p2.
    // Flags is which faces to test. Pass in faces all to test all. See eFace enum
    Bool LineIntersection(Vector3 p1, Vector3 p2, U32 cflags, Vector3 &intersectionPoint, float& t);
    
    // See other overload. Doesn't output the intersection point, just tests.
    inline Bool LineIntersection(Vector3 p1, Vector3 p2, U32 cflags)
    {
        Vector3 _v{};
        float _f{};
        return LineIntersection(p1, p2, cflags, _v, _f);
    }
    
    // Performs collision with a line. If returns true then t will contain the point (0-1) along the line
    Bool CollideWithLine(Vector3 p1, Vector3 p2, Vector3 &intersectionPoint, float& t);
    
    // Computes the enclosing sphere
    Sphere GetEnclosingSphere();
    
    // Translates the given by the the component of delta that face uses
    void TranslateFace(eFace, Vector3 delta);
    
    // Gets the center of the given face of this bounding box
    Vector3 GetFaceCenter(eFace);
    
    // Returns the face the given vector which hit it first hit on
    eFace HitFace(Vector3 hitPos);
    
    // Performs a line collision of the given ray (extending it further internally), outputting the collision point and returning the face.
    eFace HitFace(Vector3 rayPos, Vector3 rayEnd, Vector3 &outHitPos);
    
    // Returns the merged bounding box of this and rhs.
    BoundingBox Merge(const BoundingBox &rhs);
    
};

// A sphere
struct Sphere
{
    
    Vector3 _Center;
    float _Radius;
    
    Bool FullyContains(const Sphere& rhs);
    
    // Merges this and rhs sphere. The returned sphere contains this and rhs in the smallest size
    Sphere Merge(const Sphere& rhs);
    
    // Makes a sphere which encloses the cone. Used in occlusion testing for simpler collision detection
    static Sphere MakeForCone(Vector3 conePosition, Vector3 coneForward, float coneLength, float coneAngle);
    
    // Perform collision with a cone
    Bool CollideWithCone(Vector3 &conePos, Vector3 &coneNorm, float coneRadius, float dotProduct);
    
};

// Quaternion
struct Quaternion {
    
    inline Quaternion() { x = y = z = 0.f; w = 1.0f; }
    
    inline Quaternion(float _x, float _y, float _z, float _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }
    
    float x;
    float y;
    float z;
    float w;
    
    Quaternion(const Quaternion&) = default;
    
    static Quaternion kIdentity;
    static Quaternion kBackward;
    static Quaternion kUp;
    static Quaternion kDown;
    
    inline Quaternion Conjugate() const
    {
        return Quaternion(-x, -y, -z, w);
    }
    
	inline Quaternion(const Vector3& normalizedAxis, float radians)
	{
		// Calculate half angle
		float halfAngle = radians * 0.5f;
		float sine = sinf(halfAngle);
		float cosine = cosf(halfAngle);
		
		// Set the quaternion components
		x = normalizedAxis.x * sine;
		y = normalizedAxis.y * sine;
		z = normalizedAxis.z * sine;
		w = cosine;
	}

    
    inline void SetEuler(float xrot, float yrot, float zrot)
    { // expanded matrix stuff
        float xrot_sin = sinf(xrot * 0.5f);
        float xrot_cos = cosf(xrot * 0.5f);
        float yrot_sin = sinf(yrot * 0.5f);
        float yrot_cos = cosf(yrot * 0.5f);
        float zrot_sin = sinf(zrot * 0.5f);
        float zrot_cos = cosf(zrot * 0.5f);
        x = zrot_cos * yrot_cos * xrot_sin - zrot_sin * yrot_sin * xrot_cos;
        y = xrot_cos * yrot_sin * zrot_cos + xrot_sin * yrot_cos * zrot_sin;
        z = xrot_cos * yrot_cos * zrot_sin - xrot_sin * yrot_sin * zrot_cos;
        w = xrot_cos * yrot_cos * zrot_cos + xrot_sin * yrot_sin * zrot_sin;
        Normalize();
    }
    
    inline Quaternion& ExponentialMap(Vector3& v)
    {
        float v4 = v.x;
        float v5 = (float)((float)(v4 * v4) + (float)(v.y * v.y)) + (float)(v.z * v.z);
        float v6 = sqrtf(v5);
        float v7 = cosf(v6 * 0.5);
        float v8;
        if (v6 >= 0.064996749f)
            v8 = sinf(v6 * 0.5) / v6;
        else
            v8 = 0.5 - (float)(v5 * 0.020833334f);
        x = v4 * v8;
        y = v8 * v.y;
        float v10 = v8 * v.z;
        w = v7;
        z = v10;
        return *this;
    }
    
    inline Quaternion& ExpMap(Vector3& v)
    {
        return ExponentialMap(v);
    }
    
    inline Vector3 LogMap()
    {
        Vector3 v{};
        float mag = x * x + y * y + z * z;
        if(mag >= 0.000001f){
            mag = 2.0f * atan2f(mag, w);
            v.x = mag * x;
            v.y = mag * y;
            v.z = mag * z;
        }else{
            v.x = v.y = v.z = 0.f;
        }
        return v;
    }
    
    //pass in 1/deltatime to ensure units
    inline Vector3 CalcRotationVelocity(float invDt)
    {
        Vector3 v = LogMap();
        v.x *= invDt;
        v.y *= invDt;
        v.z *= invDt;
        return v;
    }
    
    inline void SetDirectionXYZ(const Vector3& normalDir)
    {
        float mag = normalDir.Magnitude();
        if (mag < 0.99999899f || mag > 1.000001f) {
            x = y = z = 0.f;
            w = 1.0f;
        }
        else {
            SetEuler(-asinf(normalDir.y), atan2f(normalDir.x, normalDir.z), 0.f);
        }
    }
    
    inline Quaternion(const Vector3& normalDir) : Quaternion()
    {
        SetDirectionXYZ(normalDir);
    }
    
    inline Bool IsWellFormed()
    {
        // Compute the sum of squares of the components and check if it is close to 1
        float t = x * x + y * y + z * z + w * w - 1.0f;
        t = fmaxf(t, 0.0f); // Ensure non-negative value (no negative squared errors)
        
        // If t is too large (meaning the quaternion is not normalized)
        if (t > 0.001f)
            return false;
        
        // Check if each component is within the valid range [-1.0f, 1.0f]
        if (x > 1.0f || y > 1.0f || z > 1.0f || w > 1.0f)
            return false;
        
        // All checks passed, quaternion is well-formed
        return true;
    }
    
    inline void GetAxisAngle(Vector3& axisout,float& angleout)
    {
        float cosw = 2.0f*acosf(w);
        if ((cosw <= -0.000001f || cosw >= 0.000001f) && (cosw <= 6.2831845f || cosw >= 6.2831864f))
        {
            axisout.x = x;
            axisout.y = y;
            axisout.z = z;
            axisout.Normalize();
        }
        else
        {
            axisout.x = 1.0f;
            axisout.y = axisout.z = 0.0f;
        }
    }
    
    inline void GetEuler(float& xrot, float& yrot, float& zrot)
    {
        float sinr_cosp = 2 * (w * x + y * z);
        float cosr_cosp = 1 - 2 * (x * x + y * y);
        xrot = atan2f(sinr_cosp, cosr_cosp);
        float sinp = sqrtf(1 + 2 * (w * y - x * z));
        float cosp = sqrtf(1 - 2 * (w * y - x * z));
        yrot = 2.0f * atan2f(sinp, cosp) - 1.5707963f;
        double siny_cosp = 2 * (w * z + x * y);
        double cosy_cosp = 1 - 2 * (y * y + z * z);
        zrot = atan2f(siny_cosp, cosy_cosp);
    }
    
    inline void GetEuler(Vector3& out)
    {
        GetEuler(out.x, out.y, out.z);
    }
    
    inline bool IsNormalized()
    {
        float v1 = (Vector4{ x, y, z, w}).MagnitudeSquared();
        return v1 >= 0.95f && v1 <= 1.05f;
    }
    
    inline void Normalize()
    {
        float mag = (Vector3{ x, y, z }).Magnitude();
        x *= mag;
        y *= mag;
        z *= mag;
    }
    
    inline Quaternion operator*(const Quaternion& rhs) const
    {
        Quaternion res{};
        res.x = (x * rhs.w) + (rhs.x * w) + (y * rhs.z) - (z * rhs.y);
        res.y = (y * rhs.w) + (rhs.y * w) + (z * rhs.x) - (rhs.z * x);
        res.z = (z * rhs.w) + (rhs.z * w) + (x * rhs.y) - (y * rhs.x);
        res.w = (w * rhs.w) - (rhs.x * w) - (rhs.y * y) - (rhs.z * z);
        return res;
    }
    
    inline Quaternion& operator*=(const Quaternion& rhs)
    {
        *this = operator*(rhs);
        return *this;
    }
    
    // Perform linear interpolation
    static Quaternion NLerp(const Quaternion& q0, const Quaternion& q, float t);
    
    // Perform spherical interpolation
    static Quaternion Slerp(Quaternion q, const Quaternion& q0, float t);
    
    // Creates a quaternion which rotates from va to vb
    static Quaternion BetweenVector3(const Vector3& va, const Vector3& vb);
    
};

// Rotate vector 3 by quaternion
Vector3 operator*(const Vector3& vec, const Quaternion& quat);

// A rotation and translation, a transformation.
struct Transform {
    
    Quaternion _Rot;
    Vector3 _Trans;
    
    inline Transform() :  _Rot{}, _Trans{} {}
    
    inline Transform(Quaternion rotation, Vector3 translation) : _Rot(rotation), _Trans(translation) {}
    
    inline Transform operator*(const Transform& rhs) const
    {
        Transform res{};
        res._Rot = _Rot * rhs._Rot;
        res._Trans = _Trans + (rhs._Trans * _Rot);
        return res;
    }
    
    inline Transform operator/(const Transform& rhs) const
    {
        Transform res{};
        Quaternion tmpQ{ -rhs._Rot.x,-rhs._Rot.y,-rhs._Rot.z, rhs._Rot.w };
        res._Rot = tmpQ * _Rot;
        res._Trans = (_Trans - rhs._Trans) * tmpQ;
        return res;
    }
    
    inline Transform& operator/=(const Transform& rhs)
    {
        Transform op = operator/(rhs);
        *this = op;
        return *this;
    }
    
    // Perform inverse
    inline Transform operator~() const
    {
        Transform ret{};
        ret._Rot = Quaternion{ -_Rot.x, -_Rot.y,-_Rot.z, _Rot.w };
        Vector3 tmpV = { -_Trans.x, -_Trans.y, -_Trans.z };
        ret._Trans = tmpV * ret._Rot;
        return ret;
    }
    
};

// Polar (spherical) coordinates: x = r cos(phi) sin(theta), y = r sin(phi) sin(theta), z = r cos(theta)
struct Polar
{
    float mR;
    float mTheta;
    float mPhi;
};

/*
 Matrix4 Written by Matthew Fisher: a 4x4 transformation matrix structure.
*/

class alignas(16) Matrix4
{
public:

    Matrix4();
    Matrix4(const Matrix4& M);
    Matrix4(const Vector3& V0, const Vector3& V1, const Vector3& V2);
	Matrix4(Vector4 Row[4]);
	
	// input in row major order, 64 bytes, of matrix
	inline Matrix4(float* entries)
	{
		memcpy(_Entries, entries, 64);
	}
    
    //
    // Assignment
    //
    Matrix4& operator = (const Matrix4& M);
    
    //
    // Math properties
    //
    float Determinant() const;
    Matrix4 Transpose() const;
    Matrix4 Inverse() const;
    
    //
    // Vector transforms
    //
    inline Vector3 TransformPoint(const Vector3& point) const
    {
        float w = point.x * _Entries[0][3] + point.y * _Entries[1][3] + point.z * _Entries[2][3] + _Entries[3][3];
        if (w)
        {
            const float invW = 1.0f / w;
            return Vector3((point.x * _Entries[0][0] + point.y * _Entries[1][0] + point.z * _Entries[2][0] + _Entries[3][0]) * invW,
                           (point.x * _Entries[0][1] + point.y * _Entries[1][1] + point.z * _Entries[2][1] + _Entries[3][1]) * invW,
                           (point.x * _Entries[0][2] + point.y * _Entries[1][2] + point.z * _Entries[2][2] + _Entries[3][2]) * invW);
        }
        else
        {
            return Vector3();
        }
    }
    
    inline Vector3 TransformNormal(const Vector3& normal) const
    {
        return Vector3(normal.x * _Entries[0][0] + normal.y * _Entries[1][0] + normal.z * _Entries[2][0],
                       normal.x * _Entries[0][1] + normal.y * _Entries[1][1] + normal.z * _Entries[2][1],
                       normal.x * _Entries[0][2] + normal.y * _Entries[1][2] + normal.z * _Entries[2][2]);
    }
    
    //
    // Accessors
    //
    inline float* operator [] (int Row)
    {
        return _Entries[Row];
    }
    inline const float* operator [] (int Row) const
    {
        return _Entries[Row];
    }
    inline void SetColumn(U32 Column, const Vector4& Values)
    {
        _Entries[0][Column] = Values.x;
        _Entries[1][Column] = Values.y;
        _Entries[2][Column] = Values.z;
        _Entries[3][Column] = Values.w;
    }
    inline void SetRow(U32 Row, const Vector4& Values)
    {
        _Entries[Row][0] = Values.x;
        _Entries[Row][1] = Values.y;
        _Entries[Row][2] = Values.z;
        _Entries[Row][3] = Values.w;
    }
    
    inline Vector4 GetColumn(U32 Column)
    {
        Vector4 Result;
        Result.x = _Entries[0][Column];
        Result.y = _Entries[1][Column];
        Result.z = _Entries[2][Column];
        Result.w = _Entries[3][Column];
        return Result;
    }
    inline Vector4 GetRow(U32 Row)
    {
        Vector4 Result;
        Result.x = _Entries[Row][0];
        Result.y = _Entries[Row][1];
        Result.z = _Entries[Row][2];
        Result.w = _Entries[Row][3];
        return Result;
    }
    
    //
    // Transformation matrices
    //
    static Matrix4 Identity();
    static Matrix4 Scaling(const Vector3& ScaleFactors);
    static Matrix4 Scaling(float ScaleFactor)
    {
        return Scaling(Vector3(ScaleFactor, ScaleFactor, ScaleFactor));
    }
    static Matrix4 Translation(const Vector3& Pos);
    static Matrix4 Rotation(const Vector3& Axis, float Angle, const Vector3& Center);
    static Matrix4 Rotation(const Vector3& Axis, float Angle);
    static Matrix4 Rotation(float Yaw, float Pitch, float Roll);
    static Matrix4 Rotation(const Vector3& Basis1, const Vector3& Basis2, const Vector3& Basis3);
    static Matrix4 RotationX(float Theta);
    static Matrix4 RotationY(float Theta);
    static Matrix4 RotationZ(float Theta);
    static Matrix4 Camera(const Vector3& Eye, const Vector3& Look, const Vector3& Up, const Vector3& Right);
    static Matrix4 LookAt(const Vector3& Eye, const Vector3& At, const Vector3& Up);
    static Matrix4 Orthogonal(float Width, float Height, float ZNear, float ZFar);
    static Matrix4 OrthogonalOffCenter(float left, float right, float bottom, float top, float ZNear, float ZFar);
    static Matrix4 Perspective(float Width, float Height, float ZNear, float ZFar);
    static Matrix4 PerspectiveFov(float FOV, float Aspect, float ZNear, float ZFar);
    static Matrix4 PerspectiveMultiFov(float FovX, float FovY, float ZNear, float ZFar);
    static Matrix4 Face(const Vector3& V0, const Vector3& V1);
    static Matrix4 Viewport(float Width, float Height);
    static Matrix4 ChangeOfBasis(const Vector3& Source0, const Vector3& Source1, const Vector3& Source2, const Vector3& SourceOrigin,
                                 const Vector3& Target0, const Vector3& Target1, const Vector3& Target2, const Vector3& TargetOrigin);
    static float CompareMatrices(const Matrix4& Left, const Matrix4& Right);
    
    float _Entries[4][4];
};

// Matrix which rotates by quaternion
Matrix4 MatrixRotation(const Quaternion& q);

// Matrix which translates by the vector
Matrix4 MatrixTranslation(const Vector3& Translation);

// Matrix which transforms
Matrix4 MatrixTransformation(const Quaternion& rot, const Vector3& Translation);

// Matrix which scales and transforms
Matrix4 MatrixTransformation(const Vector3 scale, const Quaternion& rot, const Vector3& Translation);

// Matrix which scales differently in each direction
Matrix4 MatrixScaling(float ScaleX, float ScaleY, float ScaleZ);

// Matrix which rotates by yaw pitch and roll
Matrix4 MatrixRotationYawPitchRollDegrees(float yaw, float pitch, float roll);

// GLOBAL MATRIX OPERATORS

Matrix4 operator * (const Matrix4& Left, const Matrix4& Right);
Matrix4 operator * (const Matrix4& Left, float& Right);
Matrix4 operator * (float& Left, const Matrix4& Right);
Matrix4 operator + (const Matrix4& Left, const Matrix4& Right);
Matrix4 operator - (const Matrix4& Left, const Matrix4& Right);

// Apply matrix to vector 4
inline Vector4 operator * (const Vector4& Right, const Matrix4& Left)
{
    return Vector4(Right.x * Left[0][0] + Right.y * Left[1][0] + Right.z * Left[2][0] + Right.w * Left[3][0],
                   Right.x * Left[0][1] + Right.y * Left[1][1] + Right.z * Left[2][1] + Right.w * Left[3][1],
                   Right.x * Left[0][2] + Right.y * Left[1][2] + Right.z * Left[2][2] + Right.w * Left[3][2],
                   Right.x * Left[0][3] + Right.y * Left[1][3] + Right.z * Left[2][3] + Right.w * Left[3][3]);
}

// A plane, defined by the normal vector.
struct Plane {
    
    Vector4 _Plane;
    
    // Transform by matrix
    inline void TransformBy(const Matrix4& lhs)
    {
        _Plane = _Plane * lhs;
    }
    
};

// Frustum plane index in plane array
enum FRUSTUM_PLANE_INDEX {
    FRUSTUM_PLANE_ZNEAR = 0,
    FRUSTUM_PLANE_LEFT = 1,
    FRUSTUM_PLANE_RIGHT = 2,
    FRUSTUM_PLANE_DOWN = 3,
    FRUSTUM_PLANE_UP = 4,
    FRUSTUM_PLANE_ZFAR = 5,
    FRUSTUM_PLANE_COUNT = 6
};

struct Frustum {
    
    Plane _Plane[FRUSTUM_PLANE_COUNT];
    
    inline Frustum() {}
    
    inline void TransformBy(const Matrix4& lhs){
        for (U32 i = 0; i < 6; i++)
            _Plane[i].TransformBy(lhs);
    }
    
    // Returns if the given bounding box is visible at all, and optionally will let you know if it is straddling near a plane about to be out.
    Bool Visible(const BoundingBox& box, bool* pbStraddlesNearPlane);
    
    // Returns if the given bounding box is visible at all.
    inline Bool Visible(const BoundingBox& box)
    {
        return Visible(box, nullptr);
    }
    
    // Determined if a box, defined by its 8 corners, is entirely outside the frustum (ie returns false), else true.
    inline Bool InternalVisibleTest(const Vector3 boxCorners[8]) const
    {
        for (U32 i = 0; i < 6; i++) {
            for(int j = 0; j < 8; j++){
                if (!((Vector3::Dot(Vector3(_Plane[i]._Plane), boxCorners[j]) + _Plane[i]._Plane.w) > 0.0f))
                    return false;
            }
        }
        return true;
    }
    
};

