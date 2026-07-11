# P43 — Adaptadores Unreal Engine 4 e 5

**Status:** futuro  
**Prioridade:** média-alta  
**Dependências:** P12–P14, P33, P35, P61  
**Desbloqueia:** L4 em projetos/jogos Unreal selecionados

## Objetivo

Criar uma família de adapters Unreal que trate câmera, render, UMG/Slate, input e mãos, começando em projetos próprios. Jogos comerciais sem mod/plugin permitido continuam em L1/L2/L3.

## Rotas

### Projeto com source

Plugin HaumeaVR compilado no projeto. É a referência e pode integrar profundamente.

### Jogo com sistema de mod/plugin permitido

Game adapter específico, assinatura/fingerprint e rollback.

### Jogo empacotado sem rota autorizada

Somente backends gráficos externos/permitidos. Não forçar plugin binário.

## Decisão de sessão XR

O plugin HaumeaVR não habilita o plugin OpenXR do Unreal para abrir uma segunda sessão. Ele implementa um dispositivo/bridge HaumeaVR:

- core externo é owner OpenXR;
- plugin recebe poses/views pelo P12;
- Unreal renderiza targets stereo;
- plugin publica textures/depth;
- input semântico chega pelo adapter.

Em projeto que já possui VR/OpenXR nativo, o launcher deve preferir o modo VR do próprio jogo ou executar uma rota separada; não misturar.

## Componentes

```text
HaumeaVR.uplugin
├── HaumeaVRBridge (runtime)
├── HaumeaVRRender
├── HaumeaVRInput
├── HaumeaVRInspection
└── HaumeaVREditor/Lab (development)
```

Separar UE4/UE5 por módulos/branches quando APIs divergem.

## Detecção

- executável e arquivos `.pak/.utoc/.ucas`;
- engine version quando exposta;
- UE4 versus UE5;
- D3D11/D3D12/Vulkan;
- DX12 default/launch args;
- anti-cheat;
- plugin/mod policy;
- shipping/development build.

Não tentar desempacotar asset criptografado para perfil.

## Câmera/render

- localizar local player/player controller;
- `APlayerCameraManager` e `UCameraComponent`;
- preservar câmera/base POV;
- stereo projections OpenXR;
- temporal history por eye;
- scene capture/post;
- weapon/first-person meshes;
- D3D11 RHI e D3D12 RHI;
- render thread/RDG;
- shared resource/fence;
- scene changes/world travel;
- cutscene/Sequencer contexts.

Labs devem comparar HaumeaVR à referência OpenXR nativa do Unreal.

## UI

- UMG/Slate;
- screen-space HUD em layer mono;
- world widgets;
- subtitles;
- DPI/safe zones;
- menus recebem pointer/controller;
- restauração completa.

## Input

- UE4 Action/Axis mappings;
- UE5 Enhanced Input contexts/actions;
- semantic actions P13;
- game-specific endpoints allowlisted;
- controller fallback;
- hand skeleton retargeting;
- haptics.

Não criar keys/mappings persistentes no jogo sem transação.

## Inspection

- Worlds/levels;
- Actors/components;
- camera/UMG roles;
- Enhanced Input actions;
- interactable interfaces/tags;
- IDs opacos;
- somente API do plugin.

## Labs

- UE4.27 D3D11;
- UE5 LTS/current chosen D3D12;
- OpenXR reference build;
- first/third person;
- UMG;
- Enhanced Input;
- Sequencer;
- Nanite/Lumen/TAA/upscaler no UE5;
- interactable hands.

## Fases

1. UE5 source plugin + transport.
2. Stereo render D3D12.
3. camera/head/UMG.
4. input/hands lab.
5. UE4.27 port D3D11.
6. inspection/profile.
7. provider de game permitido.
8. primeiro jogo offline.

## Testes

- editor/development/shipping;
- world travel;
- PIE não é evidência de shipping;
- multiple local players;
- cutscene;
- D3D11/D3D12;
- RDG validation;
- TAA/upscaling;
- camera shakes;
- UI focus;
- plugin absent/wrong build;
- install/remove.

## Critérios de aceite

- Labs UE4 e UE5 têm referência stereo.
- Core é único owner OpenXR.
- Shipping source-owned roda sem editor.
- UI/input/context funcionam.
- D3D12 fence/resource sem errors.
- Jogo comercial só entra após provider permitido.

## Primeiros passos do próximo chat

1. Criar UE5 source lab e plugin vazio.
2. Integrar P12 sem OpenXR no Unreal.
3. Publicar stereo targets para core.
4. Adicionar câmera/UMG/input.
5. Portar para UE4 depois do UE5 estável.

## Referências oficiais

- [Unreal OpenXR](https://dev.epicgames.com/documentation/en-us/unreal-engine/developing-for-head-mounted-experiences-with-openxr-in-unreal-engine)
- [OpenXR Input in Unreal](https://dev.epicgames.com/documentation/unreal-engine/openxr-input-in-unreal-engine)
- [Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)
- [Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
