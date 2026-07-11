using System.Collections.ObjectModel;
using System.Text.Json.Nodes;
using UniversalXRBridge.Launcher.Infrastructure;
using UniversalXRBridge.Launcher.Models;
using UniversalXRBridge.Launcher.Services;
using UniversalXRBridge.Launcher.Services.Ipc;

namespace UniversalXRBridge.Launcher.ViewModels;

public sealed class MainWindowViewModel : ObservableObject, IAsyncDisposable
{
    private readonly GameDiscoveryService discoveryService;
    private readonly EngineDetectionService detectionService;
    private readonly ProfileRepository profileRepository;
    private readonly OpenXrRuntimeService runtimeService;
    private readonly CoreProcessService coreProcessService;
    private readonly GameWindowService gameWindowService;
    private readonly UnityBridgeDeploymentService unityBridgeDeploymentService;
    private readonly AsyncCommand refreshCommand;
    private readonly AsyncCommand analyzeCommand;
    private readonly AsyncCommand saveProfileCommand;
    private readonly AsyncCommand diagnoseCommand;
    private readonly AsyncCommand startVirtualScreenCommand;
    private readonly AsyncCommand startUnityStereoCommand;
    private readonly AsyncCommand stopSessionCommand;
    private readonly AsyncCommand recenterCommand;

    private GameCandidate? selectedGame;
    private GameDetection? currentDetection;
    private GameProfile? currentProfile;
    private OpenXrRuntimeSnapshot? runtimeSnapshot;
    private RegisteredRuntime? selectedRuntime;
    private CoreControlSession? activeSession;
    private UnityBridgeDeployment? unityBridgeDeployment;
    private string activeSessionMode = string.Empty;
    private string statusMessage = "Inicializando launcher…";
    private string diagnosticOutput = string.Empty;
    private string sessionStatusText = "Núcleo parado";
    private bool isBusy;

    public MainWindowViewModel(
        GameDiscoveryService discoveryService,
        EngineDetectionService detectionService,
        ProfileRepository profileRepository,
        OpenXrRuntimeService runtimeService,
        CoreProcessService coreProcessService,
        GameWindowService gameWindowService,
        UnityBridgeDeploymentService unityBridgeDeploymentService)
    {
        this.discoveryService = discoveryService;
        this.detectionService = detectionService;
        this.profileRepository = profileRepository;
        this.runtimeService = runtimeService;
        this.coreProcessService = coreProcessService;
        this.gameWindowService = gameWindowService;
        this.unityBridgeDeploymentService = unityBridgeDeploymentService;
        refreshCommand = new AsyncCommand(() => ExecuteSafelyAsync(RefreshAsync), () => !IsBusy);
        analyzeCommand = new AsyncCommand(() => ExecuteSafelyAsync(AnalyzeSelectedAsync), () => !IsBusy && SelectedGame is not null);
        saveProfileCommand = new AsyncCommand(() => ExecuteSafelyAsync(SaveProfileAsync), () => !IsBusy && CurrentProfile is not null);
        diagnoseCommand = new AsyncCommand(() => ExecuteSafelyAsync(DiagnoseAsync), () => !IsBusy);
        startVirtualScreenCommand = new AsyncCommand(
            () => ExecuteSafelyAsync(StartVirtualScreenAsync),
            () => !IsBusy && CurrentProfile is not null && activeSession is null);
        startUnityStereoCommand = new AsyncCommand(
            () => ExecuteSafelyAsync(StartUnityStereoAsync),
            () => !IsBusy && CanStartUnityStereo);
        stopSessionCommand = new AsyncCommand(
            () => ExecuteSafelyAsync(StopSessionAsync),
            () => !IsBusy && activeSession is not null);
        recenterCommand = new AsyncCommand(
            () => ExecuteSafelyAsync(RecenterAsync),
            () => !IsBusy && activeSession is not null);
    }

    public ObservableCollection<GameCandidate> Games { get; } = [];
    public ObservableCollection<RegisteredRuntime> RegisteredRuntimes { get; } = [];
    public ObservableCollection<CompatibilityStep> CompatibilitySteps { get; } = [];
    public ObservableCollection<string> DetectionSignals { get; } = [];

