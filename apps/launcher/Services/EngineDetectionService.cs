using System.Buffers;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Text;
using UniversalXRBridge.Launcher.Models;

namespace UniversalXRBridge.Launcher.Services;

public sealed class EngineDetectionService
{
    private const int MaximumScannedFiles = 5000;
    private const int MaximumScanDepth = 4;
    private const int BinaryProbeBytes = 16 * 1024 * 1024;

    private static readonly (string Product, string[] Markers)[] AntiCheatMarkers =
    [
        ("Easy Anti-Cheat", ["easyanticheat", "easyanticheat_eos"]),
        ("BattlEye", ["battleye", "beservice"]),
        ("EA AntiCheat", ["eaanticheat", "eacore"]),
        ("Riot Vanguard", ["vgc.exe", "vgk.sys"]),
        ("FACEIT Anti-cheat", ["faceitclient", "faceit.sys"]),
        ("XIGNCODE3", ["xigncode", "x3.xem"]),
        ("nProtect GameGuard", ["gameguard", "gamemon.des"]),
    ];

    public Task<GameDetection> AnalyzeAsync(string executablePath, CancellationToken cancellationToken = default) =>
        Task.Run(() => Analyze(executablePath, cancellationToken), cancellationToken);

    private static GameDetection Analyze(string executablePath, CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(executablePath);
        var fullPath = Path.GetFullPath(executablePath);
        if (!File.Exists(fullPath) || !string.Equals(Path.GetExtension(fullPath), ".exe", StringComparison.OrdinalIgnoreCase))
        {
            throw new FileNotFoundException("O executável selecionado não existe.", fullPath);
        }

        cancellationToken.ThrowIfCancellationRequested();
        var root = Path.GetDirectoryName(fullPath) ?? throw new InvalidOperationException("O executável não possui diretório pai.");
        var baseName = Path.GetFileNameWithoutExtension(fullPath);
        var signals = new List<string>();

        var unityPlayer = Path.Combine(root, "UnityPlayer.dll");
        var dataDirectory = FindUnityDataDirectory(root, baseName);
        var gameAssembly = Path.Combine(root, "GameAssembly.dll");
        var monoDirectory = Path.Combine(root, "MonoBleedingEdge");

        var isUnity = File.Exists(unityPlayer) || dataDirectory is not null;
        if (File.Exists(unityPlayer)) signals.Add("UnityPlayer.dll");
        if (dataDirectory is not null) signals.Add(Path.GetFileName(dataDirectory));

        var boundedFiles = EnumerateFilesBounded(root, MaximumScanDepth, MaximumScannedFiles, cancellationToken);
        var fileNames = boundedFiles
            .Select(path => Path.GetFileName(path).ToLowerInvariant())
            .ToHashSet(StringComparer.OrdinalIgnoreCase);

        var isUnreal = !isUnity && (
            boundedFiles.Any(path => path.Contains($"{Path.DirectorySeparatorChar}Engine{Path.DirectorySeparatorChar}Binaries{Path.DirectorySeparatorChar}", StringComparison.OrdinalIgnoreCase))
            || fileNames.Any(name => name.EndsWith(".pak", StringComparison.OrdinalIgnoreCase))
            || baseName.Contains("UE4", StringComparison.OrdinalIgnoreCase)
            || baseName.Contains("Unreal", StringComparison.OrdinalIgnoreCase));
        if (isUnreal) signals.Add("Estrutura ou pacotes Unreal Engine");

        var engine = isUnity ? GameEngine.Unity : isUnreal ? GameEngine.Unreal : GameEngine.Generic;
        var backend = ScriptingBackend.Unknown;
        if (isUnity)
        {
            if (File.Exists(gameAssembly))
            {
                backend = ScriptingBackend.Il2Cpp;
                signals.Add("GameAssembly.dll");
            }
            else if (Directory.Exists(monoDirectory) || HasManagedAssembly(dataDirectory, "Assembly-CSharp.dll"))
            {
                backend = ScriptingBackend.Mono;
                signals.Add(Directory.Exists(monoDirectory) ? "MonoBleedingEdge" : "Managed/Assembly-CSharp.dll");
            }
        }
        else if (isUnreal)
        {
            backend = ScriptingBackend.Native;
        }

        var pipeline = DetectRenderPipeline(dataDirectory, signals);
        var graphicsApi = DetectGraphicsApi(fullPath, unityPlayer, signals, cancellationToken);
        var antiCheatProducts = DetectAntiCheat(boundedFiles);
        var antiCheat = antiCheatProducts.Count > 0 ? AntiCheatStatus.Detected : AntiCheatStatus.NotDetected;
        if (antiCheat == AntiCheatStatus.Detected)
        {
            signals.Add("Anti-cheat detectado; injeção será bloqueada");
        }

        var version = isUnity && File.Exists(unityPlayer)
            ? FileVersionInfo.GetVersionInfo(unityPlayer).FileVersion
            : null;
        var architecture = ReadPeArchitecture(fullPath);
        var hash = ComputeSha256(fullPath, cancellationToken);

        return new GameDetection(
            engine,
            string.IsNullOrWhiteSpace(version) ? null : version,
            backend,
            graphicsApi,
            pipeline,
            architecture,
            antiCheat,
            antiCheatProducts,
            signals,
            hash);
    }

