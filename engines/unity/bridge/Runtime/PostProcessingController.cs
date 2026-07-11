using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity
{
    public sealed class PostProcessingController
    {
        public void PrepareEye(Camera source, Camera eye)
        {
            // Camera.CopyFrom carries Built-in command buffers and image effects without
            // cloning gameplay scripts. Pipeline-specific policy belongs in later adapters.
            eye.CopyFrom(source);
            eye.enabled = false;
            eye.stereoTargetEye = StereoTargetEyeMask.None;
        }
    }
}
