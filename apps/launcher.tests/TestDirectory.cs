namespace UniversalXRBridge.Launcher.Tests;

internal sealed class TestDirectory : IDisposable
{
    public TestDirectory()
    {
        Path = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "uxrb-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(Path);
    }

    public string Path { get; }

    public string Combine(params string[] parts)
    {
        var path = parts.Aggregate(Path, System.IO.Path.Combine);
        Directory.CreateDirectory(System.IO.Path.GetDirectoryName(path)!);
        return path;
    }

    public void Dispose()
    {
        try { Directory.Delete(Path, recursive: true); }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
    }
}
