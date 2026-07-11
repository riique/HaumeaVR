# P60 — Perfis v2, schema, layering e repositório

**Status:** v1 funcional; v2 planejado  
**Prioridade:** crítica  
**Dependências:** P00–P02  
**Desbloqueia:** P11, P14, P16, comunidade e jogos

## Objetivo

Evoluir o profile schema v1 de configuração básica para um modelo versionado que representa build, capabilities, adapters, níveis L1–L4, interações, hardware overrides, proveniência e fallback sem incluir código.

## Estado inicial

`profiles/schema/profile.schema.json` v1 contém:

- executable/path/hash/args;
- detection;
- render preferred/fallback;
- camera;
- HUD;
- input bindings simples;
- safety.

SQLite persiste perfis por executable. O schema ainda não distingue build, runtime/headset, adapter capability, mãos/interações, assinatura ou merge.

## Separação de dados

### GameDefinition

Compartilhável:

- store IDs;
- executable match;
- engine/backend;
- display metadata sem assets protegidos.

### BuildFingerprint

- executable SHA-256/version/size;
- key module hashes;
- engine version;
- store build ID opcional;
- fingerprint algorithm version;
- confidence;
- created/tested date.

Paths absolutos ficam no `LocalInstallation`, não no perfil publicado.

### CompatibilityProfile

- supported levels/subcapabilities;
- render/camera/HUD/input/comfort;
- adapter requirements;
- interaction graph;
- fallback;
- known issues;
- test evidence references;
- status.

### DeviceOverride

- match por runtime/system/interaction profile/capability;
- render scale/grip offsets/workaround;
- nunca ativa extensão ausente.

### UserOverride

- altura/handedness/comfort;
- bindings pessoais;
- local e privado.

## Layering determinístico

Ordem:

1. defaults do app;
2. official game profile;
3. approved community profile/patch;
4. device/runtime override;
5. user override;
6. session temporary override.

Merge:

- objetos por campos allowlisted;
- arrays por semântica explícita, não concatenação genérica;
- safety usa regra mais restritiva;
- adapter/fingerprint não podem ser relaxados por comunidade/usuário;
- diff final visível;
- origem preservada por campo.

## Schema v2

Seções:

- `schemaVersion`;
- `profileId/version`;
- `game`;
- `builds[]`;
- `capabilityDeclaration`;
- `rendering`;
- `cameraContexts[]`;
- `hudLayers[]`;
- `semanticInput`;
- `handRepresentation`;
- `interactionBindings[]`;
- `haptics`;
- `comfort`;
- `runtimeOverrides[]`;
- `adapterRequirements[]`;
- `fallbackPolicy`;
- `safety`;
- `testCoverage`;
- `knownIssues`;
- `provenance/signature`.

## CapabilityDeclaration

Separar:

- visual level;
- head tracking;
- controller;
- optical hands;
- represented hands;
- interaction categories;
- haptic;
- HUD;
- cutscene;
- status/quality.

Cada claim aponta para build/runtimes/evidence IDs.

## InteractionBinding

Campos declarativos:

- binding ID/version;
- adapter endpoint ID allowlisted;
- context predicate limitado;
- source semantic action/gesture;
- hand;
- transforms/deadzone/hysteresis/cooldown;
- target semantic endpoint;
- feedback/haptic;
- fallback action;
- test cases;
- enabled.

Proibido:

- memory address;
- arbitrary method name;
- script/shell;
- DLL/base64;
- path traversal;
- network URL executável.

## Persistência

SQLite tables:

- games;
- installations;
- builds;
- profiles;
- profile_versions;
- overrides;
- adapter_inventory;
- evidence_refs;
- backups;
- sessions.

Migrations transacionais, backups da DB e constraints. JSON canônico é source de export; SQLite armazena índices/versões.

## Assinatura

- canonical JSON;
- publisher/key ID;
- signature detached ou envelope;
- hash de assets declarativos;
- expiry/revocation;
- user local profiles podem ser unsigned, marcados Local e nunca redistribuídos automaticamente;
- stable recusa package público unsigned.

## Migração v1→v2

1. backup DB;
2. converter executable para installation/build se hash existe;
3. mapear render/camera/HUD/input;
4. capability L1 por padrão, L4 apenas se adapter ready;
5. preservar user values;
6. validar;
7. manter v1 read-only export por uma release;
8. rollback em qualquer falha.

## Tooling

- validator CLI;
- canonicalizer;
- diff/merge explain;
- migrator;
- profile lint;
- signature verify;
- evidence claim check;
- editor P16.

## Testes

- valid/invalid schemas;
- unknown fields;
- v1 fixtures;
- merge conflicts;
- safety monotonicity;
- signature/canonicalization;
- corrupted DB;
- interrupted migration;
- path portability;
- malicious interaction binding;
- build mismatch;
- update/downgrade.

## Critérios de aceite

- V1 migra sem perda e com backup.
- Paths pessoais não entram em export.
- Claim L4 requer adapter/fingerprint/evidence.
- Community cannot relax safety.
- Merge é determinístico e explicável.
- Profile Studio não permite código arbitrário.
- MiSide L1 profile e labs usam v2.

## Primeiros passos do próximo chat

1. Criar exemplos JSON v2 antes do schema.
2. Modelar MiSide L1 e Unity lab.
3. Escrever schema/testes.
4. Implementar canonical merge.
5. Só então migrar SQLite/v1.

