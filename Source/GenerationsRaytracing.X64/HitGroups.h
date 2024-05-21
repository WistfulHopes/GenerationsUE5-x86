#pragma once

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
	L"CHAOS_PrimaryHitGroup",
	L"CHAOS_PrimaryHitGroup_ConstTexCoord",
	L"CHAOS_PrimaryTransparentHitGroup",
	L"CHAOS_PrimaryTransparentHitGroup_ConstTexCoord",
	L"CHAOS_SecondaryHitGroup",
	L"CHR_EYE_PrimaryHitGroup",
	L"CHR_EYE_PrimaryHitGroup_ConstTexCoord",
	L"CHR_EYE_PrimaryTransparentHitGroup",
	L"CHR_EYE_PrimaryTransparentHitGroup_ConstTexCoord",
	L"CHR_EYE_SecondaryHitGroup",
	L"CHR_EYE_FHL_PROCEDURAL_PrimaryHitGroup",
	L"CHR_EYE_FHL_PROCEDURAL_PrimaryHitGroup_ConstTexCoord",
	L"CHR_EYE_FHL_PROCEDURAL_PrimaryTransparentHitGroup",
	L"CHR_EYE_FHL_PROCEDURAL_PrimaryTransparentHitGroup_ConstTexCoord",
	L"CHR_EYE_FHL_PROCEDURAL_SecondaryHitGroup",
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
