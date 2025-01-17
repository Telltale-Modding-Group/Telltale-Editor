#include <Core/Math.hpp>

Colour Colour::Black{0.f,0.f,0.f,1.0f};
Colour Colour::White{ 1.f,1.f,1.f,1.0f };
Colour Colour::Red{ 1.f,0.f,0.f,1.0f };
Colour Colour::Green{ 0.f,1.f,0.f,1.0f };
Colour Colour::Blue{ 0.f,0.f,1.f,1.0f };
Colour Colour::Cyan{ 0.f,1.f,1.f,1.0f };
Colour Colour::Magenta{ 1.f,0.f,1.f,1.0f };
Colour Colour::Yellow{ 1.f,1.f,0.f,1.0f };
Colour Colour::DarkRed{ 0.5f,0.f,0.f,1.0f };
Colour Colour::DarkGreen{ 0.f,0.5f,0.f,1.0f };
Colour Colour::DarkBlue{ 0.f,0.f,0.5f,1.0f };
Colour Colour::DarkCyan{ 0.f,0.5f,0.5f,1.0f };
Colour Colour::DarkMagenta{ 0.5f,0.f,0.5f,1.0f };
Colour Colour::DarkYellow{ 0.5f,0.5f,0.f,1.0f };
Colour Colour::Gray{ 0.5f,0.5f,0.5f,1.0f };
Colour Colour::LightGray{ 0.75f,0.75f,0.75f,1.0f };
Colour Colour::NonMetal{ 0.4f,0.4f,0.4f,1.0f };

Vector2 Vector2::Zero{0.f, 0.f};
Vector4 Vector4::Zero{0.f, 0.f, 0.f, 0.f};
Vector3 Vector3::Up{ 0.0f, 1.0f, 0.0f };
Vector3 Vector3::Down{ 0.0f, -1.0f, 0.0f };
Vector3 Vector3::Left{ 1.0f, 0.0f, 0.0f };
Vector3 Vector3::Right{ -1.0f, 0.0f, 0.0f };
Vector3 Vector3::Forward{ 0.0f, 0.0f, 1.0f };
Vector3 Vector3::Backward{ 0.0f, 0.0f, -1.0f };
Vector3 Vector3::VeryFar{ 1e6f, 1e6f, 1e6f };
Vector3 Vector3::Identity{ 1.0f, 1.0f, 1.0f };
Vector3 Vector3::Zero{ 0.f, 0.f, 0.f };

Quaternion Quaternion::kIdentity{0.f,0.f,0.f,1.0f};
Quaternion Quaternion::kBackward{ 0.f, 0.f, 0.f, -4.3711388e-8f };
Quaternion Quaternion::kUp{ 0.f, 0.f, 0.f, 0.70710677f };//2^-0.5
Quaternion Quaternion::kDown{ 0.f, 0.f, 0.f, 0.70710677f };//2^-0.5

Colour Colour::RGBToRGBM(float ex, float scale) {
    Colour ret{};
    float exponented = powf(scale, ex);
    float RED = r;
    float exponeted = exponented;
    float GREEN = g;
    float BLUE = b;
    float maxRGB = 1.0f;
    float SCALECOPY = powf(RED, 1.0f / ex);
    float green_to_exp = powf(GREEN, 1.0f / ex);
    float v13 = powf(BLUE, 1.0 / ex) * (float)(1.0f / scale);
    maxRGB = (float)(1.0f / scale) * SCALECOPY;
    if (maxRGB >= 1.0f)
        maxRGB = 1.0f;
    maxRGB *= 255.0f;
    maxRGB = ceilf(maxRGB);
    float v16 = maxRGB * 0.0039215689f;// 1/255
    if (v16 <= 1e-6f)
        return Colour::Black;
    ret.a = v16;
    ret.r = (float)(1.0f / v16) * (float)(SCALECOPY * (float)(1.0 / scale));
    ret.g = (float)(1.0f / v16) * (float)(green_to_exp * (float)(1.0 / scale));
    ret.b = (float)(1.0f / v16) * v13;
    return ret;
}

Vector3 BoundingBox::PointOnBox(const Vector3& pt){
    Vector3 result = pt;
    Vector3 p = pt;
    if (p.x >= _Min.x)
        _Min.x = p.x;
    else
        result.x = _Min.x;
    _Min.y = _Min.y;
    if (p.y < _Min.y){
        result.y = _Min.y;
        p.y = _Min.y;
    }
    _Min.z = _Min.z;
    if (p.z < _Min.z){
        result.z = _Min.z;
        p.z = _Min.z;
    }
    _Max.x = _Max.x;
    if (_Min.x > _Max.x)
        result.x = _Max.x;
    _Max.y = _Max.y;
    if (p.y > _Max.y)
        result.y = _Max.y;
    _Max.z = _Max.z;
    if (p.z > _Max.z)
        result.z = _Max.z;
    return result;
}

