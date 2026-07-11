using UniversalXRBridge.Launcher.Models;
using UniversalXRBridge.Launcher.Services;

namespace UniversalXRBridge.Launcher.Tests;

[TestClass]
public sealed class ProfileRepositoryTests
{
    [TestMethod]
    public async Task SaveAndLoad_RoundTripsAValidatedProfile()
    {
        using var directory = new TestDirectory();
        var executable = directory.Combine("Game.exe");
        File.WriteAllBytes(executable, [1, 2, 3]);
        var game = new GameCandidate("id", "Game", executable, "Manual");
        var detection = new GameDetection(
            GameEngine.Generic,
            null,
            ScriptingBackend.Unknown,
            GraphicsApi.D3D11,
            RenderPipeline.Unknown,
            ExecutableArchitecture.X64,
            AntiCheatStatus.NotDetected,
            [],
            ["Referência a Direct3D 11"],
            new string('a', 64));
        var profile = ProfileFactory.Create(game, detection);
        var repository = new ProfileRepository(directory.Combine("data", "profiles.db"));

        await repository.InitializeAsync();
        await repository.SaveAsync(profile);
        var loaded = await repository.GetByExecutableAsync(executable);

        Assert.IsNotNull(loaded);
        Assert.AreEqual(profile.Id, loaded.Id);
        Assert.AreEqual(RenderMode.VirtualScreen, loaded.Rendering.PreferredMode);
        Assert.AreEqual(executable, loaded.Executable.Path);
        Assert.IsEmpty(ProfileValidator.Validate(loaded));
    }

    [TestMethod]
    public void DefaultDatabasePath_CopiesLegacyDatabaseOnce()
    {
        using var directory = new TestDirectory();
        var legacy = directory.Combine("UniversalXRBridge", "data", "uxrb.db");
        File.WriteAllBytes(legacy, [1, 2, 3, 4]);

        var migrated = ProfileRepository.GetDefaultDatabasePath(directory.Path);

        Assert.AreEqual(
            directory.Combine("HaumeaVR", "data", "haumeavr.db"),
            migrated);
        CollectionAssert.AreEqual(File.ReadAllBytes(legacy), File.ReadAllBytes(migrated));

        File.WriteAllBytes(legacy, [9]);
        Assert.AreEqual(migrated, ProfileRepository.GetDefaultDatabasePath(directory.Path));
        CollectionAssert.AreEqual(new byte[] { 1, 2, 3, 4 }, File.ReadAllBytes(migrated));
    }

    [TestMethod]
    public void Validate_RejectsInjectionModesWhenAntiCheatIsDetected()
    {
        using var directory = new TestDirectory();
        var executable = directory.Combine("Protected.exe");
        File.WriteAllBytes(executable, [0]);
        var game = new GameCandidate("id", "Protected", executable, "Manual");
        var detection = new GameDetection(
            GameEngine.Unity,
            null,
            ScriptingBackend.Mono,
            GraphicsApi.D3D11,
            RenderPipeline.BuiltIn,
            ExecutableArchitecture.X64,
            AntiCheatStatus.Detected,
            ["Easy Anti-Cheat"],
            [],
            new string('b', 64));
        var profile = ProfileFactory.Create(game, detection) with
        {
            Rendering = new RenderingProfile(RenderMode.UnityNative, [RenderMode.UnityNative, RenderMode.VirtualScreen], 1, 1, 2, 2.4, 0.15),
        };

        var errors = ProfileValidator.Validate(profile);

        Assert.IsTrue(errors.Any(error => error.Contains("anti-cheat", StringComparison.OrdinalIgnoreCase)));
    }
}