    private static string? FindUnityDataDirectory(string root, string baseName)
    {
        var expected = Path.Combine(root, $"{baseName}_Data");
        if (Directory.Exists(expected)) return expected;

        try
        {
            return Directory.EnumerateDirectories(root, "*_Data", SearchOption.TopDirectoryOnly)
                .FirstOrDefault(path => File.Exists(Path.Combine(path, "globalgamemanagers"))
                    || Directory.Exists(Path.Combine(path, "Managed")));
        }
        catch (UnauthorizedAccessException)
        {
            return null;
        }
    }

    private static bool HasManagedAssembly(string? dataDirectory, string assemblyName) =>
        dataDirectory is not null && File.Exists(Path.Combine(dataDirectory, "Managed", assemblyName));

    private static RenderPipeline DetectRenderPipeline(string? dataDirectory, ICollection<string> signals)
    {
        if (dataDirectory is null) return RenderPipeline.Unknown;
        var managed = Path.Combine(dataDirectory, "Managed");
        if (!Directory.Exists(managed)) return RenderPipeline.Unknown;

        if (File.Exists(Path.Combine(managed, "Unity.RenderPipelines.HighDefinition.Runtime.dll")))
        {
            signals.Add("HDRP runtime assembly");
            return RenderPipeline.Hdrp;
        }
        if (File.Exists(Path.Combine(managed, "Unity.RenderPipelines.Universal.Runtime.dll")))
        {
            signals.Add("URP runtime assembly");
            return RenderPipeline.Urp;
        }
        if (File.Exists(Path.Combine(managed, "UnityEngine.CoreModule.dll")))
        {
            signals.Add("Unity Built-in/sem SRP detectado");
            return RenderPipeline.BuiltIn;
        }
        return RenderPipeline.Unknown;
    }

    private static GraphicsApi DetectGraphicsApi(
        string executable,
        string unityPlayer,
        ICollection<string> signals,
        CancellationToken cancellationToken)
    {
        var probes = new[] { executable, unityPlayer }.Where(File.Exists);
        var imports = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var found = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var path in probes)
        {
            cancellationToken.ThrowIfCancellationRequested();
            foreach (var import in ReadPeImports(path)) imports.Add(import);
            foreach (var token in ProbeBinaryTokens(path, cancellationToken)) found.Add(token);
        }

        var importedApis = new List<(GraphicsApi Api, string Dll)>();
        if (imports.Contains("d3d12.dll")) importedApis.Add((GraphicsApi.D3D12, "D3D12.dll"));
        if (imports.Contains("d3d11.dll")) importedApis.Add((GraphicsApi.D3D11, "D3D11.dll"));
        if (imports.Contains("vulkan-1.dll")) importedApis.Add((GraphicsApi.Vulkan, "vulkan-1.dll"));
        if (imports.Contains("opengl32.dll")) importedApis.Add((GraphicsApi.OpenGl, "OPENGL32.dll"));
        var distinctImports = importedApis.DistinctBy(candidate => candidate.Api).ToArray();
        if (distinctImports.Length == 1)
        {
            signals.Add($"Import PE: {distinctImports[0].Dll}");
            return distinctImports[0].Api;
        }
        if (distinctImports.Length > 1)
        {
            signals.Add("Múltiplas APIs gráficas importadas; a API ativa depende da configuração do jogo");
            return GraphicsApi.Unknown;
        }

