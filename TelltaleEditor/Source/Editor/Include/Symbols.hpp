#ifndef _TTE_SYMBOLS_HPP

class Symbol;

#ifdef _TTE_SYMBOLS_IMPL
#define GSYMBOL(_GlobalName, _SymbolStr) String _GlobalName{_SymbolStr}; Symbol _GlobalName##Symbol{_SymbolStr, true}
#else
#define GSYMBOL(_GlobalName, _SymbolStr) extern String _GlobalName; extern Symbol _GlobalName##Symbol;
#endif

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
