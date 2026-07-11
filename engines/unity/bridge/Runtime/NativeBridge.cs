using System;
using System.Runtime.InteropServices;

namespace Haumea.UniversalXRBridge.Unity
{
    internal static class NativeBridge
    {
        internal const uint ApiVersion = 1;
        private const string Library = "uxrb_unity_bridge";

        [StructLayout(LayoutKind.Sequential)]
        internal struct EyePose
        {
            internal float PositionX;
            internal float PositionY;
            internal float PositionZ;
            internal float OrientationX;
            internal float OrientationY;
            internal float OrientationZ;
            internal float OrientationW;
            internal float AngleLeft;
            internal float AngleRight;
            internal float AngleUp;
            internal float AngleDown;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct StereoFrame
        {
            internal uint StructSize;
            internal uint ApiVersion;
            internal ulong FrameId;
            internal long PredictedDisplayTimeNanoseconds;
            internal uint RecommendedWidth;
            internal uint RecommendedHeight;
            internal uint Flags;
            internal uint Reserved;
            internal EyePose Left;
            internal EyePose Right;

            internal static StereoFrame Create()
            {
                return new StereoFrame
                {
                    StructSize = (uint)Marshal.SizeOf<StereoFrame>(),
                    ApiVersion = NativeBridge.ApiVersion,
                };
            }
        }

        [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
        internal static extern uint UxrbUnity_GetApiVersion();

        [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
        internal static extern int UxrbUnity_GetCopyStereoFrameEventId();

        [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
        internal static extern IntPtr UxrbUnity_GetRenderEventFunc();

        [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
        internal static extern int UxrbUnity_RegisterEyeTexture(
            int eye,
            IntPtr nativeTexture,
            uint expectedWidth,
            uint expectedHeight);

        [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
        internal static extern void UxrbUnity_SubmitFrame(ulong frameId);

        [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
        internal static extern int UxrbUnity_SetStereoFrame(ref StereoFrame frame);

        [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
        internal static extern int UxrbUnity_TryGetStereoFrame(ref StereoFrame frame);

        [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
        internal static extern void UxrbUnity_Reset();
    }
}
