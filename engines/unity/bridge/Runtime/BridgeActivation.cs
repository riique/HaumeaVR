using System;

namespace Haumea.UniversalXRBridge.Unity
{
    public static class BridgeActivation
    {
        public const string CommandLineSwitch = "-uxrb-enable";
        public const string EnvironmentVariable = "UXRB_UNITY_ENABLE";

        public static bool IsRequested(string[] arguments, string environmentValue)
        {
            if (arguments != null)
            {
                for (var index = 0; index < arguments.Length; index++)
                {
                    if (string.Equals(arguments[index], CommandLineSwitch, StringComparison.OrdinalIgnoreCase))
                    {
                        return true;
                    }
                }
            }

            return string.Equals(environmentValue, "1", StringComparison.Ordinal) ||
                   string.Equals(environmentValue, "true", StringComparison.OrdinalIgnoreCase);
        }

        public static bool IsRequestedForCurrentProcess()
        {
            return IsRequested(
                Environment.GetCommandLineArgs(),
                Environment.GetEnvironmentVariable(EnvironmentVariable));
        }
    }
}
