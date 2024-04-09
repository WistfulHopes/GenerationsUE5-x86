#pragma once

#include "ShaderType.h"
#include "Material.hlsli"

struct MaterialData
{
    uint ShaderType : 16;
    uint Flags : 16;
    float4 TexCoordOffsets[2];
    uint Textures[9];
    float Parameters[22];
};
Material GetMaterial(uint shaderType, MaterialData materialData)
{
   Material material = (Material) 0;
   material.Flags = materialData.Flags;
   
   switch (shaderType)
   {
    case SHADER_TYPE_SYS_ERROR:
        material.DiffuseTexture = materialData.Textures[0];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Opacity = materialData.Parameters[4];
        break;
    case SHADER_TYPE_BLEND:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.GlossTexture = materialData.Textures[2];
        material.NormalTexture = materialData.Textures[3];
        material.OpacityTexture = materialData.Textures[4];
        material.DiffuseTexture2 = materialData.Textures[5];
        material.SpecularTexture2 = materialData.Textures[6];
        material.GlossTexture2 = materialData.Textures[7];
        material.NormalTexture2 = materialData.Textures[8];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        break;
    case SHADER_TYPE_COMMON:
        material.DiffuseTexture = materialData.Textures[0];
        material.OpacityTexture = materialData.Textures[1];
        material.SpecularTexture = materialData.Textures[2];
        material.GlossTexture = materialData.Textures[3];
        material.NormalTexture = materialData.Textures[4];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        break;
    case SHADER_TYPE_INDIRECT:
        material.DiffuseTexture = materialData.Textures[0];
        material.GlossTexture = materialData.Textures[1];
        material.NormalTexture = materialData.Textures[2];
        material.DisplacementTexture = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.OffsetParam[0] = materialData.Parameters[10];
        material.OffsetParam[1] = materialData.Parameters[11];
        break;
    case SHADER_TYPE_LUMINESCENCE:
        material.DiffuseTexture = materialData.Textures[0];
        material.GlossTexture = materialData.Textures[1];
        material.NormalTexture = materialData.Textures[2];
        material.DisplacementTexture = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Emissive[0] = materialData.Parameters[4];
        material.Emissive[1] = materialData.Parameters[5];
        material.Emissive[2] = materialData.Parameters[6];
        material.Specular[0] = materialData.Parameters[7];
        material.Specular[1] = materialData.Parameters[8];
        material.Specular[2] = materialData.Parameters[9];
        material.GlossLevel[0] = materialData.Parameters[10];
        material.GlossLevel[1] = materialData.Parameters[11];
        material.Opacity = materialData.Parameters[12];
        material.Ambient[0] = materialData.Parameters[13];
        material.Ambient[1] = materialData.Parameters[14];
        material.Ambient[2] = materialData.Parameters[15];
        break;
    case SHADER_TYPE_CHR_EYE:
        material.DiffuseTexture = materialData.Textures[0];
        material.GlossTexture = materialData.Textures[1];
        material.NormalTexture = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.SonicEyeHighLightPosition[0] = materialData.Parameters[10];
        material.SonicEyeHighLightPosition[1] = materialData.Parameters[11];
        material.SonicEyeHighLightPosition[2] = materialData.Parameters[12];
        material.SonicEyeHighLightColor[0] = materialData.Parameters[13];
        material.SonicEyeHighLightColor[1] = materialData.Parameters[14];
        material.SonicEyeHighLightColor[2] = materialData.Parameters[15];
        break;
    case SHADER_TYPE_CHR_EYE_FHL:
        material.DiffuseTexture = materialData.Textures[0];
        material.LevelTexture = materialData.Textures[1];
        material.DisplacementTexture = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.ChrEyeFHL1[0] = materialData.Parameters[10];
        material.ChrEyeFHL1[1] = materialData.Parameters[11];
        material.ChrEyeFHL1[2] = materialData.Parameters[12];
        material.ChrEyeFHL1[3] = materialData.Parameters[13];
        material.ChrEyeFHL2[0] = materialData.Parameters[14];
        material.ChrEyeFHL2[1] = materialData.Parameters[15];
        material.ChrEyeFHL2[2] = materialData.Parameters[16];
        material.ChrEyeFHL2[3] = materialData.Parameters[17];
        material.ChrEyeFHL3[0] = materialData.Parameters[18];
        material.ChrEyeFHL3[1] = materialData.Parameters[19];
        material.ChrEyeFHL3[2] = materialData.Parameters[20];
        material.ChrEyeFHL3[3] = materialData.Parameters[21];
        break;
    case SHADER_TYPE_CHR_SKIN:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.GlossTexture = materialData.Textures[2];
        material.NormalTexture = materialData.Textures[3];
        material.DisplacementTexture = materialData.Textures[4];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.SonicSkinFalloffParam[0] = materialData.Parameters[10];
        material.SonicSkinFalloffParam[1] = materialData.Parameters[11];
        material.SonicSkinFalloffParam[2] = materialData.Parameters[12];
        break;
    case SHADER_TYPE_CHR_SKIN_HALF:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.ReflectionTexture = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.SonicSkinFalloffParam[0] = materialData.Parameters[10];
        material.SonicSkinFalloffParam[1] = materialData.Parameters[11];
        material.SonicSkinFalloffParam[2] = materialData.Parameters[12];
        break;
    case SHADER_TYPE_CHR_SKIN_IGNORE:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.DisplacementTexture = materialData.Textures[2];
        material.ReflectionTexture = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Ambient[0] = materialData.Parameters[4];
        material.Ambient[1] = materialData.Parameters[5];
        material.Ambient[2] = materialData.Parameters[6];
        material.Specular[0] = materialData.Parameters[7];
        material.Specular[1] = materialData.Parameters[8];
        material.Specular[2] = materialData.Parameters[9];
        material.GlossLevel[0] = materialData.Parameters[10];
        material.GlossLevel[1] = materialData.Parameters[11];
        material.Opacity = materialData.Parameters[12];
        material.SonicSkinFalloffParam[0] = materialData.Parameters[13];
        material.SonicSkinFalloffParam[1] = materialData.Parameters[14];
        material.SonicSkinFalloffParam[2] = materialData.Parameters[15];
        material.ChrEmissionParam[0] = materialData.Parameters[16];
        material.ChrEmissionParam[1] = materialData.Parameters[17];
        material.ChrEmissionParam[2] = materialData.Parameters[18];
        material.ChrEmissionParam[3] = materialData.Parameters[19];
        break;
    case SHADER_TYPE_CLOUD:
        material.NormalTexture = materialData.Textures[0];
        material.DisplacementTexture = materialData.Textures[1];
        material.ReflectionTexture = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.SonicSkinFalloffParam[0] = materialData.Parameters[10];
        material.SonicSkinFalloffParam[1] = materialData.Parameters[11];
        material.SonicSkinFalloffParam[2] = materialData.Parameters[12];
        break;
    case SHADER_TYPE_DIM:
        material.DiffuseTexture = materialData.Textures[0];
        material.GlossTexture = materialData.Textures[1];
        material.NormalTexture = materialData.Textures[2];
        material.ReflectionTexture = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Ambient[0] = materialData.Parameters[4];
        material.Ambient[1] = materialData.Parameters[5];
        material.Ambient[2] = materialData.Parameters[6];
        material.Specular[0] = materialData.Parameters[7];
        material.Specular[1] = materialData.Parameters[8];
        material.Specular[2] = materialData.Parameters[9];
        material.GlossLevel[0] = materialData.Parameters[10];
        material.GlossLevel[1] = materialData.Parameters[11];
        material.Opacity = materialData.Parameters[12];
        break;
    case SHADER_TYPE_DISTORTION:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.NormalTexture = materialData.Textures[2];
        material.NormalTexture2 = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        break;
    case SHADER_TYPE_DISTORTION_OVERLAY:
        material.DiffuseTexture = materialData.Textures[0];
        material.NormalTexture = materialData.Textures[1];
        material.NormalTexture2 = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Opacity = materialData.Parameters[4];
        break;
    case SHADER_TYPE_ENM_EMISSION:
        material.DiffuseTexture = materialData.Textures[0];
        material.NormalTexture = materialData.Textures[1];
        material.SpecularTexture = materialData.Textures[2];
        material.DisplacementTexture = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Ambient[0] = materialData.Parameters[4];
        material.Ambient[1] = materialData.Parameters[5];
        material.Ambient[2] = materialData.Parameters[6];
        material.Specular[0] = materialData.Parameters[7];
        material.Specular[1] = materialData.Parameters[8];
        material.Specular[2] = materialData.Parameters[9];
        material.GlossLevel[0] = materialData.Parameters[10];
        material.GlossLevel[1] = materialData.Parameters[11];
        material.Opacity = materialData.Parameters[12];
        material.ChrEmissionParam[0] = materialData.Parameters[13];
        material.ChrEmissionParam[1] = materialData.Parameters[14];
        material.ChrEmissionParam[2] = materialData.Parameters[15];
        material.ChrEmissionParam[3] = materialData.Parameters[16];
        break;
    case SHADER_TYPE_ENM_GLASS:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.NormalTexture = materialData.Textures[2];
        material.DisplacementTexture = materialData.Textures[3];
        material.ReflectionTexture = materialData.Textures[4];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Ambient[0] = materialData.Parameters[4];
        material.Ambient[1] = materialData.Parameters[5];
        material.Ambient[2] = materialData.Parameters[6];
        material.Specular[0] = materialData.Parameters[7];
        material.Specular[1] = materialData.Parameters[8];
        material.Specular[2] = materialData.Parameters[9];
        material.GlossLevel[0] = materialData.Parameters[10];
        material.GlossLevel[1] = materialData.Parameters[11];
        material.Opacity = materialData.Parameters[12];
        material.ChrEmissionParam[0] = materialData.Parameters[13];
        material.ChrEmissionParam[1] = materialData.Parameters[14];
        material.ChrEmissionParam[2] = materialData.Parameters[15];
        material.ChrEmissionParam[3] = materialData.Parameters[16];
        break;
    case SHADER_TYPE_ENM_IGNORE:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.DisplacementTexture = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Ambient[0] = materialData.Parameters[4];
        material.Ambient[1] = materialData.Parameters[5];
        material.Ambient[2] = materialData.Parameters[6];
        material.Specular[0] = materialData.Parameters[7];
        material.Specular[1] = materialData.Parameters[8];
        material.Specular[2] = materialData.Parameters[9];
        material.GlossLevel[0] = materialData.Parameters[10];
        material.GlossLevel[1] = materialData.Parameters[11];
        material.Opacity = materialData.Parameters[12];
        material.ChrEmissionParam[0] = materialData.Parameters[13];
        material.ChrEmissionParam[1] = materialData.Parameters[14];
        material.ChrEmissionParam[2] = materialData.Parameters[15];
        material.ChrEmissionParam[3] = materialData.Parameters[16];
        break;
    case SHADER_TYPE_FADE_OUT_NORMAL:
        material.DiffuseTexture = materialData.Textures[0];
        material.NormalTexture = materialData.Textures[1];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        break;
    case SHADER_TYPE_FALLOFF:
        material.DiffuseTexture = materialData.Textures[0];
        material.NormalTexture = materialData.Textures[1];
        material.GlossTexture = materialData.Textures[2];
        material.DisplacementTexture = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        break;
    case SHADER_TYPE_FALLOFF_V:
        material.DiffuseTexture = materialData.Textures[0];
        material.GlossTexture = materialData.Textures[1];
        material.NormalTexture = materialData.Textures[2];
        material.NormalTexture2 = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        break;
    case SHADER_TYPE_FUR:
        material.DiffuseTexture = materialData.Textures[0];
        material.DiffuseTexture2 = materialData.Textures[1];
        material.SpecularTexture = materialData.Textures[2];
        material.NormalTexture = materialData.Textures[3];
        material.NormalTexture2 = materialData.Textures[4];
        material.DisplacementTexture = materialData.Textures[5];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.SonicSkinFalloffParam[0] = materialData.Parameters[10];
        material.SonicSkinFalloffParam[1] = materialData.Parameters[11];
        material.SonicSkinFalloffParam[2] = materialData.Parameters[12];
        material.FurParam[0] = materialData.Parameters[13];
        material.FurParam[1] = materialData.Parameters[14];
        material.FurParam[2] = materialData.Parameters[15];
        material.FurParam[3] = materialData.Parameters[16];
        material.FurParam2[0] = materialData.Parameters[17];
        material.FurParam2[1] = materialData.Parameters[18];
        material.FurParam2[2] = materialData.Parameters[19];
        material.FurParam2[3] = materialData.Parameters[20];
        break;
    case SHADER_TYPE_GLASS:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.GlossTexture = materialData.Textures[2];
        material.NormalTexture = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.FresnelParam[0] = materialData.Parameters[10];
        material.FresnelParam[1] = materialData.Parameters[11];
        break;
    case SHADER_TYPE_ICE:
        material.DiffuseTexture = materialData.Textures[0];
        material.GlossTexture = materialData.Textures[1];
        material.NormalTexture = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        break;
    case SHADER_TYPE_IGNORE_LIGHT:
        material.DiffuseTexture = materialData.Textures[0];
        material.OpacityTexture = materialData.Textures[1];
        material.DisplacementTexture = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Ambient[0] = materialData.Parameters[4];
        material.Ambient[1] = materialData.Parameters[5];
        material.Ambient[2] = materialData.Parameters[6];
        material.Opacity = materialData.Parameters[7];
        material.EmissionParam[0] = materialData.Parameters[8];
        material.EmissionParam[1] = materialData.Parameters[9];
        material.EmissionParam[2] = materialData.Parameters[10];
        material.EmissionParam[3] = materialData.Parameters[11];
        break;
    case SHADER_TYPE_IGNORE_LIGHT_TWICE:
        material.DiffuseTexture = materialData.Textures[0];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Opacity = materialData.Parameters[4];
        break;
    case SHADER_TYPE_INDIRECT_NO_LIGHT:
        material.DiffuseTexture = materialData.Textures[0];
        material.DisplacementTexture = materialData.Textures[1];
        material.DisplacementTexture2 = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Opacity = materialData.Parameters[4];
        material.OffsetParam[0] = materialData.Parameters[5];
        material.OffsetParam[1] = materialData.Parameters[6];
        break;
    case SHADER_TYPE_INDIRECT_V:
        material.DiffuseTexture = materialData.Textures[0];
        material.OpacityTexture = materialData.Textures[1];
        material.GlossTexture = materialData.Textures[2];
        material.NormalTexture = materialData.Textures[3];
        material.DisplacementTexture = materialData.Textures[4];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.OffsetParam[0] = materialData.Parameters[10];
        material.OffsetParam[1] = materialData.Parameters[11];
        break;
    case SHADER_TYPE_LUMINESCENCE_V:
        material.DiffuseTexture = materialData.Textures[0];
        material.GlossTexture = materialData.Textures[1];
        material.NormalTexture = materialData.Textures[2];
        material.DisplacementTexture = materialData.Textures[3];
        material.DiffuseTexture2 = materialData.Textures[4];
        material.GlossTexture2 = materialData.Textures[5];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Ambient[0] = materialData.Parameters[4];
        material.Ambient[1] = materialData.Parameters[5];
        material.Ambient[2] = materialData.Parameters[6];
        material.Specular[0] = materialData.Parameters[7];
        material.Specular[1] = materialData.Parameters[8];
        material.Specular[2] = materialData.Parameters[9];
        material.GlossLevel[0] = materialData.Parameters[10];
        material.GlossLevel[1] = materialData.Parameters[11];
        material.Opacity = materialData.Parameters[12];
        break;
    case SHADER_TYPE_METAL:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.GlossTexture = materialData.Textures[2];
        material.NormalTexture = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        break;
    case SHADER_TYPE_MIRROR:
        material.DiffuseTexture = materialData.Textures[0];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Opacity = materialData.Parameters[4];
        material.FresnelParam[0] = materialData.Parameters[5];
        material.FresnelParam[1] = materialData.Parameters[6];
        break;
    case SHADER_TYPE_RING:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.ReflectionTexture = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.LuminanceRange = materialData.Parameters[10];
        break;
    case SHADER_TYPE_SHOE:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.GlossTexture = materialData.Textures[2];
        material.NormalTexture = materialData.Textures[3];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        break;
    case SHADER_TYPE_TIME_EATER:
        material.DiffuseTexture = materialData.Textures[0];
        material.SpecularTexture = materialData.Textures[1];
        material.OpacityTexture = materialData.Textures[2];
        material.NormalTexture = materialData.Textures[3];
        material.NormalTexture2 = materialData.Textures[4];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.Opacity = materialData.Parameters[7];
        material.GlossLevel[0] = materialData.Parameters[8];
        material.GlossLevel[1] = materialData.Parameters[9];
        material.SonicSkinFalloffParam[0] = materialData.Parameters[10];
        material.SonicSkinFalloffParam[1] = materialData.Parameters[11];
        material.SonicSkinFalloffParam[2] = materialData.Parameters[12];
        break;
    case SHADER_TYPE_TRANS_THIN:
        material.DiffuseTexture = materialData.Textures[0];
        material.GlossTexture = materialData.Textures[1];
        material.NormalTexture = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.TransColorMask[0] = materialData.Parameters[10];
        material.TransColorMask[1] = materialData.Parameters[11];
        material.TransColorMask[2] = materialData.Parameters[12];
        break;
    case SHADER_TYPE_WATER_ADD:
        material.DiffuseTexture = materialData.Textures[0];
        material.NormalTexture = materialData.Textures[1];
        material.NormalTexture2 = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.WaterParam[0] = materialData.Parameters[10];
        material.WaterParam[1] = materialData.Parameters[11];
        material.WaterParam[2] = materialData.Parameters[12];
        material.WaterParam[3] = materialData.Parameters[13];
        break;
    case SHADER_TYPE_WATER_MUL:
        material.DiffuseTexture = materialData.Textures[0];
        material.NormalTexture = materialData.Textures[1];
        material.NormalTexture2 = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Specular[0] = materialData.Parameters[4];
        material.Specular[1] = materialData.Parameters[5];
        material.Specular[2] = materialData.Parameters[6];
        material.GlossLevel[0] = materialData.Parameters[7];
        material.GlossLevel[1] = materialData.Parameters[8];
        material.Opacity = materialData.Parameters[9];
        material.WaterParam[0] = materialData.Parameters[10];
        material.WaterParam[1] = materialData.Parameters[11];
        material.WaterParam[2] = materialData.Parameters[12];
        material.WaterParam[3] = materialData.Parameters[13];
        break;
    case SHADER_TYPE_WATER_OPACITY:
        material.DiffuseTexture = materialData.Textures[0];
        material.NormalTexture = materialData.Textures[1];
        material.NormalTexture2 = materialData.Textures[2];
        material.Diffuse[0] = materialData.Parameters[0];
        material.Diffuse[1] = materialData.Parameters[1];
        material.Diffuse[2] = materialData.Parameters[2];
        material.Diffuse[3] = materialData.Parameters[3];
        material.Opacity = materialData.Parameters[4];
        material.Specular[0] = materialData.Parameters[5];
        material.Specular[1] = materialData.Parameters[6];
        material.Specular[2] = materialData.Parameters[7];
        material.GlossLevel[0] = materialData.Parameters[8];
        material.GlossLevel[1] = materialData.Parameters[9];
        material.WaterParam[0] = materialData.Parameters[10];
        material.WaterParam[1] = materialData.Parameters[11];
        material.WaterParam[2] = materialData.Parameters[12];
        material.WaterParam[3] = materialData.Parameters[13];
        break;
   }
   return material;
}
