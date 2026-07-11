using System.Globalization;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.Data.Sqlite;
using UniversalXRBridge.Launcher.Models;

namespace UniversalXRBridge.Launcher.Services;

public sealed class ProfileRepository
{
    private readonly string connectionString;
    private readonly JsonSerializerOptions serializerOptions;

    public ProfileRepository(string? databasePath = null)
    {
        databasePath ??= GetDefaultDatabasePath();
        var fullPath = Path.GetFullPath(databasePath);
        Directory.CreateDirectory(Path.GetDirectoryName(fullPath)!);
        connectionString = new SqliteConnectionStringBuilder
        {
            DataSource = fullPath,
            Mode = SqliteOpenMode.ReadWriteCreate,
            Cache = SqliteCacheMode.Shared,
        }.ToString();
        serializerOptions = new JsonSerializerOptions
        {
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
            WriteIndented = true,
            UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow,
        };
        serializerOptions.Converters.Add(new JsonStringEnumConverter());
    }

    internal static string GetDefaultDatabasePath(string? localAppData = null)
    {
        localAppData ??= Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        var newPath = Path.Combine(localAppData, "HaumeaVR", "data", "haumeavr.db");
        var legacyPath = Path.Combine(localAppData, "UniversalXRBridge", "data", "uxrb.db");
        if (!File.Exists(newPath) && File.Exists(legacyPath))
        {
            Directory.CreateDirectory(Path.GetDirectoryName(newPath)!);
            File.Copy(legacyPath, newPath, overwrite: false);
        }
        return newPath;
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        await using var connection = await OpenAsync(cancellationToken).ConfigureAwait(false);
        var command = connection.CreateCommand();
        command.CommandText = """
            CREATE TABLE IF NOT EXISTS profiles (
                id TEXT PRIMARY KEY NOT NULL,
                executable_path TEXT NOT NULL UNIQUE COLLATE NOCASE,
                display_name TEXT NOT NULL,
                document_json TEXT NOT NULL,
                updated_at TEXT NOT NULL
            );
            CREATE INDEX IF NOT EXISTS ix_profiles_display_name ON profiles(display_name COLLATE NOCASE);
            PRAGMA user_version = 1;
            """;
        await command.ExecuteNonQueryAsync(cancellationToken).ConfigureAwait(false);
    }

    public async Task<IReadOnlyList<GameProfile>> GetAllAsync(CancellationToken cancellationToken = default)
    {
        await using var connection = await OpenAsync(cancellationToken).ConfigureAwait(false);
        var command = connection.CreateCommand();
        command.CommandText = "SELECT document_json FROM profiles ORDER BY display_name COLLATE NOCASE;";
        var result = new List<GameProfile>();
        await using var reader = await command.ExecuteReaderAsync(cancellationToken).ConfigureAwait(false);
        while (await reader.ReadAsync(cancellationToken).ConfigureAwait(false))
        {
            var profile = JsonSerializer.Deserialize<GameProfile>(reader.GetString(0), serializerOptions)
                ?? throw new InvalidDataException("O banco contém um perfil vazio.");
            result.Add(profile);
        }
        return result;
    }

    public async Task<GameProfile?> GetByExecutableAsync(string executablePath, CancellationToken cancellationToken = default)
    {
        await using var connection = await OpenAsync(cancellationToken).ConfigureAwait(false);
        var command = connection.CreateCommand();
        command.CommandText = "SELECT document_json FROM profiles WHERE executable_path = $path LIMIT 1;";
        command.Parameters.AddWithValue("$path", Path.GetFullPath(executablePath));
        var document = await command.ExecuteScalarAsync(cancellationToken).ConfigureAwait(false) as string;
        return document is null ? null : JsonSerializer.Deserialize<GameProfile>(document, serializerOptions);
    }

    public async Task SaveAsync(GameProfile profile, CancellationToken cancellationToken = default)
    {
        ProfileValidator.EnsureValid(profile);
        var stored = profile with { UpdatedAt = DateTimeOffset.UtcNow };
        var document = JsonSerializer.Serialize(stored, serializerOptions);

        await using var connection = await OpenAsync(cancellationToken).ConfigureAwait(false);
        await using var transaction = await connection.BeginTransactionAsync(cancellationToken).ConfigureAwait(false);
        var command = connection.CreateCommand();
        command.Transaction = (SqliteTransaction)transaction;
        command.CommandText = """
            INSERT INTO profiles(id, executable_path, display_name, document_json, updated_at)
            VALUES ($id, $path, $name, $json, $updated)
            ON CONFLICT(executable_path) DO UPDATE SET
                id = excluded.id,
                display_name = excluded.display_name,
                document_json = excluded.document_json,
                updated_at = excluded.updated_at;
            """;
        command.Parameters.AddWithValue("$id", stored.Id.ToString("D"));
        command.Parameters.AddWithValue("$path", Path.GetFullPath(stored.Executable.Path));
        command.Parameters.AddWithValue("$name", stored.DisplayName);
        command.Parameters.AddWithValue("$json", document);
        command.Parameters.AddWithValue("$updated", stored.UpdatedAt.ToString("O", CultureInfo.InvariantCulture));
        await command.ExecuteNonQueryAsync(cancellationToken).ConfigureAwait(false);
        await transaction.CommitAsync(cancellationToken).ConfigureAwait(false);
    }

    public async Task DeleteAsync(Guid id, CancellationToken cancellationToken = default)
    {
        await using var connection = await OpenAsync(cancellationToken).ConfigureAwait(false);
        var command = connection.CreateCommand();
        command.CommandText = "DELETE FROM profiles WHERE id = $id;";
        command.Parameters.AddWithValue("$id", id.ToString("D"));
        await command.ExecuteNonQueryAsync(cancellationToken).ConfigureAwait(false);
    }

    public async Task ExportAsync(GameProfile profile, string destinationPath, CancellationToken cancellationToken = default)
    {
        ProfileValidator.EnsureValid(profile);
        var json = JsonSerializer.Serialize(profile, serializerOptions);
        await File.WriteAllTextAsync(destinationPath, json, cancellationToken).ConfigureAwait(false);
    }

    public async Task<GameProfile> ImportAsync(string sourcePath, CancellationToken cancellationToken = default)
    {
        var json = await File.ReadAllTextAsync(sourcePath, cancellationToken).ConfigureAwait(false);
        var profile = JsonSerializer.Deserialize<GameProfile>(json, serializerOptions)
            ?? throw new InvalidDataException("O arquivo não contém um perfil.");
        ProfileValidator.EnsureValid(profile);
        await SaveAsync(profile, cancellationToken).ConfigureAwait(false);
        return profile;
    }

    private async Task<SqliteConnection> OpenAsync(CancellationToken cancellationToken)
    {
        var connection = new SqliteConnection(connectionString);
        await connection.OpenAsync(cancellationToken).ConfigureAwait(false);
        return connection;
    }
}
