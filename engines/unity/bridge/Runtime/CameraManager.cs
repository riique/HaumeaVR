using System.Collections.Generic;
using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity
{
    public sealed class CameraManager
    {
        private Camera current;

        public Camera Current
        {
            get
            {
                if (!IsUsable(current))
                {
                    current = FindBestCamera();
                }
                return current;
            }
        }

        public Camera Refresh()
        {
            current = FindBestCamera();
            return current;
        }

        public static Camera FindBestCamera()
        {
            var cameras = Camera.allCameras;
            Camera best = null;
            var bestScore = int.MinValue;
            for (var index = 0; index < cameras.Length; index++)
            {
                var candidate = cameras[index];
                if (!IsUsable(candidate))
                {
                    continue;
                }

                var score = Score(candidate);
                if (score > bestScore)
                {
                    best = candidate;
                    bestScore = score;
                }
            }
            return best;
        }

        public static Camera SelectBestCamera(IList<Camera> cameras)
        {
            Camera best = null;
            var bestScore = int.MinValue;
            for (var index = 0; index < cameras.Count; index++)
            {
                var candidate = cameras[index];
                if (!IsUsable(candidate))
                {
                    continue;
                }

                var score = Score(candidate);
                if (score > bestScore)
                {
                    best = candidate;
                    bestScore = score;
                }
            }
            return best;
        }

        private static int Score(Camera camera)
        {
            var score = Mathf.RoundToInt(camera.depth * 10.0f);
            if (camera.CompareTag("MainCamera")) score += 1000;
            if (camera.targetTexture == null) score += 120;
            if (camera.cameraType == CameraType.Game) score += 100;
            if (camera.orthographic) score -= 250;
            if (camera.clearFlags == CameraClearFlags.Depth || camera.clearFlags == CameraClearFlags.Nothing) score -= 80;
            return score;
        }

        private static bool IsUsable(Camera camera)
        {
            return camera != null && camera.enabled && camera.gameObject.activeInHierarchy &&
                   camera.gameObject.scene.IsValid() && camera.cameraType == CameraType.Game;
        }
    }
}
