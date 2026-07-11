# P33 — Backend Direct3D 12

**Status:** não implementado  
**Prioridade:** crítica para Cyberpunk 2077  
**Dependências:** P10 abstraction, P12, P35, P61  
**Desbloqueia:** P44, P51 e L2/L3 em D3D12

## Objetivo

Adicionar D3D12 como backend OpenXR e como caminho de recursos compartilhados/observação, preservando sincronização e estados explícitos. Não encapsular uma cena D3D12 inteira em D3D11On12.

## Escopo

- `XR_KHR_D3D12_enable`;
- device/queue no adapter exigido pelo runtime;
- swapchain images D3D12;
- compositor/mesh/menu equivalentes ao D3D11;
- resource descriptors P12;
- shared resources/heaps/fences;
- lab D3D12;
- observer current-process opt-in;
- color/depth foundation;
- preparação REDengine/Cyberpunk.

## Fora do escopo

- geometry genérico completo na primeira fase;
- injection remota;
- ray tracing modification;
- interceptar protected resources;
- usar D3D11On12 como solução universal.

## Core D3D12

1. Consultar `xrGetD3D12GraphicsRequirementsKHR`.
2. Selecionar adapter pelo LUID exigido.
3. Criar device e direct queue compatíveis.
4. Criar session com `XrGraphicsBindingD3D12KHR`.
5. Enumerar `XrSwapchainImageD3D12KHR`.
6. Implementar barriers e command allocators por frame.
7. Renderizar scene/menu/mirror.
8. fences por frame e shutdown seguro.

`IGraphicsBinding`/`ICompositionRenderer` evitam duplicar lifecycle OpenXR.

## Interop entre processos

Contrato preferido:

- committed resource ou shared heap explicitamente shareable;
- `CreateSharedHandle/OpenSharedHandle`;
- shared `ID3D12Fence` + valores monotônicos;
- resource state inicial/final no descritor;
- owner e lifetime generation;
- sem keyed mutex D3D11.

Se o core D3D11 consumir um produtor D3D12 durante migração, criar um bridge isolado e medido. O estado final é core nativo D3D12 para sessões D3D12.

## D3D11On12

Uso permitido:

- Direct2D/UI simples sobre resource D3D12;
- port incremental de componentes auxiliares.

Uso proibido:

- reexecutar toda a cena;
- esconder state transitions;
- caminho de desempenho sem medição.

Microsoft documenta overhead e limitações; todo wrapped resource exige Acquire/Release/Flush corretos.

## Observer D3D12

Lab primeiro:

- device/command queue/swapchain;
- ExecuteCommandLists;
- Present;
- resource creation;
- RTV/DSV descriptors;
- resource barriers;
- render pass;
- command list lifecycle;
- queue/fence.

Em D3D12, handles de descriptors não carregam semântica suficiente fora do processo. Manter registry interno dos descriptors observados. Nunca ler descriptor heap além do conhecido.

## Depth/Color

- identificar back buffer e main depth por command list/use;
- garantir estado COPY_SOURCE no ponto seguro e restaurar;
- copiar para recurso shareable;
- resolver MSAA/format quando permitido;
- fence antes de publicar;
- não acessar recurso protegido;
- enviar via P12.

## Fases

1. D3D12 OpenXR test scene.
2. compositor equivalente D3D11.
3. transport shared texture/fence no lab.
4. Virtual Screen via D3D12 ou interop medido.
5. observer metadata.
6. color/depth lab.
7. Depth P31.
8. REDengine adapter host.
9. geometry research.

## Testes

- wrong LUID;
- queue type;
- barriers/state validation;
- fence timeout/stale value;
- resource generation;
- device removed;
- resize;
- multi-queue;
- HDR/formats;
- shared handle ACL/PID;
- PIX/debug layer/DRED em dev;
- Quest/PICO runtime;
- AMD/NVIDIA/Intel quando possível.

## Critérios de aceite

- cena OpenXR D3D12 igual à D3D11 em função.
- Nenhum warning de debug layer no lab.
- Shared texture/fence sobrevive a resize/restart.
- Timeout não trava frame loop.
- Device removal produz diagnóstico.
- Depth lab passa antes de Cyberpunk.
- Core escolhe backend por SessionPlan sem código vendor.

## Primeiros passos do próximo chat

1. Extrair `IGraphicsBinding` do D3D11 atual com testes.
2. Criar scene D3D12 sem observer.
3. Implementar shared texture/fence em dois processos de teste.
4. Só então iniciar observer.
5. Não usar Cyberpunk como primeiro teste.

## Referências oficiais

- [OpenXR XR_KHR_D3D12_enable](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html)
- [D3D12 interop](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-with-direct3d-11--direct-2d-and-gdi)
- [Shared heaps](https://learn.microsoft.com/en-us/windows/win32/direct3d12/shared-heaps)
- [D3D11On12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-11-on-12)

