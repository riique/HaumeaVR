using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Platform.Storage;
using UniversalXRBridge.Launcher.ViewModels;

namespace UniversalXRBridge.Launcher.Views;

public sealed partial class MainWindow : Window
{
    public MainWindow() => InitializeComponent();

    private async void AddExecutableClicked(object? sender, RoutedEventArgs e)
    {
        if (DataContext is not MainWindowViewModel viewModel) return;
        var files = await StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
        {
            Title = "Adicionar executável de jogo",
            AllowMultiple = false,
            FileTypeFilter =
            [
                new FilePickerFileType("Executável do Windows") { Patterns = ["*.exe"] },
            ],
        });
        var selected = files.FirstOrDefault();
        if (selected?.TryGetLocalPath() is { } path) await viewModel.AddManualGameAsync(path);
    }
}