Bool BoundingBox::LineIntersection(Vector3 p1, Vector3 p2, U32 cflags, Vector3 &intersectionPoint, float& t)
{
    // Initial setup
    float closestT = 2.0f; // Default high value for closest intersection parameter
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float dz = p2.z - p1.z;
    
    // Check intersection with X planes
    if (cflags & 0xC) { // Minimum and maximum X planes
        float invDx = 1.0f / dx;
        
        if (cflags & 0x4) { // Minimum X plane
            float tX = (_Min.x - p1.x) * invDx;
            Vector3 point = { p1.x + tX * dx, p1.y + tX * dy, p1.z + tX * dz };
            
            if (point.y >= _Min.y && point.y <= _Max.y &&
                point.z >= _Min.z && point.z <= _Max.z &&
                tX < closestT)
            {
                closestT = tX;
                intersectionPoint = point;
            }
        }
        
        if (cflags & 0x8) { // Maximum X plane
            float tX = (_Max.x - p1.x) * invDx;
            Vector3 point = { p1.x + tX * dx, p1.y + tX * dy, p1.z + tX * dz };
            
            if (point.y >= _Min.y && point.y <= _Max.y &&
                point.z >= _Min.z && point.z <= _Max.z &&
                tX < closestT)
            {
                closestT = tX;
                intersectionPoint = point;
            }
        }
    }
    
    // Check intersection with Y planes
    if (cflags & 0x3) { // Minimum and maximum Y planes
        float invDy = 1.0f / dy;
        
        if (cflags & 0x2) { // Minimum Y plane
            float tY = (_Min.y - p1.y) * invDy;
            Vector3 point = { p1.x + tY * dx, p1.y + tY * dy, p1.z + tY * dz };
            
            if (point.x >= _Min.x && point.x <= _Max.x &&
                point.z >= _Min.z && point.z <= _Max.z &&
                tY < closestT)
            {
                closestT = tY;
                intersectionPoint = point;
            }
        }
        
        if (cflags & 0x1) { // Maximum Y plane
            float tY = (_Max.y - p1.y) * invDy;
            Vector3 point = { p1.x + tY * dx, p1.y + tY * dy, p1.z + tY * dz };
            
            if (point.x >= _Min.x && point.x <= _Max.x &&
                point.z >= _Min.z && point.z <= _Max.z &&
                tY < closestT)
            {
                closestT = tY;
                intersectionPoint = point;
            }
        }
    }
    
    // Check intersection with Z planes
    if (cflags & 0x30) { // Minimum and maximum Z planes
        float invDz = 1.0f / dz;
        
        if (cflags & 0x10) { // Minimum Z plane
            float tZ = (_Min.z - p1.z) * invDz;
            Vector3 point = { p1.x + tZ * dx, p1.y + tZ * dy, p1.z + tZ * dz };
            
            if (point.x >= _Min.x && point.x <= _Max.x &&
                point.y >= _Min.y && point.y <= _Max.y &&
                tZ < closestT)
            {
                closestT = tZ;
                intersectionPoint = point;
            }
        }
        
        if (cflags & 0x20) { // Maximum Z plane
            float tZ = (_Max.z - p1.z) * invDz;
            Vector3 point = { p1.x + tZ * dx, p1.y + tZ * dy, p1.z + tZ * dz };
            
            if (point.x >= _Min.x && point.x <= _Max.x &&
                point.y >= _Min.y && point.y <= _Max.y &&
                tZ < closestT)
            {
                closestT = tZ;
                intersectionPoint = point;
            }
        }
    }
    
    // Final result
    if (closestT == 2.0f) // No valid intersection
        return false;
    
    t = closestT; // Output the closest t value
    return true; // Intersection found
}

Bool BoundingBox::CollideWithLine(Vector3 p1, Vector3 p2, Vector3 &intersectionPoint,float& t)
{
    U32 cflags1 = 0; // Bitmask for p1's position relative to the bounding box
    U32 cflags2 = 0; // Bitmask for p2's position relative to the bounding box
    
    // Determine position of p1 relative to the bounding box
    if (p1.x < _Min.x)
        cflags1 |= kFace_Left;
    else if (p1.x > _Max.x)
        cflags1 |= kFace_Right;
    
    if (p1.y < _Min.y)
        cflags1 |= kFace_Bottom;
    else if (p1.y > _Max.y)
        cflags1 |= kFace_Top;
    
    if (p1.z < _Min.z)
        cflags1 |= kFace_Front;
    else if (p1.z > _Max.z)
        cflags1 |= kFace_Back;
    
    // If p1 is inside the bounding box
    if (cflags1 == 0) {
        t = 0.0f; // Intersection happens at the start of the segment
        return 1;
    }
    
    // Determine position of p2 relative to the bounding box
    if (p2.x < _Min.x)
        cflags2 |= kFace_Left;
    else if (p2.x > _Max.x)
        cflags2 |= kFace_Right;
    
    if (p2.y < _Min.y)
        cflags2 |= kFace_Bottom;
    else if (p2.y > _Max.y)
        cflags2 |= kFace_Top;
    
    if (p2.z < _Min.z)
        cflags2 |= kFace_Front;
    else if (p2.z > _Max.z)
        cflags2 |= kFace_Back;
    
    // If p2 is inside the bounding box
    if (cflags2 == 0) {
        t = 1.0f; // Intersection happens at the end of the segment
        return 1;
    }
    
    // If both points are outside but in the same region, no intersection
    if ((cflags1 & cflags2) != 0) {
        return 0; // No intersection
    }
    
    // Perform precise intersection test
    return LineIntersection(p1, p2, cflags1 ^ cflags2, intersectionPoint, t);
}

Sphere BoundingBox::GetEnclosingSphere()
{
    // Calculate the center of the sphere (midpoint of the bounding box)
    float centerX = (_Max.x + _Min.x) * 0.5f;
    float centerY = (_Max.y + _Min.y) * 0.5f;
    float centerZ = (_Max.z + _Min.z) * 0.5f;
    
    // Calculate the radius of the sphere (half the diagonal length of the bounding box)
    float deltaX = _Max.x - _Min.x;
    float deltaY = _Max.y - _Min.y;
    float deltaZ = _Max.z - _Min.z;
    
    // Calculate the diagonal squared (sum of squares of differences in each dimension)
    float diagonalSquared = (deltaX * deltaX) + (deltaY * deltaY) + (deltaZ * deltaZ);
    
    // The radius is half of the diagonal's length
    float radius = sqrtf(diagonalSquared) * 0.5f;
    
    // Create and return the enclosing sphere
    Sphere enclosingSphere;
    enclosingSphere._Center = Vector3(centerX, centerY, centerZ);
    enclosingSphere._Radius = radius;
    
    return enclosingSphere;
}

void BoundingBox::TranslateFace(BoundingBox::eFace face, Vector3 delta)
{
    switch ( face )
    {
        case kFace_Top:
            _Max.y = delta.y + _Max.y;
            break;
        case kFace_Bottom:
            _Min.y = delta.y + _Min.y;
            break;
        case kFace_Left:
            _Max.x = _Max.x + delta.x;
            break;
        case kFace_Right:
            _Min.x = delta.x + _Min.x;
            break;
        case kFace_Front:
            _Max.z = delta.z + _Max.z;
            break;
        case kFace_Back:
            _Min.z = delta.z + _Min.z;
            break;
        default:
            return;
    }
}

Vector3 BoundingBox::GetFaceCenter(eFace face)
{
    Vector3 result{};
    // Determine the position based on the face
    switch (face) {
        case kFace_Top: // Top face
            result.y = _Max.y;
            result.x = (_Max.x + _Min.x) * 0.5f;
            break;
            
        case kFace_Bottom: // Bottom face
            result.y = _Min.y;
            result.x = (_Max.x + _Min.x) * 0.5f;
            break;
            
        case kFace_Front: // Front face
            result.x = _Max.x;
            result.y = (_Max.y + _Min.y) * 0.5f;
            break;
            
        case kFace_Back: // Back face
            result.x = _Min.x;
            result.y = (_Max.y + _Min.y) * 0.5f;
            break;
            
        case kFace_Left: // Left face
            result.x = (_Max.x + _Min.x) * 0.5f;
            result.y = (_Max.y + _Min.y) * 0.5f;
            result.z = _Min.z;
            break;
            
        case kFace_Right: // Right face
            result.x = (_Max.x + _Min.x) * 0.5f;
            result.y = (_Max.y + _Min.y) * 0.5f;
            result.z = _Max.z;
            break;
            
        default:
            // Handle invalid face input (if any)
            result.x = result.y = result.z = 0.0f;
            break;
    }
    
    // Ensure the z-coordinate is always set to the center of the box if needed
    if (face != kFace_Left && face != kFace_Right)
    {
        result.z = (_Max.z + _Min.z) * 0.5f;
    }
    
    return result;
}