    public AsyncCommand RefreshCommand => refreshCommand;
    public AsyncCommand AnalyzeCommand => analyzeCommand;
    public AsyncCommand SaveProfileCommand => saveProfileCommand;
    public AsyncCommand DiagnoseCommand => diagnoseCommand;
    public AsyncCommand StartVirtualScreenCommand => startVirtualScreenCommand;
    public AsyncCommand StartUnityStereoCommand => startUnityStereoCommand;
    public AsyncCommand StopSessionCommand => stopSessionCommand;
    public AsyncCommand RecenterCommand => recenterCommand;

    public GameCandidate? SelectedGame
    {
        get => selectedGame;
        set
        {
            if (!SetProperty(ref selectedGame, value)) return;
            currentDetection = null;
            currentProfile = null;
            unityBridgeDeployment = null;
            CompatibilitySteps.Clear();
            DetectionSignals.Clear();
            OnPropertyChanged(nameof(CurrentDetection));
            OnPropertyChanged(nameof(CurrentProfile));
            OnPropertyChanged(nameof(UnityBridgeStatusText));
            OnPropertyChanged(nameof(CanStartUnityStereo));
            RaiseGameDetailProperties();
            RaiseCommandStates();
        }
    }

    public GameDetection? CurrentDetection
    {
        get => currentDetection;
        private set
        {
            if (!SetProperty(ref currentDetection, value)) return;
            RaiseGameDetailProperties();
        }
    }

    public GameProfile? CurrentProfile
    {
        get => currentProfile;
        private set
        {
            if (!SetProperty(ref currentProfile, value)) return;
            OnPropertyChanged(nameof(PreferredModeText));
            OnPropertyChanged(nameof(SafetyText));
            RaiseCommandStates();
        }
    }

    public OpenXrRuntimeSnapshot? RuntimeSnapshot
    {
        get => runtimeSnapshot;
        private set
        {
            if (!SetProperty(ref runtimeSnapshot, value)) return;
            OnPropertyChanged(nameof(RuntimeStatusText));
            OnPropertyChanged(nameof(ActiveRuntimePath));
            OnPropertyChanged(nameof(ObservedAtText));
        }
    }

    public RegisteredRuntime? SelectedRuntime
    {
        get => selectedRuntime;
        set
        {
            if (!SetProperty(ref selectedRuntime, value)) return;
            OnPropertyChanged(nameof(RuntimeStatusText));
            OnPropertyChanged(nameof(ActiveRuntimePath));
        }
    }

    public string StatusMessage
    {
        get => statusMessage;
        private set => SetProperty(ref statusMessage, value);
    }

    public string DiagnosticOutput
    {
        get => diagnosticOutput;
        private set => SetProperty(ref diagnosticOutput, value);
    }

    public string SessionStatusText
    {
        get => sessionStatusText;
        private set => SetProperty(ref sessionStatusText, value);
    }

    public bool IsSessionActive => activeSession is not null && !activeSession.HasExited;

    public bool CanStartUnityStereo =>
        activeSession is null
        && CurrentProfile is not null
        && CurrentDetection is
        {
            Engine: GameEngine.Unity,
            ScriptingBackend: ScriptingBackend.Mono,
            AntiCheat: not AntiCheatStatus.Detected,
        }
        && unityBridgeDeployment?.Ready == true;

    public string UnityBridgeStatusText
    {
        get
        {
            if (CurrentDetection?.AntiCheat == AntiCheatStatus.Detected)
                return "Unity Stereo bloqueado: anti-cheat detectado.";
            if (CurrentDetection?.Engine != GameEngine.Unity)
                return "Unity Stereo requer um player Unity controlado.";
            if (CurrentDetection.ScriptingBackend != ScriptingBackend.Mono)
                return "Unity Stereo controlado está habilitado primeiro para Mono.";
            return unityBridgeDeployment?.Reason ?? "Analise o player para verificar o bridge Unity.";
        }
    }

    public bool IsBusy
    {
        get => isBusy;
        private set
        {
            if (!SetProperty(ref isBusy, value)) return;
            RaiseCommandStates();
        }
    }

