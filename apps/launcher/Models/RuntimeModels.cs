namespace UniversalXRBridge.Launcher.Models;

public sealed record RegisteredRuntime(
    string ManifestPath,
    string DisplayName,
    string? LibraryPath,
    bool ManifestExists,
    bool Enabled,
    bool Active);

public sealed record OpenXrRuntimeSnapshot(
    string? ActiveManifestPath,
    IReadOnlyList<RegisteredRuntime> RegisteredRuntimes,
    DateTimeOffset ObservedAt)
{
    public bool HasActiveRuntime => !string.IsNullOrWhiteSpace(ActiveManifestPath);
    public string StatusText => HasActiveRuntime
        ? "Runtime OpenXR ativo"
        : "Nenhum runtime OpenXR ativo";
}

public sealed record CompatibilityStep(
    int Index,
    string Name,
    string State,
    string Reason,
    bool Available,
    bool Recommended);
