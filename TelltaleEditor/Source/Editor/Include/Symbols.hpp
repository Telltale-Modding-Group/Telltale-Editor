#ifndef _TTE_SYMBOLS_HPP

class Symbol;

#ifdef _TTE_SYMBOLS_IMPL
#define GSYMBOL(_GlobalName, _SymbolStr) String _GlobalName{_SymbolStr}
#else
#define GSYMBOL(_GlobalName, _SymbolStr) extern String _GlobalName
#endif

/* SYMBOLS. SWITCH TO USE SYMBOLS INSTEAD OF STRINGS. MAYBE USEFUL IF GETTING TOO LARGE. (although, we want to try and always use strings, as the slight performance increase we dont care).
#ifdef _TTE_SYMBOLS_IMPL
#define GSYMBOL(_GlobalName, _SymbolStr) Symbol _GlobalName{_SymbolStr}
#else
#define GSYMBOL(_GlobalName, _SymbolStr) extern Symbol _GlobalName
#endif
 */

GSYMBOL(kEmptySymbol, "");

// ======================== SYMBOL SECTION <> SKELETON ================================

GSYMBOL(kSkeletonInstanceNodeName, "Skeleton Instance");
GSYMBOL(kSkeletonFile, "Skeleton File");
GSYMBOL(kSkeletonUseProceduralJointCorners, "Skeleton Use Procedural Joint Corners");
GSYMBOL(kSkeletonLegWidth, "Skeleton Leg Width");
GSYMBOL(kSkeletonArmWidth, "Skeleton Arm Width");

// ======================== SYMBOL SECTION <> ANIMATION NODES ================================

GSYMBOL(kAnimationAbsoluteNode, "absoluteNode");
GSYMBOL(kAnimationRelativeNode, "relativeNode");

// ======================== SYMBOL SECTION <> AGENT PROPERTIES ================================

GSYMBOL(kRuntimeVisibilityKey, "Runtime: Visible");

#endif