BoundingBox::eFace BoundingBox::HitFace(Vector3 hitPos)
{
    // Retrieve the x, y, and z components from mMin and mMax
    float minX = _Min.x;
    float minY = _Min.y;
    float minZ = _Min.z;
    float maxX = _Max.x;
    float maxY = _Max.y;
    float maxZ = _Max.z;
    
    // Tolerance value for comparison
    const float tolerance = 0.000001f;
    
    // Check if the hit position is on the front or back faces (z-axis)
    if (hitPos.z >= (maxZ - tolerance) && hitPos.z <= (maxZ + tolerance))
        return kFace_Back;  // Back face
    
    if (hitPos.z >= (minZ - tolerance) && hitPos.z <= (minZ + tolerance))
        return kFace_Front;  // Front face
    
    // Check if the hit position is on the left or right faces (x-axis)
    if (hitPos.x >= (maxX - tolerance) && hitPos.x <= (maxX + tolerance))
        return kFace_Right;  // Right face
    
    if (hitPos.x >= (minX - tolerance) && hitPos.x <= (minX + tolerance))
        return kFace_Left;  // Left face
    
    // Check if the hit position is on the top or bottom faces (y-axis)
    if (hitPos.y < (maxY - tolerance))
        return kFace_Bottom;  // Bottom face
    
    if (hitPos.y > (maxY + tolerance))
        return kFace_Top;  // Top face
    
    return kFace_None;  // Default case if the point is not on any face
}

BoundingBox::eFace BoundingBox::HitFace(Vector3 rayPos, Vector3 rayEnd, Vector3 &outHitPos)
{
    float t{};
    Bool collided = CollideWithLine(rayPos, rayPos + (rayEnd * 1000.f), outHitPos, t);
    if(!collided)
        return kFace_None; // no collision
    return HitFace(outHitPos);
}

BoundingBox BoundingBox::Merge(const BoundingBox &rhs)
{
    BoundingBox b{};
    // Update minimum coordinates (take smaller value)
    b._Min.x = MIN(_Min.x, rhs._Min.x);
    b._Min.y = MIN(_Min.y, rhs._Min.y);
    b._Min.z = MIN(_Min.z, rhs._Min.z);
    
    // Update maximum coordinates (take larger value)
    b._Max.x = MAX(_Max.x, rhs._Max.x);
    b._Max.y = MAX(_Max.y, rhs._Max.y);
    b._Max.z = MAX(_Max.z, rhs._Max.z);
    
    return b;
}

Bool Sphere::FullyContains(const Sphere& rhs)
{
    float rhsRadius = rhs._Radius;
    Bool result = true;
    
    if (rhsRadius != 0.0f)
    {
        float dx = _Center.x - rhs._Center.x;
        float dy = _Center.y - rhs._Center.y;
        float dz = _Center.z - rhs._Center.z;
        
        float distance = sqrtf(dx * dx + dy * dy + dz * dz) + rhsRadius;
        
        if (distance > (_Radius * 1.0001f))
        {
            result = false;
        }
    }
    
    return result;
}

Sphere Sphere::Merge(const Sphere& rhs)
{
    Sphere result{};
    float radius = this->_Radius;
    float dx = rhs._Center.x - _Center.x;
    float dy = rhs._Center.y - _Center.y;
    float dz = rhs._Center.z - _Center.z;
    
    float sqrtcache = sqrtf(dx * dx + dy * dy + dz * dz);
    
    if (radius == 0.0f || sqrtcache + radius <= rhs._Radius * 1.0001f)
    {
        return *this;
    }
    
    if (!FullyContains(rhs))
    {
        float distance = sqrtcache;
        float factor = ((rhs._Radius + distance - radius) * 0.5f) / distance;
        
        result._Center.x += factor * dx;
        result._Center.y += factor * dy;
        result._Center.z += factor * dz;
        
        result._Radius = (radius + distance + rhs._Radius) * 0.5f;
    }
    
    return result;
}

Sphere MakeForCone(Vector3 conePosition, Vector3 coneForward, float coneLength, float coneAngle)
{
    Sphere result{};
    
    // Calculate the cosine of half the cone angle
    float halfConeAngle = coneAngle * 0.5f;
    float cosHalfAngle = cosf(halfConeAngle);
    
    // Directional components of the cone's forward vector
    float coneDirX = coneForward.x;
    float coneDirY = coneForward.y;
    float coneDirZ = coneForward.z;
    
    // Determine the radius and center of the sphere
    float radius = 0.0f;
    float sphereCenterDistance = 0.0f;
    
    if (halfConeAngle <= 0.78539819f)  // If the half cone angle is <= 45 degrees (pi/4)
    {
        sphereCenterDistance = (0.5f / cosHalfAngle) * coneLength;
        radius = sphereCenterDistance;
    }
    else  // For wider cones (half angle > 45 degrees)
    {
        radius = sinf(halfConeAngle) * coneLength;
        sphereCenterDistance = cosHalfAngle * coneLength;
    }
    
    // Calculate the new center of the sphere
    result._Center.x = conePosition.x + coneDirX * sphereCenterDistance;
    result._Center.y = conePosition.y + coneDirY * sphereCenterDistance;
    result._Center.z = conePosition.z + coneDirZ * sphereCenterDistance;
    
    // Set the radius of the sphere
    result._Radius = radius;
    
    return result;
}

