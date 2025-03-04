
#pragma once

#include <Core/Math.hpp>
#include <Core/Symbol.hpp>

#include <set>

//You can define this to 1 if you want inverted depth in the camera (if _BAllowInvertedDepth)
#ifndef INVERTED_DEPTH
#define INVERTED_DEPTH 0
#endif

// Telltale CAMERA CLASS implementation

/// A camera. Screen width, height, aspect ratio and agent transform should be updated manually to the camera agent location information and screen info. Ensure if the screen size changes
/// then you update the aspect ratio and screen w/h and also call on screen modified to mark it as dirty. When the agent transform, ie camera moves, call on agent transform modified.
class Camera
{
public:
	
    Transform _AgentTransform;
    //YOU MUST UPDATE THIS YOURSELF! THEN CALL OnScreenModified!
    U32 _ScreenWidth;
    U32 _ScreenHeight;
    
private:
    
	float _AspectRatio;
    Bool _BCullObjects;
    Bool _BPushed;
    Bool _BIsViewCamera;
    Bool _BAllowInvertedDepth;
    Bool _BAllowBlending;
    Bool _BShouldUpdateBlendDestination;
    Frustum _CachedFrustum;
    Matrix4 _CachedWorldMatrix;
    Matrix4 _CachedViewMatrix;
    Matrix4 _CachedProjectionMatrix;
    Bool _BWorldTransformDirty;
    Bool _BFrustumDirty;
    Bool _BViewMatrixDirty;
    Bool _BProjectionMatrixDirty;
    
public:
    
    Symbol mName;
    std::set<Symbol> _ExcludeAgents;
    float _HFOV;
    float _HFOVScale;
    float _FocalLength;
    float _NearClip;
    float _FarClip;
    float _OrthoLeft;
    float _OrthoRight;
    float _OrthoBottom;
    float _OrthoTop;
    float _OrthoNear;
    float _OrthoFar;
    Colour _FXColour;
    float _FXColourOpacity;
    float _Exposure;
    float _FXLevelsBlack;
    float _FXLevelsWhite;
    float _FXLevelsIntensity;
    float _FXRadialBlurInnerRadius;
    float _FXRadialBlurOuterRadius;
    float _FXRadialBlurIntensity;
    Colour _FXRadialBlurTint;
    float _FXRadialBlurTintIntensity;
    float _FXRadialBlurScale;
    float _FXMotionBlurStrength;
    float _FXMotionBlurThreshold;
    float _FXMotionBlurRotationThreshold;
    float _DOFNear;
    float _DOFFar;
    float _DOFNearRamp;
    float _DOFFarRamp;
    float _DOFNearMax;
    float _DOFFarMax;
    float _DOFDebug;
    float _DOFCoverageBoost;
    Symbol _hBokehTexture;
    float _BokehBrightnessDeltaThreshold;
    float _BokehBrightnessThreshold;
    float _BokehBlurThreshold;
    float _BokehMinSize;
    float _BokehMaxSize;
    float _BokehFalloff;
    float _MaxBokehBufferVertexAmount;
    Vector3 _BokehAberrationOffsetsX;
    Vector3 _BokehAberrationOffsetsY;
    
    Bool _FXMotionBlurActive;
    Bool _FXMotionBlurThresholdActive;
    Bool _FXMotionBlurThresholdRotActive;
    Bool _MotionBlurDelay;
    Bool _BDOFEnabled;
    Bool _UseHQDOF;
    Bool _BBrushDOFEnabled;
    Bool _UseBokeh;
    Bool _BIsOrthoCamera;
    Bool _FXColourActive;
    Bool _FXLevelsActive;
    Bool _FXRadialBlurActive;
    
    /*
     Sy_Bol mAudioListenerOverrideAgentName;
     Sy_Bol mAudioPlayerOriginOverrideAgentName;
     WeakPtr<Agent> mpAudioListenerOverrideAgent;
     WeakPtr<Agent> mpAudioPlayerOriginOverrideAgent;
     Handle<SoundReverbDefinition> _HAudioReverbOverride;
     SoundEventName<1> mtReverbSnapshotOverride;
     bool _BInLensCallback;*/
    
	// Gets the world matrix. INTERNAL
	inline Matrix4& GetWorldMatrix()
	{
		if (_BWorldTransformDirty)
			_UpdateCachedTransform();
		return _CachedWorldMatrix;
	}
	
public:
    
