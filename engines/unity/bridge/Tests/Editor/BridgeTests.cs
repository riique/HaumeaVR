using NUnit.Framework;
using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity.Tests
{
    public sealed class BridgeTests
    {
        [Test]
        public void ActivationRequiresExactOptIn()
        {
            Assert.That(BridgeActivation.IsRequested(new[] { "game.exe" }, null), Is.False);
            Assert.That(BridgeActivation.IsRequested(new[] { "game.exe", "--uxrb-enable" }, null), Is.False);
            Assert.That(BridgeActivation.IsRequested(new[] { "game.exe", "-uxrb-enable" }, null), Is.True);
            Assert.That(BridgeActivation.IsRequested(new[] { "game.exe" }, "true"), Is.True);
        }

        [Test]
        public void CameraSelectionPrefersMainGameplayCamera()
        {
            var mainObject = new GameObject("Main");
            var helperObject = new GameObject("Helper");
            try
            {
                mainObject.tag = "MainCamera";
                var main = mainObject.AddComponent<Camera>();
                main.depth = 0;
                var helper = helperObject.AddComponent<Camera>();
                helper.depth = 50;
                helper.clearFlags = CameraClearFlags.Depth;
                Assert.That(CameraManager.SelectBestCamera(new[] { helper, main }), Is.SameAs(main));
            }
            finally
            {
                Object.DestroyImmediate(mainObject);
                Object.DestroyImmediate(helperObject);
            }
        }

        [Test]
        public void AsymmetricProjectionPreservesHorizontalOffset()
        {
            var projection = StereoProjection.FromAngles(-0.6f, 0.8f, 0.7f, -0.7f, 0.05f, 500.0f);
            Assert.That(projection.m02, Is.Not.EqualTo(0.0f).Within(0.0001f));
            Assert.That(projection.m00, Is.GreaterThan(0.0f));
        }
    }
}
