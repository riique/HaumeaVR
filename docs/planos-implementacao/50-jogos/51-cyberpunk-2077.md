# P51 — Plano de compatibilidade para Cyberpunk 2077

**Status:** não iniciado; instalação não inspecionada  
**Prioridade:** alvo D3D12/REDengine  
**Dependências:** P20/P21, P13–P16, P31, P33, P44, P60–P66  
**Desbloqueia:** prova de engine/API diferente de Unity

## Objetivo

Construir uma progressão segura de L1 a uma experiência REDengine específica. Cyberpunk é um alvo muito maior que MiSide: D3D12, TAA/upscalers/ray tracing, corpo/armas, veículos, diálogos, HUD e cinematics exigem subprojetos próprios.

## Gate C0 — Identificação

Antes de código:

- loja/AppID;
- install path;
- game version/build;
- base/Phantom Liberty;
- hashes do exe/engine;
- D3D12 feature mode;
- GPU/driver;
- REDmod presence/version;
- mods/loaders existentes;
- language;
- save backup;
- modo single-player.

Não assumir a versão “atual” da web; usar a instalada.

## C1 — L1 polido

- capture backend ideal;
- profile de janela/aspect/HDR;
- curved/flat;
- gamepad original;
- menu HaumeaVR;
- 60 min;
- Quest/PICO;
- documentar HDR/ray tracing/upscaler interactions.

Esse é o primeiro release possível do perfil.

## C2 — L2 D3D12 Depth

Depende de P33/P31:

- identificar back buffer/main depth;
- reverse-Z/projection;
- HUD mask;
- weapon/first-person depth;
- transparency/particles;
- dynamic resolution;
- DLSS/FSR/XeSS paths;
- ray tracing modes;
- cutscene confidence;
- limited translation;
- fallback.

Publicar presets por qualidade/build.

## C3 — Adapter host e camera diagnostics

P44 decide uma rota permitida:

- REDmod oficial onde suficiente;
- adapter binário somente após review.

Primeiro adapter é read-only:

- camera transform/FOV/context;
- gameplay/dialog/cinematic/photo/vehicle;
- HUD state;
- player/weapon anchors;
- frame timing.

Sem stereo ou input.

## C4 — L4-R estéreo/cabeça

- base camera + physical head delta;
- asymmetric projections;
- near clip;
- body/head/weapon treatment;
- TAA history per eye;
- dynamic resolution;
- post processing;
- camera shake;
- photo mode isolation;
- D3D12 shared resources/fences;
- world scale.

Contexts:

- on foot;
- combat/aim;
- dialogue;
- scripted cinematic;
- vehicle driver/passenger;
- menu/map/inventory;
- braindance/special camera;
- loading/death.

Cada context define head mode e fallback.

## C5 — HUD e interface

- minimap/health/ammo/prompts;
- subtitles/dialog choices;
- inventory/map;
- scanner/cyberware overlays;
- crosshair/aim;
- vehicle HUD;
- menus.

Separar em quad/world/original conforme elemento. Pointer/controller precisa preservar fluxo original.

## C6 — L4-I controles/háptica

Primeiro:

- controles Quest/PICO como gamepad/semantic adapter;
- locomotion/turn;
- aim/fire/reload;
- interact;
- weapon select;
- scanner;
- vehicle controls;
- UI.

Depois:

- head/hand-directed aiming opcional;
- dominant hand;
- two-handed offsets;
- recoil haptics;
- comfort constraints.

## C7 — L4-H mãos por subsets

Subprojetos:

1. mãos visuais/IK;
2. weapon pose;
3. physical aiming;
4. reload gestures selecionados;
5. interact/use;
6. vehicle wheel/control only if stable;
7. melee;
8. UI wrist/pointer opcional.

Não tentar física em todos os objetos. Cada sistema tem fallback por botão e controller.

## C8 — Conforto

- seated/standing;
- snap turn;
- head/body decoupling;
- vignette;
- vehicle horizon/rotation;
- camera shake reduction;
- cinematic virtual screen;
- melee comfort;
- height/body calibration.

## Compatibilidade com mods

- detectar mods e produzir inventário;
- clean profile é referência;
- nunca apagar mods do usuário;
- testar stack por allowlist;
- conflito gera aviso e rota separada;
- install/remove HaumeaVR transacional;
- support bundle lista IDs/versions quando o usuário consentir.

## Test matrix

- base/expansion;
- clean/modded;
- quality presets;
- HDR;
- RT off/on/path tracing somente se viável;
- DLSS/FSR/XeSS;
- on-foot/combat/dialog/cutscene/vehicle/UI;
- save/load/fast travel;
- Quest USB/Air Link;
- PICO;
- controller and optional hands;
- 2 h soak.

## Critérios de promoção

- L1: capture/controls/long session.
- L2: depth confidence/HUD/fallback.
- L4-R: todos os contexts têm estratégia.
- L4-I: full controller mapping.
- L4-H: publicar interaction subset exato.
- L4-F: campanha/cenas definidas na matriz, não apenas free roam.

## Riscos

- **Custo de D3D12 stereo:** quality scaler e L2.
- **Updates:** fingerprint hard gate.
- **Mod ecosystem:** provider isolado e compatibility inventory.
- **Cinematics/vehicles:** context-specific fallback.
- **Mãos/armas:** fases separadas.
- **Licença:** REDmod EULA/P66.

## Primeiros passos do próximo chat

1. Inspecionar instalação legal; se ausente, trabalhar apenas labs.
2. Criar profile L1.
3. Concluir D3D12 depth lab.
4. Validar C2 antes de adapter.
5. Executar P44 ADR do host e só então C3.

## Referências oficiais

- [REDmod support](https://www.cyberpunk.net/pt-br/modding-support)
- [CDPR REDmod support](https://support.cdprojektred.com/en/cyberpunk/pc/sp-technical/issue/2236/i-want-to-report-an-issue-with-redmod)