Bool Sphere::CollideWithCone(Vector3 &conePos, Vector3 &coneNorm, float coneRadius, float dotProduct)
{
    // Calculate the squared distance between the sphere's center and the cone's position
    float dx = _Center.x - conePos.x;
    float dy = _Center.y - conePos.y;
    float dz = _Center.z - conePos.z;
    float sphereRadius = _Radius;
    float distSquared = dx * dx + dy * dy + dz * dz;
    
    // If the distance between the sphere and the cone's position is greater than the sum of their radii, no collision
    if (distSquared > (sphereRadius + coneRadius) * (sphereRadius + coneRadius))
        return false;
    
    // If the distance is smaller than the sphere's radius, it's a collision
    if (distSquared < sphereRadius * sphereRadius)
        return true;
    
    // Compute the projection of the sphere's center onto the cone's normal
    float proj = dx * coneNorm.x + dy * coneNorm.y + dz * coneNorm.z;
    
    // If the projection is less than the sphere's radius, no collision
    if (proj < -sphereRadius)
        return false;
    
    // Calculate the vector from the sphere's center to the closest point on the cone's surface
    float closestX = dx - coneNorm.x * proj;
    float closestY = dy - coneNorm.y * proj;
    float closestZ = dz - coneNorm.z * proj;
    
    // Compute the distance from the sphere's center to the closest point on the cone's surface
    float closestDistSquared = closestX * closestX + closestY * closestY + closestZ * closestZ;
    
    // If the distance is very small (essentially zero), we set the distance to 1.0 to avoid dividing by zero
    float closestDist = (closestDistSquared < 1e-10f) ? 1.0f : sqrtf(closestDistSquared);
    
    // Normalize the distance and calculate the perpendicular distance from the sphere's center to the cone surface
    float inverseDist = 1.0f / closestDist;
    float cosAngle = sqrtf(1.0f - dotProduct * dotProduct);
    
    // If the closest point on the cone surface is closer than the radius, it's a collision
    if (closestDist <= (cosAngle / dotProduct) * proj)
        return true;
    
    // Project the closest point back onto the cone's surface to calculate the final distance
    float finalX = (inverseDist * closestX) * cosAngle + coneNorm.x * dotProduct;
    float finalY = (inverseDist * closestY) * cosAngle + coneNorm.y * dotProduct;
    float finalZ = (inverseDist * closestZ) * cosAngle + coneNorm.z * dotProduct;
    
    // Calculate the collision condition by comparing distances
    float penetration = MAX((finalX * dy + finalY * dx + finalZ * dz), 0.0f);
    if (penetration - coneRadius < 0.0f)
        coneRadius = penetration;
    
    // Finally, check if the sphere collides with the cone using the distance between their centers
    return (sphereRadius * sphereRadius) > ((dy - finalY * coneRadius) * (dy - finalY * coneRadius) +
                                            (dx - finalX * coneRadius) * (dx - finalX * coneRadius) +
                                            (dz - finalZ * coneRadius) * (dz - finalZ * coneRadius));
}

Quaternion Quaternion::Slerp(Quaternion q, const Quaternion& q0, float t)
{
    // Extract the components of the two quaternions and the interpolation factor
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float x0 = q0.x, y0 = q0.y, z0 = q0.z, w0 = q0.w;
    
    // Calculate the dot product between the quaternions
    float dot = (x * x0) + (y * y0) + (z * z0) + (w * w0);
    
    // Ensure the dot product is within a valid range for acos
    float t1 = 1.0f - t;
    
    Quaternion result;
    
    // If the dot product is less than 1, we need to compute the slerp
    if (dot < 0.999999f)
    {
        // Calculate the angle between the two quaternions
        float theta = acosf(dot);
        
        // Compute the sine of the angle
        float invSinTheta = 1.0f / sinf(theta);
        
        // Adjust the interpolation factors
        t1 = sinf(t1 * theta) * invSinTheta;
        t = sinf(t * theta) * invSinTheta;
    }
    
    // Perform the spherical linear interpolation
    result.x = (x0 * t1) + (x * t);
    result.y = (y0 * t1) + (y * t);
    result.z = (z0 * t1) + (z * t);
    result.w = (w0 * t1) + (w * t);
    
    return result;
}

Quaternion Quaternion::NLerp(const Quaternion& q0, const Quaternion& q, float t)
{
    // Extract components from the first quaternion (q0) and the second quaternion (q)
    float x0 = q0.x, y0 = q0.y, z0 = q0.z, w0 = q0.w;
    float x = q.x, y = q.y, z = q.z, w = q.w;
    
    // Compute the linear interpolation for each component
    Quaternion result;
    result.x = (x - x0) * t + x0;
    result.y = (y - y0) * t + y0;
    result.z = (z - z0) * t + z0;
    result.w = (w - w0) * t + w0;
    
    // Normalize the result quaternion
    result.Normalize();
    
    return result;
}

Quaternion Quaternion::BetweenVector3(const Vector3& va, const Vector3& vb)
{
    Quaternion res{};
    float dot = va.x * vb.x + va.y * vb.y + va.z * vb.z;
    
    if (dot < 1.0f)
    {
        // When the vectors are not perfectly aligned
        if (dot >= -0.99999899f)
        {
            // Calculate the axis of rotation
            float angle = sqrtf((dot + dot) + 2.0f);
            res.x = (vb.z * va.y - vb.y * va.z) / angle;
            res.y = (vb.x * va.z - vb.z * va.x) / angle;
            res.z = (vb.y * va.x - vb.x * va.y) / angle;
            res.w = angle * 0.5f;
            res.Normalize();
        }
        else
        {
            // Vectors are almost opposite, use an arbitrary perpendicular vector
            float v11 = va.y * Vector3::Left.z - va.z * Vector3::Left.y;
            float v12 = va.z * Vector3::Left.x - va.x * Vector3::Left.z;
            float v13 = va.x * Vector3::Left.y - va.y * Vector3::Left.x;
            
            float len = v11 * v11 + v12 * v12 + v13 * v13;
            float invLength = len < 1e-10f ? 1.0f : 1.0f / sqrtf(len);
            
            res.x = v11 * invLength;
            res.y = v12 * invLength;
            res.z = v13 * invLength;
            res.w = -4.37114e-08f;
            res.Normalize();
        }
    }
    else
    {
        // Vectors are already aligned, no rotation needed
        res = kIdentity;
    }
    
    return res;
}

Vector3 operator*(const Vector3& vec, const Quaternion& quat)
{
    // Extract quaternion components
    float qx = quat.x, qy = quat.y, qz = quat.z, qw = quat.w;
    
    // Compute vector quaternion components
    float vx = vec.x, vy = vec.y, vz = vec.z;
    
    // Calculate intermediary products
    float t2 = qw * vx + qy * vz - qz * vy;
    float t3 = qw * vy + qz * vx - qx * vz;
    float t4 = qw * vz + qx * vy - qy * vx;
    float t5 = -qx * vx - qy * vy - qz * vz;
    
    // Final rotated vector components
    float resultX = t2 * qw + t5 * -qx + t3 * -qz - t4 * -qy;
    float resultY = t3 * qw + t5 * -qy + t4 * -qx - t2 * -qz;
    float resultZ = t4 * qw + t5 * -qz + t2 * -qy - t3 * -qx;
    
    return Vector3(resultX, resultY, resultZ);
}

