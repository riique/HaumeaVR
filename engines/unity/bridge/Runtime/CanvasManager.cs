using System.Collections.Generic;
using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity
{
    public sealed class CanvasManager
    {
        private readonly List<Canvas> overlayCanvases = new List<Canvas>();
        private readonly List<Canvas> cameraCanvases = new List<Canvas>();
        private readonly List<Canvas> worldCanvases = new List<Canvas>();

        public IReadOnlyList<Canvas> OverlayCanvases => overlayCanvases;
        public IReadOnlyList<Canvas> CameraCanvases => cameraCanvases;
        public IReadOnlyList<Canvas> WorldCanvases => worldCanvases;

        public void Refresh()
        {
            overlayCanvases.Clear();
            cameraCanvases.Clear();
            worldCanvases.Clear();

            var canvases = Object.FindObjectsByType<Canvas>(FindObjectsInactive.Exclude, FindObjectsSortMode.None);
            for (var index = 0; index < canvases.Length; index++)
            {
                switch (canvases[index].renderMode)
                {
                    case RenderMode.ScreenSpaceOverlay:
                        overlayCanvases.Add(canvases[index]);
                        break;
                    case RenderMode.ScreenSpaceCamera:
                        cameraCanvases.Add(canvases[index]);
                        break;
                    case RenderMode.WorldSpace:
                        worldCanvases.Add(canvases[index]);
                        break;
                }
            }
        }
    }
}
