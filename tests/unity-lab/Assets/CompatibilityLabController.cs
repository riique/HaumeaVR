using UnityEngine;

namespace Haumea.UniversalXRBridge.Lab
{
    public sealed class CompatibilityLabController : MonoBehaviour
    {
        public Transform rotatingTarget;
        public Camera gameplayCamera;
        public Camera cutsceneCamera;
        private float cutsceneUntil;

        private void Update()
        {
            if (rotatingTarget != null)
            {
                rotatingTarget.Rotate(13.0f * Time.deltaTime, 28.0f * Time.deltaTime, 7.0f * Time.deltaTime);
            }

            if (Input.GetKeyDown(KeyCode.C) && gameplayCamera != null && cutsceneCamera != null)
            {
                gameplayCamera.enabled = false;
                cutsceneCamera.enabled = true;
                cutsceneUntil = Time.unscaledTime + 3.0f;
            }

            if (cutsceneCamera != null && cutsceneCamera.enabled && Time.unscaledTime >= cutsceneUntil)
            {
                cutsceneCamera.enabled = false;
                gameplayCamera.enabled = true;
            }
        }
    }
}
