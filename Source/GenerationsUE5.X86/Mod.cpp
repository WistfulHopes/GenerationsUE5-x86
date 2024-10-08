#include "Camera.h"
#include "ModelData.h"
#include "Configuration.h"
#include "D3D9.h"
#include "PictureData.h"
#include "FillTexture.h"
#include "HalfPixel.h"
#include "MaterialData.h"
#include "MessageSender.h"
#include "ProcessUtil.h"
#include "RaytracingRendering.h"
#include "InstanceData.h"
#include "MemoryAllocator.h"
#include "MeshData.h"
#include "MotionData.h"
#include "RaytracingParams.h"
#include "ShaderCache.h"
#include "Sofdec.h"
#include "ToneMap.h"
#include "TriangleStrip.h"
#include "Window.h"
#include "FileBinder.h"
#include "GroundSmokeParticle.h"
#include "LightData.h"
#include "ModelReplacer.h"
#include "QuickBoot.h"
#include "RopeRenderable.h"
#include "ShareVertexBuffer.h"
#include "SoundSystem.h"
#include "StageSelection.h"
#include "TerrainData.h"
#include "UpReelRenderable.h"
#include "SampleChunkResource.h"
#include "MetaInstancer.h"
#include "SonicPlayer.h"
#include "WallJumpBlock.h"

static constexpr LPCTSTR s_bridgeProcessNameDevelopment = TEXT("GenerationsUE5.exe");
static constexpr LPCTSTR s_bridgeProcessNameShipping = TEXT("UE5\\GenerationsUE5\\Binaries\\Win64\\GenerationsUE5-Win64-Shipping.exe");
static constexpr LPCTSTR s_bridgeProcessNameEditor = TEXT("UnrealEditor.exe");

static struct ThreadHolder
{
    std::thread processThread;
    HANDLE event;

    ThreadHolder()
        : event(CreateEvent(nullptr, FALSE, FALSE, nullptr))
    {
    }

    ~ThreadHolder()
    {
        SetEvent(event);

        if (processThread.joinable())
            processThread.join();

        CloseHandle(event);
    }
} s_threadHolder;

static void setShouldExit()
{
    *s_shouldExit = true;
    s_messageSender.notifyShouldExit();
}

extern "C" void __declspec(dllexport) FilterMod(FilterModArguments_t& args)
{
    args.handled = strcmp(args.mod->Name, "Direct3D 9 Ex") == 0 || strcmp(args.mod->ID, "bsthlc.generationsd3d11") == 0;
}

extern "C" void __declspec(dllexport) PreInit()
{
    ToneMap::preInit();
}

extern "C" void __declspec(dllexport) Init(ModInfo_t* modInfo)
{
    Configuration::init();
    D3D9::init();
    PictureData::init();
    FillTexture::init();
    // Window::init();
    HalfPixel::init();
    TriangleStrip::init();
    ModelData::init();
    InstanceData::init();
    // RaytracingRendering::init();
    MaterialData::init();
    Sofdec::init();
    ShaderCache::init();
    MotionData::init();
    MemoryAllocator::init();
    MeshData::init();
    FileBinder::init(modInfo);
    QuickBoot::init();
    ModelReplacer::init(modInfo);
    ShareVertexBuffer::init();
    UpReelRenderable::init();
    RopeRenderable::init();
    SoundSystem::init();
    StageSelection::init(modInfo);
    GroundSmokeParticle::init();
    TerrainData::init();
    LightData::init(modInfo);
    SampleChunkResource::init();
    MetaInstancer::init();
    WallJumpBlock::init();
    RaytracingParams::init();
    InstallSonicPlayer::applyPatches();
    InstallCamera::applyPatches();

#ifdef _DEBUG
#if 0
    MessageBox(NULL, TEXT("Attach to Process"), TEXT("Generations Raytracing"), MB_ICONINFORMATION | MB_OK);

    if (!GetConsoleWindow())
        AllocConsole();

    freopen("CONOUT$", "w", stdout);
#endif
#else
    terminateProcess(s_bridgeProcessNameShipping);
#endif

    const auto process = findProcess(s_bridgeProcessNameEditor);
    HANDLE processHandle = nullptr;

    if (!process)
    {
        STARTUPINFO startupInfo{};
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInformation{};

        const BOOL result = CreateProcess(
            s_bridgeProcessNameShipping,
            nullptr,
            nullptr,
            nullptr,
            FALSE,
#ifdef _DEBUG 
            0,
#else
            CREATE_NO_WINDOW | INHERIT_PARENT_AFFINITY,
#endif
            nullptr,
            nullptr,
            &startupInfo,
            &processInformation);

        assert(result == TRUE);

        processHandle = processInformation.hProcess;
    }
    else
    {
        processHandle = OpenProcess(SYNCHRONIZE, FALSE, *process);
    }

    assert(processHandle != nullptr);

    s_threadHolder.processThread = std::thread([processHandle]
    {
        const HANDLE handles[] = { s_threadHolder.event, processHandle };
        WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
        CloseHandle(processHandle);

        setShouldExit();
    });
}

extern "C" void __declspec(dllexport) OnFrame()
{
    s_messageSender.commitMessages();
}

extern "C" void __declspec(dllexport) PostInit()
{
    RaytracingRendering::postInit();
    ToneMap::postInit();
    MaterialData::postInit();
}