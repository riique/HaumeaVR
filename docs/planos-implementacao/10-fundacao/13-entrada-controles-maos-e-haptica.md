# P13 — Entrada, controles, mãos, gestos e háptica

**Status:** planejado  
**Prioridade:** crítica para experiência nativa  
**Dependências:** P10, P12  
**Desbloqueia:** P14–P16 e interações L4

## Objetivo

Criar uma camada de input cross-headset que preserve dados OpenXR, ofereça ações semânticas para adapters e deixe explícita a diferença entre rastrear uma mão e integrar essa mão ao gameplay.

## Três camadas

### Camada 1 — Hardware OpenXR

- interaction profile;
- grip/aim/palm poses;
- botões, eixos e touches;
- output haptic;
- hand joints e flags;
- origem controller/hand;
- eventos de mudança.

### Camada 2 — Semântica HaumeaVR

Ações independentes de marca:

- move, turn, jump, crouch, sprint;
- primary/secondary action;
- interact/use;
- grab left/right;
- aim/fire/reload;
- menu/pause/recenter;
- teleport/snap turn;
- UI point/select/back;
- pose/gesture primitives.

### Camada 3 — Binding do jogo

Um adapter/perfil mapeia semântica para:

- teclado/mouse em modo externo permitido;
- entrada nativa da engine;
- action map do jogo;
- método/evento/collider/interactable específico;
- animação de mãos/corpo;
- resposta háptica.

Somente a camada 3 pode afirmar que “pegar”, “abrir” ou “usar” funciona no jogo.

## OpenXR Action System

Criar action sets separados:

- `uxrb_system`: menu, recenter, emergency stop;
- `uxrb_gameplay`: poses e controles;
- `uxrb_ui`: pointer/select/scroll/back;
- `uxrb_debug`: toggles apenas em builds de desenvolvimento.

Subaction paths esquerda/direita. Priority e ativação dependem do contexto; o menu HaumeaVR deve suspender ações enviadas ao jogo para evitar cliques duplos.

## Hand tracking

Usar `XR_EXT_hand_tracking` somente quando enumerada e quando o system property confirmar suporte. Por mão:

- criar/destroir tracker com a sessão;
- localizar 26 joints no predicted display time;
- preservar `POSITION_VALID/TRACKED` e `ORIENTATION_VALID/TRACKED`;
- radius e velocities quando disponíveis;
- não inventar pose para joint inválida;
- filtrar jitter sem adicionar atraso excessivo;
- detectar perda/reentrada;
- expor source/confidence.

Extensões como `XR_EXT_hand_interaction` e recursos Meta/PICO são upgrades opcionais. Nenhuma extensão vendor é requisito do caminho comum.

## Gestos

Primitivas derivadas, versionadas:

- pinch por dedo;
- grab curl;
- point;
- open palm;
- poke;
- fist;
- hand velocity/throw estimate.

Regras:

- thresholds calibráveis por usuário/headset;
- hysteresis;
- confidence mínima;
- timestamps;
- nenhum gesto crítico acionado por um frame isolado;
- adapters podem ignorar derivados e usar joints crus.

## Modos de input

1. **Controller native:** adapter recebe ações e poses.
2. **Hand native:** adapter recebe joints/gestos e cria representação/interação.
3. **Controller emulation:** foreground-only keyboard/mouse usando APIs Windows documentadas e confirmação.
4. **Hybrid:** controle para locomoção, mãos para interação selecionada.
5. **UI-only:** controles/mãos operam apenas menu HaumeaVR.

Não adicionar driver virtual de gamepad. Se um jogo exigir XInput verdadeiro, resolver dentro de adapter oficial ou manter teclado/mouse; não instalar ViGEm/driver silenciosamente.

## Pointer e foco

- ray a partir de aim pose do controle;
- para mão, pinch/poke pose quando disponível ou ray derivado;
- cursor visual com depth e confidence;
- captura exclusiva enquanto menu aberto;
- release garantido em crash/stop;
- confirmação para entrada externa no jogo;
- respeitar janela foreground e UIPI.

## Háptica

Fluxos:

- HaumeaVR UI → feedback curto;
- adapter → `HapticRequest` semântico;
- core → `xrApplyHapticFeedback`;
- stop explícito;
- rate limit, duração/amplitude/frequência limitadas;
- capability downgrade quando runtime só suporta vibração simples;
- side/hand routing;
- métricas de requests descartados.

Adapters nunca recebem handle OpenXR.

## Calibração

Perfil de usuário local:

- handedness;
- altura/arm span opcional;
- grip offset por interaction profile;
- thresholds de pinch/grab;
- deadzones;
- snap turn;
- seated/standing;
- accessibility remaps.

Dados não entram em perfil comunitário e não são enviados remotamente.

## Implementação

1. ActionSystem base em P10.
2. DTOs v2 e input snapshots.
3. debug renderer de grip/aim/controllers.
4. hand joint collector.
5. debug hand skeleton no lab.
6. gesture recognizer puro com replay de traces sintéticos.
7. semantic action mapper.
8. UI focus router.
9. haptic queue.
10. adapter bindings no Unity lab.

## Testes

- interaction profile muda controller↔hand;
- cada action e subaction;
- tracking flags inválidas;
- perda de uma mão;
- trace de pinch com hysteresis;
- controller e hands simultâneos quando runtime suporta;
- menu captura input;
- haptic flood/timeout/stop;
- Quest e PICO com controladores;
- mão óptica somente onde snapshot comprovar exposição pelo PC runtime;
- fallback controller quando mãos não chegam ao PC.

## Critérios de aceite

- mesmo semantic action funciona em Quest e PICO controllers.
- InputState chega ao Unity lab pelo v2.
- Hand skeleton aparece apenas com joints válidos.
- Perda de tracking não deixa ação `grab` presa.
- Menu não envia gameplay enquanto está em foco.
- Háptica é limitada e para no shutdown.
- GUI diferencia “tracking de mãos disponível” de “mãos integradas neste jogo”.

## Risco central

Quest/PICO podem suportar mãos no modo standalone e não expô-las pela rota PCVR escolhida. O produto deve feature-detect isso. Um plano de jogo sempre oferece controller como fallback e nunca usa a ficha técnica do headset como evidência do runtime PC.

## Primeiros passos do próximo chat

1. Implementar poses/actions de controle antes de hand tracking.
2. Registrar interaction profiles reais de Quest/PICO.
3. Transportar snapshot de input no lab.
4. Só então habilitar `XR_EXT_hand_tracking`.
5. Gravar traces sintéticos/redigidos para testar gestos sem hardware.

## Referências oficiais

- [OpenXR 1.1 — input e interaction profiles](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html)
- [XR_EXT_hand_tracking](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XR_EXT_hand_tracking.html)
- [XR_EXT_hand_interaction](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XR_EXT_hand_interaction.html)
