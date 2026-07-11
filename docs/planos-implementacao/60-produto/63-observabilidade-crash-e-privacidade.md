# P63 — Observabilidade, crash diagnostics e privacidade

**Status:** logs básicos existentes; expansão planejada  
**Prioridade:** alta  
**Dependências:** P10–P12, P60–P61  
**Desbloqueia:** suporte, performance, updater/release

## Objetivo

Gerar diagnóstico suficiente para resolver runtime/headset/adapter/game sem capturar conteúdo sensível por padrão ou inundar disco com eventos por frame.

## Event model

Campos comuns:

- schema/app/component version;
- timestamp monotonic + wall UTC;
- session ID aleatório;
- component/process role;
- category;
- severity;
- event code;
- state/mode;
- correlation/request/frame range;
- redacted properties;
- remediation code.

Categorias:

- startup/config;
- runtime/OpenXR;
- session/fallback;
- graphics/resource;
- capture;
- adapter/engine/game;
- input/hand/haptic;
- profile/transaction;
- updater/security;
- performance;
- crash.

## Níveis

- Error: sessão/recurso falhou;
- Warning: degradação/risco;
- Info: lifecycle/decisão;
- Debug: diagnóstico ativado;
- Trace: dev e bounded, nunca default.

Não registrar uma linha por pose/frame. Usar counters, histograms e sampled windows.

## Redaction

Por padrão remover/tokenizar:

- username;
- machine name;
- home/install absolute paths;
- Steam/Meta/PICO account IDs;
- headset serial;
- IP/MAC/SSID;
- command line secrets;
- IPC token/nonces;
- named object completo quando contém random secret;
- screenshots/game frames;
- save contents;
- dump memory.

Hashes de executable podem ser sensíveis; ficam localmente e entram em bundle somente com consentimento.

## Logs locais

Diretório:

`%LOCALAPPDATA%\HaumeaVR\logs`

P05 define a leitura/migração da pasta legada `%LOCALAPPDATA%\UniversalXRBridge`.

Política:

- arquivos por component/session;
- rotação por tamanho/idade;
- limite total;
- UTF-8/JSON Lines para estruturado;
- human summary separado;
- ACL do usuário;
- cleanup na UI;
- clock anomalies handled.

## Métricas

- ring buffer in-memory;
- percentis/histogram;
- export summary;
- frameId/pose age/drops;
- CPU/GPU/VRAM;
- transport heartbeat;
- interaction profile/tracking confidence;
- fallback counts.

P35 define thresholds.

## Crash

### Processos HaumeaVR

- unhandled exception handlers seguros;
- minidump configurável;
- default: dump mínimo sem full memory;
- dump acompanhado de metadata redigida;
- crash loop detection;
- next launch recovery/journal;
- UI permite visualizar/remover/enviar.

### Game/adapter

- HaumeaVR não assume culpa automaticamente;
- registrar last heartbeat/state;
- não capturar dump do jogo sem consentimento e direito;
- support bundle separa crash HaumeaVR de game exit;
- adapter exception boundary.

## Support bundle

Conteúdo selecionável:

- app/OS/GPU versions;
- capability snapshots;
- profile sanitized;
- session journal;
- logs;
- metrics;
- transaction inventory;
- adapter manifest;
- crash dump opcional;
- matrix evidence.

Antes de gerar:

- preview;
- redaction scan;
- size;
- checkbox de hashes/paths/dump;
- ZIP local;
- não enviar automaticamente.

## Telemetria remota

Fora do MVP. Se adicionada:

- opt-in explícito;
- evento agregado mínimo;
- endpoint/privacy policy;
- delete/disable;
- sem game frames, paths, IDs de conta;
- configuração nunca muda safety;
- funcionamento offline completo.

## Diagnóstico na UI

- mensagem humana + event code;
- “copiar resumo” redigido;
- source component;
- remediation;
- timeline de fallback;
- runtime/headset capability;
- export.

## Testes

- secrets fixture;
- path/account/SSID redaction;
- log rotation/disk full;
- high event rate;
- crash core/launcher/adapter;
- support preview;
- corrupt log;
- clock shift;
- dump consent;
- no network when telemetry off.

## Critérios de aceite

- Erro de runtime/adapter é diagnosticável por code.
- Nenhum token aparece em logs.
- Support bundle passa redaction tests.
- Crash recovery integra P11/P61.
- Logs não crescem sem limite.
- Produto funciona totalmente offline.

## Primeiros passos do próximo chat

1. Definir event code registry/schema.
2. Adaptar logger nativo e C#.
3. Implementar redaction tests.
4. Criar session summary.
5. Adicionar support bundle sem dump; dumps depois.
