using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using Microsoft.Win32;
using UniversalXRBridge.Launcher.Models;

namespace UniversalXRBridge.Launcher.Services;

public sealed partial class GameDiscoveryService
{
    public Task<IReadOnlyList<GameCandidate>> DiscoverAsync(CancellationToken cancellationToken = default) =>
        Task.Run(() => Discover(cancellationToken), cancellationToken);

    public GameCandidate FromManualPath(string executablePath)
    {
        var fullPath = Path.GetFullPath(executablePath);
        if (!File.Exists(fullPath)) throw new FileNotFoundException("O executável selecionado não existe.", fullPath);
        var productName = System.Diagnostics.FileVersionInfo.GetVersionInfo(fullPath).ProductName;
        return new GameCandidate(
            StableId(fullPath),
            string.IsNullOrWhiteSpace(productName) ? Path.GetFileNameWithoutExtension(fullPath) : productName.Trim(),
            fullPath,
            "Manual");
    }

    private static IReadOnlyList<GameCandidate> Discover(CancellationToken cancellationToken)
    {
        var candidates = new Dictionary<string, GameCandidate>(StringComparer.OrdinalIgnoreCase);
        foreach (var game in DiscoverSteam(cancellationToken).Concat(DiscoverEpic(cancellationToken)))
        {
            candidates.TryAdd(game.ExecutablePath, game);
        }
        return candidates.Values
            .OrderBy(game => game.DisplayName, StringComparer.CurrentCultureIgnoreCase)
            .ToArray();
    }

    private static IEnumerable<GameCandidate> DiscoverSteam(CancellationToken cancellationToken)
    {
        var roots = FindSteamRoots().ToHashSet(StringComparer.OrdinalIgnoreCase);
        foreach (var root in roots.ToArray())
        {
            var libraryFile = Path.Combine(root, "steamapps", "libraryfolders.vdf");
            if (!File.Exists(libraryFile)) continue;
            try
            {
                var content = File.ReadAllText(libraryFile);
                foreach (Match match in VdfPathRegex().Matches(content))
                {
                    roots.Add(match.Groups[1].Value.Replace("\\\\", "\\", StringComparison.Ordinal));
                }
            }
            catch (IOException)
            {
                // Continue with the primary Steam directory.
            }
        }

        foreach (var library in roots)
        {
            cancellationToken.ThrowIfCancellationRequested();
            var steamApps = Path.Combine(library, "steamapps");
            if (!Directory.Exists(steamApps)) continue;
            IEnumerable<string> manifests;
            try { manifests = Directory.EnumerateFiles(steamApps, "appmanifest_*.acf").ToArray(); }
            catch (IOException) { continue; }

            foreach (var manifest in manifests)
            {
                cancellationToken.ThrowIfCancellationRequested();
                string content;
                try { content = File.ReadAllText(manifest); }
                catch (IOException) { continue; }
                var name = MatchVdfValue(content, "name");
                var installDirectory = MatchVdfValue(content, "installdir");
                var appId = MatchVdfValue(content, "appid");
                if (string.IsNullOrWhiteSpace(name) || string.IsNullOrWhiteSpace(installDirectory)) continue;

                var gameRoot = Path.Combine(steamApps, "common", installDirectory);
                var executable = SelectLikelyExecutable(gameRoot, name, cancellationToken);
                if (executable is null) continue;
                yield return new GameCandidate(StableId(executable), name, executable, "Steam", appId);
            }
        }
    }

    private static IEnumerable<GameCandidate> DiscoverEpic(CancellationToken cancellationToken)
    {
        var programData = Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData);
        var manifestRoot = Path.Combine(programData, "Epic", "EpicGamesLauncher", "Data", "Manifests");
        if (!Directory.Exists(manifestRoot)) yield break;

        IEnumerable<string> manifests;
        try { manifests = Directory.EnumerateFiles(manifestRoot, "*.item").ToArray(); }
        catch (IOException) { yield break; }

