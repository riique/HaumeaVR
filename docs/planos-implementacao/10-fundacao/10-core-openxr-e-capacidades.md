# P10 — Core OpenXR e camada de capacidades

**Status:** próximo plano recomendado  
**Prioridade:** crítica  
**Dependências:** P00–P04  
**Desbloqueia:** P12, P13, P15, P20–P22, P33–P35

## Objetivo

Transformar o core atual, que abre uma sessão D3D11 básica, em uma plataforma OpenXR orientada a capacidades e testável em Meta Quest e PICO sem espalhar condicionais de fabricante pelo código.

## Estado inicial

`core/src/openxr_app.cpp` já:

- usa o loader Khronos;
- enumera extensões;
- requer `XR_KHR_D3D11_enable`;
- cria instance/system/session;
- seleciona adapter por LUID;
- enumera views e formatos;
- cria duas swapchains;
- executa frame loop;
- possui três ações simples;
- respeita `XR_RUNTIME_JSON`.

Faltam snapshot persistível, extensões opcionais, controles 6DoF, mãos, háptica, debug utils, backend abstraction e testes físicos.

## Escopo

- decompor `OpenXrApp` em subsistemas sem reescrever o renderer funcional;
- negociar API OpenXR;
- enumerar e registrar capacidades;
- formalizar lifecycle/session loss;
- preparar múltiplos backends gráficos;
- expor um diagnóstico estruturado para launcher e support bundle;
- manter seleção de runtime exclusivamente por processo.

## Fora do escopo

- implementar mãos completas; P13;
- implementar D3D12/Vulkan; P33/P34;
- criar runtime ou API layer OpenXR;
- usar SDK de fabricante como dependência obrigatória.

## Arquitetura alvo

```text
OpenXrPlatform
├── RuntimeDiscovery
├── InstanceContext
├── CapabilityProbe
├── SystemContext
├── SessionStateMachine
├── SpaceManager
├── ActionSystem
├── FrameScheduler
├── CompositionManager
└── IGraphicsBinding
    ├── D3D11GraphicsBinding
    ├── D3D12GraphicsBinding
    └── VulkanGraphicsBinding
```

Os objetos RAII devem obedecer à ordem de destruição OpenXR. Não expor handles crus para o launcher.

## CapabilitySnapshot v1

Criar um DTO serializável com:

- loader/SDK build usados pelo HaumeaVR;
- manifest selecionado, hash e origem;
- `runtimeName` e `runtimeVersion`;
- API solicitada e obtida;
- `systemId`, `systemName`, vendor ID;
- orientação e posição suportadas;
- limites de swapchain/layer;
- extensões disponíveis/habilitadas;
- view configuration types;
- por view: recomendado/máximo, samples e FOV observado;
- formatos de swapchain;
- blend modes;
- reference spaces;
- backend gráfico e adapter LUID;
- action/interaction profiles ativos por top-level path;
- flags opcionais: hand tracking, hand interaction, eye gaze, depth composition, refresh control, visibility mask e debug utils;
- motivo estruturado para cada recurso ausente.

Salvar snapshots de diagnóstico em `%LOCALAPPDATA%\HaumeaVR\diagnostics`. P05 migra a pasta legada `UniversalXRBridge`, sem identificador pessoal e com retenção definida em P63.

## Implementação por fases

### Fase 1 — Refactor protegido

1. Congelar comportamento atual com testes de lifecycle e helpers matemáticos.
2. Extrair wrappers RAII de instance, session, space, swapchain e action.
3. Criar interfaces sem mover o renderer.
4. Preservar `--self-test`, `--capture-self-test` e `--diagnose`.
5. Confirmar que o mirror e Virtual Screen não regressaram.

### Fase 2 — Negociação e diagnóstico

1. Tentar OpenXR 1.1 quando loader/runtime permitirem e recuar para 1.0.
2. Enumerar API layers e extensões antes da instance.
3. Habilitar `XR_EXT_debug_utils` somente quando presente.
4. Tornar extensões opcionais uma tabela de requisitos, não macros dispersas.
5. Produzir `CapabilitySnapshot` mesmo quando `xrGetSystem` falhar.
6. Mapear `XrResult` para código HaumeaVR, operação e recomendação.

