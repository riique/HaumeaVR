using System.Text;
using UniversalXRBridge.Launcher.Models;
using UniversalXRBridge.Launcher.Services;

namespace UniversalXRBridge.Launcher.Tests;

[TestClass]
public sealed class EngineDetectionServiceTests
{
    [TestMethod]
    public async Task AnalyzeAsync_DetectsUnityMonoD3D11AndAntiCheat()
    {
        using var directory = new TestDirectory();
        var executable = directory.Combine("TestGame.exe");
        CreatePe(executable, 0x8664, "d3d11.dll");
        File.WriteAllBytes(directory.Combine("TestGame_Data", "globalgamemanagers"), [0]);
        File.WriteAllBytes(directory.Combine("TestGame_Data", "Managed", "Assembly-CSharp.dll"), [0]);
        File.WriteAllBytes(directory.Combine("TestGame_Data", "Managed", "UnityEngine.CoreModule.dll"), [0]);
        File.WriteAllBytes(directory.Combine("EasyAntiCheat_EOS_Setup.exe"), [0]);

        var detection = await new EngineDetectionService().AnalyzeAsync(executable);

        Assert.AreEqual(GameEngine.Unity, detection.Engine);
        Assert.AreEqual(ScriptingBackend.Mono, detection.ScriptingBackend);
        Assert.AreEqual(GraphicsApi.D3D11, detection.GraphicsApi);
        Assert.AreEqual(RenderPipeline.BuiltIn, detection.RenderPipeline);
        Assert.AreEqual(ExecutableArchitecture.X64, detection.Architecture);
        Assert.AreEqual(AntiCheatStatus.Detected, detection.AntiCheat);
        CollectionAssert.Contains(detection.AntiCheatProducts.ToList(), "Easy Anti-Cheat");
        Assert.AreEqual(64, detection.Sha256.Length);
    }

    [TestMethod]
    public async Task AnalyzeAsync_DetectsUnityIl2Cpp()
    {
        using var directory = new TestDirectory();
        var executable = directory.Combine("Il2CppGame.exe");
        CreatePe(executable, 0x014C, null);
        File.WriteAllBytes(directory.Combine("Il2CppGame_Data", "globalgamemanagers"), [0]);
        File.WriteAllBytes(directory.Combine("GameAssembly.dll"), [0]);

        var detection = await new EngineDetectionService().AnalyzeAsync(executable);

        Assert.AreEqual(GameEngine.Unity, detection.Engine);
        Assert.AreEqual(ScriptingBackend.Il2Cpp, detection.ScriptingBackend);
        Assert.AreEqual(ExecutableArchitecture.X86, detection.Architecture);
        Assert.AreEqual(AntiCheatStatus.NotDetected, detection.AntiCheat);
    }

    [TestMethod]
    public async Task AnalyzeAsync_PrefersPeImportOverAmbiguousBackendStrings()
    {
        using var directory = new TestDirectory();
        var executable = directory.Combine("OpenGlGame.exe");
        CreatePeWithImport(executable, "OPENGL32.dll", "vulkan");

        var detection = await new EngineDetectionService().AnalyzeAsync(executable);

        Assert.AreEqual(GraphicsApi.OpenGl, detection.GraphicsApi);
        CollectionAssert.Contains(detection.Signals.ToList(), "Import PE: OPENGL32.dll");
    }

    private static void CreatePe(string path, ushort machine, string? token)
    {
        var bytes = new byte[1024];
        bytes[0] = (byte)'M';
        bytes[1] = (byte)'Z';
        BitConverter.GetBytes(0x80).CopyTo(bytes, 0x3C);
        bytes[0x80] = (byte)'P';
        bytes[0x81] = (byte)'E';
        BitConverter.GetBytes(machine).CopyTo(bytes, 0x84);
        if (token is not null) Encoding.ASCII.GetBytes(token).CopyTo(bytes, 0x100);
        File.WriteAllBytes(path, bytes);
    }

    private static void CreatePeWithImport(string path, string importedDll, string extraToken)
    {
        var bytes = new byte[2048];
        bytes[0] = (byte)'M';
        bytes[1] = (byte)'Z';
        BitConverter.GetBytes(0x80).CopyTo(bytes, 0x3C);
        bytes[0x80] = (byte)'P';
        bytes[0x81] = (byte)'E';
        BitConverter.GetBytes((ushort)0x014C).CopyTo(bytes, 0x84);
        BitConverter.GetBytes((ushort)1).CopyTo(bytes, 0x86);
        BitConverter.GetBytes((ushort)0x00E0).CopyTo(bytes, 0x94);
        BitConverter.GetBytes((ushort)0x010B).CopyTo(bytes, 0x98);
        BitConverter.GetBytes(0x1100U).CopyTo(bytes, 0x100);
        BitConverter.GetBytes(40U).CopyTo(bytes, 0x104);

        Encoding.ASCII.GetBytes(".rdata\0\0").CopyTo(bytes, 0x178);
        BitConverter.GetBytes(0x400U).CopyTo(bytes, 0x180);
        BitConverter.GetBytes(0x1000U).CopyTo(bytes, 0x184);
        BitConverter.GetBytes(0x400U).CopyTo(bytes, 0x188);
        BitConverter.GetBytes(0x200U).CopyTo(bytes, 0x18C);

        BitConverter.GetBytes(0x1190U).CopyTo(bytes, 0x300);
        BitConverter.GetBytes(0x1180U).CopyTo(bytes, 0x30C);
        BitConverter.GetBytes(0x1190U).CopyTo(bytes, 0x310);
        Encoding.ASCII.GetBytes(importedDll + "\0").CopyTo(bytes, 0x380);
        Encoding.ASCII.GetBytes(extraToken).CopyTo(bytes, 0x500);
        File.WriteAllBytes(path, bytes);
    }
}
