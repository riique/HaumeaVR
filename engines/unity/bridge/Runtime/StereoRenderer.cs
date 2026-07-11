using System.Collections;
using UnityEngine;

namespace Haumea.UniversalXRBridge.Unity
{
    public sealed class StereoRenderer : MonoBehaviour
    {
        private readonly CameraManager cameraManager = new CameraManager();
        private readonly CanvasManager canvasManager = new CanvasManager();
        private readonly CutsceneDetector cutsceneDetector = new CutsceneDetector();
        private readonly HeadTrackingController headTracking = new HeadTrackingController();
        private readonly InputMapper inputMapper = new InputMapper();
        private readonly PostProcessingController postProcessing = new PostProcessingController();
        private readonly OpenXRSharedTransport transport = new OpenXRSharedTransport();

        private Camera leftCamera;
        private Camera rightCamera;
        private RenderTexture leftTexture;
        private RenderTexture rightTexture;
        private Coroutine renderLoop;
        private ulong fallbackFrameId;

        private void Awake()
        {
            if (!transport.Initialize())
            {
                enabled = false;
                return;
            }

            leftCamera = CreateEyeCamera("UniversalXR Left Eye");
            rightCamera = CreateEyeCamera("UniversalXR Right Eye");
            canvasManager.Refresh();
            Debug.Log(string.Format(
                "HaumeaVR enabled: {0} overlay, {1} camera and {2} world-space canvases detected.",
                canvasManager.OverlayCanvases.Count,
                canvasManager.CameraCanvases.Count,
                canvasManager.WorldCanvases.Count));
            renderLoop = StartCoroutine(RenderAtEndOfFrame());
        }

        private void Update()
        {
            var source = cameraManager.Current;
            cutsceneDetector.Observe(source);
            if (inputMapper.RecenterRequested)
            {
                headTracking.ResetAnchor();
            }
        }

        private IEnumerator RenderAtEndOfFrame()
        {
            var endOfFrame = new WaitForEndOfFrame();
            while (enabled)
            {
                yield return endOfFrame;
                RenderStereoFrame();
            }
        }

        private void RenderStereoFrame()
        {
            var source = cameraManager.Current;
            if (source == null || !transport.TryGetFrame(out var frame))
            {
                return;
            }

            var width = frame.RecommendedWidth == 0 ? 1512 : (int)frame.RecommendedWidth;
            var height = frame.RecommendedHeight == 0 ? 1680 : (int)frame.RecommendedHeight;
            EnsureTextures(width, height);
            if (!transport.RegisterTextures(leftTexture, rightTexture))
            {
                return;
            }

            RenderEye(leftCamera, leftTexture, source, frame.Left, frame.Right);
            RenderEye(rightCamera, rightTexture, source, frame.Right, frame.Left);
            transport.Submit(frame.FrameId == 0 ? ++fallbackFrameId : frame.FrameId);
        }

        private void RenderEye(
            Camera eyeCamera,
            RenderTexture target,
            Camera source,
            NativeBridge.EyePose eye,
            NativeBridge.EyePose otherEye)
        {
            postProcessing.PrepareEye(source, eyeCamera);
            eyeCamera.targetTexture = target;
            headTracking.Apply(eyeCamera, source, eye, otherEye);
            eyeCamera.projectionMatrix = StereoProjection.FromOpenXrFov(eye, source.nearClipPlane, source.farClipPlane);
            eyeCamera.Render();
            eyeCamera.targetTexture = null;
            eyeCamera.ResetProjectionMatrix();
        }

        private void EnsureTextures(int width, int height)
        {
            if (leftTexture != null && leftTexture.width == width && leftTexture.height == height) return;
            ReleaseTexture(ref leftTexture);
            ReleaseTexture(ref rightTexture);
            leftTexture = CreateEyeTexture("UniversalXR Left Shared Source", width, height);
            rightTexture = CreateEyeTexture("UniversalXR Right Shared Source", width, height);
        }

        private static Camera CreateEyeCamera(string objectName)
        {
            var eyeObject = new GameObject(objectName);
            eyeObject.hideFlags = HideFlags.HideAndDontSave;
            DontDestroyOnLoad(eyeObject);
            var camera = eyeObject.AddComponent<Camera>();
            camera.enabled = false;
            return camera;
        }

        private static RenderTexture CreateEyeTexture(string textureName, int width, int height)
        {
            var texture = new RenderTexture(width, height, 24, RenderTextureFormat.ARGB32)
            {
                name = textureName,
                antiAliasing = 1,
                useMipMap = false,
                autoGenerateMips = false,
            };
            texture.Create();
            return texture;
        }

        private static void ReleaseTexture(ref RenderTexture texture)
        {
            if (texture == null) return;
            texture.Release();
            Destroy(texture);
            texture = null;
        }

        private void OnDestroy()
        {
            if (renderLoop != null) StopCoroutine(renderLoop);
            transport.Dispose();
            ReleaseTexture(ref leftTexture);
            ReleaseTexture(ref rightTexture);
            if (leftCamera != null) Destroy(leftCamera.gameObject);
            if (rightCamera != null) Destroy(rightCamera.gameObject);
        }
    }
}