    public string RuntimeStatusText => RuntimeSnapshot?.HasActiveRuntime == true
        ? "Runtime OpenXR ativo"
        : SelectedRuntime is not null ? "Runtime OpenXR por processo" : "Nenhum runtime OpenXR disponível";
    public string ActiveRuntimePath => SelectedRuntime?.ManifestPath
        ?? RuntimeSnapshot?.ActiveManifestPath
        ?? "Nenhum manifest ativo no registro de 64 bits";
    public string ObservedAtText => RuntimeSnapshot is null ? string.Empty : $"Verificado {RuntimeSnapshot.ObservedAt:dd/MM/yyyy HH:mm:ss}";
    public string GameCountText => Games.Count == 1 ? "1 jogo" : $"{Games.Count} jogos";
    public string ExecutablePath => SelectedGame?.ExecutablePath ?? "Selecione um executável para analisar";
    public string EngineText => CurrentDetection is null ? "Não analisado" : FormatEnum(CurrentDetection.Engine);
    public string BackendText => CurrentDetection is null ? "Não analisado" : FormatEnum(CurrentDetection.ScriptingBackend);
    public string GraphicsApiText => CurrentDetection is null ? "Não analisado" : FormatEnum(CurrentDetection.GraphicsApi);
    public string PipelineText => CurrentDetection is null ? "Não analisado" : FormatEnum(CurrentDetection.RenderPipeline);
    public string ArchitectureText => CurrentDetection is null ? "Não analisado" : FormatEnum(CurrentDetection.Architecture);
    public string AntiCheatText => CurrentDetection?.AntiCheat switch
    {
        AntiCheatStatus.Detected => string.Join(", ", CurrentDetection.AntiCheatProducts),
        AntiCheatStatus.NotDetected => "Não detectado",
        _ => "Não analisado",
    };
    public string PreferredModeText => CurrentProfile?.Rendering.PreferredMode switch
    {
        RenderMode.UnityNative => "Unity Native",
        RenderMode.GeometryStereo => "Geometry Stereo",
        RenderMode.Depth3D => "Depth 3D",
        RenderMode.VirtualScreen => "Virtual Screen",
        _ => "Aguardando análise",
    };
    public string SafetyText => CurrentProfile is null
        ? "Aguardando análise"
        : CurrentProfile.Safety.SafeMode ? "Modo seguro obrigatório" : "Modo seguro disponível";

    public async Task InitializeAsync()
    {
        await ExecuteSafelyAsync(async () =>
        {
            await profileRepository.InitializeAsync();
            await RefreshAsync();
        });
    }

    public async Task AddManualGameAsync(string path)
    {
        await ExecuteSafelyAsync(async () =>
        {
            var candidate = discoveryService.FromManualPath(path);
            var existing = Games.FirstOrDefault(game =>
                string.Equals(game.ExecutablePath, candidate.ExecutablePath, StringComparison.OrdinalIgnoreCase));
            if (existing is null)
            {
                Games.Add(candidate);
                SelectedGame = candidate;
                OnPropertyChanged(nameof(GameCountText));
            }
            else
            {
                SelectedGame = existing;
            }
            await AnalyzeSelectedAsync();
        });
    }

    private async Task RefreshAsync()
    {
        StatusMessage = "Lendo runtimes OpenXR e bibliotecas instaladas…";
        var previousRuntimePath = SelectedRuntime?.ManifestPath;
        var runtimeTask = runtimeService.InspectAsync();
        var gamesTask = discoveryService.DiscoverAsync();
        var profilesTask = profileRepository.GetAllAsync();
        await Task.WhenAll(runtimeTask, gamesTask, profilesTask);

        RuntimeSnapshot = runtimeTask.Result;
        RegisteredRuntimes.Clear();
        foreach (var runtime in RuntimeSnapshot.RegisteredRuntimes) RegisteredRuntimes.Add(runtime);
        SelectedRuntime = RegisteredRuntimes.FirstOrDefault(runtime =>
                string.Equals(runtime.ManifestPath, previousRuntimePath, StringComparison.OrdinalIgnoreCase))
            ?? RegisteredRuntimes.FirstOrDefault(runtime => runtime.Active && runtime.ManifestExists)
            ?? RegisteredRuntimes.FirstOrDefault(runtime => runtime.Enabled && runtime.ManifestExists);

        var previousPath = SelectedGame?.ExecutablePath;
        var discovered = gamesTask.Result.ToDictionary(game => game.ExecutablePath, StringComparer.OrdinalIgnoreCase);
        foreach (var profile in profilesTask.Result)
        {
            discovered.TryAdd(profile.Executable.Path, new GameCandidate(
                profile.Id.ToString("N"),
                profile.DisplayName,
                profile.Executable.Path,
                "Perfil local"));
        }
        Games.Clear();
        foreach (var game in discovered.Values.OrderBy(game => game.DisplayName, StringComparer.CurrentCultureIgnoreCase)) Games.Add(game);
        SelectedGame = previousPath is null
            ? Games.FirstOrDefault()
            : Games.FirstOrDefault(game => string.Equals(game.ExecutablePath, previousPath, StringComparison.OrdinalIgnoreCase));
        OnPropertyChanged(nameof(GameCountText));
        StatusMessage = $"{GameCountText} encontrados; {RegisteredRuntimes.Count} runtimes registrados.";
    }