    // Constructor to defaults
    inline Camera()
    {
        _CachedProjectionMatrix = _CachedViewMatrix = _CachedWorldMatrix = Matrix4::Identity();
        _BWorldTransformDirty = false;
        _BViewMatrixDirty = _BFrustumDirty = _BProjectionMatrixDirty = true;
        _HFOVScale = 1.0f;
        _HFOV = 1.0f;
        _NearClip = _FarClip = -1.f;
        _BIsOrthoCamera = false;
        _OrthoLeft = _OrthoBottom = _OrthoNear = 0.f;
        _OrthoTop = _OrthoRight = _OrthoFar = 1.0f;
        _FXColour = Colour::Black;
        _FXLevelsActive = _FXColourActive = _FXRadialBlurActive = false;
        _Exposure = _FXColourOpacity = 0.f;
        _FXLevelsBlack = _FXLevelsWhite = 0.f;
        _FXLevelsIntensity = 0.f;
        _FXRadialBlurInnerRadius = 0.f;
        _FXRadialBlurOuterRadius = 1.f;
        _FXRadialBlurIntensity = 0.f;
        _FXRadialBlurTint = Colour::Black;
        _FXRadialBlurTintIntensity = 0.f;
        _FXMotionBlurActive = _FXMotionBlurThresholdActive = _FXMotionBlurThresholdRotActive = _MotionBlurDelay = false;
        _FXMotionBlurStrength = _FXMotionBlurRotationThreshold = _FXMotionBlurRotationThreshold = 0.f;
        _DOFNear = 2.0f;
        _DOFFar = 4.0f;
        _DOFNearRamp = 1.0f;
        _DOFFarRamp = 4.0f;
        _DOFNearMax = 1.0f;
        _DOFDebug = 0.f;
        _DOFCoverageBoost = 0.f;
        _BDOFEnabled = false;
        _UseHQDOF = true;
        _BBrushDOFEnabled = false;
        _UseBokeh = false;
        _BokehBrightnessDeltaThreshold = 0.075f;
        _BokehBrightnessThreshold = _BokehBlurThreshold = 0.25f;
        _BokehMinSize = 0.005f;
        _BokehMaxSize = 0.025f;
        _BokehFalloff = 0.7f;
        _MaxBokehBufferVertexAmount = 0.f;
        _BokehAberrationOffsetsX = _BokehAberrationOffsetsY = Vector3();
    }
    
    // Tests if an agent name is excluded
    inline bool IsAgentExcluded(Symbol agent)
    {
        return _ExcludeAgents.find(agent) != _ExcludeAgents.cend();
    }
    
    // Returns the relative up vector in this camera
    inline Vector3 Up()
    {
        if (_BWorldTransformDirty)
            _UpdateCachedTransform();
        return _CachedWorldMatrix.TransformNormal(Vector3::Up);
    }
    
    // Returns the relative forward vector in this camera
    inline Vector3 Forward()
    {
        if (_BWorldTransformDirty)
            _UpdateCachedTransform();
        return _CachedWorldMatrix.TransformNormal(Vector3::Forward);
    }
    
    // Computes the local view matrix
    inline Matrix4 ComputeLocalViewMatrix()
    {
        return Matrix4::LookAt({ 0.f,0.f,0.f }, Forward(), Up());
    }
    
    // Computes the project matrix, not inverted even if invert depth is set.
    inline Matrix4 ComputeNonInvertedProjectionMatrix()
    {
        return _BuildProjectionMatrix(_NearClip, _FarClip, false);
    }
    
    // Builds the internal project matrix
    inline Matrix4 _BuildProjectionMatrix(float znear, float zfar, bool bInvertDepth)
    {
        if(znear==zfar){
            znear = _NearClip;
            zfar = _FarClip;
        }
        if(bInvertDepth){
            float tmp = znear;
            znear = zfar;
            zfar = tmp;
        }
        if(_BIsOrthoCamera){
            return Matrix4::OrthogonalOffCenter(_OrthoLeft, _OrthoRight, _OrthoBottom, _OrthoTop, znear, zfar);
        }else{
            return Matrix4::PerspectiveFov((_HFOV * _HFOVScale) * 0.01308997f/*pi/240*/, _AspectRatio, znear, zfar);
        }
    }
    
