# P62 — Testes, labs, hardware e matriz de compatibilidade

**Status:** suíte inicial funcional; expansão planejada  
**Prioridade:** crítica  
**Dependências:** P01–P04  
**Desbloqueia:** promoção de todos os planos

## Objetivo

Criar um sistema de validação que detecte regressões sem hardware, mas exija hardware/jogo real antes de claims. Labs substituem “testar primeiro em jogo comercial”.

## Baseline

Na última evidência:

- 7/7 testes nativos;
- 11/11 testes launcher;
- Debug/Release;
- IPC real;
- D3D11 self/capture;
- Unity transport;
- Depth compute;
- pacote limpo.

Ainda faltam HMD físico e Unity Editor/labs construídos.

## Pirâmide

### Unit

- math/projection;
- schemas/merge;
- protocol/concurrency;
- detector;
- state machines;
- gestures;
- depth linearization;
- signatures/transactions.

### Component

- fake OpenXR dispatch/lifecycle;
- two-process transports;
- D3D11/D3D12/Vulkan resources;
- adapter mock;
- capture sources;
- SQLite/migrations;
- updater verifier.

### Lab engine

- Unity versions/backends/pipelines;
- Unreal UE4/UE5;
- D3D11/D3D12/Vulkan render labs;
- REDengine concepts via generic D3D12 lab, sem código do jogo.

### Hardware

- P22 H1–H6;
- Quest/PICO routes;
- GPUs;
- long sessions.

### Game

- clean legal install;
- fingerprint;
- context catalog;
- saves/checkpoints;
- mods;
- install/rollback.

### Distribution

- clean VM/Windows Sandbox quando compatível;
- no SDK/compiler/.NET global;
- install/repair/update/uninstall;
- source absence;
- signatures/hashes.

## Labs requeridos

### OpenXR Capability Lab

- scene/cube/grid;
- controller visualization;
- hand skeleton;
- haptic buttons;
- reference spaces;
- quad layers;
- sleep/loss.

### Graphics Labs

- D3D11 color/depth/matrices;
- D3D12 queues/resources/depth;
- Vulkan images/layouts;
- ground-truth stereo;
- reverse-Z/MSAA/HDR/TAA.

### Unity Compatibility Lab

- P40 matrix;
- Mono/IL2CPP;
- cameras/canvas/input/hands/interactable.

### Unreal Lab

- P43 matrix.

### Failure Lab

- slow/crash adapter;
- stale texture;
- invalid protocols;
- runtime loss;
- transaction interruption.

## Test data

- gerar geometria/textures no teste;
- pequenos golden images próprios;
- traces de mão sintéticos;
- manifests/profile fixtures;
- nenhum asset/binário/screenshot de jogo redistribuído sem permissão;
- hashes/evidence privados.

## CI

Em cada mudança:

- configure/build Debug;
- native/launcher tests;
- schema lint;
- formatting/static analysis definidos;
- package vulnerability scan;
- source/license scan;
- protocol ABI checks.

Em main/release:

- Release clean;
- publish/package;
- SBOM/checksums;
- install smoke em VM;
- deterministic/repro comparison onde possível;
- hardware runs manuais anexados.

CI não marca hardware pass por simulation.

## Game test case

Cada caso:

- ID/context;
- precondition/save;
- steps;
- expected render/head/HUD/input/hands/audio;
- performance threshold;
- fallback;
- cleanup;
- evidence.

## Regressão visual

- lab screenshots per eye;
- depth/heatmaps;
- perceptual diff com tolerância;
- camera pose golden;
- HUD pixel/legibility tests;
- manual HMD confirmation obrigatória para conforto.

## Flakiness

- random seeds fixos;
- timeouts explícitos;
- retries só para diagnóstico, não para esconder falha;
- quarantine com owner/expiry;
- hardware version captured;
- no sleeps longos arbitrários.

## Critérios de aceite

- Todo plano de código adiciona teste.
- Labs cobrem risco antes do jogo.
- Hardware evidence segue P22.
- Release package passa máquina limpa.
- Rollback é testado.
- Claim público é derivado da matriz.
- Testes podem ser executados por outro chat com documentação.

## Primeiros passos do próximo chat

1. Criar taxonomy/test IDs.
2. Converter evidências existentes.
3. Implementar OpenXR Capability Lab.
4. Construir Unity labs.
5. Integrar JSON evidence e P22.