    private async Task AnalyzeSelectedAsync()
    {
        if (SelectedGame is null) return;
        StatusMessage = $"Analisando {SelectedGame.DisplayName}…";
        var detection = await detectionService.AnalyzeAsync(SelectedGame.ExecutablePath);
        CurrentDetection = detection;
        DetectionSignals.Clear();
        foreach (var signal in detection.Signals) DetectionSignals.Add(signal);

        unityBridgeDeployment = detection.Engine == GameEngine.Unity
            ? unityBridgeDeploymentService.Inspect(SelectedGame.ExecutablePath)
            : null;
        var capabilities = new RuntimeCapabilities(
            UnityNative: unityBridgeDeployment?.Ready == true,
            GeometryStereo: false,
            Depth3D: false,
            VirtualScreen: true);
        CurrentProfile = await profileRepository.GetByExecutableAsync(SelectedGame.ExecutablePath)
            ?? ProfileFactory.Create(SelectedGame, detection, capabilities);
        OnPropertyChanged(nameof(UnityBridgeStatusText));
        OnPropertyChanged(nameof(CanStartUnityStereo));
        CompatibilitySteps.Clear();
        foreach (var step in ProfileFactory.PlanCompatibility(detection, capabilities)) CompatibilitySteps.Add(step);
        StatusMessage = detection.AntiCheat == AntiCheatStatus.Detected
            ? "Anti-cheat detectado: somente Virtual Screen será permitido."
            : $"Análise concluída; modo recomendado: {PreferredModeText}.";
    }

    private async Task SaveProfileAsync()
    {
        if (CurrentProfile is null) return;
        await profileRepository.SaveAsync(CurrentProfile);
        StatusMessage = $"Perfil de {CurrentProfile.DisplayName} salvo localmente.";
    }

    private async Task DiagnoseAsync()
    {
        StatusMessage = "Executando diagnóstico nativo…";
        var result = await coreProcessService.DiagnoseAsync();
        DiagnosticOutput = result.Output;
        StatusMessage = result.Successful
            ? "Diagnóstico OpenXR concluído com sucesso."
            : $"Diagnóstico concluído com código {result.ExitCode}; consulte a saída.";
        RuntimeSnapshot = await runtimeService.InspectAsync();
    }