    // Converts a world position by projecting it, into screen positions, from 0 to 1 in x and y. (0,0) is TL, (1,1) is BR
    inline Vector3 WorldPosToLogicalScreenPos(const Vector3& world_pos)
    {
        Vector3 v = world_pos - _AgentTransform._Trans;
        v = v * _AgentTransform._Rot.Conjugate();
        if (v.z == 0.0f)
            v.z = -0.000001f;
        float tmp = ((float)_ScreenHeight * 0.5f) / (tanf(_HFOVScale * _HFOV) * /*to rad, pi/240*/0.01308997f * 0.5f/*tan theta over 2, so * 0.5 */);//simple trig
        tmp /= v.z;//perspective divide
        v.x = (((float)_ScreenWidth * 0.5f) - (v.x * tmp)) / (float)_ScreenWidth;
        v.y = (((float)_ScreenHeight * 0.5f) - (v.y * tmp)) / (float)_ScreenHeight;
        return v;
    }

    // At the near clip. See other overload.
    inline Vector3 ScreenPosToViewportPos(int sx, int sy)
    {
        return ScreenPosToViewportPos(sx, sy, _NearClip);
    }
    
    // Converts a screen position, 0 to screen width and height in x and y, to a viewport position
    inline Vector3 ScreenPosToViewportPos(int sx, int sy, float worldSpaceDepth)
    {
        if (_ScreenHeight == 0 || _ScreenWidth == 0)
            return Vector3::Forward;
        
        // Compute the scaling factor based on field of view and depth
        float scale = tanf(_HFOVScale * _HFOV * 0.0065449849f) * worldSpaceDepth / (_ScreenHeight * 0.5f);
        
        // Compute the relative screen coordinates from pixel positions
        float relX = ((float)sx / _ScreenWidth) - 0.5f;
        float relY = 0.5f - ((float)sy / _ScreenHeight);
        
        // Scale the relative coordinates
        Vector3 screenOffset(relX * _ScreenWidth * 0.5f, relY * _ScreenHeight * 0.5f, 0.0f);
        screenOffset *= scale;
        
        // Get the world matrix and apply the transformation
        Matrix4 worldmat = GetWorldMatrix();
        Vector3 worldPos = worldmat.TransformPoint(screenOffset);
        
        // Adjust for depth using the world matrix
        worldPos.z = worldmat._Entries[2][0] * worldPos.x + worldmat._Entries[2][1] * worldPos.y + worldmat._Entries[2][2] * worldPos.z + worldSpaceDepth;
        
        // Return the final transformed position
        return worldPos + Vector3(worldmat._Entries[3][0], worldmat._Entries[3][1], worldmat._Entries[3][2]);
    }

    // Determines if a sphere is visible, after transforming it by the given transform. Optionally specify a rendering scale.
    inline Bool Visible(const Sphere& sphere, const Transform& sphere_xform, const Vector3& renderScale)
    {
        if (!_BCullObjects)
            return true;
        Frustum f = GetFrustum();
        Vector3 v = (sphere._Center * sphere_xform._Rot) + sphere_xform._Trans;
        float threshold = sphere._Radius * -1.0f * renderScale.x;//only x needed as its a sphere
        int i = 0;
        while ((Vector3::Dot(Vector3(f._Plane[i]._Plane), v) + f._Plane[i]._Plane.w) >= threshold) {
            if (++i >= 6) {
                return true;
            }
        }
        return false;
    }
    
    // Determines if the bounding box is visible, after transforming it by the given transform. Optionally specify a rendering scale.
    inline Bool Visible(const BoundingBox& bbox, const Transform& bbox_xform, const Vector3& renderScale)
    {
        if(_BCullObjects){
            Frustum f = GetFrustum();
            Matrix4 mat = MatrixTransformation(renderScale, bbox_xform._Rot, bbox_xform._Trans);
            BoundingBox transfomed{};
            transfomed._Min = mat.TransformPoint(bbox._Min);
            transfomed._Max = mat.TransformPoint(bbox._Max);
            return f.Visible(transfomed);
        }
        return true;
    }
    
    // Relative screen position to a screen position in pixels
    inline Vector2 ViewportRelativeToAbsolute(const Vector2& pos)
    {
        // Clamp pos.x and pos.y to be between 0 and 1
        float x = fmaxf(0.0f, fminf(pos.x, 1.0f));
        float y = fmaxf(0.0f, fminf(pos.y, 1.0f));
        
        // Scale relative position to absolute screen position
        float absX = x * _ScreenWidth;
        float absY = y * _ScreenHeight;
        
        // Return the absolute position
        return Vector2(absX, absY);
    }
    