        foreach (var path in manifests)
        {
            cancellationToken.ThrowIfCancellationRequested();
            JsonDocument document;
            try { document = JsonDocument.Parse(File.ReadAllText(path)); }
            catch (Exception exception) when (exception is IOException or JsonException) { continue; }
            using (document)
            {
                var root = document.RootElement;
                if (!TryGetString(root, "InstallLocation", out var installLocation)
                    || !TryGetString(root, "LaunchExecutable", out var launchExecutable)) continue;
                TryGetString(root, "DisplayName", out var displayName);
                TryGetString(root, "CatalogItemId", out var catalogItemId);
                var executable = Path.GetFullPath(Path.Combine(installLocation!, launchExecutable!));
                if (!File.Exists(executable)) continue;
                yield return new GameCandidate(
                    StableId(executable),
                    string.IsNullOrWhiteSpace(displayName) ? Path.GetFileNameWithoutExtension(executable) : displayName!,
                    executable,
                    "Epic Games",
                    catalogItemId);
            }
        }
    }

    private static IEnumerable<string> FindSteamRoots()
    {
        var values = new List<string?>();
        try { values.Add(Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam")?.GetValue("SteamPath") as string); }
        catch (Exception exception) when (exception is UnauthorizedAccessException or System.Security.SecurityException) { }

        foreach (var view in new[] { RegistryView.Registry64, RegistryView.Registry32 })
        {
            try
            {
                using var hive = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, view);
                values.Add(hive.OpenSubKey(@"SOFTWARE\Valve\Steam")?.GetValue("InstallPath") as string);
            }
            catch (Exception exception) when (exception is UnauthorizedAccessException or System.Security.SecurityException) { }
        }
        return values.Where(path => !string.IsNullOrWhiteSpace(path) && Directory.Exists(path))!.Cast<string>();
    }

    internal static string? SelectLikelyExecutable(string root, string displayName, CancellationToken cancellationToken)
    {
        if (!Directory.Exists(root)) return null;
        var candidates = new List<(FileInfo File, int Depth)>();
        var queue = new Queue<(string Path, int Depth)>();
        queue.Enqueue((root, 0));
        while (queue.Count > 0 && candidates.Count < 250)
        {
            cancellationToken.ThrowIfCancellationRequested();
            var (directory, depth) = queue.Dequeue();
            try
            {
                foreach (var file in Directory.EnumerateFiles(directory, "*.exe"))
                {
                    var name = Path.GetFileNameWithoutExtension(file);
                    if (IsHelperExecutable(name)) continue;
                    candidates.Add((new FileInfo(file), depth));
                }
                if (depth < 2)
                {
                    foreach (var child in Directory.EnumerateDirectories(directory)) queue.Enqueue((child, depth + 1));
                }
            }
            catch (Exception exception) when (exception is UnauthorizedAccessException or IOException) { }
        }

        return candidates
            .OrderByDescending(candidate => ScoreExecutable(root, displayName, candidate.File, candidate.Depth))
            .ThenByDescending(candidate => candidate.File.Length)
            .Select(candidate => candidate.File.FullName)
            .FirstOrDefault();
    }

    private static int ScoreExecutable(string root, string displayName, FileInfo file, int depth)
    {
        var score = depth switch { 0 => 1200, 1 => 500, _ => 100 };
        var displayTokens = Regex.Matches(displayName.ToLowerInvariant(), "[a-z0-9]+")
            .Select(match => match.Value)
            .Where(token => token.Length >= 3 && token is not "the" and not "and" and not "for")
            .Distinct(StringComparer.Ordinal)
            .ToArray();
        var searchableName = Normalize(Path.GetFileNameWithoutExtension(file.Name));
        try
        {
            var productName = System.Diagnostics.FileVersionInfo.GetVersionInfo(file.FullName).ProductName;
            if (!string.IsNullOrWhiteSpace(productName)) searchableName += Normalize(productName);
        }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException) { }
        score += displayTokens.Count(token => searchableName.Contains(Normalize(token), StringComparison.Ordinal)) * 300;

        var relativeDirectory = Path.GetRelativePath(root, file.DirectoryName ?? root);
        var directorySegments = relativeDirectory.Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
            .Select(segment => segment.ToLowerInvariant());
        string[] helperDirectories =
        [
            "tool", "tools", "modding", "sdk", "editor", "extras",
            "redist", "_commonredist", "redistributables", "crashreporter",
        ];
        if (directorySegments.Any(segment => helperDirectories.Contains(segment, StringComparer.Ordinal))) score -= 2500;
        return score;
    }

    private static bool IsHelperExecutable(string name)
    {
        var lowered = name.ToLowerInvariant();
        string[] blocked =
        [
            "unins", "uninstall", "crash", "reporter", "unitycrashhandler",
            "vc_redist", "dxsetup", "launcher", "setup", "moduploader",
        ];
        return blocked.Any(lowered.Contains);
    }

    private static string MatchVdfValue(string content, string key)
    {
        var match = Regex.Match(content, $"\"{Regex.Escape(key)}\"\\s*\"([^\"]*)\"", RegexOptions.IgnoreCase);
        return match.Success ? match.Groups[1].Value : string.Empty;
    }

    private static bool TryGetString(JsonElement element, string property, out string? value)
    {
        value = null;
        return element.TryGetProperty(property, out var item)
            && item.ValueKind == JsonValueKind.String
            && (value = item.GetString()) is not null;
    }

    private static string StableId(string path) =>
        Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(Path.GetFullPath(path).ToUpperInvariant())))
            .ToLowerInvariant()[..20];

    private static string Normalize(string value) =>
        new(value.Where(char.IsLetterOrDigit).Select(char.ToLowerInvariant).ToArray());

    [GeneratedRegex("\\\"path\\\"\\s*\\\"([^\\\"]+)\\\"", RegexOptions.IgnoreCase)]
    private static partial Regex VdfPathRegex();
}
