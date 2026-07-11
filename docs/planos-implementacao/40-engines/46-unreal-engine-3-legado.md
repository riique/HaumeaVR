# P46 — Adapter Unreal Engine 3 legado

**Status:** não implementado; primeiro alvo Outlast  
**Prioridade:** alta depois do backend D3D9  
**Dependências:** P12–P14, P35, P36, P61, P66  
**Desbloqueia:** P52 Outlast L4

## Objetivo

Criar uma família de integração para jogos UE3 offline selecionados, reconhecendo que builds comerciais antigos não possuem o mesmo sistema de plugins e extensões OpenXR de UE4/UE5.

## Princípios

- game adapter por fingerprint, não plugin UE3 universal;
- provider de carga precisa ser autorizado e revisado;
- core HaumeaVR continua único owner OpenXR;
- UE3 produz câmera/frames para o transporte;
- D3D9/D3D11 é determinado pela build real;
- L1 continua disponível sem módulo in-process.

## Detecção

- executável/arquitetura;
- folders/packages UE3;
- engine/game version strings;
- D3D9/D3D9Ex/D3D11;
- UnrealScript/native modules;
- config INI;
- anti-cheat;
- DLC/build;
- mod policy.

Não desempacotar ou redistribuir packages/assets.

## Câmera

Conceitos a resolver por adapter:

- local PlayerController;
- PlayerCamera/CameraActor;
- view target;
- POV/FOV;
- camera modifiers/shakes;
- scripted cameras/matinee;
- first-person mesh;
- near clip;
- base transform + head delta.

IDs e endpoints ficam em adapter oficial. Perfis não contêm pointers/offsets.

## Renderização

Rotas:

1. adapter obtém view/projection e integra stereo pass;
2. D3D9/D3D11 resources são publicados pelo P12;
3. UI/pós-processamento por eye ou layer;
4. efeitos incompatíveis recebem strategy por jogo;
5. Depth P31 como híbrido;
6. fallback L1.

## UI

UE3 pode usar Canvas, Scaleform/GFx e overlays customizados. O adapter deve:

- classificar HUD/subtitles/prompts/menus;
- renderizar mono em quad layer quando possível;
- preservar input original;
- tratar cutscenes e fullscreen effects;
- restaurar tudo no stop.

## Input

- P13 semantic actions;
- UE3 input bindings/PlayerInput via endpoints allowlisted;
- controller Quest/PICO;
- gamepad/keyboard fallback;
- mãos apenas no game adapter;
- haptics por semantic event.

## Labs

Como UE3 source/UDK possui disponibilidade/licenciamento próprios:

- usar primeiro labs D3D9 próprios;
- usar um projeto UE3/UDK apenas se toolchain/licença legalmente acessíveis;
- separar artefatos do engine;
- nenhum arquivo de Outlast no repositório;
- mockar camera/context/input concepts quando toolchain não estiver disponível.

## Fases

1. Detection/fingerprint.
2. D3D9 lab P36.
3. Adapter ABI mock UE3.
4. Outlast read-only probe.
5. Camera/context catalog.
6. L4-R stereo/head.
7. HUD/cutscene.
8. L4-I.
9. hands subsets Outlast.
10. segundo jogo UE3 para provar reuso.

## Testes

- D3D9/D3D11 variants;
- camera change/matinee;
- UI/Scaleform;
- level transition;
- device reset;
- input contexts;
- adapter crash/fallback;
- fingerprint mismatch;
- install/remove;
- 60 min/2 h.

## Critérios de aceite

- Nenhum componente UE4/UE5 é presumido.
- Adapter recusa build desconhecida.
- Core continua único owner OpenXR.
- Camera/HUD/input funcionam no lab/Outlast definido.
- L1 permanece sem alteração do jogo.
- Segundo título comprova o que é comum versus game-specific.

## Primeiros passos do próximo chat

1. Inspecionar uma instalação legal de Outlast.
2. Confirmar engine build e graphics API.
3. Completar P36 color path.
4. Criar read-only probe.
5. Implementar camera antes de input/mãos.