    inline Vector3 ViewportPosToDirVector(U32 sx, U32 sy) {
        // Ensure valid screen dimensions
        if (_ScreenHeight && _ScreenWidth) {
            
            // Clamp input coordinates (sx, sy) to screen bounds
            sx = MIN(sx, _ScreenWidth);
            sy = MIN(sy, _ScreenHeight);
            
            // Calculate the offset of the screen position from the center
            float offsetX = ((float)_ScreenWidth * 0.5f) - (float)sx;
            float offsetY = ((float)_ScreenHeight * 0.5f) - (float)sy;
            
            // Calculate the field of view scaling factor
            float projectionFactor = ((float)_ScreenHeight * 0.5f) / tanf((_HFOVScale * _HFOV) * 0.0065449849f);
            
            // Retrieve the world matrix
            Matrix4 worldMatrix = GetWorldMatrix();
            
            // Calculate direction vector components by transforming the screen position into world space
            float dirX = (offsetY * worldMatrix._Entries[1][1]) + (offsetX * worldMatrix._Entries[0][1]);
            float dirY = (offsetY * worldMatrix._Entries[1][0]) + (offsetX * worldMatrix._Entries[0][0]) + (projectionFactor * worldMatrix._Entries[2][0]);
            float dirZ = (offsetY * worldMatrix._Entries[1][2]) + (offsetX * worldMatrix._Entries[0][2]) + (projectionFactor * worldMatrix._Entries[2][2]);
            
            // Normalize the result direction vector
            Vector3 dirVector(dirX, dirY, dirZ);
            dirVector.Normalize();
            
            return dirVector;
        }
        
        // If screen size is not valid, return a default forward direction
        return Vector3::Forward;
    }

    
    // Updates the internally cached world matrix
    inline void _UpdateCachedTransform()
    {
        _BWorldTransformDirty = false;
        _CachedWorldMatrix = MatrixTransformation(_AgentTransform._Rot, _AgentTransform._Trans);
    }
    
    // Gets the adjusted vertical and horizontal FOV, scaled if needed.
    inline void GetAdjustedFOV(float& hfov_radians, float& vfov_radians)
    {
        vfov_radians = (_HFOV * _HFOVScale) * 0.01308997f/*pi/240*/;
        hfov_radians = vfov_radians * _AspectRatio;
    }
    
    // Get the view matrix
	inline Matrix4 GetViewMatrix()
	{
		if(_BViewMatrixDirty){
			if (_BWorldTransformDirty)
				_UpdateCachedTransform();
			
			Vector3 Eye = Vector3(_CachedWorldMatrix.GetRow(3));  // Extract camera position
			Vector3 ForwardDir = Vector3(_CachedWorldMatrix.GetRow(2));  // Third row is forward in row-major
			Vector3 At = Eye + ForwardDir;
			
			_CachedViewMatrix = Matrix4::LookAt(Eye, At, Up());
			_BViewMatrixDirty = false;
		}
		return _CachedViewMatrix;
	}

    
    // Return true if depth is inverted
    inline bool IsInvertedDepth() const
    {
        return _BAllowInvertedDepth && INVERTED_DEPTH;
    }
    
    // Gets the frustum defined by this camera
    inline Frustum GetFrustum()
    {
        if(_BFrustumDirty){
            Matrix4 viewProjection = GetViewMatrix() * _BuildProjectionMatrix(_NearClip, _FarClip, IsInvertedDepth());
            _CachedFrustum._Plane[FRUSTUM_PLANE_ZNEAR]._Plane = Vector4(Vector3::Forward, 1.0f);
            _CachedFrustum._Plane[FRUSTUM_PLANE_LEFT]._Plane = Vector4(Vector3::Left, 1.0f);
            _CachedFrustum._Plane[FRUSTUM_PLANE_RIGHT]._Plane = Vector4(Vector3::Right, 1.0f);
            _CachedFrustum._Plane[FRUSTUM_PLANE_DOWN]._Plane = Vector4(Vector3::Down, 1.0f);
            _CachedFrustum._Plane[FRUSTUM_PLANE_UP]._Plane = Vector4(Vector3::Up, 1.0f);
            _CachedFrustum._Plane[FRUSTUM_PLANE_ZFAR]._Plane = Vector4(Vector3::Backward, 1.0f);
            _CachedFrustum.TransformBy(viewProjection);
            for (int i = 0; i < 6; i++) {
                Vector3 normal = Vector3(_CachedFrustum._Plane[i]._Plane);
                float mag = normal.Magnitude();
                normal /= mag;
                _CachedFrustum._Plane[i]._Plane = Vector4(normal, mag * _CachedFrustum._Plane[i]._Plane.w);
            }
            _BFrustumDirty = false;
        }
        return _CachedFrustum;
    }
	
