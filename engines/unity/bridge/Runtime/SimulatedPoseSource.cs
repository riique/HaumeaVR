using System;
using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity
{
    internal sealed class SimulatedPoseSource : MonoBehaviour
    {
        internal const string CommandLineSwitch = "-uxrb-simulated-pose";
        private ulong frameId;

        internal static bool IsRequested()
        {
            var arguments = Environment.GetCommandLineArgs();
            for (var index = 0; index < arguments.Length; index++)
            {
                if (string.Equals(arguments[index], CommandLineSwitch, StringComparison.OrdinalIgnoreCase))
                {
                    return true;
                }
            }
            return false;
        }

        private void Update()
        {
            var frame = NativeBridge.StereoFrame.Create();
            frame.FrameId = ++frameId;
            frame.RecommendedWidth = 1512;
            frame.RecommendedHeight = 1680;
            frame.PredictedDisplayTimeNanoseconds = (long)(Time.realtimeSinceStartupAsDouble * 1_000_000_000.0);
            frame.Left = CreateEye(-0.032f);
            frame.Right = CreateEye(0.032f);
            NativeBridge.UxrbUnity_SetStereoFrame(ref frame);
        }

        private static NativeBridge.EyePose CreateEye(float x)
        {
            return new NativeBridge.EyePose
            {
                PositionX = x,
                OrientationW = 1.0f,
                AngleLeft = -0.78f,
                AngleRight = 0.78f,
                AngleUp = 0.78f,
                AngleDown = -0.78f,
            };
        }
    }
}
