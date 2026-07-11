# P01 — Baseline atual e mapa de lacunas

**Status:** fotografia verificada em 11/07/2026  
**Prioridade:** reler antes de qualquer implementação  
**Dependências:** P00  
**Desbloqueia:** estimativas e validação de todos os planos

## Objetivo

Evitar que um novo chat reimplemente o que já existe ou trate uma fundação experimental como recurso pronto. Antes de agir, o executor deve confrontar esta fotografia com o código e atualizar apenas as divergências comprovadas.

## Repositório e toolchain

- Workspace: `C:\Users\Henri\Desktop\HaumeaVR`.
- Nome público e projeto CMake atual: `HaumeaVR 0.1.0`. Namespaces, protocolos e alguns nomes de arquivo internos ainda usam identificadores legados até a migração P05.
- Nativo: C++20, CMake 3.28+, MSVC, runtime C/C++ ligado estaticamente.
- Launcher: .NET 10, Avalonia 12.1, SQLite.
- OpenXR SDK/loader fixado em `release-1.1.58`.
- Microsoft Detours fixado em `v4.0.1`, sem helpers de criação/injeção remota compilados.
- Unity NativeRenderingPlugin fixado por commit.
- A árvore inteira aparece como não rastreada no Git nesta fotografia; não assumir que existe um commit-base recuperável.

## O que funciona

### Core OpenXR

Arquivos principais:

- `core/include/uxrb/openxr_app.hpp`
- `core/src/openxr_app.cpp`
- `core/src/d3d11_renderer.cpp`
- `core/src/mirror_window.cpp`

Capacidades:

- loader oficial Khronos;
- `XR_KHR_D3D11_enable`;
- seleção da GPU por LUID informado pelo runtime;
- sessão, frame loop, poses previstas, duas views, swapchains e projection layer;
- mirror window;
- estados de sessão e mensagens de erro úteis;
- ações simples de recenter, ajuste de tela e curvatura;
- seleção de runtime por processo.

Limites:

- solicita OpenXR 1.0 apesar do SDK 1.1;
- exige exatamente duas views;
- não mantém um snapshot formal de extensões/capacidades;
- não possui poses de controle, hand joints, háptica ou semantic actions;
- não há menu em quad layer.

### Launcher

Arquivos principais:

- `apps/launcher/ViewModels/MainWindowViewModel.cs`
- `apps/launcher/Services/GameDiscoveryService.cs`
- `apps/launcher/Services/EngineDetectionService.cs`
- `apps/launcher/Services/OpenXrRuntimeService.cs`
- `apps/launcher/Services/ProfileRepository.cs`

Capacidades:

- descoberta Steam/Epic/manual;
- detecção de Unity, backend de scripting, API gráfica, pipeline e anti-cheat;
- perfis SQLite com schema JSON v1;
- runtime OpenXR registrado e override local;
- lançamento de Virtual Screen;
- lançamento de Unity Stereo apenas em player Mono controlado e previamente equipado;
- IPC autenticado por token aleatório.

Limites:

- UI é funcional, não é ainda o Profile Studio proposto;
- não há catálogo de capability snapshots;
- Geometry/Depth permanecem marcados como indisponíveis na interface;
- não há gestão transacional de adapters/mods;
- não há instalador nem updater assinado.

### Virtual Screen

- Usa DXGI Desktop Duplication e recorte da janela.
- Renderiza tela plana/curva em D3D11.
- É o único modo amplo e pronto para usuário final.
- Não fornece 3D geométrico, câmera dentro do jogo ou mãos.

### Unity controlada

Arquivos:

- `engines/unity/bridge/Runtime/*`
- `engines/unity/native/*`
- `shared/unity_transport/*`
- `tests/unity-lab/*`

Capacidades:

- bridge gerenciado;
- câmeras por olho com projeção assimétrica;
- plug-in nativo na render thread;
- texturas D3D11 compartilhadas e keyed mutex;
- transporte de pose/FOV/dimensões do core para Unity e estado/texturas no sentido inverso;
- processo produtor, nomes e formatos validados.

Limites:

- caminho disponível apenas em player Mono controlado que já contém os módulos;
- o lab está fixado em Unity 6000.3, mas o editor físico não estava instalado na última validação;
- não existe bootstrap comercial genérico;
- transporte não carrega input, mãos, depth, motion vectors ou haptic requests;
- não existe integração IL2CPP de produção.