	// Calculate and set aspect ratio as ration of screen width and height
	inline void SetAspectRatio()
	{
		_AspectRatio = (float)_ScreenWidth / (float)_ScreenHeight;
		_BProjectionMatrixDirty = true;
	}
    
    // Call when the camera agent transform has been modified
    inline void OnAgentTransformModified()
    {
        _BWorldTransformDirty = true;
    }
    
    // Gets the calculated projection matrix
    inline Matrix4 GetProjectionMatrix()
    {
        if(_BProjectionMatrixDirty){
            float zNear = _BIsOrthoCamera ? _OrthoNear : _NearClip;
            float zFar = _BIsOrthoCamera ? _OrthoFar : _FarClip;
            _CachedProjectionMatrix = _BuildProjectionMatrix(zNear, zFar, IsInvertedDepth());
            _BProjectionMatrixDirty = false;
        }
        return _CachedProjectionMatrix;
    }
    
    // Make the camera look at the given position (make it the center of the screen)
	inline void LookAt(const Vector3& worldAt)
	{
		if (_BWorldTransformDirty)
			_UpdateCachedTransform();
		
		// Extract translation from row-major matrix (position is stored in row 3)
		Vector3 Translation = Vector3(_CachedWorldMatrix.GetRow(3));
		
		// Compute new forward direction
		Vector3 normalDir = worldAt - Translation;
		normalDir.Normalize();
		
		// Convert forward direction to quaternion
		Quaternion Rotation = Quaternion(normalDir);
		
		_CachedWorldMatrix = MatrixTransformation(Rotation, Translation);
		
		_BFrustumDirty = true;
		_BViewMatrixDirty = true;
		_BWorldTransformDirty = false;
	}

    
    // Make the camera look at the given position, worldAt, given the worldEye position where you want the camera.
    inline void LookAt(const Vector3& worldEye, const Vector3& worldAt)
    {
        Vector3 normalDir = worldAt - worldEye;
        normalDir.Normalize();
        Quaternion Rotation = Quaternion(normalDir);
        _CachedWorldMatrix = MatrixTransformation(Rotation, worldEye);
        _BFrustumDirty = true;
        _BViewMatrixDirty = true;
    }

    // Manually set the view matrix
    inline void SetViewMatrix(const Matrix4& lhs)
    {
        _CachedWorldMatrix = Matrix4::Identity();
        _CachedViewMatrix = lhs;
        _BFrustumDirty = _BViewMatrixDirty = false;
    }
    
    // Manually set the world matrix
    inline void SetWorldMatrix(const Matrix4& lhs)
    {
        _CachedWorldMatrix = lhs;
        _BViewMatrixDirty = true;
        _BFrustumDirty = true;
        _BWorldTransformDirty = false;
    }
    
	// Set the position of the camera in row-major order
	inline void SetWorldPosition(const Vector3& position){
		_CachedWorldMatrix.SetRow(3, Vector4(position, 1.0f)); // Store translation in the last row
		_BFrustumDirty = _BViewMatrixDirty = true;
		_BWorldTransformDirty = false;
	}
	
	// Set the camera's rotation in row-major order
	inline void SetWorldQuaternion(const Quaternion& quat){
		if (_BWorldTransformDirty)
			_UpdateCachedTransform();
		
		// Extract current translation from row-major matrix (last row)
		Vector3 position = Vector3(_CachedWorldMatrix.GetRow(3));
		
		// Apply new rotation while keeping the translation
		_CachedWorldMatrix = MatrixTransformation(quat, position);
		
		_BFrustumDirty = _BViewMatrixDirty = true;
		_BWorldTransformDirty = false;
	}
    
    // Set the transform of the camera, its rotation and translation
    inline void SetWorldTransform(const Transform& transform)
    {
        _CachedWorldMatrix = MatrixTransformation(transform._Rot, transform._Trans);
        _BFrustumDirty = _BViewMatrixDirty = true;
    }
    
    // Set the orthographic projection camera parameters
    inline void SetOrthoParameters(float orthoLeft, float orthoRight, float orthoTop, float orthoBottom, float orthoNear, float orthoFar)
    {
        _OrthoBottom = orthoBottom;
        _OrthoLeft = orthoLeft;
        _OrthoFar = orthoFar;
        _OrthoRight = orthoRight;
        _OrthoTop = orthoTop;
        _OrthoNear = orthoNear;
        _BIsOrthoCamera = true;
        _BProjectionMatrixDirty = true;
        _BFrustumDirty = true;
    }
    