        var referencedApis = new List<GraphicsApi>();
        if (found.Contains("d3d12.dll") || found.Contains("direct3d12")) referencedApis.Add(GraphicsApi.D3D12);
        if (found.Contains("d3d11.dll") || found.Contains("direct3d11")) referencedApis.Add(GraphicsApi.D3D11);
        if (found.Contains("vulkan-1.dll") || found.Contains("vulkan")) referencedApis.Add(GraphicsApi.Vulkan);
        if (found.Contains("opengl32.dll")) referencedApis.Add(GraphicsApi.OpenGl);
        var distinctReferences = referencedApis.Distinct().ToArray();
        if (distinctReferences.Length == 1)
        {
            signals.Add($"Referência binária provável: {distinctReferences[0]}");
            return distinctReferences[0];
        }
        if (distinctReferences.Length > 1)
        {
            signals.Add("Múltiplos backends gráficos presentes; API ativa não determinada");
        }
        return GraphicsApi.Unknown;
    }

    internal static IReadOnlySet<string> ReadPeImports(string path)
    {
        var imports = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        try
        {
            using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
            using var reader = new BinaryReader(stream, Encoding.ASCII, leaveOpen: true);
            if (stream.Length < 256 || reader.ReadUInt16() != 0x5A4D) return imports;
            stream.Position = 0x3C;
            var peOffset = reader.ReadInt32();
            if (peOffset < 0 || peOffset + 24 > stream.Length) return imports;
            stream.Position = peOffset;
            if (reader.ReadUInt32() != 0x00004550) return imports;
            stream.Position = peOffset + 6;
            var sectionCount = reader.ReadUInt16();
            stream.Position = peOffset + 20;
            var optionalHeaderSize = reader.ReadUInt16();
            var optionalHeaderStart = peOffset + 24L;
            if (optionalHeaderStart + optionalHeaderSize > stream.Length || optionalHeaderSize < 104) return imports;
            stream.Position = optionalHeaderStart;
            var magic = reader.ReadUInt16();
            var directoryStart = magic switch
            {
                0x010B => optionalHeaderStart + 96,
                0x020B => optionalHeaderStart + 112,
                _ => -1,
            };
            if (directoryStart < 0 || directoryStart + 16 > optionalHeaderStart + optionalHeaderSize) return imports;
            stream.Position = directoryStart + 8;
            var importTableRva = reader.ReadUInt32();
            var importTableSize = reader.ReadUInt32();
            if (importTableRva == 0 || importTableSize < 20) return imports;

            var sections = new List<PeSection>(sectionCount);
            stream.Position = optionalHeaderStart + optionalHeaderSize;
            for (var index = 0; index < sectionCount; index++)
            {
                if (stream.Position + 40 > stream.Length) return imports;
                stream.Position += 8;
                var virtualSize = reader.ReadUInt32();
                var virtualAddress = reader.ReadUInt32();
                var rawSize = reader.ReadUInt32();
                var rawOffset = reader.ReadUInt32();
                sections.Add(new PeSection(virtualAddress, virtualSize, rawOffset, rawSize));
                stream.Position += 16;
            }

            long? MapRva(uint rva)
            {
                foreach (var section in sections)
                {
                    var span = Math.Max(section.VirtualSize, section.RawSize);
                    if (rva < section.VirtualAddress || (ulong)rva >= (ulong)section.VirtualAddress + span) continue;
                    var offset = (ulong)section.RawOffset + (rva - section.VirtualAddress);
                    return offset < (ulong)stream.Length ? (long)offset : null;
                }
                return rva < stream.Length ? rva : null;
            }

            var importOffset = MapRva(importTableRva);
            if (importOffset is null) return imports;
            var maximumDescriptors = Math.Min(importTableSize / 20U, 4096U);
            for (uint index = 0; index < maximumDescriptors; index++)
            {
                var descriptorOffset = importOffset.Value + index * 20L;
                if (descriptorOffset + 20 > stream.Length) break;
                stream.Position = descriptorOffset;
                var originalFirstThunk = reader.ReadUInt32();
                var timestamp = reader.ReadUInt32();
                var forwarderChain = reader.ReadUInt32();
                var nameRva = reader.ReadUInt32();
                var firstThunk = reader.ReadUInt32();
                if (originalFirstThunk == 0 && timestamp == 0 && forwarderChain == 0 && nameRva == 0 && firstThunk == 0) break;
                var nameOffset = MapRva(nameRva);
                if (nameOffset is null) continue;
                stream.Position = nameOffset.Value;
                var bytes = new List<byte>(64);
                while (stream.Position < stream.Length && bytes.Count < 512)
                {
                    var value = reader.ReadByte();
                    if (value == 0) break;
                    bytes.Add(value);
                }
                if (bytes.Count > 0) imports.Add(Encoding.ASCII.GetString(bytes.ToArray()));
            }
        }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException or EndOfStreamException) { }
        return imports;
    }

    private readonly record struct PeSection(uint VirtualAddress, uint VirtualSize, uint RawOffset, uint RawSize);

    private static IEnumerable<string> ProbeBinaryTokens(string path, CancellationToken cancellationToken)
    {
        var tokens = new[] { "d3d11.dll", "direct3d11", "d3d12.dll", "direct3d12", "vulkan-1.dll", "vulkan", "opengl32.dll" };
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        var length = (int)Math.Min(stream.Length, BinaryProbeBytes);
        var rented = ArrayPool<byte>.Shared.Rent(length);
        try
        {
            var read = 0;
            while (read < length)
            {
                cancellationToken.ThrowIfCancellationRequested();
                var count = stream.Read(rented, read, length - read);
                if (count == 0) break;
                read += count;
            }
            var text = Encoding.Latin1.GetString(rented, 0, read).ToLowerInvariant();
            return tokens.Where(text.Contains).ToArray();
        }
        finally
        {
            ArrayPool<byte>.Shared.Return(rented);
        }
    }

    private static List<string> DetectAntiCheat(IEnumerable<string> files)
    {
        var paths = files.Select(path => path.ToLowerInvariant()).ToArray();
        return AntiCheatMarkers
            .Where(marker => marker.Markers.Any(token => paths.Any(path => path.Contains(token, StringComparison.OrdinalIgnoreCase))))
            .Select(marker => marker.Product)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .Order(StringComparer.CurrentCultureIgnoreCase)
            .ToList();
    }

    private static IReadOnlyList<string> EnumerateFilesBounded(
        string root,
        int maximumDepth,
        int maximumFiles,
        CancellationToken cancellationToken)
    {
        var result = new List<string>(Math.Min(maximumFiles, 512));
        var queue = new Queue<(string Path, int Depth)>();
        queue.Enqueue((root, 0));
        while (queue.Count > 0 && result.Count < maximumFiles)
        {
            cancellationToken.ThrowIfCancellationRequested();
            var (directory, depth) = queue.Dequeue();
            try
            {
                foreach (var file in Directory.EnumerateFiles(directory))
                {
                    result.Add(file);
                    if (result.Count >= maximumFiles) break;
                }
                if (depth < maximumDepth)
                {
                    foreach (var child in Directory.EnumerateDirectories(directory))
                    {
                        queue.Enqueue((child, depth + 1));
                    }
                }
            }
            catch (Exception exception) when (exception is UnauthorizedAccessException or IOException)
            {
                // A partial scan is safer than failing game discovery because of one protected directory.
            }
        }
        return result;
    }

    internal static ExecutableArchitecture ReadPeArchitecture(string executablePath)
    {
        using var stream = new FileStream(executablePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        using var reader = new BinaryReader(stream, Encoding.UTF8, leaveOpen: true);
        if (stream.Length < 64 || reader.ReadUInt16() != 0x5A4D) return ExecutableArchitecture.Unknown;
        stream.Position = 0x3C;
        var peOffset = reader.ReadInt32();
        if (peOffset < 0 || peOffset + 6 > stream.Length) return ExecutableArchitecture.Unknown;
        stream.Position = peOffset;
        if (reader.ReadUInt32() != 0x00004550) return ExecutableArchitecture.Unknown;
        return reader.ReadUInt16() switch
        {
            0x014C => ExecutableArchitecture.X86,
            0x8664 => ExecutableArchitecture.X64,
            0xAA64 => ExecutableArchitecture.Arm64,
            _ => ExecutableArchitecture.Unknown,
        };
    }

    private static string ComputeSha256(string path, CancellationToken cancellationToken)
    {
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        using var hash = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);
        var buffer = ArrayPool<byte>.Shared.Rent(1024 * 1024);
        try
        {
            int read;
            while ((read = stream.Read(buffer, 0, buffer.Length)) > 0)
            {
                cancellationToken.ThrowIfCancellationRequested();
                hash.AppendData(buffer, 0, read);
            }
            return Convert.ToHexString(hash.GetHashAndReset()).ToLowerInvariant();
        }
        finally
        {
            ArrayPool<byte>.Shared.Return(buffer);
        }
    }
}
