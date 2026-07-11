using System.Windows.Input;

namespace UniversalXRBridge.Launcher.Infrastructure;

public sealed class AsyncCommand : ICommand
{
    private readonly Func<Task> execute;
    private readonly Func<bool>? canExecute;
    private bool running;

    public AsyncCommand(Func<Task> execute, Func<bool>? canExecute = null)
    {
        this.execute = execute ?? throw new ArgumentNullException(nameof(execute));
        this.canExecute = canExecute;
    }

    public event EventHandler? CanExecuteChanged;

    public bool CanExecute(object? parameter) => !running && (canExecute?.Invoke() ?? true);

    public async void Execute(object? parameter)
    {
        if (!CanExecute(parameter))
        {
            return;
        }

        running = true;
        RaiseCanExecuteChanged();
        try
        {
            await execute().ConfigureAwait(true);
        }
        finally
        {
            running = false;
            RaiseCanExecuteChanged();
        }
    }

    public void RaiseCanExecuteChanged() => CanExecuteChanged?.Invoke(this, EventArgs.Empty);
}