    // Set the near clip plane Z position
    inline void SetNearClip(float near_clip, int renderDeviceDepthSize = 24)
    {
        if (near_clip != _NearClip)
        {
            if (renderDeviceDepthSize < 24)
                near_clip = fmaxf(near_clip, 0.1f);
            _NearClip = near_clip;
            _BFrustumDirty = 1;
            _BProjectionMatrixDirty = 1;
        }
    }
    
    // Set the maximum bokeh buffer vertex amount
    inline void SetMaxBokehBufferVertexAmount(float value)
    {
        float v2;
        v2 = fminf(value, 1.0f);
        if (v2 > -0.0f)
            this->_MaxBokehBufferVertexAmount = v2;
        else
            _MaxBokehBufferVertexAmount = 0;
    }
    
    // Sets the horizontal FOV scaling factpr
    inline void SetHFOVScale(float scale)
    {
        Camera* v2;
        float v3;
        
        v2 = this;
        if (scale != _HFOVScale)
        {
            _HFOVScale = scale;
            v3 = tanf((float)(scale * this->_HFOV) * 0.0087266462f);//pi/360
            v2->_BFrustumDirty = 1;
            v2->_BProjectionMatrixDirty = 1;
            v2->_FocalLength = 0.5f / v3;
        }
    }
    
    // Set the horizontal FOV, in degrees
    inline void SetHFOV(float fov_degrees)
    {
        Camera* v2;
        float v3;
        
        v2 = this;
        if (fov_degrees != this->_HFOV)
        {
            this->_HFOV = fov_degrees;
            v3 = tanf((float)(fov_degrees * this->_HFOVScale) * 0.0087266462f);
            v2->_BFrustumDirty = 1;
            v2->_BProjectionMatrixDirty = 1;
            v2->_FocalLength = 0.5f / v3;
        }
    }
    
    // Sets the far clip plane Z coordinate
    inline void SetFarClip(float far_clip)
    {
        if (far_clip != this->_FarClip)
        {
            _FarClip = far_clip;
            _BFrustumDirty = 1;
            _BProjectionMatrixDirty = 1;
        }
    }
    
    // Manually set the projection matrix
    inline void SetProjectionMatrix(const Matrix4& proj)
    {
        _CachedWorldMatrix = _CachedViewMatrix = Matrix4::Identity();
        _CachedProjectionMatrix = proj;
        _BWorldTransformDirty = _BViewMatrixDirty = _BFrustumDirty = _BProjectionMatrixDirty = false;
    }
    
    // Parameters
    inline void SetFXRadialBlurTintIntensity(float tintIntensity)
    {
        float v2 = fmaxf(tintIntensity, 2.f);
        if (v2 < 2.0f)
            this->_FXRadialBlurTintIntensity = v2;
        else
            this->_FXRadialBlurTintIntensity = 2.0f;
    }
    
    // Parameters
    inline void SetFXRadialBlurTint(const Colour& tint)
    {
        this->_FXRadialBlurTint = tint;
    }
    
    // Parameters
    inline void SetFXRadialBlurScale(float scale)
    {
        this->_FXRadialBlurScale = fmaxf(scale, 0.0f);
    }
    
    // Parameters
    inline void SetFXRadialBlurOutRadius(float radius)
    {
        float v2;
        float v3;
        
        v2 = 1.0f;
        v3 = fmaxf(radius, 0.0f);
        if (v3 < 1.0f)
            v2 = v3;
        if (v2 > this->_FXRadialBlurInnerRadius)
            this->_FXRadialBlurOuterRadius = v2;
    }
    
    // Parameters
    inline void SetFXRadialBlurIntensity(float intensity)
    {
        float v2;
        
        v2 = fmaxf(intensity, 0.0f);
        if (v2 < 2.0f)
            this->_FXRadialBlurIntensity = v2;
        else
            this->_FXRadialBlurIntensity = 2.0f;
    }
    
    // Parameters
    inline void SetFXRadialBlurInRadius(float radius)
    {
        float v2;
        float v3;
        
        v2 = 1.0f;
        v3 = fmaxf(radius, 0.0f);
        if (v3 < 1.0f)
            v2 = v3;
        if (v2 < this->_FXRadialBlurOuterRadius)
            this->_FXRadialBlurInnerRadius = v2;
    }
    
