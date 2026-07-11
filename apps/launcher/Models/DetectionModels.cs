using System.Text.Json.Serialization;

namespace UniversalXRBridge.Launcher.Models;

[JsonConverter(typeof(JsonStringEnumConverter<GameEngine>))]
public enum GameEngine
{
    [JsonStringEnumMemberName("unknown")]
    Unknown,
    [JsonStringEnumMemberName("unity")]
    Unity,
    [JsonStringEnumMemberName("unreal")]
    Unreal,
    [JsonStringEnumMemberName("generic")]
    Generic,
}

[JsonConverter(typeof(JsonStringEnumConverter<ScriptingBackend>))]
public enum ScriptingBackend
{
    [JsonStringEnumMemberName("unknown")]
    Unknown,
    [JsonStringEnumMemberName("mono")]
    Mono,
    [JsonStringEnumMemberName("il2cpp")]
    Il2Cpp,
    [JsonStringEnumMemberName("native")]
    Native,
}

[JsonConverter(typeof(JsonStringEnumConverter<GraphicsApi>))]
public enum GraphicsApi
{
    [JsonStringEnumMemberName("unknown")]
    Unknown,
    [JsonStringEnumMemberName("d3d11")]
    D3D11,
    [JsonStringEnumMemberName("d3d12")]
    D3D12,
    [JsonStringEnumMemberName("vulkan")]
    Vulkan,
    [JsonStringEnumMemberName("opengl")]
    OpenGl,
}

[JsonConverter(typeof(JsonStringEnumConverter<RenderPipeline>))]
public enum RenderPipeline
{
    [JsonStringEnumMemberName("unknown")]
    Unknown,
    [JsonStringEnumMemberName("builtin")]
    BuiltIn,
    [JsonStringEnumMemberName("urp")]
    Urp,
    [JsonStringEnumMemberName("hdrp")]
    Hdrp,
    [JsonStringEnumMemberName("custom")]
    Custom,
}

[JsonConverter(typeof(JsonStringEnumConverter<ExecutableArchitecture>))]
public enum ExecutableArchitecture
{
    [JsonStringEnumMemberName("unknown")]
    Unknown,
    [JsonStringEnumMemberName("x86")]
    X86,
    [JsonStringEnumMemberName("x64")]
    X64,
    [JsonStringEnumMemberName("arm64")]
    Arm64,
}

[JsonConverter(typeof(JsonStringEnumConverter<AntiCheatStatus>))]
public enum AntiCheatStatus
{
    [JsonStringEnumMemberName("unknown")]
    Unknown,
    [JsonStringEnumMemberName("not-detected")]
    NotDetected,
    [JsonStringEnumMemberName("detected")]
    Detected,
}

public sealed record GameDetection(
    GameEngine Engine,
    string? EngineVersion,
    ScriptingBackend ScriptingBackend,
    GraphicsApi GraphicsApi,
    RenderPipeline RenderPipeline,
    ExecutableArchitecture Architecture,
    AntiCheatStatus AntiCheat,
    IReadOnlyList<string> AntiCheatProducts,
    IReadOnlyList<string> Signals,
    string Sha256);

public sealed record GameCandidate(
    string Id,
    string DisplayName,
    string ExecutablePath,
    string Source,
    string? StoreId = null);
