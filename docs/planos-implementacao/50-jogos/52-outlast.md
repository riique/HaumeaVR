# P52 — Plano de experiência VR para Outlast

**Status:** identidade confirmada; instalação local não encontrada  
**Prioridade:** terceiro jogo-alvo e primeiro UE3/legado  
**Dependências:** P13–P16, P20–P22, P30–P31, P35–P36, P46, P60–P66  
**Desbloqueia:** prova de compatibilidade com jogo/engine/API legados

## Objetivo

Levar o Outlast original de Virtual Screen a uma experiência VR integrada, com atenção especial à câmera em primeira pessoa, camcorder/night vision, perseguições, esconderijos, portas, animações roteirizadas e conforto.

## Identidade confirmada

- Título: **Outlast**.
- Steam AppID: `238320`.
- Desenvolvedor/publisher: Red Barrels.
- Lançamento original no PC: 4 de setembro de 2013.
- Modo oficial listado: single-player.
- Requisitos Windows oficiais da loja: DirectX 9.0c.
- A Epic lista Outlast como título movido por Unreal.
- Família técnica esperada: Unreal Engine 3; a versão exata e o renderer precisam ser confirmados na instalação.
- DLC Whistleblower deve ter coverage/fingerprint separado.

## Estado inicial

- Outlast não foi encontrado nas bibliotecas locais acessíveis em 11/07/2026.
- Nenhum hash/build/executável local foi validado.
- L1 é tecnicamente viável por captura externa quando o jogo estiver instalado.
- L2/L4 dependem de P36/P46 e da inspeção da build.

## Gate O0 — Instalação e fingerprint

1. Instalar uma cópia legal.
2. Capturar store build ID, versão, executável e hashes.
3. Confirmar arquitetura.
4. Inspecionar DLLs/config para UE3 e D3D9/D3D9Ex/D3D11.
5. Identificar DLC.
6. Detectar mods e anti-cheat.
7. Confirmar save paths e criar backup opcional.
8. Criar profile v2 L1.

Nenhum adapter é instalado neste gate.

## Gate O1 — Virtual Screen

- WGC/Desktop Duplication;
- windowed/borderless/fullscreen;
- aspect/gamma;
- tela curva/plana;
- gamepad/teclado original;
- menu HaumeaVR;
- audio/foco;
- Quest/PICO;
- 60 minutos.

Esse é o primeiro perfil publicável.

## Gate O2 — Backend legado e Depth

P36/P31:

- detectar device/renderer real;
- capturar color sem CPU quando possível;
- avaliar acesso ao depth;
- projection/near/far;
- HUD/camcorder mask;
- normal/reverse Z;
- night vision;
- blur/grain/fullscreen effects;
- confidence e fallback.

Se depth confiável não estiver disponível, Outlast permanece L1 até o adapter UE3.

## Gate O3 — Probe UE3 read-only

P46:

- engine/build;
- PlayerController/PlayerCamera/ViewTarget;
- camera contexts;
- HUD/Scaleform/Canvas;
- input actions;
- camcorder/night vision state;
- level transitions;
- scripted camera/matinee;
- nenhuma alteração.

IDs são opacos e presos ao fingerprint.

## Gate O4 — L4-R câmera e estéreo

- base camera + head delta;
- asymmetric projection;
- head rotation;
- 6DoF limitado e depois completo onde seguro;
- near clip/body/head handling;
- camera bob/shake reduction;
- first-person hands/body;
- scripted animations;
- stereo resources pelo P12;
- fallback instantâneo L1.

Contextos:

- exploração;
- corrida/perseguição;
- agachar/rastejar;
- esconderijo;
- interação roteirizada;
- queda/escalada/ledge;
- dano/morte;
- cutscene;
- camcorder/night vision;
- menu.

Cada contexto define tracking mode e comfort.

## Gate O5 — HUD, camcorder e cutscenes

- prompts;
- subtitles;
- menus/pause;
- camcorder frame;
- battery/night vision overlays;
- damage/fear effects;
- fades/loading;
- scripted cinematics.

Camcorder pode ser:

- head-aligned screen effect inicialmente;
- objeto virtual na mão em L4-H;
- overlay separado com reticle/battery;
- fallback original.

Night vision precisa permanecer legível em ambos os olhos sem duplicar exposição incorretamente.

## Gate O6 — L4-I controles

Mapear:

- move/turn/look;
- run/crouch/jump;
- interact/use;
- open/close;
- camcorder toggle;
- night vision;
- reload battery;
- peek;
- pause/UI;
- ações contextuais.

Primeiro controles Quest/PICO, depois gestos. Sempre manter botão fallback.

## Gate O7 — Mãos e interações

Subsets:

1. mãos visuais;
2. camcorder segurado/posicionado;
3. abrir/fechar portas;
4. pegar itens/baterias/documentos;
5. usar botões/objetos essenciais;
6. esconder/entrar/sair;
7. ledges/escalada somente se estável;
8. animações roteirizadas com blend/fallback.

Não transformar toda animação do jogo em física. Interações não catalogadas usam ação semântica original.

## Conforto

- camera bob/shake reduction;
- snap/smooth turn;
- vignette em corrida/perseguição;
- crouch físico opcional;
- seated/standing;
- translation clamp;
- cutscene virtual screen/rotation-only;
- horizon/roll control;
- intensity presets;
- emergency fallback.

## Test matrix

Por Quest/PICO:

- intro/menu;
- exploração;
- camcorder/night vision;
- doors/items;
- chase/run;
- hiding;
- crouch/ledge;
- scripted sequences;
- death/reload checkpoint;
- subtitles;
- Whistleblower separado;
- 60 min/2 h;
- install/remove/rollback.

## Critérios de aceite

- Identidade/build/renderer confirmados.
- L1 funciona sem módulo in-process.
- L2 só é anunciado com depth confiável.
- L4 adapter recusa fingerprint desconhecido.
- Camcorder/night vision/HUD são legíveis.
- Contextos intensos têm comfort strategy.
- Mãos publicam subset exato e controller fallback.
- Whistleblower não herda suporte sem teste.

## Primeiros passos do próximo chat

1. Instalar/localizar Outlast legalmente.
2. Executar Gate O0 e criar profile L1.
3. Confirmar D3D9/D3D9Ex/D3D11 e UE3.
4. Executar P36 lab antes do jogo.
5. Fazer probe UE3 read-only e só então L4-R.

## Referências oficiais

- [Outlast no Steam](https://store.steampowered.com/app/238320/Outlast/)
- [Outlast listado pela Unreal Engine](https://www.unrealengine.com/blog/happy-playstation-4-launch-day)