    private async Task StartVirtualScreenAsync()
    {
        if (CurrentProfile is null) return;
        ProfileValidator.EnsureValid(CurrentProfile);
        if (!CurrentProfile.Rendering.FallbackOrder.Contains(RenderMode.VirtualScreen))
            throw new InvalidOperationException("O perfil selecionado não permite Virtual Screen.");

        var runtimeManifest = SelectedRuntime?.ManifestPath ?? RuntimeSnapshot?.ActiveManifestPath;
        if (string.IsNullOrWhiteSpace(runtimeManifest) || !File.Exists(runtimeManifest))
            throw new FileNotFoundException("Selecione um runtime OpenXR com manifest válido antes de iniciar.");

        StatusMessage = $"Localizando ou iniciando a janela de {CurrentProfile.DisplayName}…";
        var gameWindow = await gameWindowService.FindOrLaunchAsync(
            CurrentProfile,
            TimeSpan.FromSeconds(30));
        StatusMessage = $"Conectando {gameWindow.Title} ao núcleo OpenXR…";

        var session = await coreProcessService.StartControlSessionAsync(runtimeManifest);
        try
        {
            var rendering = CurrentProfile.Rendering;
            var start = await session.RequestAsync(
                "start-virtual-screen",
                new JsonObject
                {
                    ["windowHandle"] = gameWindow.Handle.ToInt64(),
                    ["widthMeters"] = rendering.ScreenWidthMeters,
                    ["distanceMeters"] = rendering.ScreenDistanceMeters,
                    ["curvature"] = rendering.ScreenCurvature,
                });
            ThrowIfCoreError(start);

            await Task.Delay(900);
            var status = await session.RequestAsync("get-status");
            ThrowIfCoreError(status);
            var state = status.Payload["data"]?["state"]?.GetValue<string>() ?? "unknown";
            if (state == "failed")
            {
                var lastError = status.Payload["data"]?["lastError"]?.GetValue<string>()
                    ?? "O runtime OpenXR encerrou a sessão durante a inicialização.";
                throw new InvalidOperationException(lastError);
            }

            activeSession = session;
            activeSessionMode = "Virtual Screen";
            SessionStatusText = $"Virtual Screen · PID {session.ProcessId}";
            DiagnosticOutput = string.Join(
                Environment.NewLine,
                $"Janela: {gameWindow.Title}",
                $"HWND: 0x{gameWindow.Handle.ToInt64():X}",
                $"Runtime: {runtimeManifest}",
                $"Estado: {state}",
                session.Output);
            StatusMessage = "Virtual Screen iniciado. Use Recentrar ou F8 para reposicionar a tela.";
            OnPropertyChanged(nameof(IsSessionActive));
            RaiseCommandStates();
        }
        catch
        {
            await session.DisposeAsync();
            throw;
        }
    }

    private async Task StartUnityStereoAsync()
    {
        if (CurrentProfile is null || CurrentDetection is null) return;
        ProfileValidator.EnsureValid(CurrentProfile);
        if (CurrentDetection.AntiCheat == AntiCheatStatus.Detected)
            throw new InvalidOperationException("Unity Stereo é bloqueado quando um anti-cheat é detectado.");
        if (CurrentDetection.Engine != GameEngine.Unity || CurrentDetection.ScriptingBackend != ScriptingBackend.Mono)
            throw new InvalidOperationException("O caminho Unity Stereo controlado requer um player Unity Mono.");
        if (unityBridgeDeployment?.Ready != true)
            throw new InvalidOperationException(unityBridgeDeployment?.Reason ?? "Bridge Unity não implantado neste player.");

        var runtimeManifest = SelectedRuntime?.ManifestPath ?? RuntimeSnapshot?.ActiveManifestPath;
        if (string.IsNullOrWhiteSpace(runtimeManifest) || !File.Exists(runtimeManifest))
            throw new FileNotFoundException("Selecione um runtime OpenXR com manifest válido antes de iniciar.");

        StatusMessage = "Preparando transporte Unity para o núcleo OpenXR…";
        var session = await coreProcessService.StartControlSessionAsync(runtimeManifest);
        try
        {
            StatusMessage = $"Iniciando {CurrentProfile.DisplayName} com o bridge Unity controlado…";
            var gameWindow = await gameWindowService.LaunchUnityBridgeAsync(
                CurrentProfile,
                session.UnityTransportName,
                TimeSpan.FromSeconds(30));
            var start = await session.RequestAsync(
                "start-unity-stereo",
                new JsonObject
                {
                    ["transportName"] = session.UnityTransportName,
                    ["processId"] = gameWindow.ProcessId,
                });
            ThrowIfCoreError(start);

            await Task.Delay(900);
            var status = await session.RequestAsync("get-status");
            ThrowIfCoreError(status);
            var state = status.Payload["data"]?["state"]?.GetValue<string>() ?? "unknown";
            if (state == "failed")
            {
                var lastError = status.Payload["data"]?["lastError"]?.GetValue<string>()
                    ?? "O runtime OpenXR encerrou a sessão Unity durante a inicialização.";
                throw new InvalidOperationException(lastError);
            }

            activeSession = session;
            activeSessionMode = "Unity Stereo";
            SessionStatusText = $"Unity Stereo - jogo PID {gameWindow.ProcessId}";
            DiagnosticOutput = string.Join(
                Environment.NewLine,
                $"Player: {gameWindow.Title}",
                $"PID do jogo: {gameWindow.ProcessId}",
                $"Runtime: {runtimeManifest}",
                $"Transporte: {session.UnityTransportName}",
                $"Estado: {state}",
                session.Output);
            StatusMessage = "Unity Stereo iniciado; poses e texturas D3D11 estão ligadas ao OpenXR.";
            OnPropertyChanged(nameof(IsSessionActive));
            OnPropertyChanged(nameof(CanStartUnityStereo));
            RaiseCommandStates();
        }
        catch
        {
            await session.DisposeAsync();
            throw;
        }
    }

