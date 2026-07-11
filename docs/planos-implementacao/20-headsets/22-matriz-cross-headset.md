# P22 — Matriz cross-headset e política de compatibilidade

**Status:** planejado  
**Prioridade:** crítica para release  
**Dependências:** P10, P13, P20, P21, P62  
**Desbloqueia:** afirmações públicas de suporte e P67

## Objetivo

Impedir que “funcionou em um Quest” ou “abriu no SteamVR” seja convertido em suporte genérico. A unidade de compatibilidade é uma combinação explícita.

## Chave da matriz

```text
HaumeaVR version
× OS/build
× GPU vendor/model/driver
× headset model/firmware
× connection route
× connection software/version
× OpenXR runtime/manifest/version
× refresh rate/resolution
× input source/profile
× game/build
× HaumeaVR mode/adapter/profile
```

Uma linha pode herdar apenas testes independentes de hardware, nunca capabilities.

## Camadas da matriz

### Runtime/headset

- instance/system/session;
- views/formats;
- reference spaces;
- controller profiles;
- haptic;
- hand tracking;
- lifecycle;
- menu layer.

### Graphics

- D3D11/D3D12/Vulkan;
- resource sharing;
- color/depth;
- mirror;
- frame pacing.

### Engine

- Unity version/backend/pipeline;
- Unreal family;
- REDengine build;
- generic API.

### Game

- menus/gameplay/cutscenes;
- render/camera/HUD;
- input/mãos/háptica;
- save/load;
- long session.

## Estados

- **Not tested**
- **Probe only**
- **Experimental**
- **Beta**
- **Supported**
- **Blocked**
- **Regressed**
- **Retired**

Cada estado possui evidence URI local, data, tester, build e issue IDs. “Supported” expira quando runtime/game major build muda até smoke retest.

## Test suite canônica

### H0 — Sem headset

- build/test;
- protocol;
- self-test;
- profile validation;
- packaging.

### H1 — Diagnose

- loader/runtime/system;
- capability snapshot;
- no global registry writes.

### H2 — Visual baseline

- scene;
- both eyes;
- 6DoF;
- mirror;
- recenter;
- menu.

### H3 — Input

- profiles;
- buttons/axes;
- grip/aim;
- haptic;
- focus.

### H4 — Lifecycle

- sleep/wake;
- disconnect;
- runtime restart;
- game exit;
- core stop.

### H5 — Performance

- 30/60 minute;
- refresh rates;
- frame misses;
- CPU/GPU/VRAM;
- thermal/network observations.

### H6 — Feature optional

- hands;
- eye/depth/passthrough;
- adapter.

## Formato de registro

Usar JSON para automação e Markdown gerado para leitura. Campos:

- IDs e versões;
- capability snapshot hash;
- test IDs/results/duration;
- metrics summary;
- artifacts;
- known issues/workarounds;
- reviewer;
- expiry trigger.

Não guardar serial do headset, usuário Steam/Meta/PICO, nome da máquina ou paths pessoais no bundle público.

## Política de claims

- “Meta Quest suportado” exige ao menos dois modelos/rotas definidos em P20, com limitações listadas.
- “PICO suportado” exige a rota PICO Connect/SteamVR e hardware físico.
- “Mãos suportadas” é por runtime+rota+modelo e ainda não significa interação de jogo.
- “Jogo suportado” inclui fingerprint/build e níveis por dimensão.
- Um resultado Virtual Desktop não valida Meta Link.
- Um resultado SteamVR/PICO não valida SteamVR/Index nem vice-versa.

## Automação

- `scripts/TestHardware.ps1` coordena, sem mudar runtime global;
- core emite JSON;
- launcher exporta session evidence;
- script valida schema e redaction;
- gerador cria tabela;
- CI valida apenas H0 e schemas;
- hardware runs são assinados/anexados manualmente.

## Critérios de aceite

- Matriz contém ao menos um Quest e um PICO reais.
- Toda capability pública aponta a evidence row.
- Alteração de runtime/game invalida somente linhas afetadas.
- Redaction test impede dados pessoais.
- Release P67 falha se claim não tiver linha Supported.

## Primeiros passos do próximo chat

1. Definir schemas de evidence.
2. Converter validação existente em H0/H1 fixture.
3. Criar comando `--diagnose --json`.
4. Implementar runner sem qualquer alteração global.
5. Preencher primeiras linhas físicas em P20/P21.
