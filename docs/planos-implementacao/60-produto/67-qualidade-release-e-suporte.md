# P67 — Qualidade, canais de release e suporte

**Status:** futuro; define conclusão do produto  
**Prioridade:** gate final e contínuo  
**Dependências:** P22, P35, P62–P66  
**Desbloqueia:** releases estáveis confiáveis

## Objetivo

Definir quando uma build pode sair, o que cada canal promete e como diagnosticar, reverter e encerrar suporte a combinações antigas.

## Versionamento

Separado:

- app/core version;
- control protocol;
- frame transport;
- adapter API;
- engine adapter;
- game adapter;
- profile schema/profile version;
- installer/update metadata.

SemVer onde aplicável. Game fingerprint não é versão semântica.

## Canais

### Developer

- local;
- unsigned/dev key;
- diagnostics detalhados;
- labs;
- não indicado a usuário final.

### Beta

- assinado;
- experimental adapters opt-in;
- limitações explícitas;
- rollback;
- telemetria ainda opt-in;
- hardware matrix parcial.

### Stable

- somente capabilities Supported;
- package/signatures;
- clean install/update;
- support window;
- no dev adapter;
- claim matrix completa.

## Release gates

### Código

- clean build Debug/Release;
- all tests;
- warnings as errors;
- dependency/security/license scan;
- ABI/schema checks;
- no secrets.

### Produto

- migration;
- install/repair/update/uninstall;
- rollback;
- localization;
- accessibility smoke;
- docs.

### Hardware

- Quest/PICO lines exigidas;
- GPU coverage;
- route versions;
- lifecycle;
- performance/soak.

### Jogos

- exact build;
- context catalog;
- clean/modded policy;
- restore;
- capability dimensions.

### Supply chain

- SBOM;
- hashes;
- signatures/timestamps;
- provenance;
- download/metadata verified.

## Go/no-go

No-go:

- security critical/high sem mitigação;
- data loss/rollback failure;
- protected process path;
- signature/update failure;
- crash loop;
- global runtime mutation;
- claim sem evidence;
- license blocker.

Beta-only:

- hand subset incompleto;
- performance limitada;
- runtime version workaround;
- game contexts faltantes, se claramente listados.

## Release notes

Informar:

- capabilities adicionadas;
- headset/runtime/game builds;
- breaking migrations;
- known issues;
- safety changes;
- install/update;
- rollback;
- retired support.

Não escrever “melhor compatibilidade” sem matriz.

## Support bundle/triage

Fluxo:

1. identificar app/session code;
2. pedir support bundle redigido;
3. classificar runtime/headset, graphics, adapter, profile, transaction;
4. reproduzir em lab;
5. issue com severity;
6. workaround/fallback;
7. fix+regression test.

Nunca pedir ao usuário que desative anti-cheat/proteção ou envie game binary/save sem necessidade e permissão.

## SLOs/indicadores internos

- crash-free session rate por modo;
- successful launch/fallback;
- install/update/rollback;
- frame misses/overhead;
- unresolved regression age;
- profile invalidation after game update;
- support bundle redaction pass.

Definir números depois de baseline suficiente; não inventar meta sem dados.

## Compatibilidade e EOL

- support window por app/adapter;
- runtime/game update pode mover build a Retested Pending;
- stable mantém versão anterior instalável por rollback;
- revogar adapter inseguro;
- profile local continua exportável;
- EOL comunicado com fallback L1 quando possível.

## Definition of Done do programa “completo”

O produto nunca termina no sentido de suportar todo jogo. Uma versão major pode ser chamada completa quando:

- Quest e PICO passam matriz PCVR;
- L1 é robusto;
- L2 funciona em D3D11/D3D12 selecionados;
- L4 existe em Unity Mono/IL2CPP, com MiSide;
- uma engine diferente tem integração validada;
- Cyberpunk tem nível claramente publicado;
- GUI desktop/VR/Profile Studio estão operacionais;
- profiles/adapters são seguros/versionados;
- instalador/update/rollback assinados;
- suporte/compliance existem.

## Critérios de aceite

- Pipeline bloqueia no-go automaticamente onde possível.
- Stable claims vêm de P22.
- Rollback package está disponível.
- Support reproduz erros por event code.
- EOL/revocation funcionam.
- Release é instalável por um amigo sem source/toolchain.

## Primeiros passos do próximo chat

1. Criar release checklist machine-readable.
2. Mapear scripts/testes existentes.
3. Implementar gate de schema/security/package.
4. Gerar release candidate local.
5. Não publicar stable antes de P20/P21 hardware.

