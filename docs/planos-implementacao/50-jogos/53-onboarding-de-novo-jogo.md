# P53 — Playbook de onboarding para qualquer novo jogo

**Status:** planejado  
**Prioridade:** alta  
**Dependências:** P11, P22, P30, P45, P60–P63  
**Desbloqueia:** escala para novos títulos/engines

## Objetivo

Transformar “quero este jogo em VR” em um processo reproduzível que primeiro entrega L1, depois prova cada nível superior sem comprometer segurança.

## Entrada mínima

- título/loja/AppID ou executable;
- instalação legal acessível;
- objetivo do usuário: tela, 3D, 6DoF, controles, mãos;
- headset/rota;
- offline/multiplayer;
- tolerância a mods e backups.

## Etapa 1 — Intake

- confirmar identidade;
- preservar nome oficial;
- criar game slug;
- registrar licença/mod policy disponível;
- não executar jogo ainda.

## Etapa 2 — Scan read-only

Usar P45:

- fingerprint;
- engine/backend;
- anti-cheat;
- mods;
- VR existente;
- window/capture;
- report com evidence.

Saída: `profiles/candidates/<slug>/detection.json` local.

## Etapa 3 — Safety classification

- ProtectedBlocked → L1 externo;
- UnknownNeedsReview → L1 até revisão;
- OfflineAdapterAllowed → labs/provider;
- VRNative → launch/profile, não conversão.

## Etapa 4 — L1

- profile v2;
- capture backend;
- aspect/HDR;
- controller pass-through;
- Quest/PICO smoke;
- fallback.

L1 deve ser entregue antes de pesquisa longa, salvo se captura for impossível.

## Etapa 5 — Escolher L2/L3/L4

Score:

- engine adapter disponível;
- build/fingerprint estável;
- mod route permitida;
- camera/HUD complexity;
- graphics API;
- depth access;
- anti-cheat;
- performance;
- demand/value;
- hardware available.

Recomendação:

- L4 se adapter/route seguros;
- L2 quando depth é alcançável;
- L3 somente se lab/profile justificarem;
- manter L1.

## Etapa 6 — Context catalog

Para L4:

- scenes/maps;
- camera states;
- gameplay/UI/cutscene/vehicle;
- actions;
- hand interaction categories;
- HUD/subtitles;
- save checkpoints;
- performance modes.

O catálogo define o que “completo” significa naquele jogo.

## Etapa 7 — Labs

Antes do jogo:

- engine version/API lab;
- render/camera;
- input;
- hands/interactable;
- failure/fallback;
- packaging/rollback.

Se não é possível reproduzir em lab, documentar por que o risco no jogo é aceitável ou parar.

## Etapa 8 — Adapter/profile incremental

Gates:

1. read-only attach;
2. render;
3. camera;
4. HUD/cutscene;
5. controller;
6. visual hands;
7. interactions;
8. long run;
9. release package.

Cada gate pode ser revertido independentemente.

## Artefatos por jogo

```text
profiles/games/<slug>/
  profile.schema-v2.json
  compatibility.md
  test-matrix.json
  changelog.md
adapters/games/<slug>/       # somente se L4
tests/game-labs/<slug>/      # sem assets do jogo
```

Evidências privadas com paths/hashes completos ficam em LocalAppData, não no perfil público.

## Atualização de jogo

1. detectar fingerprint novo;
2. desabilitar adapter antes do launch;
3. manter L1;
4. diff read-only;
5. rodar smoke/labs;
6. publicar profile/adapter novo;
7. nunca reaproveitar whitelist ampla.

## Critérios de aceite

- Novo jogo recebe report em uma sessão de análise.
- L1 não depende de engine.
- L2/L3/L4 têm gates próprios.
- Context catalog define cobertura.
- Update não carrega adapter antigo.
- Nenhum asset/binário do jogo é redistribuído.

## Primeiros passos do próximo chat

1. Confirmar identidade e objetivo.
2. Rodar P45 read-only.
3. Criar profile L1.
4. Classificar engine/safety.
5. Só então abrir plano engine e criar plano game-specific.