Matrix4 MatrixRotation(const Quaternion& q) {
    Matrix4 ret{};
    
    // Precomputed terms for efficiency
    float twoQx = q.x + q.x;
    float twoQy = q.y + q.y;
    float twoQz = q.z + q.z;
    
    float twoQxQy = q.x * twoQy;
    float twoQxQz = q.x * twoQz;
    float twoQxQw = q.w * twoQx;
    
    float twoQyQz = q.y * twoQz;
    float twoQyQw = q.w * twoQy;
    
    float twoQzQw = q.w * twoQz;
    
    float twoQxSq = q.x * twoQx;
    float twoQySq = q.y * twoQy;
    float twoQzSq = q.z * twoQz;
    
    // Fill in the rotation matrix
    ret._Entries[0][0] = 1.0f - twoQySq - twoQzSq;
    ret._Entries[0][1] = twoQxQy + twoQzQw;
    ret._Entries[0][2] = twoQxQz - twoQyQw;
    ret._Entries[0][3] = 0.0f;
    
    ret._Entries[1][0] = twoQxQy - twoQzQw;
    ret._Entries[1][1] = 1.0f - twoQxSq - twoQzSq;
    ret._Entries[1][2] = twoQyQz + twoQxQw;
    ret._Entries[1][3] = 0.0f;
    
    ret._Entries[2][0] = twoQxQz + twoQyQw;
    ret._Entries[2][1] = twoQyQz - twoQxQw;
    ret._Entries[2][2] = 1.0f - twoQxSq - twoQySq;
    ret._Entries[2][3] = 0.0f;
    
    ret._Entries[3][0] = 0.0f;
    ret._Entries[3][1] = 0.0f;
    ret._Entries[3][2] = 0.0f;
    ret._Entries[3][3] = 1.0f;
    
    return ret;
}


Matrix4 MatrixTranslation(const Vector3& Translation)
{
    Matrix4 ret{};
    ret._Entries[3][0] = Translation.x;
    ret._Entries[3][1] = Translation.y;
    ret._Entries[3][2] = Translation.z;
    ret._Entries[3][3] = 1.0f;
    return ret;
}

Matrix4 MatrixTransformation(const Quaternion& rot, const Vector3& Translation)
{
    Matrix4 ret = MatrixRotation(rot);
    ret._Entries[3][0] += Translation.x;
    ret._Entries[3][1] += Translation.y;
    ret._Entries[3][2] += Translation.z;
    return ret;
}

Matrix4 MatrixTransformation(const Vector3 scale, const Quaternion& rot, const Vector3& Translation)
{
    Matrix4 ret = MatrixTransformation(rot, Translation);
    return ret * Matrix4::Scaling(scale);
}

Matrix4 MatrixScaling(float ScaleX, float ScaleY, float ScaleZ)
{
    Matrix4 ret = Matrix4::Identity();
    ret._Entries[0][0] = ScaleX;
    ret._Entries[1][1] = ScaleY;
    ret._Entries[2][2] = ScaleZ;
    return ret;
}

Matrix4 MatrixRotationYawPitchRollDegrees(float yaw, float pitch, float roll)
{
    return Matrix4::RotationX(roll) * Matrix4::RotationY(pitch) * Matrix4::RotationZ(yaw);
}

Bool Frustum::Visible(const BoundingBox& box, bool* pbStraddlesNearPlane) {
    
    // corners
    Vector3 c[8] = {
        box._Min,
        {box._Min.x, box._Min.y, box._Max.z},
        {box._Min.x, box._Max.y, box._Max.z},
        {box._Min.x, box._Max.y, box._Min.z},
        {box._Max.x, box._Min.y, box._Min.z},
        {box._Max.x, box._Min.y, box._Max.z},
        {box._Max.x, box._Max.y, box._Max.z},
        {box._Max.x, box._Max.y, box._Min.z}
    };
    
    // if we want to determine if it straddles near any of the planes
    if (pbStraddlesNearPlane) {
        if (InternalVisibleTest(c)) {
            bool straddles = false;
            for (int i = 0; i < 8; i++) {
                if ((Vector3::Dot(Vector3(_Plane[0]._Plane), c[i]) + _Plane[0]._Plane.w) <= 0.0f) {
                    straddles = true;
                    break;
                }
            }
            *pbStraddlesNearPlane = straddles;
            return true;
        }
        *pbStraddlesNearPlane = false;
        return false;
    }
    
    return InternalVisibleTest(c);
}


/*
 Matrix4.cpp
 Written by Matthew Fisher
 */

Matrix4::Matrix4()
{
    
}

Matrix4::Matrix4(const Matrix4& M)
{
    for (U32 Row = 0; Row < 4; Row++)
    {
        for (U32 Col = 0; Col < 4; Col++)
        {
            _Entries[Row][Col] = M[Row][Col];
        }
    }
}

Matrix4::Matrix4(const Vector3& V0, const Vector3& V1, const Vector3& V2)
{
    _Entries[0][0] = V0.x;
    _Entries[0][1] = V0.y;
    _Entries[0][2] = V0.z;
    _Entries[0][3] = 0.0f;
    
    _Entries[1][0] = V1.x;
    _Entries[1][1] = V1.y;
    _Entries[1][2] = V1.z;
    _Entries[1][3] = 0.0f;
    
    _Entries[2][0] = V2.x;
    _Entries[2][1] = V2.y;
    _Entries[2][2] = V2.z;
    _Entries[2][3] = 0.0f;
    
    _Entries[3][0] = 0.0f;
    _Entries[3][1] = 0.0f;
    _Entries[3][2] = 0.0f;
    _Entries[3][3] = 1.0f;
}

Matrix4& Matrix4::operator = (const Matrix4& M)
{
    for (U32 Row = 0; Row < 4; Row++)
    {
        for (U32 Col = 0; Col < 4; Col++)
        {
            _Entries[Row][Col] = M[Row][Col];
        }
    }
    return (*this);
}