    private async Task StopSessionAsync()
    {
        var session = activeSession;
        if (session is null) return;
        activeSession = null;
        try
        {
            await session.StopAsync();
            if (!string.IsNullOrWhiteSpace(session.Output)) DiagnosticOutput = session.Output;
        }
        finally
        {
            await session.DisposeAsync();
            SessionStatusText = "Núcleo parado";
            StatusMessage = string.IsNullOrWhiteSpace(activeSessionMode)
                ? "Sessão OpenXR encerrada."
                : $"Sessão {activeSessionMode} encerrada.";
            activeSessionMode = string.Empty;
            OnPropertyChanged(nameof(IsSessionActive));
            OnPropertyChanged(nameof(CanStartUnityStereo));
            RaiseCommandStates();
        }
    }

    private async Task RecenterAsync()
    {
        if (activeSession is null) return;
        var response = await activeSession.RequestAsync("recenter");
        ThrowIfCoreError(response);
        StatusMessage = "Recentramento solicitado para o próximo frame rastreado.";
    }

    private static void ThrowIfCoreError(UxrbMessage message)
    {
        if (message.Kind != UxrbMessageKind.Error) return;
        var error = message.Payload["data"]?["message"]?.GetValue<string>() ?? "O núcleo nativo rejeitou o comando.";
        throw new InvalidOperationException(error);
    }

    public async ValueTask DisposeAsync()
    {
        var session = activeSession;
        activeSession = null;
        if (session is null) return;
        try { await session.DisposeAsync(); }
        catch { }
    }

    private async Task ExecuteSafelyAsync(Func<Task> operation)
    {
        if (IsBusy) return;
        IsBusy = true;
        try
        {
            await operation();
        }
        catch (Exception exception)
        {
            StatusMessage = exception.Message;
            DiagnosticOutput = exception.ToString();
        }
        finally
        {
            IsBusy = false;
        }
    }

    private void RaiseCommandStates()
    {
        refreshCommand.RaiseCanExecuteChanged();
        analyzeCommand.RaiseCanExecuteChanged();
        saveProfileCommand.RaiseCanExecuteChanged();
        diagnoseCommand.RaiseCanExecuteChanged();
        startVirtualScreenCommand.RaiseCanExecuteChanged();
        startUnityStereoCommand.RaiseCanExecuteChanged();
        stopSessionCommand.RaiseCanExecuteChanged();
        recenterCommand.RaiseCanExecuteChanged();
    }

    private void RaiseGameDetailProperties()
    {
        OnPropertyChanged(nameof(ExecutablePath));
        OnPropertyChanged(nameof(EngineText));
        OnPropertyChanged(nameof(BackendText));
        OnPropertyChanged(nameof(GraphicsApiText));
        OnPropertyChanged(nameof(PipelineText));
        OnPropertyChanged(nameof(ArchitectureText));
        OnPropertyChanged(nameof(AntiCheatText));
    }

    private static string FormatEnum<T>(T value) where T : struct, Enum => value switch
    {
        GameEngine.Unity => "Unity",
        GameEngine.Unreal => "Unreal Engine",
        GameEngine.Generic => "Genérico",
        ScriptingBackend.Il2Cpp => "IL2CPP",
        ScriptingBackend.Mono => "Mono",
        ScriptingBackend.Native => "Nativo",
        GraphicsApi.D3D11 => "Direct3D 11",
        GraphicsApi.D3D12 => "Direct3D 12",
        GraphicsApi.OpenGl => "OpenGL",
        GraphicsApi.Vulkan => "Vulkan",
        RenderPipeline.BuiltIn => "Built-in",
        RenderPipeline.Urp => "URP",
        RenderPipeline.Hdrp => "HDRP",
        RenderPipeline.Custom => "Customizado",
        ExecutableArchitecture.X64 => "x64",
        ExecutableArchitecture.X86 => "x86",
        ExecutableArchitecture.Arm64 => "ARM64",
        _ => "Desconhecido",
    };
}
