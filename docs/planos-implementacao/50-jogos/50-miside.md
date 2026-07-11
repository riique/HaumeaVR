# P50 — Plano de experiência nativa para MiSide

**Status:** L1 disponível; L4 bloqueado por P42  
**Prioridade:** primeiro jogo L4  
**Dependências:** P20/P21 hardware, P12–P16, P40, P42, P60–P63  
**Desbloqueia:** prova real Unity IL2CPP

## Objetivo

Levar MiSide de Virtual Screen a uma experiência VR integrada por etapas verificáveis: estéreo/cabeça, HUD/cutscenes, controles, mãos visuais e um catálogo explícito de interações físicas. Não instalar um bridge experimental antes dos labs e rollback.

## Build local observado em 11/07/2026

- Steam AppID: `2527500`.
- Executável: `C:\Program Files (x86)\Steam\steamapps\common\MiSide\MiSideFull.exe`.
- Unity: `2021.3.35f1`.
- Backend: IL2CPP.
- `GameAssembly.dll`: presente.
- `MonoBleedingEdge`: ausente.
- `global-metadata.dat`: presente, 5.173.460 bytes.
- bridge HaumeaVR: ausente.
- SHA-256 do exe: `562723C950C2BE95ECBDC516530AEE56B97837E2AC6610B8B5BCE394E23022C0`.
- SHA-256 do UnityPlayer: `1C5B91FCCB8458999AEDC4D83B5AB08B6A4C7251F2038C0A8A43D01A5AE1C6DD`.

Esses hashes são uma fotografia, não um whitelist eterno. Recalcular antes de qualquer deployment.

## Nível atual

- L1 Virtual Screen: disponível.
- L2 Depth: não disponível.
- L3 Geometry: não disponível.
- L4-R/I/H: não disponível.

## Princípio de implementação

MiSide recebe um Official Game Adapter sobre o Official Unity IL2CPP Adapter. Regras específicas do jogo não entram em `Haumea.Unity.Common`.

```text
Core OpenXR
↕ P12 poses/input/textures
Unity IL2CPP Provider
↕ P40 common camera/render/HUD
MiSide Adapter
  camera roles
  contexts/cutscenes
  semantic actions
  hand anchors
  interaction catalog
```

## Gate M0 — Identidade, segurança e reversibilidade

1. Fechar jogo/Steam update.
2. Ler Steam build ID, game version e hashes.
3. Detectar mods/preexisting loaders.
4. Confirmar single-player/offline.
5. Criar backup transacional dos arquivos que um provider tocar.
6. Verificar restauração byte por byte.
7. Criar profile v2 ainda em L1.
8. Bloquear L4 em fingerprint divergente.

Saída: report, manifest de backup e botão Restaurar.

## Gate M1 — Lab equivalente

Construir Unity `2021.3.35f1`:

- IL2CPP x64;
- D3D11;
- pipeline detectada no jogo;
- stripping semelhante;
- multiple scenes/cameras/canvases;
- first-person camera;
- cutscene camera;
- input/UI;
- interactable door/item/button;
- hand anchors.

P42 precisa produzir stereo, input e inspection nesse lab antes do jogo.

## Gate M2 — Probe read-only

Em MiSide, com provider autorizado:

- attach e health somente;
- Unity version/domain;
- cenas ativas;
- lista paginada de cameras/canvases;
- render pipeline;
- input systems;
- nenhuma mudança de camera/render;
- stop/restauração.

Comparar IDs/roles entre execuções. Não salvar pointer.

## Gate M3 — L4-R câmera e estéreo

1. Identificar câmera de gameplay e base transform.
2. Detectar trocas de câmera/scene.
3. Aplicar stereo cameras script-free.
4. Projeções assimétricas.
5. Head rotation.
6. Translação limitada, depois 6DoF.
7. Near clip/world scale/body clipping.
8. Render textures D3D11 e P12.
9. Mirror/performance.
10. Fallback imediato L1.

Primeiro teste sem mãos e com gamepad/teclado original.

## Gate M4 — HUD, UI e cutscenes

Catalogar:

- main menu;
- pause/settings;
- gameplay HUD/prompts;
- subtitles/dialog;
- loading;
- minigames/overlays;
- cutscenes/camera changes.

Estratégias:

- HUD mono em quad layer quando seguro;
- world-space only para elementos selecionados;
- pointer por controle;
- cutscene rotation-only, virtual screen ou profile-specific;
- transição com fade para evitar camera snap;
- preservar input original em menu.

## Gate M5 — L4-I controles

Mapear ações semânticas:

- move/turn/look;
- interact/use;
- primary/secondary;
- pause/back;
- UI point/select;
- ações contextuais identificadas no catalog.

Começar com controles Quest/PICO. Validar que o mesmo profile semântico funciona nos dois runtimes. Não mapear gesto físico diretamente a método arbitrário.

## Gate M6 — Mãos visuais

- tracking source real quando disponível;
- controller-driven hand poses como fallback;
- retarget para rig/proxies próprios sem distribuir assets do jogo;
- left/right anchors;
- ocultar mãos em tracking loss/cutscene;
- evitar atravessar câmera/corpo;
- animation blending;
- haptic events.

Mãos visuais não alteram gameplay ainda.

## Gate M7 — Interaction Catalog

Descobrir e implementar por categoria:

- UI pointer;
- examine/inspect;
- pick up/use selected items;
- doors/drawers/buttons;
- context prompts;
- minigames essenciais;
- scripted sequences.

Para cada binding:

- context;
- source gesture/action;
- target endpoint allowlisted;
- preconditions;
- animation/hand pose;
- collision/attach rule;
- success/failure feedback;
- haptic;
- fallback controller;
- scenes/builds testados.

Objetos não catalogados continuam por botão Interact. Isso é preferível a física errada.

## Gate M8 — Conforto e completude

- seated/standing;
- height calibration;
- snap/smooth turn;
- camera decoupling;
- cutscene profile;
- vignette;
- room-scale bounds;
- subtitles;
- 60 min/2 h;
- saves e progressão em checkpoints definidos.

## Arquivos previstos

- `engines/unity/il2cpp/*`;
- `adapters/games/miside/*`;
- `profiles/games/miside/<fingerprint>.json`;
- `tests/game-labs/miside/*` sem arquivos do jogo;
- manifests em `distribution/adapters/miside/*`;
- evidence local fora do pacote público.

## Critérios de aceite

Por Quest e PICO:

- launch/restore;
- menu;
- gameplay walk/look;
- scene transitions;
- HUD/subtitles;
- cutscenes;
- input;
- visual hands;
- cada interaction category;
- save/load;
- fallback;
- stop game/core independently;
- performance.

L4-F só quando o conjunto de capítulos/cenas definido para a build passa. Até lá, anunciar L4-R/I/H separadamente.

## Riscos

- **Update muda metadata:** fingerprint invalida.
- **Provider IL2CPP instável:** lab gate e L1.
- **Câmeras scripted:** context/cutscene strategies.
- **Muitas interações únicas:** catálogo incremental com botão fallback.
- **Assets/animações:** criar assets HaumeaVR próprios ou usar runtime references; não redistribuir conteúdo.

## Primeiros passos do próximo chat

1. Recalcular build/hashes e salvar P60 profile L1.
2. Não tocar no jogo se P42 lab 2021.3.35 não estiver concluído.
3. Construir lab equivalente.
4. Fazer probe read-only.
5. Implementar M3 sem input/mãos e testar fallback.