Matrix4 Matrix4::Inverse() const
{
    //
    // Inversion by Cramer's rule.  Code taken from an Intel publication
    //
    double Result[4][4];
    double tmp[12]; /* temp array for pairs */
    double src[16]; /* array of transpose source matrix */
    double det; /* determinant */
    /* transpose matrix */
    for (U32 i = 0; i < 4; i++)
    {
        src[i + 0] = (*this)[i][0];
        src[i + 4] = (*this)[i][1];
        src[i + 8] = (*this)[i][2];
        src[i + 12] = (*this)[i][3];
    }
    /* calculate pairs for first 8 elements (cofactors) */
    tmp[0] = src[10] * src[15];
    tmp[1] = src[11] * src[14];
    tmp[2] = src[9] * src[15];
    tmp[3] = src[11] * src[13];
    tmp[4] = src[9] * src[14];
    tmp[5] = src[10] * src[13];
    tmp[6] = src[8] * src[15];
    tmp[7] = src[11] * src[12];
    tmp[8] = src[8] * src[14];
    tmp[9] = src[10] * src[12];
    tmp[10] = src[8] * src[13];
    tmp[11] = src[9] * src[12];
    /* calculate first 8 elements (cofactors) */
    Result[0][0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7];
    Result[0][0] -= tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7];
    Result[0][1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7];
    Result[0][1] -= tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7];
    Result[0][2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7];
    Result[0][2] -= tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7];
    Result[0][3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6];
    Result[0][3] -= tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6];
    Result[1][0] = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3];
    Result[1][0] -= tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3];
    Result[1][1] = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3];
    Result[1][1] -= tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3];
    Result[1][2] = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3];
    Result[1][2] -= tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3];
    Result[1][3] = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2];
    Result[1][3] -= tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2];
    /* calculate pairs for second 8 elements (cofactors) */
    tmp[0] = src[2] * src[7];
    tmp[1] = src[3] * src[6];
    tmp[2] = src[1] * src[7];
    tmp[3] = src[3] * src[5];
    tmp[4] = src[1] * src[6];
    tmp[5] = src[2] * src[5];
    
    tmp[6] = src[0] * src[7];
    tmp[7] = src[3] * src[4];
    tmp[8] = src[0] * src[6];
    tmp[9] = src[2] * src[4];
    tmp[10] = src[0] * src[5];
    tmp[11] = src[1] * src[4];
    /* calculate second 8 elements (cofactors) */
    Result[2][0] = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15];
    Result[2][0] -= tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15];
    Result[2][1] = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15];
    Result[2][1] -= tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15];
    Result[2][2] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15];
    Result[2][2] -= tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15];
    Result[2][3] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14];
    Result[2][3] -= tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14];
    Result[3][0] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9];
    Result[3][0] -= tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10];
    Result[3][1] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10];
    Result[3][1] -= tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8];
    Result[3][2] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8];
    Result[3][2] -= tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9];
    Result[3][3] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9];
    Result[3][3] -= tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8];
    /* calculate determinant */
    det = src[0] * Result[0][0] + src[1] * Result[0][1] + src[2] * Result[0][2] + src[3] * Result[0][3];
    /* calculate matrix inverse */
    det = 1.0f / det;
    
    Matrix4 FloatResult;
    for (U32 i = 0; i < 4; i++)
    {
        for (U32 j = 0; j < 4; j++)
        {
            FloatResult[i][j] = float(Result[i][j] * det);
        }
    }
    return FloatResult;
}

Matrix4 Matrix4::Transpose() const
{
    Matrix4 Result;
    for (U32 i = 0; i < 4; i++)
    {
        for (U32 i2 = 0; i2 < 4; i2++)
        {
            Result[i2][i] = _Entries[i][i2];
        }
    }
    return Result;
}

Matrix4 Matrix4::Identity()
{
    Matrix4 Result;
    for (U32 i = 0; i < 4; i++)
    {
        for (U32 i2 = 0; i2 < 4; i2++)
        {
            if (i == i2)
            {
                Result[i][i2] = 1.0f;
            }
            else
            {
                Result[i][i2] = 0.0f;
            }
        }
    }
    return Result;
}

Matrix4 Matrix4::Rotation(const Vector3& _Basis1, const Vector3& _Basis2, const Vector3& _Basis3)
{
    //
    // Verify everything is normalized
    //
    Vector3 Basis1 = _Basis1;
    Vector3 Basis2 = _Basis2;
    Vector3 Basis3 = _Basis3;
    Basis1.Normalize();
    Basis2.Normalize();
    Basis3.Normalize();
    
    Matrix4 Result;
    Result[0][0] = Basis1.x;
    Result[1][0] = Basis1.y;
    Result[2][0] = Basis1.z;
    Result[3][0] = 0.0f;
    
    Result[0][1] = Basis2.x;
    Result[1][1] = Basis2.y;
    Result[2][1] = Basis2.z;
    Result[3][1] = 0.0f;
    
    Result[0][2] = Basis3.x;
    Result[1][2] = Basis3.y;
    Result[2][2] = Basis3.z;
    Result[3][2] = 0.0f;
    
    Result[0][3] = 0.0f;
    Result[1][3] = 0.0f;
    Result[2][3] = 0.0f;
    Result[3][3] = 1.0f;
    return Result;
}

Matrix4 Matrix4::Camera(const Vector3& Eye, const Vector3& _Look, const Vector3& _Up, const Vector3& _Right)
{
    //
    // Verify everything is normalized
    //
    Vector3 Look = Vector3::Normalize(_Look);
    Vector3 Up = Vector3::Normalize(_Up);
    Vector3 Right = Vector3::Normalize(_Right);
    
    Matrix4 Result;
    Result[0][0] = Right.x;
    Result[1][0] = Right.y;
    Result[2][0] = Right.z;
    Result[3][0] = -Vector3::Dot(Right, Eye);
    
    Result[0][1] = Up.x;
    Result[1][1] = Up.y;
    Result[2][1] = Up.z;
    Result[3][1] = -Vector3::Dot(Up, Eye);
    
    Result[0][2] = Look.x;
    Result[1][2] = Look.y;
    Result[2][2] = Look.z;
    Result[3][2] = -Vector3::Dot(Look, Eye);
    
    Result[0][3] = 0.0f;
    Result[1][3] = 0.0f;
    Result[2][3] = 0.0f;
    Result[3][3] = 1.0f;
    return Result;
}

Matrix4 Matrix4::LookAt(const Vector3& Eye, const Vector3& At, const Vector3& Up)
{
    Vector3 XAxis, YAxis, ZAxis;
    ZAxis = Vector3::Normalize(Eye - At);
    XAxis = Vector3::Normalize(Vector3::Cross(Up, ZAxis));
    YAxis = Vector3::Normalize(Vector3::Cross(ZAxis, XAxis));
    
    Matrix4 Result;
    Result[0][0] = XAxis.x;
    Result[1][0] = XAxis.y;
    Result[2][0] = XAxis.z;
    Result[3][0] = -Vector3::Dot(XAxis, Eye);
    
    Result[0][1] = YAxis.x;
    Result[1][1] = YAxis.y;
    Result[2][1] = YAxis.z;
    Result[3][1] = -Vector3::Dot(YAxis, Eye);
    
    Result[0][2] = ZAxis.x;
    Result[1][2] = ZAxis.y;
    Result[2][2] = ZAxis.z;
    Result[3][2] = -Vector3::Dot(ZAxis, Eye);
    
    Result[0][3] = 0.0f;
    Result[1][3] = 0.0f;
    Result[2][3] = 0.0f;
    Result[3][3] = 1.0f;
    return Result;
}

