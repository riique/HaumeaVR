# P16 — Launcher, interface gráfica própria e Profile Studio

**Status:** planejado  
**Prioridade:** alta  
**Dependências:** P11, P14, P15, P60, P63  
**Desbloqueia:** configuração acessível de engines/jogos e suporte

## Objetivo

Evoluir o launcher Avalonia existente para a interface principal do produto: biblioteca, setup de headset, leitura honesta de compatibilidade, sessões, edição de perfil e inspeção segura de adapters.

## Por que uma GUI própria

É vantajosa porque o produto precisa explicar uma matriz que launchers genéricos não entendem: runtime, rota PCVR, engine, build fingerprint, nível L1–L4, câmera, HUD, controles, mãos, adapter, backup e fallback. A GUI não substitui OpenXR ou a interface do jogo; ela organiza essas camadas.

## Direção de UX

### Navegação principal

1. **Início:** headset/runtime, sessão ativa e ação recomendada.
2. **Biblioteca:** jogos detectados e status por dimensão.
3. **Jogo:** análise, modos, perfil, adapter, backups e iniciar.
4. **Profile Studio:** edição avançada/inspeção.
5. **Dispositivos:** Quest/PICO, runtimes e capability snapshots.
6. **Diagnóstico:** logs, support bundle, testes.
7. **Atualizações:** app, adapter packs e perfis.
8. **Configurações:** idioma, privacidade, developer mode.

### Cartão de compatibilidade

Exibir separadamente:

- Visual: L1/L2/L3/L4-R;
- Cabeça: painel/3DoF/6DoF;
- Entrada: UI/emulada/nativa;
- Mãos: indisponível/rastreada/representada/interações validadas;
- Háptica;
- HUD/cutscene;
- headset/runtime/build testados;
- estabilidade: experimental/beta/stable.

Nunca usar um único selo verde para tudo.

## Profile Studio

### Offline

- editar render/câmera/HUD/input/comfort;
- validar schema em tempo real;
- comparar perfil base e override;
- visualizar fallback;
- exportar/importar bundle;
- mostrar fingerprint e compatibilidade.

### Live Inspector

Somente com adapter P14:

- árvore paginada de cenas/entidades;
- busca por nome/tipo/tag;
- filtros por Camera/Canvas/Transform/Collider/Input;
- selecionar e destacar;
- atribuir roles;
- preview de eye textures/HUD/depth;
- testar semantic action;
- capturar sugestão de binding;
- salvar IDs resilientes fornecidos pelo adapter.

Não ler memória arbitrária nem mostrar pointers/offsets.

### Interaction Mapper

Interface de grafo:

- source: controller action, joint/gesture ou context;
- conditions: gameplay/cutscene/menu/item state;
- target: ação semântica/adapter endpoint;
- transforms: deadzone, scale, hysteresis, cooldown;
- haptic response;
- fallback;
- teste/replay.

Perfil comunitário armazena apenas endpoints allowlisted pelo adapter.

## Arquitetura de frontend

- manter Avalonia/.NET 10;
- MVVM por feature;
- serviços de domínio fora do ViewModel;
- navegação e dialog service testáveis;
- design tokens comuns;
- localization resources;
- virtualização de listas/árvores;
- background tasks canceláveis;
- UI nunca bloqueia esperando processo/hardware;
- estado vem de observables/snapshots do orquestrador.

Estrutura alvo:

```text
apps/launcher/Features/
  Home/
  Library/
  GameDetails/
  Devices/
  Sessions/
  ProfileStudio/
  Diagnostics/
  Updates/
apps/launcher/Design/
apps/launcher/Localization/
```

## Fluxos críticos

### Primeiro uso

1. detectar runtimes;
2. orientar Meta Quest ou PICO;
3. rodar teste de headset;
4. mostrar capabilities;
5. descobrir jogos;
6. recomendar Virtual Screen.

### Preparar L4

1. analisar build;
2. explicar adapter e alterações;
3. criar backup;
4. instalar transacionalmente;
5. validar;
6. iniciar;
7. oferecer rollback.

### Falha

1. mensagem humana;
2. razão técnica expansível;
3. fallback seguro;
4. link para diagnóstico;
5. restaurar quando necessário.

## Fases

1. Extrair lógica do `MainWindowViewModel`.
2. Criar shell/navegação e tokens.
3. Home/Devices com capability snapshot.
4. Library/GameDetails com taxonomia.
5. Session view ligada a P11.
6. Profile editor v2.
7. Live Inspector no lab.
8. Interaction Mapper.
9. Diagnostics/update.
10. acessibilidade/localização/visual QA.

## Testes

- ViewModels/commands sem UI;
- navegação e cancelamento;
- 0, 1 e milhares de jogos;
- runtime ausente;
- perfil corrupto/migração;
- adapter indisponível;
- inspector com árvore grande/paginação;
- sessão muda durante navegação;
- PT-BR/EN e strings longas;
- 100/125/150/200% DPI;
- teclado, controle e leitor de tela no desktop;
- screenshots golden em resoluções alvo.

## Critérios de aceite

- usuário comum consegue L1 sem abrir área avançada.
- Usuário técnico cria/ajusta perfil sem editar JSON.
- Cada capability tem fonte/evidência.
- Nenhuma ação mutável ocorre sem resumo e rollback.
- Live Inspector funciona no lab e recusa adapter sem API.
- UI permanece responsiva durante scan/build/session.
- Menu VR e desktop mostram o mesmo estado.

## Riscos

- **Interface vira painel técnico confuso:** progressive disclosure e defaults seguros.
- **ViewModel monolítico reaparece:** feature services e testes.
- **Editor permite perfil perigoso:** schema/allowlist e preview de diff.
- **GUI promete recurso:** capability model único, sem flags duplicadas.

## Primeiros passos do próximo chat

1. Inventariar bindings da view atual e seus testes.
2. Criar modelos de compatibilidade multidimensional.
3. Extrair Session service antes do redesign visual.
4. Implementar shell + Home/Devices.
5. Só iniciar Inspector depois do mock P14 fornecer dados.