    inline void SetFXRadialBlurActive(bool active)
    {
        this->_FXRadialBlurActive = active;
    }
    
    inline void SetFXMotionBlurRotationThresholdActive(bool active)
    {
        this->_FXMotionBlurThresholdRotActive = active;
    }
    
    inline void SetFXMotionBlurRotationThreshold(float threshold)
    {
        this->_FXMotionBlurRotationThreshold = threshold;
    }
    
    inline void SetFXMotionBlurMovementThresholdActive(bool active)
    {
        this->_FXMotionBlurThresholdActive = active;
    }
    
    inline void SetFXMotionBlurMovementThreshold(float threshold)
    {
        this->_FXMotionBlurThreshold = threshold;
    }
    
    inline void SetFXMotionBlurIntensity(float strength)
    {
        this->_FXMotionBlurStrength = strength;
    }
    
    inline void SetFXMotionBlurDelay(bool delay)
    {
        this->_MotionBlurDelay = delay;
    }
    
    inline void SetFXMotionBlurActive(bool active)
    {
        this->_FXMotionBlurActive = active;
    }
    
    inline void SetFXLevelsWhite(float white)
    {
        this->_FXLevelsWhite = white;
    }
    
    inline void SetFXLevelsIntensity(float intensity)
    {
        this->_FXLevelsIntensity = intensity;
    }
    
    inline void SetFXLevelsBlack(float black)
    {
        this->_FXLevelsBlack = black;
    }
    
    inline void SetFXLevelsActive(bool active)
    {
        this->_FXLevelsActive = active;
    }
    
    inline void SetFXColourOpacity(float opacity)
    {
        if (opacity != this->_FXColourOpacity)
        {
            this->_FXColourOpacity = opacity;
        }
    }
    
    inline void SetFXColourActive(bool active)
    {
        if (active != this->_FXColourActive)
        {
            this->_FXColourActive = active;
        }
    }
    
    inline void SetFXColour(const Colour& Colour)
    {
        if (this->_FXColour.r != Colour.r
            || this->_FXColour.g != Colour.g
            || this->_FXColour.b != Colour.b
            || this->_FXColour.a != Colour.a)
        {
            this->_FXColour = Colour;
        }
    }
    
    inline void SetExposure(float v)
    {
        this->_Exposure = v;
    }
    
    inline void SetDOFNearMax(float DOFNearMax)
    {
        this->_DOFNearMax = DOFNearMax;
    }
    
    inline void SetDOFNearFallOff(float DOFNearRamp)
    {
        this->_DOFNearRamp = DOFNearRamp;
    }
    
    inline void SetDOFNear(float DOFNear)
    {
        this->_DOFNear = DOFNear;
    }
    
    inline void SetDOFFarMax(float DOFFarMax)
    {
        this->_DOFFarMax = DOFFarMax;
    }
    
    inline void SetDOFFarFallOff(float DOFFarRamp)
    {
        this->_DOFFarRamp = DOFFarRamp;
    }
    
    inline void SetDOFFar(float DOFFar)
    {
        this->_DOFFar = DOFFar;
    }
    
    inline void SetDOFEnabled(bool isEnabled)
    {
        this->_BDOFEnabled = isEnabled;
    }
    
    inline void SetDOFDebug(float v)
    {
        this->_DOFDebug = v;
    }
    
    inline void SetDOFCoverageBoost(float v)
    {
        this->_DOFCoverageBoost = v;
    }
    
    inline void SetBokehMinSize(float v)
    {
        this->_BokehMinSize = v;
    }
    
    inline void SetBokehMaxSize(float v)
    {
        this->_BokehMaxSize = v;
    }
    
    inline void SetBokehFalloff(float v)
    {
        this->_BokehFalloff = v;
    }
    
    inline void SetBokehBrightnessThreshold(float v)
    {
        this->_BokehBrightnessThreshold = v;
    }
    
    inline void SetBokehBrightnessDeltaThreshold(float v)
    {
        this->_BokehBrightnessDeltaThreshold = v;
    }
    
    inline void SetBokehBlurThreshold(float v)
    {
        this->_BokehBlurThreshold = v;
    }
    
    inline void SetBokehAberrationOffsetsY(const Vector3& v)
    {
        this->_BokehAberrationOffsetsY = v;
    }
    
    inline void SetBokehAberrationOffsetsX(const Vector3& v)
    {
        this->_BokehAberrationOffsetsX = v;
    }
    
