# P14 — SDK, manifesto e host de adaptadores

**Status:** planejado  
**Prioridade:** crítica para múltiplas engines  
**Dependências:** P12, P13, P60 mínimo, P61 política  
**Desbloqueia:** P40–P45, P50–P53, Profile Studio

## Objetivo

Definir uma forma única, segura e versionada de adicionar integrações de Unity, Unreal, REDengine e jogos específicos sem colocar lógica de engine no core nem aceitar DLL comunitária arbitrária.

## Tipos de pacote

### Official Engine Adapter

Binário assinado pelo projeto, reutilizável por uma família de engine/build. Ex.: Unity Mono bridge.

### Official Game Adapter

Binário assinado e limitado a fingerprints exatos. Ex.: MiSide interaction adapter.

### Declarative Profile

JSON/assets de configuração sem código. Pode ser comunitário após assinatura/revisão.

### Lab Adapter

Somente desenvolvimento, certificado/test key distinto, recusado em release estável.

## Manifesto

Campos obrigatórios:

- schema version;
- adapter ID, nome, versão e publisher;
- tipo;
- host API major/minor;
- arquiteturas;
- engines e intervalos de versão;
- fingerprints/executáveis;
- backends gráficos;
- capacidades L4-R/I/H;
- método de deployment;
- arquivos e hashes;
- permissões declaradas;
- incompatibilidades;
- dependências/licenças;
- assinatura e cadeia;
- rollback recipe ID;
- status experimental/beta/stable.

O launcher valida o manifesto antes de copiar qualquer arquivo.

## ABI C estável

Conjunto conceitual:

- `UxrbAdapter_GetDescriptor`;
- `UxrbAdapter_Probe`;
- `UxrbAdapter_Start`;
- `UxrbAdapter_AttachTransport`;
- `UxrbAdapter_ApplyConfig`;
- `UxrbAdapter_TickMainThread`;
- `UxrbAdapter_OnRenderEvent`;
- `UxrbAdapter_GetInteractionCatalog`;
- `UxrbAdapter_Inspect`;
- `UxrbAdapter_Stop`;
- `UxrbAdapter_GetLastError`.

Todas as funções:

- retornam status explícito;
- recebem structs com type/version/size;
- nunca lançam exceção através da fronteira;
- usam buffers fornecidos pelo caller ou callbacks versionados;
- são idempotentes onde indicado;
- têm thread affinity documentada.

## Capability negotiation

No start:

1. host oferece API/protocolo e capabilities core;
2. adapter declara obrigatórias/opcionais;
3. interseção vira `AdapterSessionCapabilities`;
4. adapter recusa fingerprint ou feature ausente com código;
5. orquestrador escolhe fallback.

## DeploymentProvider

Não existe “injector universal” no SDK. Cada provider usa um caminho autorizado:

- **BuiltWithBridge:** projetos/labs que já incluem o módulo;
- **ManagedModLoader:** loader documentado e aprovado para um título offline;
- **NativeModFramework:** integração específica após licença/security review;
- **OfficialGameMod:** mecanismo oficial do jogo;
- **GraphicsObserver:** módulo atual-process opt-in por host controlado;
- **ExternalOnly:** nenhum módulo no jogo.

O provider declara passos, backup, preconditions e compensações para P61.

## Adapter host e isolamento

Um módulo dentro do jogo não pode ser plenamente sandboxed. Mitigações:

- somente assinatura oficial;
- hash e fingerprint exatos;
- superfície ABI mínima;
- privilégios do usuário, nunca admin por padrão;
- comunicação local autenticada;
- timeouts/watchdog no sidecar;
- crash containment no core;
- adapter pode morrer sem levar a sessão OpenXR;
- Profile Studio consulta uma API de inspeção limitada.

Ferramentas de desenvolvimento podem carregar builds não assinados apenas com modo developer explícito, watermark e configuração fora do canal stable.

## Inspection API

Para permitir GUI própria:

- árvore paginada de cenas/entidades expostas;
- IDs opacos estáveis na sessão, nunca pointers;
- tipo, nome, tags e componentes permitidos;
- roles sugeridos: main camera, HUD canvas, player root, left/right hand anchor, interactable;
- properties allowlisted;
- ações de teste allowlisted;
- streaming de seleção/highlight;
- rate limit;
- adapter decide o que é seguro expor.

Não implementar memory scanner genérico no Profile Studio.

## Interaction Catalog

Adapter publica:

- semantic actions aceitas;
- interactables/classes suportadas;
- gestures aceitos;
- anchors/skeleton;
- haptic events;
- context states (menu, gameplay, cutscene, vehicle);
- bindings default;
- limitações.

P60 armazena overrides declarativos por build.

## Estrutura de source

```text
shared/adapter_api/
  include/uxrb/adapter_api.h
  schemas/adapter-manifest.schema.json
  schemas/interaction-catalog.schema.json
  tests/
engines/common/adapter_host/
apps/launcher/Features/Adapters/
distribution/adapters/
```

## Implementação por fases

1. Manifest schema + validator + test signatures fake.
2. Header ABI + mock adapter.
3. Host dentro do Unity lab.
4. Capability negotiation.
5. DeploymentProvider ExternalOnly/BuiltWithBridge.
6. Transaction integration P61.
7. Inspection API no lab.
8. Interaction catalog.
9. Packaging/signing real.
10. Providers específicos por engine.

## Testes

- manifest válido/inválido;
- assinatura, hash e downgrade;
- major/minor ABI;
- adapter que lança/crasha/trava;
- timeout;
- double start/stop;
- wrong thread;
- fingerprint mismatch;
- capability mismatch;
- inspection pagination/rate limit;
- path traversal em arquivos;
- rollback de deployment;
- stable recusa dev adapter.

## Critérios de aceite

- mock adapter e Unity lab usam o mesmo ABI.
- Core não contém include de Unity/Unreal/REDengine.
- Perfil comunitário não carrega binário.
- Todo deployment tem rollback.
- Crash do mock adapter produz fallback, não crash do launcher/core.
- Profile Studio inspeciona somente objetos fornecidos por adapter.

## Primeiros passos do próximo chat

1. Criar manifesto/schema e fixtures.
2. Escrever `adapter_api.h` sem dependência C++.
3. Implementar mock adapter in-process no lab controlado.
4. Integrar capability negotiation ao transporte.
5. Adiar qualquer loader comercial até P61 estar funcional.

