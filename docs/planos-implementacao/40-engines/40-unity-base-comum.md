# P40 — Base comum de adaptadores Unity

**Status:** fundação parcial existente  
**Prioridade:** crítica  
**Dependências:** P12–P14, P35, P60–P61  
**Desbloqueia:** P41, P42, P50

## Objetivo

Separar a lógica Unity reutilizável de Mono/IL2CPP e de jogos específicos: câmera, estéreo, HUD, input, cena, cutscene, mãos e transporte. O mesmo comportamento deve rodar em labs Mono e IL2CPP.

## Estado inicial

`engines/unity/bridge/Runtime` possui:

- bootstrap;
- camera/stereo renderer;
- head tracking;
- canvas manager;
- input mapper;
- cutscene/post-processing;
- native bridge e shared transport.

O caminho está validado em código/testes, mas só é habilitado em player Mono controlado. O lab usa Unity 6000.3; MiSide usa 2021.3.35f1 IL2CPP.

## Camadas

```text
Haumea.Unity.Common
├── Lifecycle/SceneTracker
├── CameraDiscovery
├── StereoRig
├── RenderPipelineStrategy
├── CanvasStrategy
├── InputSemanticBridge
├── HandRigBridge
├── CutsceneContext
├── Quality/Comfort
└── TransportClient

Bootstrap provider
├── Mono provider
└── IL2CPP provider

Game adapter
└── roles, actions, interactables, fixes
```

Common não referencia BepInEx/Il2CppInterop. Providers implement interfaces pequenas.

## Famílias alvo

Labs mínimos:

- Unity 2019.4 LTS;
- Unity 2021.3 LTS, prioridade MiSide;
- Unity 2022.3 LTS;
- Unity 6 LTS/6000.3;
- x64 Windows;
- D3D11 primeiro, D3D12 depois.

Cada combinação não precisa ter todas as pipelines de início. A matriz declara cobertura real.

## Descoberta

Detectar:

- versão;
- Mono/IL2CPP;
- Built-in/URP/HDRP/custom;
- legacy/new Input System;
- D3D11/D3D12/Vulkan;
- cameras/camera stack;
- active scene changes;
- XR já ativo no jogo;
- anti-cheat/mod policy;
- known mod loaders.

Se o jogo já usa OpenXR/VR nativo, HaumeaVR não deve abrir integração concorrente; oferecer launch nativo ou L1.

## Câmera

### Seleção

Score por:

- enabled/active;
- target display/texture;
- depth/order;
- tag MainCamera;
- AudioListener;
- pixel rect;
- movimento junto ao player;
- scene role;
- adapter override.

Nunca depender só de `Camera.main`.

### Stereo rig

- criar câmeras auxiliares sem scripts/gameplay/audio;
- copiar propriedades render relevantes;
- aplicar poses/FOV assimétricos do core;
- preservar clear flags/culling masks;
- não duplicar simulation/update;
- render em targets compartilhados;
- restaurar estado;
- acompanhar camera replacement/scene load;
- camera stacking/pipelines por strategy.

### Head tracking

- coupled, decoupled, hybrid, rotation-only, 6DoF;
- base pose do jogo + delta físico;
- world scale;
- neck model;
- collision clamp via game adapter opcional;
- cutscene mode;
- vehicle mode.

## Pipelines

### Built-in

Primeiro alvo para labs:

- cameras explícitas;
- command buffers;
- image effects;
- depth texture;
- forward/deferred.

### URP

- camera stacking;
- renderer data/features;
- render scale;
- post-processing;
- XR compatibility differences por versão;
- history por eye.

### HDRP

- HDAdditionalCameraData;
- TAA/history;
- dynamic resolution/upscalers;
- volumetrics/exposure;
- custom passes;
- ray tracing;
- custo elevado.

### Custom

Começa em fallback L2/L1 até strategy específica.

## HUD/Canvas

Classificar:

- Screen Space Overlay;
- Screen Space Camera;
- World Space;
- TextMeshPro/subtitles;
- multiple event systems.

Estratégias:

- original por eye;
- mono texture em OpenXR quad;
- curved/head/world locked;
- world-space corrigido;
- per-canvas overrides;
- cutscene/subtitles safe zone.

Não destruir Canvas do jogo; aplicar alterações reversíveis por session e restaurar.

## Input

- semantic actions P13;
- legacy `Input`, new Input System ou game endpoint;
- não escrever direto em internals comuns sem adapter;
- context detection: gameplay/UI/cutscene;
- controller fallback;
- hand anchors e animation parameters por adapter;
- haptic requests de eventos do jogo.

## Mãos

Common fornece:

- skeleton retargeting interface;
- left/right anchors;
- pose blending;
- visibility/confidence;
- collision proxy interface;
- grab/point primitives.

O game adapter define quais objetos/regras aceitam interação. Sem catálogo, renderizar mão opcional não concede gameplay.

## Threading

- Unity API somente na main thread;
- GPU copy no render thread por plugin event;
- transport snapshots lock-free/bounded;
- callbacks de scene lifecycle;
- shutdown segura antes de unload;
- nenhuma chamada Unity em callback OpenXR thread.

## Labs

Criar gerador de projetos/cenas:

- multiple cameras;
- camera replacement;
- Built-in/URP/HDRP;
- Canvas modes;
- particles/post/TAA;
- legacy/new input;
- Mono/IL2CPP;
- D3D11/D3D12;
- interactable cube/door/item;
- cutscene/vehicle simulation;
- regression screenshots/poses.

## Critérios de aceite

- Common assembly/source lógico roda nos dois providers.
- Camera selection troca de cena sem leak.
- Stereo não duplica gameplay/audio.
- HUD legível e restaurado.
- Input semantic chega ao lab.
- Hand skeleton/interactable lab funciona quando tracking existe.
- Ausência de mão usa controle.
- Matrix por versão/pipeline é publicada.

## Primeiros passos do próximo chat

1. Extrair interfaces comuns sem quebrar bridge atual.
2. Criar lab Unity 2021.3 Built-in além do 6000.3.
3. Adicionar camera selection tests.
4. Migrar transporte para P12.
5. Só depois implementar providers Mono/IL2CPP.

## Referências oficiais

- [Unity native graphics plug-in API](https://docs.unity3d.com/Manual/low-level-native-plugin-rendering-extensions.html)
- [Camera.SetStereoProjectionMatrix](https://docs.unity3d.com/ScriptReference/Camera.SetStereoProjectionMatrix.html)
