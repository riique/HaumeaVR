# P45 — Engine genérica ou desconhecida

**Status:** detector parcial; pipeline planejado  
**Prioridade:** alta para escala  
**Dependências:** P11, P30–P34, P60–P63  
**Desbloqueia:** P53 e cobertura além das engines principais

## Objetivo

Criar um onboarding seguro que determine o melhor nível possível para um executável mesmo quando a engine não é conhecida, sem lançar instrumentação invasiva como primeira tentativa.

## Pipeline

### 1. Identidade

- storefront/AppID;
- executable/architecture/hash/signature;
- version/build;
- install dir;
- redistributables;
- online/multiplayer status informado.

### 2. Static signals

- DLLs/filenames;
- PE imports;
- version resources;
- Unity/Unreal/Godot/CryEngine/custom markers;
- D3D11/D3D12/Vulkan/OpenGL;
- anti-cheat modules/services/config;
- mod support docs.

Não decompilar nem extrair asset protegido como análise padrão.

### 3. External runtime observation

- process/window lifecycle;
- graphics API via loaded modules/ETW permitido;
- swapchain/window properties;
- capture test;
- runtime performance.

### 4. Safety verdict

- ExternalOnly;
- OfflineAdapterAllowed;
- ProtectedBlocked;
- UnknownNeedsReview.

Unknown não vira “safe”.

### 5. Mode recommendation

- L1 if capturable;
- L2 only if supported observer/provider and depth probe;
- L3 only with validated profile;
- L4 only with engine/game adapter.

## Detection report

JSON + UI:

- evidence com confidence;
- conflicts;
- capability recommendation;
- required plans;
- risks;
- exact next probe;
- files read;
- nenhuma alteração feita.

## Plugin architecture

Detectors separados e puros:

- `IEngineDetector`;
- `IGraphicsApiDetector`;
- `IAntiCheatDetector`;
- `IModSupportDetector`;
- `IStoreMetadataProvider`.

Resultado combina evidence; detector não inicia processo nem instala módulo.

## Heurísticas

- cada signal tem weight/source;
- “UnityPlayer.dll” forte; nome de pasta fraco;
- conflicting signals resultam Unknown;
- version detection nunca inventa;
- hashes são locais;
- detector update é assinado.

## Testes

Fixtures de:

- Unity Mono/IL2CPP;
- Unreal UE4/UE5;
- REDengine;
- custom D3D11/D3D12;
- Vulkan;
- x86/x64;
- anti-cheat;
- false positives;
- missing/corrupt install;
- renamed exe;
- store library unavailable.

## Critérios de aceite

- Análise é read-only.
- Report explica cada conclusão.
- Unknown não ativa module in-process.
- L1 funciona sem engine.
- Perfil é preso a fingerprint.
- Detector não envia paths/hashes remotamente.
- Onboarding P53 consome report sem código especial.

## Primeiros passos do próximo chat

1. Extrair detector atual para evidence model.
2. Criar fixtures.
3. Adicionar PE/import/version readers limitados.
4. Integrar safety verdict.
5. Gerar report e UI.

