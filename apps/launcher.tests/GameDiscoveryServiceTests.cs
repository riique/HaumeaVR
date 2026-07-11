using UniversalXRBridge.Launcher.Services;

namespace UniversalXRBridge.Launcher.Tests;

[TestClass]
public sealed class GameDiscoveryServiceTests
{
    [TestMethod]
    public void PrefersRootGameExecutableOverLargeToolBinary()
    {
        using var directory = new TestDirectory();
        var game = directory.Combine("isaac-ng.exe");
        var tool = directory.Combine("tools", "ModUploader", "ModUploader.exe");
        File.WriteAllBytes(game, [0x4D, 0x5A]);
        File.WriteAllBytes(tool, new byte[1024 * 1024]);

        var selected = GameDiscoveryService.SelectLikelyExecutable(
            directory.Path,
            "The Binding of Isaac: Rebirth",
            CancellationToken.None);

        Assert.AreEqual(game, selected, ignoreCase: true);
    }
}
