# Como implementar todo o HaumeaVR

Este guia transforma os planos separados em um programa de execução completo. A implementação não deve ocorrer como uma única reescrita: cada fase entrega um slice utilizável, testado e reversível.

## Regra de trabalho

Cada novo chat:

1. lê o índice, P01, P02, P03 e o plano primário;
2. verifica o código e dependências atuais;
3. implementa um slice vertical;
4. adiciona testes;
5. executa build/test;
6. atualiza status/evidências;
7. indica o próximo plano desbloqueado.

Um chat não deve tentar concluir simultaneamente core, engine, jogo e instalador. As interfaces entre eles são os contratos P02/P12/P14/P60.

## Preparação zero

Antes de novas features:

- estabelecer um baseline versionado do repositório;
- rodar Debug/Release e todas as suítes;
- guardar o ZIP 0.1.0 legado como rollback;
- concluir P05 R1/R2;
- confirmar que o launcher publicado chama-se `HaumeaVR.Launcher.exe`;
- validar migração do banco legado;
- registrar limitações atuais.

Comandos-base:

```powershell
.\scripts\Configure.ps1 -Fresh
.\scripts\Build.ps1 -Preset windows-msvc-debug
.\scripts\Test.ps1 -Preset windows-msvc-debug
.\scripts\TestLauncher.ps1
.\scripts\Publish.ps1
.\scripts\Package.ps1
```

## Fase 1 — HaumeaVR cross-headset

Executar:

- P10 Core OpenXR/capabilities;
- P20 Meta Quest;
- P21 PICO;
- P22 matriz;
- primeira parte de P62.

Entrega:

- diagnóstico estruturado;
- cena/Virtual Screen em Quest e PICO reais;
- controladores, háptica e lifecycle;
- nenhum runtime global alterado.

Portão: pelo menos um Quest e um PICO precisam de evidência física. Simulation não fecha a fase.

## Fase 2 — Contratos para experiência nativa

Executar:

- P12 protocolo/transporte v2;
- P13 controles/mãos/háptica;
- P14 adapter SDK;
- P60 profile schema v2;
- P11 fallback;
- P61 segurança;
- P63 logs.

Entrega:

- core publica pose/olhos/input/mãos;
- lab publica color/depth/resources;
- adapter mock pode falhar sem derrubar o jogo;
- profile declara capabilities honestamente;
- install/remove é transacional.

Portão: Unity lab controlado funciona por v2; protocol/security adversarial tests passam.

## Fase 3 — Interface gráfica completa

Executar:

- P15 menu VR;
- P16 launcher/Profile Studio;
- P30 Virtual Screen.

Entrega:

- biblioteca, dispositivos, detalhes do jogo e sessão;
- menu no headset;
- editor de profile;
- live inspector apenas por adapter;
- fallback visível;
- Virtual Screen polido.

Portão: usuário novo consegue adicionar um jogo e iniciar L1 sem editar JSON.

## Fase 4 — Unity e MiSide

Executar:

- P40 Unity comum;
- P41 Mono;
- P42 IL2CPP;
- P50 MiSide;
- P35 performance.

Ordem:

1. labs Unity 2021.3/2022.3/6000.3;
2. Mono source-owned;
3. IL2CPP source-owned;
4. provider IL2CPP no lab;
5. MiSide probe read-only;
6. MiSide L4-R;
7. HUD/cutscene;
8. controller;
9. visual hands;
10. interaction subsets.

Portão: build exata de MiSide passa render, cabeça, HUD, input, fallback e rollback em Quest/PICO.

## Fase 5 — Outlast e legado

Executar:

- P36 D3D9/OpenGL;
- P46 UE3;
- P52 Outlast.

Ordem:

1. instalar/inspecionar Outlast legal;
2. entregar L1;
3. D3D9Ex color lab;
4. avaliar depth;
5. UE3 read-only probe;
6. camera/stereo;
7. camcorder/night vision/HUD;
8. controls;
9. hand subsets;
10. Whistleblower separado.

Portão: renderer/build são confirmados; o adapter nunca depende de offsets publicados em profile.

## Fase 6 — D3D12 e Cyberpunk

Executar:

- P33 D3D12;
- P31 Depth;
- P44 REDengine;
- P51 Cyberpunk.

Ordem:

1. D3D12 OpenXR lab;
2. shared resource/fence;
3. depth ground truth;
4. Cyberpunk L1/L2;
5. decidir host permitido;
6. camera/context diagnostics;
7. L4-R;
8. HUD/vehicles/cinematics;
9. controls;
10. hand/weapon subsets.

Portão: clean/modded policies, D3D12 validation, context catalog e game build fingerprint.

## Fase 7 — Cobertura ampliada

Executar:

- P32 Geometry D3D11;
- P34 Vulkan;
- P43 Unreal 4/5;
- P45 engine desconhecida;
- P53 onboarding.

Entrega:

- novos jogos entram por um report read-only;
- L1 sempre primeiro;
- L2/L3 apenas com confidence/profile;
- Unreal source-owned de referência;
- Vulkan por processo, sem global layer.

## Fase 8 — Produto distribuível

Executar:

- P64 installer/updater/signing;
- P65 comunidade;
- P66 compliance;
- P67 release/suporte.

Entrega:

- ZIP e instalador HaumeaVR assinados;
- nenhum source/compiler para o amigo;
- repair/update/uninstall/rollback;
- adapters oficiais separados;
- profiles comunitários declarativos;
- SBOM/licenças;
- claims derivados da matriz.

## Organização recomendada de releases

### 0.2 — Marca e headsets

HaumeaVR, Quest/PICO, Virtual Screen e diagnóstico.

### 0.3 — Plataforma de adapters

Transport v2, input/mãos, profiles v2, menu VR.

### 0.4 — Unity

Labs, Mono/IL2CPP e MiSide beta.

### 0.5 — Legado

D3D9/UE3 e Outlast beta.

### 0.6 — D3D12

Depth D3D12 e Cyberpunk experimental.

### 0.7 — Tooling/comunidade

Profile Studio, onboarding e repository.

### 1.0 — Produto estável

Installer assinado, updater, rollback, matriz Quest/PICO e jogos com níveis publicados.

## Quando algo é considerado implementado

Uma feature só está concluída quando:

- código real existe;
- teste de sucesso e falha existe;
- diagnóstico existe;
- rollback/fallback existe;
- documentação foi atualizada;
- hardware/jogo real foi testado quando necessário;
- package publicado contém a mudança;
- claim público descreve a limitação.

## Recursos necessários

- Windows 10/11 x64;
- Visual Studio Build Tools/MSVC/CMake;
- .NET 10 SDK;
- Quest e PICO físicos;
- Meta Quest Link, PICO Connect e SteamVR;
- GPUs AMD/NVIDIA e, depois, Intel;
- Unity Editors das versões dos labs;
- Unreal source projects para UE4/UE5;
- instalações legais de MiSide, Outlast e Cyberpunk;
- certificado de code signing antes de stable.

## Estratégia de risco

- jogo comercial nunca é o primeiro teste;
- adapter nunca carrega em fingerprint desconhecido;
- anti-cheat/unknown multiplayer bloqueia in-process;
- sem driver/kernel;
- sem runtime global;
- sem community DLL;
- todo mod possui backup/journal;
- toda feature nativa possui L1 fallback.

## Próxima implementação concreta

Depois da renomeação, o próximo trabalho deve ser **P10**:

1. criar `CapabilitySnapshot`;
2. refatorar OpenXR em wrappers testáveis;
3. transportar o snapshot ao launcher;
4. validar Quest;
5. validar PICO;
6. só então iniciar P12/P13.