### Fase 3 — Espaços e lifecycle

1. Preferir `LOCAL_FLOOR` quando disponível, depois `STAGE`/`LOCAL` conforme modo.
2. Manter transform de recenter do HaumeaVR separado do runtime.
3. Lidar com headset sleep, perda de foco, `LOSS_PENDING` e instance loss.
4. Parar submissão sem destruir configuração do launcher.
5. Permitir recriar sessão no mesmo processo apenas se o runtime suportar o fluxo e o estado estiver limpo; caso contrário, restart controlado.

### Fase 4 — ActionSystem base

1. Criar subaction paths esquerda/direita.
2. Ações: grip pose, aim pose, select, squeeze, trigger, thumbstick, primary, secondary, menu e haptic.
3. Sugerir perfis padronizados suportados pelo runtime; começar por generic controller, Khronos simple e Oculus Touch onde válidos.
4. Consultar `xrGetCurrentInteractionProfile` e tratar evento de mudança.
5. Entregar estados a P13 sem conhecer ações de jogo.

### Fase 5 — CompositionManager

1. Projection layer atual.
2. Quad layer para menu/HUD.
3. Depth info opcional quando `XR_KHR_composition_layer_depth` estiver habilitada.
4. Respeitar `maxLayerCount` e blend mode.
5. Visibilidade e alpha explicitamente testados.

## Arquivos previstos

- novos `core/include/uxrb/openxr/*.hpp`;
- novos `core/src/openxr/*.cpp`;
- `shared/protocol` para DTO/schema do snapshot;
- `apps/launcher/Services/OpenXrRuntimeService.cs` para leitura/exibição;
- novos testes `tests/openxr_*_tests.cpp`;
- manter facade compatível em `openxr_app.*` durante migração.

## Testes

### Automatizados

- parser/version negotiation;
- extension requirement table;
- serialization estável do snapshot;
- state transitions válidas/inválidas;
- ordem de destruição por fake dispatch table;
- runtime unavailable/form factor unavailable;
- mais/menos de duas views sem acesso fora do limite;
- layer budget;
- interaction profile changed;
- redaction de manifest/path.

### Em hardware

Para cada Quest/PICO:

1. `--diagnose --json <arquivo>`;
2. iniciar cena de teste;
3. confirmar pose rotacional/posicional;
4. confirmar grip/aim e botões;
5. sleep/wake;
6. retirar/recolocar headset;
7. desconectar/reconectar rota PCVR;
8. encerrar via launcher e via menu VR.

## Critérios de aceite

- `CapabilitySnapshot` é o único caminho usado para habilitar feature opcional.
- Core funciona com OpenXR 1.0 e 1.1 conforme runtime.
- D3D11 e Virtual Screen permanecem funcionais.
- Nenhuma escrita em `ActiveRuntime` global.
- Falta de mão/eye/depth é uma capability ausente, não crash.
- Um snapshot real de pelo menos um Quest e um PICO está em artefato de teste.
- Lifecycle passa por sleep/wake e perda de runtime sem deadlock.

## Riscos e mitigação

- **Refactor quebra timing:** slices pequenos e captura de métricas antes/depois.
- **Runtime reporta extensão mas falha:** probe funcional separado de presença nominal e workaround por runtime+versão.
- **API 1.1 incompatível:** fallback testado para 1.0.
- **Path de interação inválido:** gerar/sugerir somente perfis presentes no registry usado pelo build.

## Primeiros passos do próximo chat

1. Rodar todas as suítes e salvar baseline de `--diagnose`.
2. Ler `openxr_app.*` por inteiro.
3. Criar `CapabilitySnapshot` e teste de serialização antes do refactor.
4. Extrair `RuntimeDiscovery` e `CapabilityProbe` mantendo a facade.
5. Não iniciar P13 até que o snapshot chegue ao launcher.

## Referências oficiais

- [OpenXR 1.1 Specification](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html)
- [OpenXR Loader](https://registry.khronos.org/OpenXR/specs/1.1/loader.html)
- [OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK)