### D3D11 e Depth

- Há observer D3D11 somente no processo atual, ativado explicitamente.
- Existe veto forte ao detectar anti-cheat conhecido.
- Há fundação de reconstrução compute para Depth 3D.
- Não há injector remoto.
- Color/depth do jogo ainda não percorrem um transporte completo até o core como capacidade de usuário.
- Geometry Stereo ainda não é recurso funcional.

### IPC e segurança

- Named pipe local, framing binário + JSON, limite de 1 MiB.
- Handshake com token aleatório de 256 bits e comparação constante.
- Uma conexão local por processo.
- Transporte Unity aleatório, versionado e preso ao PID esperado.
- Nenhum driver, serviço, runtime global alterado ou código comunitário carregado.

### Build e distribuição

- Debug e Release MSVC validados.
- 7/7 testes nativos e 11/11 testes do launcher na última evidência.
- `scripts/Publish.ps1` produz launcher self-contained.
- `scripts/Package.ps1` produz ZIP portátil sem source.
- Bundle 0.1.0 já foi extraído e testado em pasta limpa.
- Ainda não há instalador de um clique assinado.

## Estado de runtimes/headset da última validação

- Registro global não tinha `ActiveRuntime` 32/64-bit e não foi modificado.
- Meta Horizon estava registrado com manifest ausente.
- Virtual Desktop tinha manifest existente e carregava por `XR_RUNTIME_JSON`.
- Nenhum HMD estava exposto; `xrGetSystem` retornou `XR_ERROR_FORM_FACTOR_UNAVAILABLE`.
- Não houve validação física photon-to-photon em Quest ou PICO.

Essa informação é volátil. P20/P21 devem revalidá-la na máquina atual.

## MiSide observado localmente

Instalação:

- Steam AppID `2527500`;
- executável `C:\Program Files (x86)\Steam\steamapps\common\MiSide\MiSideFull.exe`;
- Unity `2021.3.35f1`;
- `GameAssembly.dll` presente;
- `MonoBleedingEdge` ausente;
- `global-metadata.dat` presente, 5.173.460 bytes;
- build IL2CPP, sem módulos HaumeaVR (identificadores legados UXRB também ausentes).

Hashes da fotografia:

- `MiSideFull.exe`: `562723C950C2BE95ECBDC516530AEE56B97837E2AC6610B8B5BCE394E23022C0`;
- `UnityPlayer.dll`: `1C5B91FCCB8458999AEDC4D83B5AB08B6A4C7251F2038C0A8A43D01A5AE1C6DD`.

Hoje ele recebe apenas L1. L4 requer P42 e P50.

## Lacunas ordenadas

1. Capability layer OpenXR e testes físicos Quest/PICO.
2. Transporte v2 com controles, mãos, háptica e recursos gráficos opcionais.
3. SDK/host de adaptadores e instalação transacional.
4. Perfis schema v2 e session fallback formal.
5. Menu VR e GUI/Profile Studio.
6. Unity IL2CPP + adaptador MiSide.
7. D3D12 + REDengine/Cyberpunk.
8. Depth 3D e Geometry Stereo reais.
9. Instalador, assinatura, updater, matriz de qualidade e suporte.

## Comandos de revalidação

```powershell
git status --short
.\scripts\Configure.ps1 -Fresh
.\scripts\Build.ps1 -Preset windows-msvc-debug
.\scripts\Test.ps1 -Preset windows-msvc-debug
.\scripts\TestLauncher.ps1
.\build\windows-msvc-debug\bin\uxrb_core.exe --self-test
.\build\windows-msvc-debug\bin\uxrb_core.exe --diagnose
```

## Critérios de aceite

- Mudanças são baseadas em código/teste atual, não em expectativa.
- Cada recurso é classificado como fundação, experimental, beta ou pronto.
- Evidência física informa headset, conexão, runtime e versões.
- Hashes de jogos são atualizados somente depois de nova inspeção.
- Nenhuma lacuna é removida apenas porque uma classe ou stub foi criado.

## Primeiros passos do próximo chat

1. Executar os comandos de revalidação acima.
2. Comparar resultados, versões e arquivos com esta fotografia.
3. Atualizar somente divergências comprovadas.
4. Preservar a distinção entre fundação, implementação e validação física.
