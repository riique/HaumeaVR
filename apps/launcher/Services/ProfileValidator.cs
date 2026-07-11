using UniversalXRBridge.Launcher.Models;

namespace UniversalXRBridge.Launcher.Services;

public static class ProfileValidator
{
    public static IReadOnlyList<string> Validate(GameProfile profile)
    {
        var errors = new List<string>();
        if (profile.SchemaVersion != 1) errors.Add("Versão de schema não suportada.");
        if (profile.Id == Guid.Empty) errors.Add("O perfil não possui identificador.");
        if (string.IsNullOrWhiteSpace(profile.DisplayName)) errors.Add("O perfil não possui nome.");
        if (string.IsNullOrWhiteSpace(profile.Executable.Path) || !Path.IsPathFullyQualified(profile.Executable.Path))
            errors.Add("O caminho do executável deve ser absoluto.");
        if (profile.Executable.Sha256 is { Length: > 0 } hash
            && (hash.Length != 64 || hash.Any(character => !Uri.IsHexDigit(character))))
            errors.Add("O SHA-256 do executável é inválido.");

        var modes = profile.Rendering.FallbackOrder;
        if (modes.Count == 0) errors.Add("O perfil precisa de ao menos um modo de renderização.");
        if (modes.Count > 0 && modes[0] != profile.Rendering.PreferredMode)
            errors.Add("O primeiro fallback deve ser o modo preferido.");
        if (modes.Distinct().Count() != modes.Count) errors.Add("A lista de fallback contém modos repetidos.");
        if (profile.Rendering.RenderScale is < 0.25 or > 2.0) errors.Add("Render scale fora do intervalo 0,25–2,0.");
        if (profile.Rendering.ScreenDistanceMeters is < 0.5 or > 10.0) errors.Add("Distância da tela fora do intervalo permitido.");
        if (profile.Rendering.ScreenWidthMeters is < 0.5 or > 10.0) errors.Add("Largura da tela fora do intervalo permitido.");
        if (profile.Rendering.ScreenCurvature is < 0.0 or > 1.0) errors.Add("Curvatura fora do intervalo 0–1.");
        if (!profile.Safety.BlockInjectionWithAntiCheat) errors.Add("O bloqueio de injeção com anti-cheat é obrigatório.");
        if (profile.Safety.AllowUnsignedModules) errors.Add("Módulos não assinados não podem ser habilitados por perfil.");

        if (profile.Detection.AntiCheat == AntiCheatStatus.Detected)
        {
            if (modes.Any(mode => mode != RenderMode.VirtualScreen))
                errors.Add("Perfis com anti-cheat só podem usar Virtual Screen.");
            if (!profile.Safety.SafeMode) errors.Add("Perfis com anti-cheat devem permanecer em modo seguro.");
        }
        return errors;
    }

    public static void EnsureValid(GameProfile profile)
    {
        var errors = Validate(profile);
        if (errors.Count > 0) throw new InvalidDataException(string.Join(Environment.NewLine, errors));
    }
}
