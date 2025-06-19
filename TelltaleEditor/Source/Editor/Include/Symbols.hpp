#ifdef _TTE_SYMBOLS_IMPL

#undef GSYMBOL
#undef _TTE_SYMBOLS_HPP
#define GSYMBOL(_GlobalName, _SymbolStr) String _GlobalName{_SymbolStr}; Symbol _GlobalName##Symbol{_SymbolStr, true}

#elif !defined(GSYMBOL)

#define GSYMBOL(_GlobalName, _SymbolStr) extern String _GlobalName; extern Symbol _GlobalName##Symbol

#endif

#ifndef _TTE_SYMBOLS_HPP
#define _TTE_SYMBOLS_HPP

class Symbol;

GSYMBOL(kEmptySymbol, "");

// ======================== SYMBOL SECTION <> SKELETON ================================

GSYMBOL(kSkeletonInstanceNodeName, "Skeleton Instance");
GSYMBOL(kSkeletonFile, "Skeleton File");
GSYMBOL(kSkeletonUseProceduralJointCorners, "Skeleton Use Procedural Joint Corners");
GSYMBOL(kSkeletonLegWidth, "Skeleton Leg Width");
GSYMBOL(kSkeletonArmWidth, "Skeleton Arm Width");

// ======================== SYMBOL SECTION <> RENDERABLE ================================

