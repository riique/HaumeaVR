using System;
using System.IO;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.Rendering;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

namespace Haumea.UniversalXRBridge.Lab
{
    public static class CompatibilityLabBuilder
    {
        private const string ScenePath = "Assets/Scenes/CompatibilityLab.unity";
        private const string GeneratedFolder = "Assets/Generated";

        [MenuItem("UniversalXR/Generate Compatibility Lab")]
        public static void GenerateScene()
        {
            EnsureFolder("Assets/Scenes");
            EnsureFolder(GeneratedFolder);
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

            var neutral = CreateMaterial("Neutral", new Color(0.68f, 0.70f, 0.72f), 0.1f, 0.4f);
            var red = CreateMaterial("Red", new Color(0.92f, 0.08f, 0.07f), 0.0f, 0.35f);
            var blue = CreateMaterial("Blue", new Color(0.02f, 0.12f, 0.95f), 0.1f, 0.3f);
            var mirror = CreateMaterial("Mirror", new Color(0.75f, 0.82f, 0.88f), 0.95f, 0.05f);
            var glass = CreateTransparentMaterial("Glass", new Color(0.05f, 0.55f, 1.0f, 0.35f));
            var water = CreateTransparentMaterial("Water", new Color(0.0f, 0.22f, 0.58f, 0.58f));

            CreateLight();
            var floor = CreatePrimitive(PrimitiveType.Plane, "Ground Grid", Vector3.zero, new Vector3(4.0f, 1.0f, 4.0f), neutral);
            floor.GetComponent<Renderer>().material.mainTextureScale = new Vector2(8.0f, 8.0f);

            var rotating = CreatePrimitive(PrimitiveType.Cube, "Rotating Geometry", new Vector3(0.0f, 1.15f, 1.5f), Vector3.one, red);
            CreatePrimitive(PrimitiveType.Sphere, "Transparent Sphere", new Vector3(-2.0f, 1.0f, 2.2f), Vector3.one * 1.35f, glass);
            CreatePrimitive(PrimitiveType.Capsule, "Depth Reference Near", new Vector3(2.1f, 1.0f, 0.0f), Vector3.one, blue);
            CreatePrimitive(PrimitiveType.Cylinder, "Depth Reference Far", new Vector3(2.6f, 1.0f, 5.5f), Vector3.one, neutral);
            CreatePrimitive(PrimitiveType.Plane, "Water Surface", new Vector3(0.0f, 0.08f, 6.0f), new Vector3(2.2f, 1.0f, 1.2f), water);
            CreatePrimitive(PrimitiveType.Quad, "Mirror Surface", new Vector3(-3.8f, 1.6f, 4.2f), new Vector3(2.3f, 2.3f, 1.0f), mirror)
                .transform.rotation = Quaternion.Euler(0.0f, 32.0f, 0.0f);
            CreatePortalFrame(3.8f, 4.5f, blue);
            CreateParticles();

            var gameplayCamera = CreateCamera("Gameplay Camera", new Vector3(0.0f, 1.65f, -6.0f), Quaternion.identity, 0.0f, true);
            var overlayCamera = CreateCamera("Secondary Depth Camera", new Vector3(0.0f, 8.0f, 0.0f), Quaternion.Euler(90.0f, 0.0f, 0.0f), -10.0f, false);
            overlayCamera.clearFlags = CameraClearFlags.Depth;
            overlayCamera.cullingMask = 0;
            var cutsceneCamera = CreateCamera("Cutscene Camera", new Vector3(5.5f, 3.2f, -1.0f), Quaternion.Euler(12.0f, -52.0f, 0.0f), 20.0f, false);
            cutsceneCamera.enabled = false;

            CreateOverlayCanvas();
            CreateWorldCanvas();
            CreateEventSystem();

            var controller = new GameObject("Compatibility Lab Controller").AddComponent<CompatibilityLabController>();
            controller.rotatingTarget = rotating.transform;
            controller.gameplayCamera = gameplayCamera;
            controller.cutsceneCamera = cutsceneCamera;

            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, ScenePath);
            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) };
            AssetDatabase.SaveAssets();
            Debug.Log("UniversalXR Unity Compatibility Lab generated at " + ScenePath);
        }

        public static void BuildMonoPlayer()
        {
            BuildPlayer(ScriptingImplementation.Mono2x, "Mono");
        }

        public static void BuildIl2CppPlayer()
        {
            BuildPlayer(ScriptingImplementation.IL2CPP, "IL2CPP");
        }

        private static void BuildPlayer(ScriptingImplementation backend, string backendName)
        {
            GenerateScene();
            PlayerSettings.productName = "UniversalXR Unity Compatibility Lab";
            PlayerSettings.companyName = "Haumea";
            PlayerSettings.SetScriptingBackend(NamedBuildTarget.Standalone, backend);
            PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.StandaloneWindows64, false);
            PlayerSettings.SetGraphicsAPIs(BuildTarget.StandaloneWindows64, new[] { GraphicsDeviceType.Direct3D11 });
            PlayerSettings.fullScreenMode = FullScreenMode.Windowed;
            PlayerSettings.defaultScreenWidth = 1280;
            PlayerSettings.defaultScreenHeight = 720;
            PlayerSettings.runInBackground = true;

            var output = ReadCommandLineValue("-uxrb-output");
            if (string.IsNullOrWhiteSpace(output))
            {
                output = Path.GetFullPath(Path.Combine(Application.dataPath, "../../../artifacts/unity-lab/" + backendName));
            }
            Directory.CreateDirectory(output);

            var report = BuildPipeline.BuildPlayer(new BuildPlayerOptions
            {
                scenes = new[] { ScenePath },
                locationPathName = Path.Combine(output, "UniversalXRCompatibilityLab.exe"),
                target = BuildTarget.StandaloneWindows64,
                options = BuildOptions.Development,
            });
            if (report.summary.result != BuildResult.Succeeded)
            {
                throw new BuildFailedException(
                    string.Format("Unity lab {0} build failed with {1} errors.", backendName, report.summary.totalErrors));
            }
            Debug.Log(string.Format(
                "Unity lab {0} build completed: {1} bytes in {2}.",
                backendName,
                report.summary.totalSize,
                report.summary.outputPath));
        }

        private static Camera CreateCamera(
            string objectName,
            Vector3 position,
            Quaternion rotation,
            float depth,
            bool mainCamera)
        {
            var cameraObject = new GameObject(objectName);
            cameraObject.transform.SetPositionAndRotation(position, rotation);
            if (mainCamera) cameraObject.tag = "MainCamera";
            var camera = cameraObject.AddComponent<Camera>();
            camera.depth = depth;
            camera.nearClipPlane = 0.05f;
            camera.farClipPlane = 500.0f;
            camera.fieldOfView = 70.0f;
            camera.clearFlags = CameraClearFlags.Skybox;
            if (mainCamera) cameraObject.AddComponent<AudioListener>();
            return camera;
        }

        private static void CreateLight()
        {
            var lightObject = new GameObject("Directional Light");
            lightObject.transform.rotation = Quaternion.Euler(42.0f, -30.0f, 0.0f);
            var light = lightObject.AddComponent<Light>();
            light.type = LightType.Directional;
            light.intensity = 1.25f;
            light.shadows = LightShadows.Soft;
            RenderSettings.ambientLight = new Color(0.32f, 0.34f, 0.4f);
        }

        private static GameObject CreatePrimitive(
            PrimitiveType type,
            string objectName,
            Vector3 position,
            Vector3 scale,
            Material material)
        {
            var value = GameObject.CreatePrimitive(type);
            value.name = objectName;
            value.transform.position = position;
            value.transform.localScale = scale;
            value.GetComponent<Renderer>().sharedMaterial = material;
            return value;
        }

        private static void CreatePortalFrame(float x, float z, Material material)
        {
            CreatePrimitive(PrimitiveType.Cube, "Portal Left", new Vector3(x - 1.0f, 1.5f, z), new Vector3(0.18f, 3.0f, 0.18f), material);
            CreatePrimitive(PrimitiveType.Cube, "Portal Right", new Vector3(x + 1.0f, 1.5f, z), new Vector3(0.18f, 3.0f, 0.18f), material);
            CreatePrimitive(PrimitiveType.Cube, "Portal Top", new Vector3(x, 3.0f, z), new Vector3(2.18f, 0.18f, 0.18f), material);
        }

        private static void CreateParticles()
        {
            var particles = new GameObject("Transparent Particles").AddComponent<ParticleSystem>();
            particles.transform.position = new Vector3(-1.0f, 0.2f, 4.2f);
            var main = particles.main;
            main.startLifetime = 3.5f;
            main.startSpeed = 1.1f;
            main.startSize = 0.12f;
            main.startColor = new Color(0.2f, 0.65f, 1.0f, 0.7f);
            main.maxParticles = 500;
            var emission = particles.emission;
            emission.rateOverTime = 55.0f;
            var shape = particles.shape;
            shape.shapeType = ParticleSystemShapeType.Cone;
            shape.angle = 18.0f;
        }

        private static void CreateOverlayCanvas()
        {
            var canvasObject = new GameObject("Overlay HUD", typeof(Canvas), typeof(CanvasScaler), typeof(GraphicRaycaster));
            var canvas = canvasObject.GetComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            var scaler = canvasObject.GetComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1280.0f, 720.0f);

            var panel = new GameObject("HUD Panel", typeof(RectTransform), typeof(Image));
            panel.transform.SetParent(canvasObject.transform, false);
            var panelRect = panel.GetComponent<RectTransform>();
            panelRect.anchorMin = new Vector2(0.0f, 1.0f);
            panelRect.anchorMax = new Vector2(0.0f, 1.0f);
            panelRect.pivot = new Vector2(0.0f, 1.0f);
            panelRect.anchoredPosition = new Vector2(28.0f, -28.0f);
            panelRect.sizeDelta = new Vector2(430.0f, 116.0f);
            panel.GetComponent<Image>().color = new Color(0.02f, 0.02f, 0.02f, 0.84f);

            var textObject = new GameObject("Instructions", typeof(RectTransform), typeof(Text));
            textObject.transform.SetParent(panel.transform, false);
            var textRect = textObject.GetComponent<RectTransform>();
            textRect.anchorMin = Vector2.zero;
            textRect.anchorMax = Vector2.one;
            textRect.offsetMin = new Vector2(18.0f, 12.0f);
            textRect.offsetMax = new Vector2(-18.0f, -12.0f);
            var text = textObject.GetComponent<Text>();
            text.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            text.fontSize = 19;
            text.color = Color.white;
            text.alignment = TextAnchor.MiddleLeft;
            text.text = "HAUMEAVR COMPATIBILITY LAB\nC — cutscene camera   F8 — recenter\nLaunch: -uxrb-enable -uxrb-simulated-pose";
        }

        private static void CreateWorldCanvas()
        {
            var canvasObject = new GameObject("World Space HUD", typeof(Canvas));
            canvasObject.transform.position = new Vector3(0.0f, 2.6f, 3.8f);
            canvasObject.transform.rotation = Quaternion.Euler(0.0f, 180.0f, 0.0f);
            canvasObject.transform.localScale = Vector3.one * 0.004f;
            var canvas = canvasObject.GetComponent<Canvas>();
            canvas.renderMode = RenderMode.WorldSpace;
            var rect = canvas.GetComponent<RectTransform>();
            rect.sizeDelta = new Vector2(420.0f, 100.0f);

            var textObject = new GameObject("World Label", typeof(RectTransform), typeof(Text));
            textObject.transform.SetParent(canvasObject.transform, false);
            var textRect = textObject.GetComponent<RectTransform>();
            textRect.anchorMin = Vector2.zero;
            textRect.anchorMax = Vector2.one;
            textRect.offsetMin = Vector2.zero;
            textRect.offsetMax = Vector2.zero;
            var text = textObject.GetComponent<Text>();
            text.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            text.fontSize = 36;
            text.color = Color.white;
            text.alignment = TextAnchor.MiddleCenter;
            text.text = "WORLD-SPACE HUD";
        }

        private static void CreateEventSystem()
        {
            new GameObject("EventSystem", typeof(EventSystem), typeof(StandaloneInputModule));
        }

        private static Material CreateMaterial(string name, Color color, float metallic, float smoothness)
        {
            var path = GeneratedFolder + "/" + name + ".mat";
            AssetDatabase.DeleteAsset(path);
            var material = new Material(Shader.Find("Standard"))
            {
                name = name,
                color = color,
            };
            material.SetFloat("_Metallic", metallic);
            material.SetFloat("_Glossiness", smoothness);
            AssetDatabase.CreateAsset(material, path);
            return material;
        }

        private static Material CreateTransparentMaterial(string name, Color color)
        {
            var material = CreateMaterial(name, color, 0.0f, 0.55f);
            material.SetFloat("_Mode", 3.0f);
            material.SetInt("_SrcBlend", (int)BlendMode.SrcAlpha);
            material.SetInt("_DstBlend", (int)BlendMode.OneMinusSrcAlpha);
            material.SetInt("_ZWrite", 0);
            material.DisableKeyword("_ALPHATEST_ON");
            material.EnableKeyword("_ALPHABLEND_ON");
            material.DisableKeyword("_ALPHAPREMULTIPLY_ON");
            material.renderQueue = 3000;
            return material;
        }

        private static void EnsureFolder(string path)
        {
            var segments = path.Split('/');
            var current = segments[0];
            for (var index = 1; index < segments.Length; index++)
            {
                var next = current + "/" + segments[index];
                if (!AssetDatabase.IsValidFolder(next)) AssetDatabase.CreateFolder(current, segments[index]);
                current = next;
            }
        }

        private static string ReadCommandLineValue(string key)
        {
            var arguments = Environment.GetCommandLineArgs();
            for (var index = 0; index + 1 < arguments.Length; index++)
            {
                if (string.Equals(arguments[index], key, StringComparison.OrdinalIgnoreCase))
                {
                    return arguments[index + 1];
                }
            }
            return null;
        }
    }
}
