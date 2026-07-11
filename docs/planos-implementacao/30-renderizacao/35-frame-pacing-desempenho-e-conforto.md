# P35 — Frame pacing, desempenho, qualidade adaptativa e conforto

**Status:** planejado transversal  
**Prioridade:** crítica para qualquer beta  
**Dependências:** P10, P12, P63  
**Desbloqueia:** promoção de todos os modos

## Objetivo

Medir e controlar latência/overhead para que um modo visualmente correto não seja inutilizável em VR. Também centralizar configurações de conforto por tipo de jogo.

## Orçamento de frame

| Refresh | Budget total |
| ---: | ---: |
| 72 Hz | 13,89 ms |
| 80 Hz | 12,50 ms |
| 90 Hz | 11,11 ms |
| 120 Hz | 8,33 ms |

O jogo, runtime, streaming e HaumeaVR compartilham esse orçamento. HaumeaVR não pode prometer refresh só pela ficha do headset.

## Métricas

### CPU

- wait/begin/end frame;
- pose/input sampling;
- adapter producer;
- IPC;
- capture;
- command recording;
- submit;
- menu;
- thread stalls/context switches.

### GPU

- game render quando observável;
- resource copy;
- Depth warp;
- Geometry second-eye;
- compositor HaumeaVR;
- mirror;
- queue/fence waits;
- VRAM.

### End-to-end proxies

- produced frame ID versus submitted;
- pose age;
- capture-to-submit;
- missed/repeated/dropped;
- runtime reprojection state quando exposto;
- network/encode stats apenas se rota expuser.

## Targets iniciais

Targets são gates, não garantias públicas:

- core idle/menu fechado: sem polling excessivo;
- L1 copy+mesh p95 ≤ 20% do frame budget na GPU de referência;
- L2 extra p95 ≤ 35%, ajustável;
- CPU HaumeaVR p95 ≤ 10% do budget por frame, excluindo adapter game logic;
- zero wait sem timeout no caminho quente;
- zero crescimento contínuo de VRAM/handles em 2 h;
- frame misses devem ser comparados a jogo/runtime sem HaumeaVR.

L3/L4 têm budgets por perfil porque podem duplicar render.

## Frame alignment

- predicted display time do core é autoridade;
- transportar frame ID/deadline;
- adapter usa pose prevista correspondente;
- late update de cabeça somente onde engine permite sem quebrar simulação;
- não misturar pose de frame N com textura N-2 sem marcar;
- política para producer late: repeat, reprojection ou fallback;
- clocks/timestamps convertidos e documentados.

## Quality scaler

Entradas:

- GPU/CPU headroom;
- miss streak;
- producer latency;
- temperature/network se disponível;
- confidence visual.

Ações graduais:

1. reduzir mirror/diagnóstico;
2. reduzir menu update;
3. render scale;
4. Depth quality/fill radius;
5. Geometry groups/shadows;
6. refresh recommendation ao usuário;
7. fallback.

Não alterar automaticamente world scale, comfort ou input.

## Comfort profile

- seated/standing/room-scale;
- rotation-only/limited/full 6DoF;
- translation clamp;
- snap/smooth turn e ângulo;
- vignette;
- horizon/roll lock;
- neck model;
- camera decoupling;
- cutscene behavior;
- vehicle mode;
- recenter/height;
- dominant hand;
- motion intensity warning.

Defaults variam por nível:

- L1: world-locked panel;
- L2: translation limitada;
- L3: perfil;
- L4: adapter/game context.

## Instrumentação

- GPU timestamp queries por backend;
- ETW/PresentMon/PIX somente em dev e com ferramentas disponíveis;
- ring buffer local de métricas;
- percentis em janela, não spam por frame;
- overlay compacto P15;
- export redigido P63;
- benchmark scene reproduzível.

## Testes

- refresh rates disponíveis;
- CPU/GPU bound;
- synthetic producer late;
- sleep/wake;
- quality transitions com hysteresis;
- 2 h soak;
- Quest USB/Air Link;
- PICO streaming/DP quando possível;
- RX 6600 baseline + NVIDIA + Intel/segunda AMD;
- mirror on/off;
- menu open/closed.

## Critérios de aceite

- Todo modo em beta publica baseline e overhead.
- Quality scaler não oscila.
- Frame ID/pose age aparecem no diagnóstico.
- Sem wait infinito.
- Comfort defaults por modo são seguros.
- Sessão longa não vaza.
- UI não confunde runtime reprojection com frame nativo.

## Primeiros passos do próximo chat

1. Instrumentar frame loop atual sem mudar comportamento.
2. Criar schema de métricas P63.
3. Medir self-test/Virtual Screen.
4. Adicionar GPU timestamps.
5. Só então implementar quality scaler.
