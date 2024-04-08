#ifndef SHADER_TYPE_H_INCLUDED
#define SHADER_TYPE_H_INCLUDED
#define SHADER_TYPE_SYS_ERROR 0
#define SHADER_TYPE_BLEND 1
#define SHADER_TYPE_COMMON 2
#define SHADER_TYPE_INDIRECT 3
#define SHADER_TYPE_LUMINESCENCE 4
#define SHADER_TYPE_CHR_EYE 5
#define SHADER_TYPE_CHR_EYE_FHL 6
#define SHADER_TYPE_CHR_SKIN 7
#define SHADER_TYPE_CHR_SKIN_HALF 8
#define SHADER_TYPE_CHR_SKIN_IGNORE 9
#define SHADER_TYPE_CLOTH 10
#define SHADER_TYPE_CLOUD 11
#define SHADER_TYPE_DIM 12
#define SHADER_TYPE_DISTORTION 13
#define SHADER_TYPE_DISTORTION_OVERLAY 14
#define SHADER_TYPE_ENM_EMISSION 15
#define SHADER_TYPE_ENM_GLASS 16
#define SHADER_TYPE_ENM_IGNORE 17
#define SHADER_TYPE_FADE_OUT_NORMAL 18
#define SHADER_TYPE_FALLOFF 19
#define SHADER_TYPE_FALLOFF_V 20
#define SHADER_TYPE_FUR 21
#define SHADER_TYPE_GLASS 22
#define SHADER_TYPE_ICE 23
#define SHADER_TYPE_IGNORE_LIGHT 24
#define SHADER_TYPE_IGNORE_LIGHT_TWICE 25
#define SHADER_TYPE_IGNORE_LIGHT_V 26
#define SHADER_TYPE_INDIRECT_NO_LIGHT 27
#define SHADER_TYPE_INDIRECT_V 28
#define SHADER_TYPE_LUMINESCENCE_V 29
#define SHADER_TYPE_METAL 30
#define SHADER_TYPE_MIRROR 31
#define SHADER_TYPE_RING 32
#define SHADER_TYPE_SHOE 33
#define SHADER_TYPE_TIME_EATER 34
#define SHADER_TYPE_TRANS_THIN 35
#define SHADER_TYPE_WATER_ADD 36
#define SHADER_TYPE_WATER_MUL 37
#define SHADER_TYPE_WATER_OPACITY 38
#define SHADER_TYPE_MAX 39
#ifdef __cplusplus
#include <string_view>
inline std::pair<std::string_view, size_t> s_shaderTypes[] =
{
	{"BillboardParticle_", SHADER_TYPE_SYS_ERROR},
	{"BillboardParticleY_", SHADER_TYPE_SYS_ERROR},
	{"BlbBlend_", SHADER_TYPE_BLEND},
	{"BlbCommon_", SHADER_TYPE_COMMON},
	{"BlbIndirect_", SHADER_TYPE_INDIRECT},
	{"BlbLuminescence_", SHADER_TYPE_LUMINESCENCE},
	{"Blend_", SHADER_TYPE_BLEND},
	{"Chaos_", SHADER_TYPE_SYS_ERROR},
	{"ChaosV_", SHADER_TYPE_SYS_ERROR},
	{"ChrEye_", SHADER_TYPE_CHR_EYE},
	{"ChrEyeFHL", SHADER_TYPE_CHR_EYE_FHL},
	{"ChrSkin_", SHADER_TYPE_CHR_SKIN},
	{"ChrSkinHalf_", SHADER_TYPE_CHR_SKIN_HALF},
	{"ChrSkinIgnore_", SHADER_TYPE_CHR_SKIN_IGNORE},
	{"Cloak_", SHADER_TYPE_SYS_ERROR},
	{"Cloth_", SHADER_TYPE_CLOTH},
	{"Cloud_", SHADER_TYPE_CLOUD},
	{"Common_", SHADER_TYPE_COMMON},
	{"Deformation_", SHADER_TYPE_SYS_ERROR},
	{"DeformationParticle_", SHADER_TYPE_SYS_ERROR},
	{"Dim_", SHADER_TYPE_DIM},
	{"DimIgnore_", SHADER_TYPE_SYS_ERROR},
	{"Distortion_", SHADER_TYPE_DISTORTION},
	{"DistortionOverlay_", SHADER_TYPE_DISTORTION_OVERLAY},
	{"DistortionOverlayChaos_", SHADER_TYPE_SYS_ERROR},
	{"EnmCloud_", SHADER_TYPE_CLOUD},
	{"EnmEmission_", SHADER_TYPE_ENM_EMISSION},
	{"EnmGlass_", SHADER_TYPE_ENM_GLASS},
	{"EnmIgnore_", SHADER_TYPE_ENM_IGNORE},
	{"EnmMetal_", SHADER_TYPE_CHR_SKIN},
	{"FadeOutNormal_", SHADER_TYPE_FADE_OUT_NORMAL},
	{"FakeGlass_", SHADER_TYPE_ENM_GLASS},
	{"FallOff_", SHADER_TYPE_FALLOFF},
	{"FallOffV_", SHADER_TYPE_FALLOFF_V},
	{"Fur", SHADER_TYPE_FUR},
	{"Glass_", SHADER_TYPE_GLASS},
	{"GlassRefraction_", SHADER_TYPE_SYS_ERROR},
	{"Ice_", SHADER_TYPE_ICE},
	{"IgnoreLight_", SHADER_TYPE_IGNORE_LIGHT},
	{"IgnoreLightTwice_", SHADER_TYPE_IGNORE_LIGHT_TWICE},
	{"IgnoreLightV_", SHADER_TYPE_IGNORE_LIGHT_V},
	{"Indirect_", SHADER_TYPE_INDIRECT},
	{"IndirectNoLight_", SHADER_TYPE_INDIRECT_NO_LIGHT},
	{"IndirectV_", SHADER_TYPE_INDIRECT_V},
	{"IndirectVnoGIs_", SHADER_TYPE_INDIRECT_V},
	{"Lava_", SHADER_TYPE_SYS_ERROR},
	{"Luminescence_", SHADER_TYPE_LUMINESCENCE},
	{"LuminescenceV_", SHADER_TYPE_LUMINESCENCE_V},
	{"MeshParticle_", SHADER_TYPE_SYS_ERROR},
	{"MeshParticleLightingShadow_", SHADER_TYPE_SYS_ERROR},
	{"MeshParticleRef_", SHADER_TYPE_SYS_ERROR},
	{"Metal", SHADER_TYPE_METAL},
	{"Mirror_", SHADER_TYPE_MIRROR},
	{"Mirror2_", SHADER_TYPE_MIRROR},
	{"Myst_", SHADER_TYPE_SYS_ERROR},
	{"Parallax_", SHADER_TYPE_SYS_ERROR},
	{"Ring_", SHADER_TYPE_RING},
	{"Shoe", SHADER_TYPE_SHOE},
	{"TimeEater_", SHADER_TYPE_TIME_EATER},
	{"TimeEaterDistortion_", SHADER_TYPE_DISTORTION},
	{"TimeEaterEmission_", SHADER_TYPE_ENM_EMISSION},
	{"TimeEaterGlass_", SHADER_TYPE_ENM_GLASS},
	{"TimeEaterIndirect_", SHADER_TYPE_INDIRECT_NO_LIGHT},
	{"TimeEaterMetal_", SHADER_TYPE_CHR_SKIN},
	{"TransThin_", SHADER_TYPE_TRANS_THIN},
	{"Water_Add", SHADER_TYPE_WATER_ADD},
	{"Water_Mul", SHADER_TYPE_WATER_MUL},
	{"Water_Opacity", SHADER_TYPE_WATER_OPACITY},
};
inline const wchar_t* s_shaderHitGroups[] =
{
	L"SYS_ERROR_PrimaryHitGroup",
	L"SYS_ERROR_PrimaryHitGroup_ConstTexCoord",
	L"SYS_ERROR_PrimaryTransparentHitGroup",
	L"SYS_ERROR_PrimaryTransparentHitGroup_ConstTexCoord",
	L"SYS_ERROR_SecondaryHitGroup",
	L"BLEND_PrimaryHitGroup",
	L"BLEND_PrimaryHitGroup_ConstTexCoord",
	L"BLEND_PrimaryTransparentHitGroup",
	L"BLEND_PrimaryTransparentHitGroup_ConstTexCoord",
	L"BLEND_SecondaryHitGroup",
	L"COMMON_PrimaryHitGroup",
	L"COMMON_PrimaryHitGroup_ConstTexCoord",
	L"COMMON_PrimaryTransparentHitGroup",
	L"COMMON_PrimaryTransparentHitGroup_ConstTexCoord",
	L"COMMON_SecondaryHitGroup",
	L"INDIRECT_PrimaryHitGroup",
	L"INDIRECT_PrimaryHitGroup_ConstTexCoord",
	L"INDIRECT_PrimaryTransparentHitGroup",
	L"INDIRECT_PrimaryTransparentHitGroup_ConstTexCoord",
	L"INDIRECT_SecondaryHitGroup",
	L"LUMINESCENCE_PrimaryHitGroup",
	L"LUMINESCENCE_PrimaryHitGroup_ConstTexCoord",
	L"LUMINESCENCE_PrimaryTransparentHitGroup",
	L"LUMINESCENCE_PrimaryTransparentHitGroup_ConstTexCoord",
	L"LUMINESCENCE_SecondaryHitGroup",
	L"CHR_EYE_PrimaryHitGroup",
	L"CHR_EYE_PrimaryHitGroup_ConstTexCoord",
	L"CHR_EYE_PrimaryTransparentHitGroup",
	L"CHR_EYE_PrimaryTransparentHitGroup_ConstTexCoord",
	L"CHR_EYE_SecondaryHitGroup",
	L"CHR_EYE_FHL_PrimaryHitGroup",
	L"CHR_EYE_FHL_PrimaryHitGroup_ConstTexCoord",
	L"CHR_EYE_FHL_PrimaryTransparentHitGroup",
	L"CHR_EYE_FHL_PrimaryTransparentHitGroup_ConstTexCoord",
	L"CHR_EYE_FHL_SecondaryHitGroup",
	L"CHR_SKIN_PrimaryHitGroup",
	L"CHR_SKIN_PrimaryHitGroup_ConstTexCoord",
	L"CHR_SKIN_PrimaryTransparentHitGroup",
	L"CHR_SKIN_PrimaryTransparentHitGroup_ConstTexCoord",
	L"CHR_SKIN_SecondaryHitGroup",
	L"CHR_SKIN_HALF_PrimaryHitGroup",
	L"CHR_SKIN_HALF_PrimaryHitGroup_ConstTexCoord",
	L"CHR_SKIN_HALF_PrimaryTransparentHitGroup",
	L"CHR_SKIN_HALF_PrimaryTransparentHitGroup_ConstTexCoord",
	L"CHR_SKIN_HALF_SecondaryHitGroup",
	L"CHR_SKIN_IGNORE_PrimaryHitGroup",
	L"CHR_SKIN_IGNORE_PrimaryHitGroup_ConstTexCoord",
	L"CHR_SKIN_IGNORE_PrimaryTransparentHitGroup",
	L"CHR_SKIN_IGNORE_PrimaryTransparentHitGroup_ConstTexCoord",
	L"CHR_SKIN_IGNORE_SecondaryHitGroup",
	L"CLOTH_PrimaryHitGroup",
	L"CLOTH_PrimaryHitGroup_ConstTexCoord",
	L"CLOTH_PrimaryTransparentHitGroup",
	L"CLOTH_PrimaryTransparentHitGroup_ConstTexCoord",
	L"CLOTH_SecondaryHitGroup",
	L"CLOUD_PrimaryHitGroup",
	L"CLOUD_PrimaryHitGroup_ConstTexCoord",
	L"CLOUD_PrimaryTransparentHitGroup",
	L"CLOUD_PrimaryTransparentHitGroup_ConstTexCoord",
	L"CLOUD_SecondaryHitGroup",
	L"DIM_PrimaryHitGroup",
	L"DIM_PrimaryHitGroup_ConstTexCoord",
	L"DIM_PrimaryTransparentHitGroup",
	L"DIM_PrimaryTransparentHitGroup_ConstTexCoord",
	L"DIM_SecondaryHitGroup",
	L"DISTORTION_PrimaryHitGroup",
	L"DISTORTION_PrimaryHitGroup_ConstTexCoord",
	L"DISTORTION_PrimaryTransparentHitGroup",
	L"DISTORTION_PrimaryTransparentHitGroup_ConstTexCoord",
	L"DISTORTION_SecondaryHitGroup",
	L"DISTORTION_OVERLAY_PrimaryHitGroup",
	L"DISTORTION_OVERLAY_PrimaryHitGroup_ConstTexCoord",
	L"DISTORTION_OVERLAY_PrimaryTransparentHitGroup",
	L"DISTORTION_OVERLAY_PrimaryTransparentHitGroup_ConstTexCoord",
	L"DISTORTION_OVERLAY_SecondaryHitGroup",
	L"ENM_EMISSION_PrimaryHitGroup",
	L"ENM_EMISSION_PrimaryHitGroup_ConstTexCoord",
	L"ENM_EMISSION_PrimaryTransparentHitGroup",
	L"ENM_EMISSION_PrimaryTransparentHitGroup_ConstTexCoord",
	L"ENM_EMISSION_SecondaryHitGroup",
	L"ENM_GLASS_PrimaryHitGroup",
	L"ENM_GLASS_PrimaryHitGroup_ConstTexCoord",
	L"ENM_GLASS_PrimaryTransparentHitGroup",
	L"ENM_GLASS_PrimaryTransparentHitGroup_ConstTexCoord",
	L"ENM_GLASS_SecondaryHitGroup",
	L"ENM_IGNORE_PrimaryHitGroup",
	L"ENM_IGNORE_PrimaryHitGroup_ConstTexCoord",
	L"ENM_IGNORE_PrimaryTransparentHitGroup",
	L"ENM_IGNORE_PrimaryTransparentHitGroup_ConstTexCoord",
	L"ENM_IGNORE_SecondaryHitGroup",
	L"FADE_OUT_NORMAL_PrimaryHitGroup",
	L"FADE_OUT_NORMAL_PrimaryHitGroup_ConstTexCoord",
	L"FADE_OUT_NORMAL_PrimaryTransparentHitGroup",
	L"FADE_OUT_NORMAL_PrimaryTransparentHitGroup_ConstTexCoord",
	L"FADE_OUT_NORMAL_SecondaryHitGroup",
	L"FALLOFF_PrimaryHitGroup",
	L"FALLOFF_PrimaryHitGroup_ConstTexCoord",
	L"FALLOFF_PrimaryTransparentHitGroup",
	L"FALLOFF_PrimaryTransparentHitGroup_ConstTexCoord",
	L"FALLOFF_SecondaryHitGroup",
	L"FALLOFF_V_PrimaryHitGroup",
	L"FALLOFF_V_PrimaryHitGroup_ConstTexCoord",
	L"FALLOFF_V_PrimaryTransparentHitGroup",
	L"FALLOFF_V_PrimaryTransparentHitGroup_ConstTexCoord",
	L"FALLOFF_V_SecondaryHitGroup",
	L"FUR_PrimaryHitGroup",
	L"FUR_PrimaryHitGroup_ConstTexCoord",
	L"FUR_PrimaryTransparentHitGroup",
	L"FUR_PrimaryTransparentHitGroup_ConstTexCoord",
	L"FUR_SecondaryHitGroup",
	L"GLASS_PrimaryHitGroup",
	L"GLASS_PrimaryHitGroup_ConstTexCoord",
	L"GLASS_PrimaryTransparentHitGroup",
	L"GLASS_PrimaryTransparentHitGroup_ConstTexCoord",
	L"GLASS_SecondaryHitGroup",
	L"ICE_PrimaryHitGroup",
	L"ICE_PrimaryHitGroup_ConstTexCoord",
	L"ICE_PrimaryTransparentHitGroup",
	L"ICE_PrimaryTransparentHitGroup_ConstTexCoord",
	L"ICE_SecondaryHitGroup",
	L"IGNORE_LIGHT_PrimaryHitGroup",
	L"IGNORE_LIGHT_PrimaryHitGroup_ConstTexCoord",
	L"IGNORE_LIGHT_PrimaryTransparentHitGroup",
	L"IGNORE_LIGHT_PrimaryTransparentHitGroup_ConstTexCoord",
	L"IGNORE_LIGHT_SecondaryHitGroup",
	L"IGNORE_LIGHT_TWICE_PrimaryHitGroup",
	L"IGNORE_LIGHT_TWICE_PrimaryHitGroup_ConstTexCoord",
	L"IGNORE_LIGHT_TWICE_PrimaryTransparentHitGroup",
	L"IGNORE_LIGHT_TWICE_PrimaryTransparentHitGroup_ConstTexCoord",
	L"IGNORE_LIGHT_TWICE_SecondaryHitGroup",
	L"IGNORE_LIGHT_V_PrimaryHitGroup",
	L"IGNORE_LIGHT_V_PrimaryHitGroup_ConstTexCoord",
	L"IGNORE_LIGHT_V_PrimaryTransparentHitGroup",
	L"IGNORE_LIGHT_V_PrimaryTransparentHitGroup_ConstTexCoord",
	L"IGNORE_LIGHT_V_SecondaryHitGroup",
	L"INDIRECT_NO_LIGHT_PrimaryHitGroup",
	L"INDIRECT_NO_LIGHT_PrimaryHitGroup_ConstTexCoord",
	L"INDIRECT_NO_LIGHT_PrimaryTransparentHitGroup",
	L"INDIRECT_NO_LIGHT_PrimaryTransparentHitGroup_ConstTexCoord",
	L"INDIRECT_NO_LIGHT_SecondaryHitGroup",
	L"INDIRECT_V_PrimaryHitGroup",
	L"INDIRECT_V_PrimaryHitGroup_ConstTexCoord",
	L"INDIRECT_V_PrimaryTransparentHitGroup",
	L"INDIRECT_V_PrimaryTransparentHitGroup_ConstTexCoord",
	L"INDIRECT_V_SecondaryHitGroup",
	L"LUMINESCENCE_V_PrimaryHitGroup",
	L"LUMINESCENCE_V_PrimaryHitGroup_ConstTexCoord",
	L"LUMINESCENCE_V_PrimaryTransparentHitGroup",
	L"LUMINESCENCE_V_PrimaryTransparentHitGroup_ConstTexCoord",
	L"LUMINESCENCE_V_SecondaryHitGroup",
	L"METAL_PrimaryHitGroup",
	L"METAL_PrimaryHitGroup_ConstTexCoord",
	L"METAL_PrimaryTransparentHitGroup",
	L"METAL_PrimaryTransparentHitGroup_ConstTexCoord",
	L"METAL_SecondaryHitGroup",
	L"MIRROR_PrimaryHitGroup",
	L"MIRROR_PrimaryHitGroup_ConstTexCoord",
	L"MIRROR_PrimaryTransparentHitGroup",
	L"MIRROR_PrimaryTransparentHitGroup_ConstTexCoord",
	L"MIRROR_SecondaryHitGroup",
	L"RING_PrimaryHitGroup",
	L"RING_PrimaryHitGroup_ConstTexCoord",
	L"RING_PrimaryTransparentHitGroup",
	L"RING_PrimaryTransparentHitGroup_ConstTexCoord",
	L"RING_SecondaryHitGroup",
	L"SHOE_PrimaryHitGroup",
	L"SHOE_PrimaryHitGroup_ConstTexCoord",
	L"SHOE_PrimaryTransparentHitGroup",
	L"SHOE_PrimaryTransparentHitGroup_ConstTexCoord",
	L"SHOE_SecondaryHitGroup",
	L"TIME_EATER_PrimaryHitGroup",
	L"TIME_EATER_PrimaryHitGroup_ConstTexCoord",
	L"TIME_EATER_PrimaryTransparentHitGroup",
	L"TIME_EATER_PrimaryTransparentHitGroup_ConstTexCoord",
	L"TIME_EATER_SecondaryHitGroup",
	L"TRANS_THIN_PrimaryHitGroup",
	L"TRANS_THIN_PrimaryHitGroup_ConstTexCoord",
	L"TRANS_THIN_PrimaryTransparentHitGroup",
	L"TRANS_THIN_PrimaryTransparentHitGroup_ConstTexCoord",
	L"TRANS_THIN_SecondaryHitGroup",
	L"WATER_ADD_PrimaryHitGroup",
	L"WATER_ADD_PrimaryHitGroup_ConstTexCoord",
	L"WATER_ADD_PrimaryTransparentHitGroup",
	L"WATER_ADD_PrimaryTransparentHitGroup_ConstTexCoord",
	L"WATER_ADD_SecondaryHitGroup",
	L"WATER_MUL_PrimaryHitGroup",
	L"WATER_MUL_PrimaryHitGroup_ConstTexCoord",
	L"WATER_MUL_PrimaryTransparentHitGroup",
	L"WATER_MUL_PrimaryTransparentHitGroup_ConstTexCoord",
	L"WATER_MUL_SecondaryHitGroup",
	L"WATER_OPACITY_PrimaryHitGroup",
	L"WATER_OPACITY_PrimaryHitGroup_ConstTexCoord",
	L"WATER_OPACITY_PrimaryTransparentHitGroup",
	L"WATER_OPACITY_PrimaryTransparentHitGroup_ConstTexCoord",
	L"WATER_OPACITY_SecondaryHitGroup",
};
#endif
#endif
