using System;
using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity
{
    internal sealed class OpenXRSharedTransport : IDisposable
    {
        private IntPtr renderEvent;
        private int renderEventId;
        private bool available;
        private RenderTexture registeredLeft;
        private RenderTexture registeredRight;

        public bool IsAvailable => available;

        public bool Initialize()
        {
            try
            {
                available = NativeBridge.UxrbUnity_GetApiVersion() == NativeBridge.ApiVersion;
                if (!available) return false;
                renderEvent = NativeBridge.UxrbUnity_GetRenderEventFunc();
                renderEventId = NativeBridge.UxrbUnity_GetCopyStereoFrameEventId();
                available = renderEvent != IntPtr.Zero;
                return available;
            }
            catch (Exception exception) when (
                exception is DllNotFoundException || exception is EntryPointNotFoundException || exception is BadImageFormatException)
            {
                Debug.LogError("HaumeaVR native plug-in is unavailable: " + exception.Message);
                available = false;
                return false;
            }
        }

        public bool TryGetFrame(out NativeBridge.StereoFrame frame)
        {
            frame = NativeBridge.StereoFrame.Create();
            return available && NativeBridge.UxrbUnity_TryGetStereoFrame(ref frame) == 1;
        }

        public bool RegisterTextures(RenderTexture left, RenderTexture right)
        {
            if (!available || left == null || right == null) return false;
            if (registeredLeft == left && registeredRight == right) return true;

            left.Create();
            right.Create();
            var leftAccepted = NativeBridge.UxrbUnity_RegisterEyeTexture(
                0, left.GetNativeTexturePtr(), (uint)left.width, (uint)left.height) == 1;
            var rightAccepted = NativeBridge.UxrbUnity_RegisterEyeTexture(
                1, right.GetNativeTexturePtr(), (uint)right.width, (uint)right.height) == 1;
            if (leftAccepted && rightAccepted)
            {
                registeredLeft = left;
                registeredRight = right;
                return true;
            }
            return false;
        }

        public void Submit(ulong frameId)
        {
            if (!available) return;
            NativeBridge.UxrbUnity_SubmitFrame(frameId);
            GL.IssuePluginEvent(renderEvent, renderEventId);
        }

        public void Dispose()
        {
            if (!available) return;
            NativeBridge.UxrbUnity_RegisterEyeTexture(0, IntPtr.Zero, 0, 0);
            NativeBridge.UxrbUnity_RegisterEyeTexture(1, IntPtr.Zero, 0, 0);
            NativeBridge.UxrbUnity_Reset();
            available = false;
        }
    }
}
