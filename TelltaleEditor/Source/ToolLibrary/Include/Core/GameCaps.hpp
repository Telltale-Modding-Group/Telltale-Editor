#include <Core/BitSet.hpp>

// These describe game specific functionality. We then adjust runtime and library functionality suited alongside this bitset which is found in the meta state.

enum class GameCapability
{
    NONE = -1,
    
    // Transforms are split into vec3 and quat keys. used in very old games. Type of AVI must be POSE to take effect and be keyframed quats/vec3s.
    SEPARATE_ANIMATION_TRANSFORM,
    // Enable TMAP files (ie, they exist). See docs.
    ALLOW_TRANSITION_MAPS,
    // Uses 'LocationInfo' type for agent attachment information and initial transform. Only used in very new games. (eg WDC. Not used in MC2 etc)
    USES_LOCATION_INFO,
    // Uses .LENC
    USES_LENC,
    // Scripts dont have encryption
    SCRIPT_ENCRYPTION_DISABLED,
    // Type names are not 'tool', ie they have 'class ' etc.
    RAW_TYPE_NAMES,
    
    NUM,
};

struct GameCapDesc
{
    CString Name;
    GameCapability Cap;
};

using GameCapabilitiesBitSet = BitSet<GameCapability, (U32)GameCapability::NUM, (GameCapability)0>;

constexpr GameCapDesc GameCapDescs[] =
{
    {"kGameCapSeparateAnimationTransform", GameCapability::SEPARATE_ANIMATION_TRANSFORM},
    {"kGameCapAllowTransitionMaps", GameCapability::ALLOW_TRANSITION_MAPS},
    {"kGameCapUsesLocationInfo", GameCapability::USES_LOCATION_INFO},
    {"kGameCapUsesLenc", GameCapability::USES_LENC},
    {"kGameCapNoScriptEncryption", GameCapability::USES_LENC},
    {"kGameCapRawClassNames", GameCapability::RAW_TYPE_NAMES},
    {"", GameCapability::NONE},
};
