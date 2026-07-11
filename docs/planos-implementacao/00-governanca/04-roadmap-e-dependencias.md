# P04 — Roadmap e grafo de dependências

**Status:** planejado  
**Prioridade:** guia de sequência  
**Dependências:** P00–P03  
**Desbloqueia:** coordenação de todos os planos

## Objetivo

Ordenar o trabalho pelo valor entregue e pelos riscos reais. O roadmap não usa datas artificiais; cada fase termina por critérios verificáveis.

## Fase A — Plataforma cross-headset confiável

Planos:

- P10 Core OpenXR e capability snapshot;
- P20 Meta Quest PCVR;
- P21 PICO PCVR;
- P22 matriz cross-headset;
- P62 harness inicial.

Saída:

- cubo/test scene e Virtual Screen validados fisicamente em pelo menos um Quest e um PICO;
- diagnóstico registra runtime, rota, views, input e extensões;
- seleção de runtime continua por processo;
- controller actions básicas funcionam nos dois;
- recursos não expostos aparecem como indisponíveis, não como erro fatal.

## Fase B — Contratos de produto

Planos:

- P11 orquestrador/fallback;
- P12 transporte v2;
- P13 input/mãos/háptica;
- P14 SDK de adapters;
- P60 perfil v2;
- P61 segurança/rollback;
- P63 observabilidade.

Saída:

- lab publica/consome poses de cabeça, controles, mãos e frames;
- fallback é uma state machine testada;
- adapter possui manifest/ABI/fingerprint;
- perfil v1 migra para v2 sem perda;
- transações de instalação/remoção restauram byte por byte.

## Fase C — Interface própria completa

Planos:

- P15 menu VR;
- P16 launcher/Profile Studio;
- P30 evolução do Virtual Screen.

Saída:

- usuário ajusta sessão sem tirar o headset;
- desktop explica capacidade por dimensão;
- Profile Studio inspeciona dados expostos pelo lab/adapter, nunca memória arbitrária;
- perfis podem ser exportados/importados;
- UX de segurança e rollback é explícita.

## Fase D — Primeiro L4 completo: Unity

Planos:

- P40 Unity comum;
- P41 Unity Mono;
- P42 Unity IL2CPP;
- P50 MiSide;
- P35 desempenho/conforto.

Saída:

- labs Mono e IL2CPP cobrem versões/pipelines escolhidos;
- MiSide atinge primeiro L4-R, depois L4-I e então L4-H em interações definidas;
- cutscenes/HUD/save paths possuem testes;
- atualização de build invalida o adapter até revalidação.

## Fase E — Backends genéricos

Planos:

- P31 Depth 3D;
- P32 Geometry Stereo D3D11;
- P33 D3D12;
- P34 Vulkan;
- P45 engine desconhecida;
- P53 onboarding.

Saída:

- Depth é capacidade real em jogos D3D11/D3D12 validados;
- Geometry possui profile tooling e fallback;
- D3D12 core/transport está pronto para REDengine;
- Vulkan tem lab e pelo menos um título controlado;
- onboarding produz relatório e nível recomendado sem intervenção manual no código.

## Fase F — Cyberpunk e outras engines

Planos:

- P43 Unreal;
- P44 REDengine;
- P51 Cyberpunk 2077;
- P36 backend legado;
- P46 Unreal Engine 3;
- P52 Outlast.

Saída:

- Unreal possui plugin para projetos autorizados e fallback para comerciais;
- Cyberpunk começa em L1/L2, progride a L4 somente por integração específica segura;
- Outlast começa em L1, usa backend legado e progride por adapter UE3 específico.

## Fase G — Distribuição pública

Planos:

- P64 instalador/update/signing;
- P65 comunidade;
- P66 compliance;
- P67 release/suporte.

Saída:

- amigo recebe um instalador ou ZIP assinado, nunca source/compilador;
- atualizações são verificadas e reversíveis;
- adapter packs oficiais são separados de perfis declarativos;
- licenças e third-party notices estão completos;
- release só anuncia combinações presentes na matriz.

## Dependências duras

| Plano | Só começar depois de |
| --- | --- |
| P12 Transporte v2 | P10 capability model e P02 contratos |
| P13 mãos | P10 action/capability baseline |
| P14 adapters | P12 e P13 contratos estáveis |
| P15 menu VR | P10 composition + P13 input |
| P16 Profile Studio | P14 inspection API + P60 perfil v2 |
| P20/P21 | P10 diagnóstico extensível |
| P31/P32 | P12 resource transport |
| P33 | P10 backend abstraction + P12 sync/resource descriptors |
| P36 | P12 resource transport + P30 fallback |
| P40 | P12/P13/P14 |
| P42 | P40 e lab IL2CPP reproduzível |
| P50 | P42 e P61 |
| P44 | P14 e P33 |
| P46 | P14, P36 e P61 |
| P51 | P33, P44 e P61 |
| P52 | P36, P46 e P61 |
| P64 | P61, P63 e build reprodutível |
| P67 | P22, P62, P64 e P66 |

## Trabalho que pode ocorrer em paralelo

- Após P10: P20 e P21.
- Após P12/P13: P15 e P60.
- Após abstração gráfica: P31, P33 e preparação de P34 em labs distintos.
- Após P14: P40 e P43.
- P63 pode evoluir junto de qualquer fase, mantendo privacidade.
- P64 pode criar protótipo local cedo, mas assinatura/update público só no fim.

## Portões de promoção

### Fundação para experimental

- testes automatizados;
- diagnóstico;
- failure path;
- nenhum hardware necessário para a suíte básica.

### Experimental para beta

- dois modelos de GPU;
- pelo menos um Quest e um PICO quando cross-headset for relevante;
- três sessões de 30 minutos;
- rollback;
- documentação de limitações.

### Beta para estável

- build exato em matriz;
- atualização limpa e downgrade/rollback;
- crash-free target definido em P67;
- pacote assinado;
- suporte e coleta de diagnóstico;
- nenhuma pendência crítica de segurança/licença.

## Próxima ação recomendada

Começar por P10. Sem capability snapshot e teste físico, qualquer trabalho de mãos ou vendor-specific fica baseado em suposição. Em seguida, executar P20 e P21 antes de expandir o transporte.

## Critérios de aceite

- Cada plano tem dependências coerentes com esta tabela.
- Nenhum jogo específico aparece antes da engine/backend necessário.
- Toda fase entrega algo demonstrável, não apenas infraestrutura.
- Os gates de hardware e assinatura não são marcados como concluídos por teste simulado.

## Primeiros passos do próximo chat

1. Confirmar se P10 ainda é o primeiro bloqueador incompleto.
2. Se for, executar P10 e registrar capability snapshots.
3. Se não for, selecionar o primeiro plano incompleto no caminho crítico.
4. Não iniciar um plano de jogo antes de seus gates de engine, segurança e hardware.