    inline void SetAspectRatio(float aspect)
    {
        if (aspect != this->_AspectRatio)
        {
            _AspectRatio = aspect;
            _BFrustumDirty = 1;
            _BProjectionMatrixDirty = 1;
        }
    }
    
    inline void SetAllowInvertDepth(bool v)
    {
        if (_BAllowInvertedDepth != v)
        {
            _BAllowInvertedDepth = v;
            _BProjectionMatrixDirty = 1;
        }
    }
    
    inline void SetAllowBlending(bool v)
    {
        if (_BAllowBlending != v)
            _BAllowBlending = v;
    }
    
    // Get the depth of field parameters into the output argument references
    inline void GetDOFParameters(float& outDOFFar, float& outDOFNear, float& outDOFFarRamp,
            float& outDOFNearRamp, float& outDOFFarMax, float& outDOFNearMax, float& outDOFDebug, float& outCoverageBoost)
    {
        outDOFFar = this->_DOFFar;
        outDOFNear = this->_DOFNear;
        outDOFFarRamp = this->_DOFFarRamp;
        outDOFNearRamp = this->_DOFNearRamp;
        outDOFFarMax = this->_DOFFarMax;
        outDOFNearMax = this->_DOFNearMax;
        outDOFDebug = this->_DOFDebug;
        outCoverageBoost = this->_DOFCoverageBoost;
    }
    
#define CGETTER(var, fn, type) inline type fn(){ return var; }
    
    CGETTER(_UseHQDOF, GetUseHQDOF, bool);
    CGETTER(_UseBokeh, GetUseBokeh, bool);
    CGETTER(_FXRadialBlurTintIntensity, GetFXRadialBlurTintIntensity, float);
    CGETTER(_FXRadialBlurTint, GetFXRadialBlurTint, Colour);
    CGETTER(_FXRadialBlurScale, GetFXRadialBlurScale, float);
    CGETTER(_FXRadialBlurOuterRadius, GetFXRadialBlurOutRadius, float);
    CGETTER(_FXRadialBlurIntensity, GetFXRadialBlurIntensity, float);
    CGETTER(_FXRadialBlurInnerRadius, GetFXRadialBlurInRadius, float);
    CGETTER(_FXRadialBlurActive, GetFXRadialBlurActive, bool);
    CGETTER(_FXMotionBlurThresholdRotActive, GetFXMotionBlurThresholdRotationActive, bool);
    CGETTER(_FXMotionBlurRotationThreshold, GetFXMotionBlurRotationThreshold, float);
    CGETTER(_FXMotionBlurThresholdActive, GetFXMotionBlurThresholdActive, bool);
    CGETTER(_FXMotionBlurThreshold, GetFXMotionBlurThreshold, float);
    CGETTER(_FXMotionBlurStrength, GetFXMotionBlurIntensity, float);
    CGETTER(_FXMotionBlurActive, GetFXMotionBlurActive, bool);
    CGETTER(_DOFNearMax, GetDOFNearMax, float);
    CGETTER(_DOFNear, GetDOFNear, float);
    CGETTER(_DOFNearRamp, GetDOFNearFallOff, float);
    CGETTER(_DOFFarMax, GetDOFFarMax, float);
    CGETTER(_DOFFar, GetDOFFar, float);
    CGETTER(_DOFFarRamp, GetDOFFarFallOff, float);
    CGETTER(_DOFDebug, GetDOFDebug, float);
    CGETTER(_DOFCoverageBoost, GetDOFCoverageBoost, float);
    CGETTER(_hBokehTexture, GetBokenTexture, Symbol);
    CGETTER(_BokehMinSize, GetBokehMinSize, float);
    CGETTER(_BokehMaxSize, GetBokehMaxSize, float);
    CGETTER(_BokehFalloff, GetBokehFalloff, float);
    CGETTER(_BokehBrightnessThreshold, GetBokehBrightnessThreshold, float);
    
    CGETTER(_BokehBrightnessDeltaThreshold, GetBokehBrightnessDeltaThreshold, float);
    CGETTER(_BokehBlurThreshold, GetBokehBlurThreshold, float);
    CGETTER(_BokehAberrationOffsetsY, GetBokehAberrationOffsetsY, Vector3);
    CGETTER(_BokehAberrationOffsetsX, GetBokehAberrationOffsetsX, Vector3);
    CGETTER(_AspectRatio, GetAspectRatio, float);
    
};

#undef CGETTER
