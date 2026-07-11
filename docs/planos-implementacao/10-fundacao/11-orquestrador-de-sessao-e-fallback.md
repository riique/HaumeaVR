# P11 — Orquestrador de sessão e fallback

**Status:** planejado  
**Prioridade:** crítica  
**Dependências:** P02, P10, P60 (modelo mínimo)  
**Desbloqueia:** experiência robusta em todos os modos e jogos

## Objetivo

Substituir decisões espalhadas pelo ViewModel por uma state machine explícita que faça preflight, inicie componentes na ordem correta, degrade entre L4/L3/L2/L1 e sempre saiba como parar ou restaurar.

## Estado inicial

`MainWindowViewModel.cs` inicia diretamente Virtual Screen ou Unity Stereo. O perfil já contém uma ordem de fallback, mas não existe um executor comum com razões estruturadas, timeouts, compensações e histórico.

## Modelo de estado

```text
Idle
  -> Inspecting
  -> Preflight
  -> PreparingTransaction
  -> StartingCore
  -> StartingAdapter
  -> WaitingForGame
  -> Running
  -> Degrading
  -> Running(next mode)
  -> Stopping
  -> Restoring
  -> Idle

Any non-terminal state -> Failed
Failed -> Restoring -> Idle
```

Cada transição recebe `SessionEvent`, produz `SessionDecision` e persiste um evento redigido.

## SessionPlan imutável

Criado antes de iniciar processos:

- session ID aleatório;
- game/build fingerprint;
- profile ID/version;
- headset/runtime capability snapshot ID;
- modo solicitado;
- candidatos de fallback;
- adapter e versão;
- runtime manifest por processo;
- executável/argumentos;
- operações transacionais;
- timeouts;
- políticas de input/comfort;
- consentimentos;
- anti-cheat verdict;
- diretórios de log.

Mudança de configuração durante a sessão gera `ConfigRevision`; não altera silenciosamente o plano original.

## Preflight

Validar em ordem:

1. perfil/schema/assinatura;
2. fingerprint;
3. jogo e arquivos presentes;
4. anti-cheat e modo offline;
5. runtime/manifest/headset;
6. capabilities obrigatórias do modo;
7. adapter/host/hash/assinatura;
8. backup e espaço em disco;
9. conflito com processo do jogo já aberto;
10. permissões de diretório;
11. orçamento mínimo de GPU/memória configurado;
12. fallback L1 disponível.

Preflight não modifica o jogo.

## Razões estruturadas

Criar enum/catálogo:

- `RuntimeUnavailable`;
- `HeadsetUnavailable`;
- `CapabilityMissing`;
- `AdapterNotInstalled`;
- `AdapterFingerprintMismatch`;
- `ProtectedProcessDetected`;
- `UnknownAntiCheatState`;
- `GraphicsResourceRejected`;
- `FrameProducerTimeout`;
- `DepthUnreliable`;
- `StereoClassificationLost`;
- `GameExited`;
- `RuntimeLost`;
- `TransactionFailed`;
- `UserRequestedFallback`.

Cada razão define severidade, mensagem, ação sugerida, elegibilidade para fallback e necessidade de rollback.

## Política de fallback

- Automático apenas se o próximo modo não amplia permissões.
- Nunca trocar de L1 externo para módulo in-process sem nova confirmação.
- Anti-cheat/protected process só permite L1 externo.
- Fingerprint mismatch invalida L4; pode tentar L2/L1.
- Depth confidence baixa por N frames tenta L1 ou L3 já validado, não modo aleatório.
- Timeout de adapter interrompe o adapter antes de reutilizar recursos.
- Fallback deve aparecer no headset por overlay e no launcher.
- Usuário pode fixar “não degradar”, caso em que a sessão para com explicação.

## Compensações e rollback

Cada passo mutável registra uma função compensatória:

| Ação | Compensação |
| --- | --- |
| criar mapping/pipe | fechar handles e remover owner |
| instalar adapter | restaurar manifesto de arquivos |
| alterar config do jogo | restaurar backup hash-verificado |
| iniciar core | shutdown autenticado, depois terminate controlado |
| iniciar jogo | não matar por padrão; oferecer manter limpo aberto |
| aplicar input capture | liberar cursor/keys e limpar estado |

Compensações rodam em ordem reversa e são idempotentes.

## Implementação

1. Criar modelos em `apps/launcher/Features/Sessions`.
2. Extrair serviços atuais por interface.
3. Implementar reducer/state machine pura e testes.
4. Criar `SessionJournal` append-only local.
5. Integrar primeiro Virtual Screen.
6. Integrar Unity lab.
7. Adicionar runtime events do IPC.
8. Conectar overlay P15.
9. Expor status ao ViewModel sem lógica de negócio duplicada.

## Testes

- tabela de todas as transições;
- cancelamento em cada etapa;
- timeout em core, jogo e adapter;
- jogo fecha durante start;
- runtime some durante sessão;
- fingerprint muda;
- anti-cheat detectado entre análise e launch;
- fallback L4→L1;
- configuração “não degradar”;
- compensação falha uma vez e é repetida;
- crash do launcher com journal e recuperação na próxima abertura.

## Critérios de aceite

- Um único orquestrador inicia todos os modos.
- ViewModels não possuem sequência de processos/rollback.
- Cada sessão tem journal e razão final.
- Falha simulada em todo passo deixa core/handles/input limpos.
- O jogo pode permanecer aberto sem adapter quando isso for seguro.
- L1 sempre pode ser escolhido sem tocar no processo do jogo.

## Primeiros passos do próximo chat

1. Mapear os fluxos de `StartVirtualScreenAsync` e `StartUnityStereoAsync`.
2. Escrever a tabela de estado e teste puro.
3. Migrar Virtual Screen antes de Unity.
4. Injetar serviços atuais; não instanciar processo/IO no reducer.
5. Só apagar fluxos antigos depois de testes de integração equivalentes.