Matrix4 Matrix4::OrthogonalOffCenter(float left, float right, float bottom, float top, float ZNear, float ZFar)
{
    float invRL = 1.0f / (right - left);
    float invTB = 1.0f / (top - bottom);
    float invFN = 1.0f / (ZFar - ZNear);
    Matrix4 Result;
    Result[0][0] = 2.0f * invRL;
    Result[1][0] = 0.0f;
    Result[2][0] = 0.0f;
    Result[3][0] = -(right + left) * invRL;;
    
    Result[0][1] = 0.0f;
    Result[1][1] = 2.0f * invTB;
    Result[2][1] = 0.0f;
    Result[3][1] = -(top + bottom) * invTB;;
    
    Result[0][2] = 0.0f;
    Result[1][2] = 0.0f;
    Result[2][2] = -2.0f * invFN;
    Result[3][2] = -(ZFar + ZNear) * invFN;
    
    Result[0][3] = 0.0f;
    Result[1][3] = 0.0f;
    Result[2][3] = 0.0f;
    Result[3][3] = 1.0f;
    return Result;
}

Matrix4 Matrix4::Orthogonal(float Width, float Height, float ZNear, float ZFar)
{
    Matrix4 Result;
    Result[0][0] = 2.0f / Width;
    Result[1][0] = 0.0f;
    Result[2][0] = 0.0f;
    Result[3][0] = 0.0f;
    
    Result[0][1] = 0.0f;
    Result[1][1] = 2.0f / Height;
    Result[2][1] = 0.0f;
    Result[3][1] = 0.0f;
    
    Result[0][2] = 0.0f;
    Result[1][2] = 0.0f;
    Result[2][2] = 1.0f / (ZNear - ZFar);
    Result[3][2] = ZNear / (ZNear - ZFar);
    
    Result[0][3] = 0.0f;
    Result[1][3] = 0.0f;
    Result[2][3] = 0.0f;
    Result[3][3] = 1.0f;
    return Result;
}

Matrix4 Matrix4::Perspective(float Width, float Height, float ZNear, float ZFar)
{
    Matrix4 Result;
    Result[0][0] = 2.0f * ZNear / Width;
    Result[1][0] = 0.0f;
    Result[2][0] = 0.0f;
    Result[3][0] = 0.0f;
    
    Result[0][1] = 0.0f;
    Result[1][1] = 2.0f * ZNear / Height;
    Result[2][1] = 0.0f;
    Result[3][1] = 0.0f;
    
    Result[0][2] = 0.0f;
    Result[1][2] = 0.0f;
    Result[2][2] = ZFar / (ZNear - ZFar);
    Result[3][2] = ZFar * ZNear / (ZNear - ZFar);
    
    Result[0][3] = 0.0f;
    Result[1][3] = 0.0f;
    Result[2][3] = -1.0f;
    Result[3][3] = 0.0f;
    return Result;
}

Matrix4 Matrix4::PerspectiveFov(float FOV, float Aspect, float ZNear, float ZFar)
{
    float Width = 1.0f / tanf(FOV / 2.0f), Height = Aspect / tanf(FOV / 2.0f);
    
    Matrix4 Result;
    Result[0][0] = Width;
    Result[1][0] = 0.0f;
    Result[2][0] = 0.0f;
    Result[3][0] = 0.0f;
    
    Result[0][1] = 0.0f;
    Result[1][1] = Height;
    Result[2][1] = 0.0f;
    Result[3][1] = 0.0f;
    
    Result[0][2] = 0.0f;
    Result[1][2] = 0.0f;
    Result[2][2] = ZFar / (ZNear - ZFar);
    Result[3][2] = ZFar * ZNear / (ZNear - ZFar);
    
    Result[0][3] = 0.0f;
    Result[1][3] = 0.0f;
    Result[2][3] = -1.0f;
    Result[3][3] = 0.0f;
    return Result;
}

Matrix4 Matrix4::PerspectiveMultiFov(float FovX, float FovY, float ZNear, float ZFar)
{
    float Width = 1.0f / tanf(FovX / 2.0f), Height = 1.0f / tanf(FovY / 2.0f);
    
    Matrix4 Result;
    Result[0][0] = Width;
    Result[1][0] = 0.0f;
    Result[2][0] = 0.0f;
    Result[3][0] = 0.0f;
    
    Result[0][1] = 0.0f;
    Result[1][1] = Height;
    Result[2][1] = 0.0f;
    Result[3][1] = 0.0f;
    
    Result[0][2] = 0.0f;
    Result[1][2] = 0.0f;
    Result[2][2] = ZFar / (ZNear - ZFar);
    Result[3][2] = ZFar * ZNear / (ZNear - ZFar);
    
    Result[0][3] = 0.0f;
    Result[1][3] = 0.0f;
    Result[2][3] = -1.0f;
    Result[3][3] = 0.0f;
    return Result;
}

Matrix4 Matrix4::Rotation(const Vector3& Axis, float Angle)
{
    float c = cosf(Angle);
    float s = sinf(Angle);
    float t = 1.0f - c;
    
    Vector3 NormalizedAxis = Vector3::Normalize(Axis);
    float x = NormalizedAxis.x;
    float y = NormalizedAxis.y;
    float z = NormalizedAxis.z;
    
    Matrix4 Result;
    Result[0][0] = 1 + t * (x * x - 1);
    Result[0][1] = z * s + t * x * y;
    Result[0][2] = -y * s + t * x * z;
    Result[0][3] = 0.0f;
    
    Result[1][0] = -z * s + t * x * y;
    Result[1][1] = 1 + t * (y * y - 1);
    Result[1][2] = x * s + t * y * z;
    Result[1][3] = 0.0f;
    
    Result[2][0] = y * s + t * x * z;
    Result[2][1] = -x * s + t * y * z;
    Result[2][2] = 1 + t * (z * z - 1);
    Result[2][3] = 0.0f;
    
    Result[3][0] = 0.0f;
    Result[3][1] = 0.0f;
    Result[3][2] = 0.0f;
    Result[3][3] = 1.0f;
    return Result;
}

Matrix4 Matrix4::Rotation(float Yaw, float Pitch, float Roll)
{
    return RotationY(Yaw) * RotationX(Pitch) * RotationZ(Roll);
}

