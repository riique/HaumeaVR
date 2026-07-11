using UniversalXRBridge.Launcher.Models;

namespace UniversalXRBridge.Launcher.Services;

public sealed record RuntimeCapabilities(
    bool UnityNative,
    bool GeometryStereo,
    bool Depth3D,
    bool VirtualScreen)
{
    public static RuntimeCapabilities Current { get; } = new(
        UnityNative: false,
        GeometryStereo: false,
        Depth3D: false,
        VirtualScreen: true);
}

public static class ProfileFactory
{
    public static GameProfile Create(
        GameCandidate game,
        GameDetection detection,
        RuntimeCapabilities? capabilities = null)
    {
        capabilities ??= RuntimeCapabilities.Current;
        var modes = BuildFallbackOrder(detection, capabilities);
        if (modes.Count == 0)
        {
            throw new InvalidOperationException("Nenhum modo de renderização disponível para o perfil.");
        }

        var safeMode = detection.AntiCheat != AntiCheatStatus.NotDetected;
        return new GameProfile(
            SchemaVersion: 1,
            Id: Guid.NewGuid(),
            DisplayName: game.DisplayName,
            Executable: new ExecutableProfile(game.ExecutablePath, detection.Sha256, []),
            Detection: new DetectionProfile(
                detection.Engine,
                detection.EngineVersion,
                detection.ScriptingBackend,
                detection.GraphicsApi,
                detection.RenderPipeline,
                detection.Architecture,
                detection.AntiCheat,
                detection.AntiCheatProducts),
            Rendering: new RenderingProfile(
                PreferredMode: modes[0],
                FallbackOrder: modes,
                RenderScale: 1.0,
                WorldScale: 1.0,
                ScreenDistanceMeters: 2.0,
                ScreenWidthMeters: 2.4,
                ScreenCurvature: 0.15),
            Camera: new CameraProfile(
                TrackingMode: TrackingMode.RotationOnly,
                PositionScale: 0.0,
                RotationScale: 1.0,
                CutsceneMode: "rotation-only",
                HeightOffsetMeters: 0.0,
                TranslationLimitMeters: 0.25),
            Hud: new HudProfile(
                Mode: HudMode.Original,
                Distance: 1.5,
                Scale: 1.0,
                Curvature: 0.0,
                Opacity: 1.0),
            Input: new InputProfile(ControllerMode.None, []),
            Safety: new SafetyProfile(
                SafeMode: safeMode,
                BlockInjectionWithAntiCheat: true,
                AllowUnsignedModules: false),
            UpdatedAt: DateTimeOffset.UtcNow);
    }

    public static IReadOnlyList<CompatibilityStep> PlanCompatibility(
        GameDetection detection,
        RuntimeCapabilities? capabilities = null)
    {
        capabilities ??= RuntimeCapabilities.Current;
        var antiCheatBlocked = detection.AntiCheat == AntiCheatStatus.Detected;
        var unityCompatible = detection.Engine == GameEngine.Unity
            && detection.ScriptingBackend == ScriptingBackend.Mono
            && detection.RenderPipeline is RenderPipeline.BuiltIn or RenderPipeline.Unknown;
        var d3d11Compatible = detection.GraphicsApi is GraphicsApi.D3D11 or GraphicsApi.Unknown;
        var ordered = BuildFallbackOrder(detection, capabilities);

        return
        [
            Step(1, "Unity Native", capabilities.UnityNative && unityCompatible && !antiCheatBlocked,
                antiCheatBlocked ? "Bloqueado: anti-cheat detectado."
                : !unityCompatible ? "Requer Unity Mono com pipeline Built-in compatível."
                : !capabilities.UnityNative ? "O bridge controlado ainda não está implantado neste player."
                : "Câmera e geometria da Unity disponíveis.", ordered),
            Step(2, "Geometry Stereo", capabilities.GeometryStereo && d3d11Compatible && !antiCheatBlocked,
                antiCheatBlocked ? "Bloqueado: anti-cheat detectado."
                : !d3d11Compatible ? "Requer pipeline Direct3D 11 compatível."
                : !capabilities.GeometryStereo ? "Analisador de matrizes ainda não está disponível neste build."
                : "Duplicação seletiva de draw calls disponível.", ordered),
            Step(3, "Depth 3D", capabilities.Depth3D && d3d11Compatible && !antiCheatBlocked,
                antiCheatBlocked ? "Bloqueado: anti-cheat detectado."
                : !d3d11Compatible ? "Requer captura de depth em Direct3D 11."
                : !capabilities.Depth3D ? "Reconstrução por profundidade ainda não está disponível neste build."
                : "Depth buffer compatível disponível.", ordered),
            Step(4, "Virtual Screen", capabilities.VirtualScreen,
                capabilities.VirtualScreen
                    ? "Fallback externo sem injeção no processo do jogo."
                    : "Modo Virtual Screen indisponível.", ordered),
        ];
    }

    private static CompatibilityStep Step(
        int index,
        string name,
        bool available,
        string reason,
        IReadOnlyList<RenderMode> order)
    {
        var mode = index switch
        {
            1 => RenderMode.UnityNative,
            2 => RenderMode.GeometryStereo,
            3 => RenderMode.Depth3D,
            _ => RenderMode.VirtualScreen,
        };
        var recommended = order.Count > 0 && order[0] == mode;
        return new CompatibilityStep(
            index,
            name,
            recommended ? "RECOMENDADO" : available ? "DISPONÍVEL" : "INDISPONÍVEL",
            reason,
            available,
            recommended);
    }

    private static IReadOnlyList<RenderMode> BuildFallbackOrder(
        GameDetection detection,
        RuntimeCapabilities capabilities)
    {
        if (detection.AntiCheat == AntiCheatStatus.Detected)
        {
            return capabilities.VirtualScreen ? [RenderMode.VirtualScreen] : [];
        }

        var modes = new List<RenderMode>(4);
        if (capabilities.UnityNative
            && detection.Engine == GameEngine.Unity
            && detection.ScriptingBackend == ScriptingBackend.Mono)
        {
            modes.Add(RenderMode.UnityNative);
        }
        if (capabilities.GeometryStereo && detection.GraphicsApi is GraphicsApi.D3D11 or GraphicsApi.Unknown)
        {
            modes.Add(RenderMode.GeometryStereo);
        }
        if (capabilities.Depth3D && detection.GraphicsApi is GraphicsApi.D3D11 or GraphicsApi.Unknown)
        {
            modes.Add(RenderMode.Depth3D);
        }
        if (capabilities.VirtualScreen) modes.Add(RenderMode.VirtualScreen);
        return modes;
    }
}
