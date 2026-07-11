using UnityEngine;
using UnityEngine.Rendering;

namespace Haumea.UniversalXRBridge.Unity
{
    public static class UnityBridgeBootstrap
    {
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void StartIfRequested()
        {
            if (!BridgeActivation.IsRequestedForCurrentProcess())
            {
                return;
            }

            if (SystemInfo.graphicsDeviceType != GraphicsDeviceType.Direct3D11)
            {
                Debug.LogError("HaumeaVR requires Direct3D 11 and stopped without modifying the game camera.");
                return;
            }

            if (Object.FindFirstObjectByType<StereoRenderer>() != null)
            {
                return;
            }

            var bridgeObject = new GameObject("HaumeaVR");
            bridgeObject.hideFlags = HideFlags.HideAndDontSave;
            Object.DontDestroyOnLoad(bridgeObject);
            if (SimulatedPoseSource.IsRequested())
            {
                bridgeObject.AddComponent<SimulatedPoseSource>();
                Debug.LogWarning("HaumeaVR is using the compatibility-lab simulated pose source.");
            }
            bridgeObject.AddComponent<StereoRenderer>();
        }
    }
}
