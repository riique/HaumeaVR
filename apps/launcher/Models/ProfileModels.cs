using System.Text.Json.Serialization;

namespace UniversalXRBridge.Launcher.Models;

[JsonConverter(typeof(JsonStringEnumConverter<RenderMode>))]
public enum RenderMode
{
    [JsonStringEnumMemberName("unity-native")]
    UnityNative,
    [JsonStringEnumMemberName("geometry-stereo")]
    GeometryStereo,
    [JsonStringEnumMemberName("depth-3d")]
    Depth3D,
    [JsonStringEnumMemberName("virtual-screen")]
    VirtualScreen,
}

[JsonConverter(typeof(JsonStringEnumConverter<TrackingMode>))]
public enum TrackingMode
{
    [JsonStringEnumMemberName("coupled")]
    Coupled,
    [JsonStringEnumMemberName("decoupled")]
    Decoupled,
    [JsonStringEnumMemberName("hybrid")]
    Hybrid,
    [JsonStringEnumMemberName("rotation-only")]
    RotationOnly,
    [JsonStringEnumMemberName("six-dof")]
    SixDof,
}

[JsonConverter(typeof(JsonStringEnumConverter<HudMode>))]
public enum HudMode
{
    [JsonStringEnumMemberName("head-locked")]
    HeadLocked,
    [JsonStringEnumMemberName("world")]
    World,
    [JsonStringEnumMemberName("curved")]
    Curved,
    [JsonStringEnumMemberName("original")]
    Original,
    [JsonStringEnumMemberName("openxr-layer")]
    OpenXrLayer,
}

[JsonConverter(typeof(JsonStringEnumConverter<ControllerMode>))]
public enum ControllerMode
{
    [JsonStringEnumMemberName("none")]
    None,
    [JsonStringEnumMemberName("xinput")]
    XInput,
    [JsonStringEnumMemberName("keyboard-mouse")]
    KeyboardMouse,
    [JsonStringEnumMemberName("mixed")]
    Mixed,
}

public sealed record ExecutableProfile(string Path, string? Sha256, IReadOnlyList<string> LaunchArguments);

public sealed record DetectionProfile(
    GameEngine Engine,
    string? EngineVersion,
    ScriptingBackend ScriptingBackend,
    GraphicsApi GraphicsApi,
    RenderPipeline RenderPipeline,
    ExecutableArchitecture Architecture,
    AntiCheatStatus AntiCheat,
    IReadOnlyList<string> AntiCheatProducts);

public sealed record RenderingProfile(
    RenderMode PreferredMode,
    IReadOnlyList<RenderMode> FallbackOrder,
    double RenderScale,
    double WorldScale,
    double ScreenDistanceMeters,
    double ScreenWidthMeters,
    double ScreenCurvature);

public sealed record CameraProfile(
    TrackingMode TrackingMode,
    double PositionScale,
    double RotationScale,
    string CutsceneMode,
    double HeightOffsetMeters,
    double TranslationLimitMeters);

public sealed record HudProfile(HudMode Mode, double Distance, double Scale, double Curvature, double Opacity);

public sealed record InputBinding(string Source, string Target, double Scale = 1.0, double Deadzone = 0.1);

public sealed record InputProfile(ControllerMode ControllerMode, IReadOnlyList<InputBinding> Bindings);

public sealed record SafetyProfile(bool SafeMode, bool BlockInjectionWithAntiCheat, bool AllowUnsignedModules);

public sealed record GameProfile(
    int SchemaVersion,
    Guid Id,
    string DisplayName,
    ExecutableProfile Executable,
    DetectionProfile Detection,
    RenderingProfile Rendering,
    CameraProfile Camera,
    HudProfile Hud,
    InputProfile Input,
    SafetyProfile Safety,
    DateTimeOffset UpdatedAt);
