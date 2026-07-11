using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity
{
    public static class StereoProjection
    {
        internal static Matrix4x4 FromOpenXrFov(NativeBridge.EyePose eye, float nearClip, float farClip)
        {
            return FromAngles(
                eye.AngleLeft,
                eye.AngleRight,
                eye.AngleUp,
                eye.AngleDown,
                nearClip,
                farClip);
        }

        public static Matrix4x4 FromAngles(
            float angleLeft,
            float angleRight,
            float angleUp,
            float angleDown,
            float nearClip,
            float farClip)
        {
            var left = Mathf.Tan(angleLeft) * nearClip;
            var right = Mathf.Tan(angleRight) * nearClip;
            var top = Mathf.Tan(angleUp) * nearClip;
            var bottom = Mathf.Tan(angleDown) * nearClip;
            return Matrix4x4.Frustum(left, right, bottom, top, nearClip, farClip);
        }
    }
}
