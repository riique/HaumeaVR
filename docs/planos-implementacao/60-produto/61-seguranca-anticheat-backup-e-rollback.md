# P61 — Segurança, anti-cheat, transações, backup e rollback

**Status:** invariantes parciais implementadas; expansão planejada  
**Prioridade:** bloqueador de adapters e release  
**Dependências:** P02, P11, P14, P60  
**Desbloqueia:** qualquer deployment em jogo e P64

## Objetivo

Garantir que ampliar compatibilidade não transforme HaumeaVR em bypass, persistência oculta ou causa de instalações corrompidas. Toda alteração em jogo deve ser autorizada, inventariada e reversível.

## Invariantes existentes a preservar

- sem kernel driver;
- sem serviço;
- sem remote injector;
- Detours process creation/injection fora do build;
- observer current-process opt-in;
- hard veto a anti-cheat reconhecido;
- runtime OpenXR por processo;
- IPC local autenticado;
- shared transport preso a PID;
- módulos comunitários não executáveis.

## Threat model

Proteger contra:

- perfil/adapter/update malicioso;
- path traversal/symlink/reparse point;
- mapping/pipe precreation;
- PID reuse;
- stale resource handles;
- DLL replacement;
- downgrade;
- mod conflict/data loss;
- crash no meio de install/remove;
- execução em jogo protegido;
- vazamento de paths/logs/dumps;
- privilege escalation.

## SafetyVerdict

Estados:

- `ExternalOnly`;
- `OfflineAdapterAllowed`;
- `ProtectedBlocked`;
- `UnknownNeedsReview`.

Inputs:

- static modules/files;
- loaded modules;
- services/drivers relacionados;
- launcher/store/game mode;
- profile;
- user declaration offline;
- provider policy.

Reavaliar imediatamente antes do launch e depois que módulos do processo carregarem. Detecção tardia encerra adapter/observer e degrada para L1 quando seguro.

## Política anti-cheat

- qualquer detecção conhecida bloqueia in-process;
- desconhecido em jogo multiplayer/protegido também bloqueia;
- não oferecer botão “ignorar” no stable;
- developer build não remove hard block em protected process;
- não mascarar HaumeaVR, alterar driver, spoofar ou contornar integridade;
- Virtual Screen externo pode funcionar quando a captura normal é permitida.

## Transaction Manager

### Prepare

- resolver paths canônicos;
- verificar que estão dentro do install root esperado;
- detectar reparse/symlink;
- listar operações;
- verificar permissões/espaço;
- capturar hashes/ACL/timestamps relevantes;
- detectar conflitos;
- obter consentimento com diff.

### Stage

- copiar novos arquivos para staging no mesmo volume;
- verificar hashes/signatures;
- criar backup imutável local;
- fsync/flush apropriado;
- escrever journal.

### Commit

- atomic replace/rename quando possível;
- nunca sobrescrever arquivo de usuário sem backup;
- atualizar inventory;
- validar instalação.

### Compensate

- reverse order;
- hash-check antes de restaurar;
- preservar arquivo modificado após backup com conflito separado;
- idempotente;
- journal permite recuperação depois de reboot/crash.

## Backup

- content-addressed por SHA-256;
- deduplicado;
- manifest assinado/local MAC;
- game/profile/build/adapter IDs;
- original path relativo;
- tamanho/hash/ACL;
- retention configurável;
- “Restore clean” e “Remove HaumeaVR only” distintos;
- nunca incluir saves sem consentimento específico;
- export opcional, criptografado se contiver dados pessoais.

## Privilege model

- per-user e sem admin por padrão;
- elevation somente para path que exige e em processo broker mínimo;
- UI mostra cada operação elevada;
- token/session não atravessa log;
- broker aceita plano assinado/nonce e paths allowlisted;
- nunca manter processo elevado.

## Binários e assinatura

- official adapter/installer/update signed;
- verify Authenticode + project manifest hash;
- dev key separada e recusada no stable;
- revocation list;
- no DLL search path inseguro;
- use absolute paths e safe DLL directories;
- loader lock/thread constraints.

## OpenXR/runtime

- ler runtimes registrados;
- usar `XR_RUNTIME_JSON` no child;
- nunca escrever `ActiveRuntime`;
- mostrar instrução manual se outro app exigir global;
- cleanup de environment.

## Testes adversariais

- anti-cheat fixture e late load;
- unknown multiplayer;
- path `..\`/UNC/reparse;
- file locked;
- disk full em cada etapa;
- power/crash simulation;
- hash changes after backup;
- rollback twice;
- preexisting mod conflict;
- malicious signed/unsigned manifest;
- PID reuse;
- mapping collision;
- low privilege/elevation cancel;
- uninstall com sessão ativa.

## Critérios de aceite

- Falha em qualquer etapa restaura ou deixa journal recuperável.
- “Remove HaumeaVR” não remove mod do usuário.
- Protected process nunca carrega adapter.
- Stable não possui bypass.
- Runtime global fica byte/registro idêntico.
- Support bundle não contém token/secret.
- Security test suite bloqueia release.

## Primeiros passos do próximo chat

1. Escrever threat model/test matrix no source.
2. Criar TransactionPlan/Journal puros.
3. Implementar backup content-addressed.
4. Testar falha em cada boundary.
5. Integrar primeiro em um lab, nunca MiSide.
