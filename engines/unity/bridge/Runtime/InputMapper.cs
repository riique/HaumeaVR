using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity
{
    public sealed class InputMapper
    {
        public bool RecenterRequested => Input.GetKeyDown(KeyCode.F8);
    }
}