GSYMBOL(kRenderablePropKey3DAlpha, "Render 3D Alpha");
GSYMBOL(kRenderablePropKeyAllowNPRLines, "Render Allow NPR Lines");
GSYMBOL(kRenderablePropKeyAlphaAntialiasing, ""); // Empty ones haven't found the string for yet (check other executables, will be there)
GSYMBOL(kRenderablePropKeyAlphaMultiply, "");
GSYMBOL(kRenderablePropKeyAmbientOcclusionLightmap, "Render Ambient Occlusion Lightmap");
GSYMBOL(kRenderablePropKeyAutoFocusEnable, "Render Auto Focus Enable");
GSYMBOL(kRenderablePropKeyAxisScale, "Render Axis Scale");
GSYMBOL(kRenderablePropKeyBakeAsStatic, "");
GSYMBOL(kRenderablePropKeyBrushFarDetailBias, "Render Brush Far Detail Bias");
GSYMBOL(kRenderablePropKeyBrushForceEnable, "Render Brush Force Enable");
GSYMBOL(kRenderablePropKeyBrushNearDetailBias, "Render Brush Near Detail Bias");
GSYMBOL(kRenderablePropKeyBrushScale, "Render Brush Scale");
GSYMBOL(kRenderablePropKeyBrushScaleByVertex, "Render Brush Scale By Vertex");
GSYMBOL(kRenderablePropKeyCameraFacing, "Camera Facing");
GSYMBOL(kRenderablePropKeyCameraFacingType, "Camera Facing Type");
GSYMBOL(kRenderablePropKeyCastShadow, "Cast Shadow");
GSYMBOL(kRenderablePropKeyCastShadowsAsStatic, "Cast Shadow As Static");
GSYMBOL(kRenderablePropKeyColorCorrection, "Render Color Correction");
GSYMBOL(kRenderablePropKeyColorWrite, "Render Color Write");
GSYMBOL(kRenderablePropKeyConstantAlpha, "Render Constant Alpha");
GSYMBOL(kRenderablePropKeyCubeBakeEnable, "");
GSYMBOL(kRenderablePropKeyD3DMesh, "D3D Mesh");
GSYMBOL(kRenderablePropKeyD3DMeshList, "D3D Mesh List");
GSYMBOL(kRenderablePropKeyDepthTest, "Render Depth Test");
GSYMBOL(kRenderablePropKeyDepthTestFunc, "Render Depth Test Function");
GSYMBOL(kRenderablePropKeyDepthWrite, "Render Depth Write");
GSYMBOL(kRenderablePropKeyDepthWriteAlpha, "Render Depth Write Alpha");
GSYMBOL(kRenderablePropKeyDiffuseColor, "Render Diffuse Color");
GSYMBOL(kRenderablePropKeyDisableLightBake, "Render Disable Light Bake");
GSYMBOL(kRenderablePropKeyDoMotionBlur, "");
GSYMBOL(kRenderablePropKeyEmissionColor, "Render Emission Color");
GSYMBOL(kRenderablePropKeyFogColor, "");
GSYMBOL(kRenderablePropKeyFogEnabled, "");
GSYMBOL(kRenderablePropKeyFogFarPlane, "");
GSYMBOL(kRenderablePropKeyFogNearPlane, "");
GSYMBOL(kRenderablePropKeyFogOverride, "");
GSYMBOL(kRenderablePropKeyForceAsAlpha, "Render Force As Alpha");
GSYMBOL(kRenderablePropKeyForceLinearDepthWrite, "");
GSYMBOL(kRenderablePropKeyFXColorEnabled, "");
GSYMBOL(kRenderablePropKeyGlobalScale, "Render Global Scale");
GSYMBOL(kRenderablePropKeyLightCinematicRig, "");
GSYMBOL(kRenderablePropKeyLightEnvEnable, "");
GSYMBOL(kRenderablePropKeyLightEnvGroup, "");
GSYMBOL(kRenderablePropKeyLightEnvIntensity, "");
GSYMBOL(kRenderablePropKeyLightEnvNode, "");
GSYMBOL(kRenderablePropKeyLightEnvReflectionEnable, "");
GSYMBOL(kRenderablePropKeyLightEnvReflectionIntensity, "");
GSYMBOL(kRenderablePropKeyLightEnvShadowCastEnable, "");
GSYMBOL(kRenderablePropKeyLightEnvShadowCastGroups, "");
GSYMBOL(kRenderablePropKeyLightingGroups, "Render Lighting Groups");
GSYMBOL(kRenderablePropKeyLightmapScale, "Render Lightmap Scale");
GSYMBOL(kRenderablePropKeyLightmapUVGenerationType, "");
GSYMBOL(kRenderablePropKeyLODBias, "Render LOD Bias");
GSYMBOL(kRenderablePropKeyLODScale, "Render LOD Scale");
GSYMBOL(kRenderablePropKeyMaskTest, "Render Mask Test");
GSYMBOL(kRenderablePropKeyMaskWrite, "Render Mask Write");
GSYMBOL(kRenderablePropKeyMaterialTime, "Render Material Time");
GSYMBOL(kRenderablePropKeyNPRLineBias, "Render NPR Line Bias");
GSYMBOL(kRenderablePropKeyNPRLineFalloff, "Render NPR Line Falloff");
GSYMBOL(kRenderablePropKeyNPRLineFalloffBiasOverride, "");
GSYMBOL(kRenderablePropKeyOverrideToonOutlineColor, "");
GSYMBOL(kRenderablePropKeyRecieveShadows, "Receive Shadows");
GSYMBOL(kRenderablePropKeyRecieveShadowsDecal, "Receive Shadows Decal");
GSYMBOL(kRenderablePropKeyRecieveShadowsDoublesided, "Receive Shadows Doublesided");
GSYMBOL(kRenderablePropKeyRecieveShadowsIntensity, "");
GSYMBOL(kRenderablePropKeyRenderAfterAntiAliasing, "");
GSYMBOL(kRenderablePropKeyRenderAfterPostEffects, "");
GSYMBOL(kRenderablePropKeyRenderCull, "Render Cull");
GSYMBOL(kRenderablePropKeyRenderLayer, "Render Layer");
GSYMBOL(kRenderablePropKeyRenderToonOutline, "Render Toon Outline");
GSYMBOL(kRenderablePropKeyRimBumpScale, "Render Rim Bump Scale");
GSYMBOL(kRenderablePropKeySceneEnlightenData, "Render Scene Enlighten Data");
GSYMBOL(kRenderablePropKeySceneLightmapData, "Render Scene Lightmap Data");
GSYMBOL(kRenderablePropKeyShadowCastGroup, "Render Shadow Cast Group");
GSYMBOL(kRenderablePropKeyShadowReceiveGroup, "Render Shadow Receive Group");
GSYMBOL(kRenderablePropKeyShadowUseLowLOD, "");
GSYMBOL(kRenderablePropKeyShadowVisibleThresholdScale, "");
GSYMBOL(kRenderablePropKeySSLineEnable, "Render SS Line Enable");
GSYMBOL(kRenderablePropKeyStatic, "Render Static");
GSYMBOL(kRenderablePropKeyTextureOverrides, "Render Texture Overrides");
GSYMBOL(kRenderablePropKeyToonOutlineColor, "Toon Outline Color");
GSYMBOL(kRenderablePropKeyVisibleThresholdScale, "Render Visible Threshold Scale");


