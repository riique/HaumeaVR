# P12 — Protocolo e transporte de alta frequência v2

**Status:** planejado  
**Prioridade:** crítica  
**Dependências:** P02, P10  
**Desbloqueia:** P13–P16, P31–P34, P40–P44

## Objetivo

Evoluir o IPC v1 e o transporte Unity atual para um contrato independente de engine/backend que carregue frames, poses, controles, mãos, depth, motion vectors, métricas e háptica sem usar JSON por frame.

## Princípios

- named pipe permanece o plano de controle;
- memória compartilhada + recursos GPU formam o plano de dados;
- core cria/possui o canal e autoriza um PID produtor;
- protocolos são versionados e testados entre versões;
- nenhuma espera de adapter bloqueia indefinidamente OpenXR;
- cada backend tem descritor e sincronização explícitos;
- nenhuma estrutura C++ com STL/vtable cruza processos.

## Canais

### ControlPipe v2

Mensagens adicionais:

- `prepare-session`;
- `attach-adapter`;
- `capability-snapshot`;
- `apply-config-revision`;
- `request-fallback`;
- `interaction-catalog`;
- `inspection-request/response`;
- `transaction-status`;
- `support-bundle`.

Manter framing existente e ampliar schema. O handshake negocia major/minor/features.

### FrameTransport v2

Um mapping por sessão com regiões fixas:

```text
TransportHeader
CoreToAdapterFrameState[triple-buffered]
AdapterToCoreFrameState[triple-buffered]
InputState[double-buffered]
HapticRequestRing
DiagnosticEventRing
ResourceTable
StringTable(bounded)
```

Usar índices/generation IDs, não ponteiros.

## Header obrigatório

- magic `UXR2`;
- major/minor;
- total size;
- endianness/architecture;
- session UUID;
- core PID e adapter PID esperado;
- capability bits negociadas;
- offsets/counts/sizes de cada região;
- nonce derivado da sessão;
- creation timestamp;
- atomics para heartbeat e shutdown;
- reservados zerados.

Todo consumer valida overflow de `offset + count * stride` antes de mapear uma região.

## FrameState core → adapter

- `frameId`;
- predicted display time;
- shouldRender;
- active view count;
- base/reference space revision;
- head/base transform;
- pose/FOV/viewport por view;
- recommended e active dimensions;
- input snapshot sequence;
- config revision;
- deadline/quality hint;
- tracking flags.

## InputState

Por mão:

- current interaction profile ID;
- grip/aim/palm poses e validity/velocity flags;
- trigger/squeeze/thumbstick/buttons/touches;
- 26 joints OpenXR com radius, pose, velocity e confidence flags quando disponíveis;
- hand active/source;
- gesture primitives produzidos por P13;
- timestamp/frameId.

O transporte preserva dados crus e derivados; adapters podem escolher.

## FrameState adapter → core

- frameId produzido;
- adapter state/health;
- generation do resource set;
- color resource por view;
- depth resource por view opcional;
- motion vector resource opcional;
- HUD layer resources opcionais;
- color space, alpha mode, orientation e viewport;
- CPU/GPU timestamps;
- quality/confidence;
- drop reason;
- inspection revision.

## ResourceDescriptor

Campos comuns:

- type, version, size;
- backend: D3D11/D3D12/Vulkan/CPU-test-only;
- resource ID/generation;
- owner PID;
- width/height/array/mips/samples;
- format;
- usage/semantic;
- sync primitive;
- name/handle-table index;
- expected state/layout;
- lifetime begin/end generation.

Backends:

- D3D11: named NT shared handle + keyed mutex keys/timeouts;
- D3D12: shared resource/heap handle + shared fence/value + resource states;
- Vulkan: external memory/semaphore Win32 handles + format/layout/queue family;
- CPU somente para testes pequenos, nunca sessão de produção.

## Concorrência

- seqlock ou triple buffer para snapshots;
- release/acquire atomics documentados;
- ring buffers single-producer/single-consumer onde possível;
- timeouts inferiores ao frame budget;
- heartbeat dos dois lados;
- core nunca espera GPU resource sem limite;
- producer não reutiliza generation até ack ou janela segura.

## Segurança

- nome aleatório `Local\HaumeaVR-v2-<id>`;
- DACL restrita ao usuário/processos esperados;
- owner cria mapping e recusa preexistente;
- PID/start time/image hash conferidos;
- handles duplicados somente para PID autorizado;
- limites rígidos de tamanho/count/string;
- nomes não são confiança; descritor real é revalidado na API;
- token do control pipe autoriza o attach;
- cleanup em crash por owner + journal.

## Migração

1. Especificar schemas/headers em `shared/protocol_v2`.
2. Criar biblioteca C ABI e testes sem OpenXR.
3. Implementar bridge v1↔v2 somente para Unity lab.
4. Core publica v2 e ainda aceita v1 atrás de feature flag.
5. Migrar native Unity plugin.
6. Migrar launcher control messages.
7. Remover v1 apenas após duas releases e teste de pacote antigo com mensagem clara.

## Arquivos previstos

- `shared/protocol_v2/include/uxrb/protocol_v2/*.h`;
- `shared/protocol_v2/src/*.cpp`;
- `shared/protocol_v2/schemas/*.json`;
- bindings C# gerados ou escritos com layout testado;
- `tests/protocol_v2_*`;
- documentação wire format em `shared/protocol_v2/README.md`.

## Testes

- golden binary layouts x64;
- size/offset overflow;
- major mismatch e minor forward compatibility;
- mapping collision;
- PID/start-time mismatch;
- seqlock sob milhões de iterações;
- producer crash;
- stale generation;
- dropped frames;
- ring full;
- haptic flood rate limited;
- D3D11 invalid handle/format/mutex timeout;
- D3D12/Vulkan descriptors parseáveis antes dos backends;
- fuzz de control JSON e headers.

## Critérios de aceite

- transporte sustenta pose/input/frame sem alocação por frame no caminho quente;
- adapter lento não bloqueia frame loop;
- Unity lab funciona integralmente por v2;
- métricas de drop/latência são visíveis;
- protocolo rejeita todos os casos adversariais dos testes;
- documentação contém layout e ownership suficientes para outro adapter.

## Primeiros passos do próximo chat

1. Ler `shared/unity_transport` e `shared/ipc` completos.
2. Medir tamanhos/layouts v1 e criar testes de compatibilidade.
3. Escrever wire spec v2 antes dos headers.
4. Implementar mapping/header/snapshot sem recursos GPU.
5. Adicionar D3D11 somente depois dos testes de concorrência.
