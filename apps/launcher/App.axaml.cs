using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using UniversalXRBridge.Launcher.Services;
using UniversalXRBridge.Launcher.ViewModels;
using UniversalXRBridge.Launcher.Views;

namespace UniversalXRBridge.Launcher;

public sealed partial class App : Application
{
    public override void Initialize() => AvaloniaXamlLoader.Load(this);

    public override void OnFrameworkInitializationCompleted()
    {
        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            var viewModel = new MainWindowViewModel(
                new GameDiscoveryService(),
                new EngineDetectionService(),
                new ProfileRepository(),
                new OpenXrRuntimeService(),
                new CoreProcessService(),
                new GameWindowService(),
                new UnityBridgeDeploymentService());
            var window = new MainWindow { DataContext = viewModel };
            window.Opened += async (_, _) => await viewModel.InitializeAsync();
            window.Closed += async (_, _) => await viewModel.DisposeAsync();
            desktop.MainWindow = window;
        }
        base.OnFrameworkInitializationCompleted();
    }
}
