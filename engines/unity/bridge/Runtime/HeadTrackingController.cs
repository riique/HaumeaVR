using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity
{
    public enum HeadTrackingMode
    {
        RotationOnly,
        FullPosition,
        Decoupled,
    }

    internal sealed class HeadTrackingController
    {
        private bool anchored;
        private Vector3 sourcePosition;
        private Quaternion sourceRotation;
        private Vector3 xrCenterPosition;
        private Quaternion xrCenterRotation;

        public HeadTrackingMode Mode { get; set; } = HeadTrackingMode.RotationOnly;
        public float WorldScale { get; set; } = 1.0f;
        public float MaximumTranslation { get; set; } = 0.35f;

        public void Recenter(Camera source, NativeBridge.EyePose left, NativeBridge.EyePose right)
        {
            sourcePosition = source.transform.position;
            sourceRotation = source.transform.rotation;
            xrCenterPosition = (ToPosition(left) + ToPosition(right)) * 0.5f;
            xrCenterRotation = Quaternion.Slerp(ToRotation(left), ToRotation(right), 0.5f);
            anchored = true;
        }

        public void Apply(Camera target, Camera source, NativeBridge.EyePose eye, NativeBridge.EyePose otherEye)
        {
            if (!anchored)
            {
                Recenter(source, eye, otherEye);
            }

            var relativeRotation = Quaternion.Inverse(xrCenterRotation) * ToRotation(eye);
            target.transform.rotation = Mode == HeadTrackingMode.Decoupled
                ? source.transform.rotation
                : sourceRotation * relativeRotation;

            if (Mode == HeadTrackingMode.FullPosition)
            {
                var offset = (ToPosition(eye) - xrCenterPosition) * WorldScale;
                offset = Vector3.ClampMagnitude(offset, MaximumTranslation);
                target.transform.position = sourcePosition + sourceRotation * offset;
            }
            else
            {
                var center = (ToPosition(eye) + ToPosition(otherEye)) * 0.5f;
                var eyeOffset = (ToPosition(eye) - center) * WorldScale;
                target.transform.position = source.transform.position + source.transform.rotation * eyeOffset;
            }
        }

        public void ResetAnchor()
        {
            anchored = false;
        }

        private static Vector3 ToPosition(NativeBridge.EyePose eye)
        {
            // OpenXR is right-handed with -Z forward; Unity is left-handed with +Z forward.
            return new Vector3(eye.PositionX, eye.PositionY, -eye.PositionZ);
        }

        private static Quaternion ToRotation(NativeBridge.EyePose eye)
        {
            return new Quaternion(-eye.OrientationX, -eye.OrientationY, eye.OrientationZ, eye.OrientationW);
        }
    }
}