// ======================== SYMBOL SECTION <> ANIMATION NODES ================================

GSYMBOL(kAnimationAbsoluteNode, "absoluteNode");
GSYMBOL(kAnimationRelativeNode, "relativeNode");

// ======================== SYMBOL SECTION <> ALL TOOL MODULES ================================

GSYMBOL(kScenePropName, "module_scene.prop");
GSYMBOL(kAnimationPropName, "module_animation.prop");
GSYMBOL(kLogicGroupLogicItemDlgNodePropName, "module_logicGroup_logicItem_dlgNode.prop");
GSYMBOL(k3dSoundPropName, "module_sound_3d.prop");
GSYMBOL(kAgentStatePropName, "module_agent_state.prop");
GSYMBOL(kAnimationConstraintsPropName, "module_animation_constraints.prop");
GSYMBOL(kCameraPropName, "module_camera.prop");
GSYMBOL(kChorecorderPropName, "module_chorecorder.prop");
GSYMBOL(kCinematicLightBlockingPropName, "module_cin_rigLightRotation.prop");
GSYMBOL(kCinematicLightPropName, "module_cin_light.prop");
GSYMBOL(kCinematicLightRigPropName, "module_cin_lightrig.prop");
GSYMBOL(kContextMenuPropName, "module_context_menu.prop");
GSYMBOL(kCursorPropName, "module_cursor.prop");
GSYMBOL(kDecalPropName, "module_decal.prop");
GSYMBOL(kDialogPropName, "module_dialog.prop");
GSYMBOL(kEnlightenAdaptiveProbeVolumePropName, "module_enlightenadaptiveprobevolume.prop");
GSYMBOL(kEnlightenAutoProbeVolumePropName, "module_enlightenautoprobevolume.prop");
GSYMBOL(kEnlightenCubemapPropName, "module_enlightencubemap.prop");
GSYMBOL(kEnlightenCubemapTextureEnvPropName, "module_cubemaptextureenv.prop");
GSYMBOL(kEnlightenProbeVolumePropName, "module_enlightenprobevolume.prop");
GSYMBOL(kEnlightenPropName, "module_enlighten.prop");
GSYMBOL(kEnlightenSystemPropName, "module_enlightensystem.prop");
GSYMBOL(kEnvironmentLightGroupPropName, "module_env_lightgroup.prop");
GSYMBOL(kEnvironmentLightPropName, "module_env_light.prop");
GSYMBOL(kEnvironmentPropName, "module_environment.prop");
GSYMBOL(kEnvironmentTilePropName, "module_env_tile.prop");
GSYMBOL(kFootstepsPropName, "module_footsteps.prop");
GSYMBOL(kFootsteps2PropName, "module_footsteps2.prop");
GSYMBOL(kHLSPlayerPropName, "module_hls.prop");
GSYMBOL(kLightProbePropName, "module_lightprobe.prop");
GSYMBOL(kLightPropName, "module_light.prop");
GSYMBOL(kLipSyncPropName, "module_lipsync.prop");
GSYMBOL(kLookAtBlockingPropName, "module_lookat_blocking.prop");
GSYMBOL(kMoviePlayerPropName, "module_movieplayer.prop");
GSYMBOL(kNavCamPropName, "module_nav_cam.prop");
GSYMBOL(kParticleAffectorPropName, "module_particleaffector.prop");
GSYMBOL(kParticleEmitterBasePropName, "module_base_particleemitter.prop");
GSYMBOL(kParticleEmitterMaterialPropName, "module_material_particleemitter.prop");
GSYMBOL(kParticleEmitterPropName, "module_particleemitter.prop");
GSYMBOL(kPathToPropName, "module_path_to.prop");
GSYMBOL(kPhysicsObjectPropName, "module_physicsobject.prop");
GSYMBOL(kPostMaterialScenePropName, "module_post_effect.prop");
GSYMBOL(kRenderablePropName, "module_renderable.prop");
GSYMBOL(kRolloverPropName, "module_rollover.prop");
GSYMBOL(kSceneAutofocusPropName, "module_scene_autofocus.prop");
GSYMBOL(kSceneBrushPropName, "module_scene_brush.prop");
GSYMBOL(kSceneBrushOutlinePropName, "module_scene_brushoutline.prop");
GSYMBOL(kSceneDOFPropName, "module_scene_dof.prop");
GSYMBOL(kSceneHBAOPropName, "module_scene_hbao.prop");
GSYMBOL(kSceneLightEnvPropName, "module_scene_lightnv.prop");
GSYMBOL(kSceneLightEnvAdvancedPropName, "module_scene_lightenv_advanced.prop");
GSYMBOL(kSceneSSLinesPropName, "module_scene_sslines.prop");
GSYMBOL(kSceneTonemapPropName, "module_scene_tonemap.prop");
GSYMBOL(kSceneTonemapRGBPropName, "module_scene_tonemap_rgb.prop");
GSYMBOL(kSceneViewportPropName, "module_scene_viewport.prop");
GSYMBOL(kSceneVignettePropName, "module_scene_vignette.prop");
GSYMBOL(kSelectablePropName, "module_selectable.prop");
GSYMBOL(kSkeletonPropName, "module_skeleton.prop");
GSYMBOL(kSoundAmbientInterfacePropName, "module_sound_ambient_interface.prop");
GSYMBOL(kSoundEventEmitterPropName, "module_soundevent_emitter.prop");
GSYMBOL(kSoundEventPreloadInterfacePropName, "module_sound_event_preload_interface.prop");
GSYMBOL(kSoundListenerInterfacePropName, "module_sound_listener_interface.prop");
GSYMBOL(kSoundMusicInterfacePropName, "module_sound_music_interface.prop");
GSYMBOL(kSoundReverbInterfacePropName, "module_sound_reverb_interface.prop");
GSYMBOL(kSoundSfxInterfacePropName, "module_sound_sfx_interface.prop");
GSYMBOL(kSoundSnapshotPropName, "module_soundsnapshot.prop");
GSYMBOL(kStylePropName, "module_style.prop");
GSYMBOL(kTextPropName, "module_text.prop");
GSYMBOL(kText2PropName, "module_text2.prop");
GSYMBOL(kTriggerPropName, "module_trigger.prop");
GSYMBOL(kUIDialogPropName, "module_dialog_choice.prop");
GSYMBOL(kVfxGroupPropName, "module_vfx_group.prop");
GSYMBOL(kViewportPropName, "module_viewport.prop");
GSYMBOL(kVoiceSpeakerPropName, "module_voicespeaker.prop");
GSYMBOL(kWalkAnimatorPropName, "module_walk_animator.prop");
GSYMBOL(kProceduralLookAtPropName, "module_procedural_look_at.prop");
GSYMBOL(kStyleIdleTransitionsPropName, "module_style_idle_transitions.prop");
GSYMBOL(kIKAttachPropName, "module_inversekinematicsattach.prop");
GSYMBOL(kIKPropName, "module_inversekinematics.prop");
GSYMBOL(kMaterialLegacyPropName, "module_legacy_material.prop");
GSYMBOL(kMaterialDecalPropName, "module_decal_material.prop");
GSYMBOL(kMaterialPostPropName, "module_post_material.prop");
GSYMBOL(kMaterialParticlePropName, "module_particle_material.prop");
GSYMBOL(kMaterialPropName, "module_material.prop");
GSYMBOL(kParticleIKPropName, "module_particleinversekinematics.prop");
GSYMBOL(kSound3dParametersPropName, "module_sound_3d_params.prop");
GSYMBOL(kSoundDataPropName, "module_sound_data.prop");

// ======================== SYMBOL SECTION <> AGENT PROPERTIES ================================

GSYMBOL(kRuntimeVisibilityKey, "Runtime: Visible");

#endif