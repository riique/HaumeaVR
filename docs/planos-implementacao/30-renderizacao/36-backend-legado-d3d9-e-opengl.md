# P36 — Backend legado Direct3D 9 e OpenGL

**Status:** não implementado; necessário para Outlast e títulos antigos  
**Prioridade:** alta para Outlast, média no produto geral  
**Dependências:** P12, P30, P31, P35, P61  
**Desbloqueia:** P46 e P52

## Objetivo

Adicionar rotas seguras para jogos anteriores ao D3D11, mantendo Virtual Screen como fallback e evitando readback de CPU quando interop GPU for possível.

## Escopo

- detecção de D3D9/D3D9Ex e OpenGL/WGL;
- L1 externo por WGC/Desktop Duplication;
- observer current-process somente por provider autorizado;
- transporte de cor e, quando possível, depth;
- interop para o core D3D11/OpenXR;
- labs com ground truth;
- fallback em APIs/formatos incompatíveis.

## Direct3D 9

### Detecção

- `d3d9.dll` e device creation;
- D3D9 versus D3D9Ex;
- presentation parameters;
- swapchain/back buffer;
- adapter LUID quando recuperável;
- formato, multisampling e fullscreen.

### Cor

Ordem:

1. D3D9Ex shared resource handle compatível com D3D11;
2. GPU copy para surface intermediária shareable;
3. compositor no mesmo processo e publicação controlada;
4. readback/upload somente como modo experimental de baixa performance;
5. L1 externo.

Validar adapter, format, pool, usage e sincronização. Nunca presumir que um handle D3D9 comum é importável.

### Depth

Depth D3D9 frequentemente não é sampleable/shareable. Estratégias:

- formatos INTZ/DF24/RAWZ apenas quando driver e jogo realmente expuserem;
- depth mirror criado por provider no momento de criação;
- engine adapter fornece projection/depth;
- reconstrução por cor não substitui depth real;
- ausência cai para L1.

Não aplicar formatos vendor sem probe.

### Geometry

Não é primeira entrega. Reexecução D3D9 tem os mesmos riscos de P32 e exige profile por shader/constant slot/draw. Priorizar adapter UE3.

## OpenGL/WGL

Rotas:

- L1 externo como padrão;
- adapter de engine;
- interop WGL/DX quando extensão/driver suportar;
- FBO color/depth em provider autorizado;
- PBO/readback somente em lab;
- nenhuma implicit/global layer.

OpenGL recebe prioridade após D3D9/Outlast.

## Core

O core continua OpenXR D3D11 inicialmente. P12 recebe resources convertidos/importáveis. P10 não ganha uma sessão OpenXR D3D9, pois OpenXR no Windows não define binding D3D9.

## Labs

- D3D9 e D3D9Ex;
- windowed/fullscreen;
- color formats;
- MSAA;
- normal/reverse depth;
- device lost/reset;
- resolution change;
- same/cross adapter;
- OpenGL FBO/WGL.

## Fases

1. Detector e report.
2. D3D9Ex color sharing lab.
3. device reset/lifetime.
4. transport P12.
5. Outlast L1/profile.
6. Outlast read-only graphics probe.
7. depth feasibility.
8. adapter UE3 P46.
9. OpenGL lab.

## Testes

- D3D9/D3D9Ex mismatch;
- invalid shared handle;
- wrong adapter;
- device lost/reset;
- fullscreen alt-tab;
- unsupported depth;
- CPU fallback disabled by default;
- frame timing;
- Quest/PICO;
- protected/anti-cheat block.

## Critérios de aceite

- L1 funciona mesmo sem backend legado in-process.
- D3D9Ex lab transfere cor sem CPU.
- Unsupported depth não ativa L2.
- Device reset recupera ou degrada.
- Nenhuma configuração global/driver é instalada.
- Outlast usa somente capacidades comprovadas na build inspecionada.

## Primeiros passos do próximo chat

1. Criar D3D9/D3D9Ex lab próprio.
2. Implementar detection report.
3. Provar color sharing em dois dispositivos/processos.
4. Medir reset/fullscreen.
5. Só depois executar probe read-only em Outlast.

## Referências oficiais

- [Direct3D 9 documentation](https://learn.microsoft.com/en-us/windows/win32/direct3d9/dx9-graphics)
- [Sharing resources between Direct3D 9 and Direct3D 11](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/surface-sharing-between-windows-graphics-apis)