Matrix4 Matrix4::Rotation(const Vector3& Axis, float Angle, const Vector3& Center)
{
    return Translation(-Center) * Rotation(Axis, Angle) * Translation(Center);
}

Matrix4 Matrix4::RotationX(float Theta)
{
    float CosT = cosf(Theta);
    float SinT = sinf(Theta);
    
    Matrix4 Result = Identity();
    Result[1][1] = CosT;
    Result[1][2] = SinT;
    Result[2][1] = -SinT;
    Result[2][2] = CosT;
    return Result;
}

Matrix4 Matrix4::RotationY(float Theta)
{
    float CosT = cosf(Theta);
    float SinT = sinf(Theta);
    
    Matrix4 Result = Identity();
    Result[0][0] = CosT;
    Result[0][2] = SinT;
    Result[2][0] = -SinT;
    Result[2][2] = CosT;
    return Result;
}

Matrix4 Matrix4::RotationZ(float Theta)
{
    float CosT = cosf(Theta);
    float SinT = sinf(Theta);
    
    Matrix4 Result = Identity();
    Result[0][0] = CosT;
    Result[0][1] = SinT;
    Result[1][0] = -SinT;
    Result[1][1] = CosT;
    return Result;
}

Matrix4 Matrix4::Scaling(const Vector3& ScaleFactors)
{
    Matrix4 Result;
    Result[0][0] = ScaleFactors.x;
    Result[1][0] = 0.0f;
    Result[2][0] = 0.0f;
    Result[3][0] = 0.0f;
    
    Result[0][1] = 0.0f;
    Result[1][1] = ScaleFactors.y;
    Result[2][1] = 0.0f;
    Result[3][1] = 0.0f;
    
    Result[0][2] = 0.0f;
    Result[1][2] = 0.0f;
    Result[2][2] = ScaleFactors.z;
    Result[3][2] = 0.0f;
    
    Result[0][3] = 0.0f;
    Result[1][3] = 0.0f;
    Result[2][3] = 0.0f;
    Result[3][3] = 1.0f;
    return Result;
}

Matrix4 Matrix4::Translation(const Vector3& Pos)
{
    Matrix4 Result;
    Result[0][0] = 1.0f;
    Result[1][0] = 0.0f;
    Result[2][0] = 0.0f;
    Result[3][0] = Pos.x;
    
    Result[0][1] = 0.0f;
    Result[1][1] = 1.0f;
    Result[2][1] = 0.0f;
    Result[3][1] = Pos.y;
    
    Result[0][2] = 0.0f;
    Result[1][2] = 0.0f;
    Result[2][2] = 1.0f;
    Result[3][2] = Pos.z;
    
    Result[0][3] = 0.0f;
    Result[1][3] = 0.0f;
    Result[2][3] = 0.0f;
    Result[3][3] = 1.0f;
    return Result;
}

Matrix4 Matrix4::ChangeOfBasis(const Vector3& Source0, const Vector3& Source1, const Vector3& Source2, const Vector3& SourceOrigin,
                               const Vector3& Target0, const Vector3& Target1, const Vector3& Target2, const Vector3& TargetOrigin)
{
    Matrix4 RotationComponent = Matrix4(Source0, Source1, Source2).Inverse() * Matrix4(Target0, Target1, Target2);
    //Matrix4 TranslationComponent = Translation(TargetOrigin - SourceOrigin);
    Matrix4 Result = Translation(-SourceOrigin) * RotationComponent * Translation(TargetOrigin);
    return Result;
    //return Translation(TargetOrigin - SourceOrigin);
}

Matrix4 Matrix4::Face(const Vector3& V0, const Vector3& V1)
{
    //
    // Rotate about the cross product of the two vectors by the angle between the two vectors
    //
    Vector3 Axis = Vector3::Cross(V0, V1);
    float Angle = Vector3::AngleBetween(V0, V1);
    
    if (Angle == 0.0f)
    {
        return Identity();
    }
    else if (Axis.MagnitudeSquared() <= 0.00001f)
    {
        Vector3 basis0, basis1;
        Vector3::CompleteOrthonormalBasis(V0, basis0, basis1);
        return Rotation(basis0, Angle);
    }
    else
    {
        return Rotation(Axis, Angle);
    }
}

Matrix4 Matrix4::Viewport(float Width, float Height)
{
    return Matrix4::Scaling(Vector3(Width * 0.5f, -Height * 0.5f, 1.0f)) * Matrix4::Translation(Vector3(Width * 0.5f, Height * 0.5f, 0.0f));
}

float Matrix4::CompareMatrices(const Matrix4& Left, const Matrix4& Right)
{
    float Sum = 0.0f;
    for (U32 i = 0; i < 4; i++)
    {
        for (U32 i2 = 0; i2 < 4; i2++)
        {
            Sum += fabsf(Left[i][i2] - Right[i][i2]);
        }
    }
    return Sum / 16.0f;
}

Matrix4 operator * (const Matrix4& Left, const Matrix4& Right)
{
    Matrix4 Result;
    for (U32 i = 0; i < 4; i++)
    {
        for (U32 i2 = 0; i2 < 4; i2++)
        {
            float Total = 0.0f;
            for (U32 i3 = 0; i3 < 4; i3++)
            {
                Total += Left[i][i3] * Right[i3][i2];
            }
            Result[i][i2] = Total;
        }
    }
    return Result;
}

Matrix4 operator * (const Matrix4& Left, float& Right)
{
    Matrix4 Result;
    for (U32 i = 0; i < 4; i++)
    {
        for (U32 i2 = 0; i2 < 4; i2++)
        {
            Result[i][i2] = Left[i][i2] * Right;
        }
    }
    return Result;
}

Matrix4 operator * (float& Left, const Matrix4& Right)
{
    Matrix4 Result;
    for (U32 i = 0; i < 4; i++)
    {
        for (U32 i2 = 0; i2 < 4; i2++)
        {
            Result[i][i2] = Right[i][i2] * Left;
        }
    }
    return Result;
}

Matrix4 operator + (const Matrix4& Left, const Matrix4& Right)
{
    Matrix4 Result;
    for (U32 i = 0; i < 4; i++)
    {
        for (U32 i2 = 0; i2 < 4; i2++)
        {
            Result[i][i2] = Left[i][i2] + Right[i][i2];
        }
    }
    return Result;
}

Matrix4 operator - (const Matrix4& Left, const Matrix4& Right)
{
    Matrix4 Result;
    for (U32 i = 0; i < 4; i++)
    {
        for (U32 i2 = 0; i2 < 4; i2++)
        {
            Result[i][i2] = Left[i][i2] - Right[i][i2];
        }
    }
    return Result;
}
