using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity
{
    public sealed class CutsceneDetector
    {
        private Camera previous;
        private float lastChangeTime;

        public bool IsLikelyCutscene { get; private set; }

        public void Observe(Camera current)
        {
            if (current != previous)
            {
                previous = current;
                lastChangeTime = Time.unscaledTime;
                IsLikelyCutscene = true;
                return;
            }
            IsLikelyCutscene = Time.unscaledTime - lastChangeTime < 1.5f;
        }
    }
}
