# P42 — Adapter Unity IL2CPP

**Status:** não implementado; bloqueador de MiSide  
**Prioridade:** crítica  
**Dependências:** P14, P40, P61, lab IL2CPP reproduzível  
**Desbloqueia:** P50 e muitos jogos Unity modernos

## Objetivo

Executar a base Unity comum em players IL2CPP offline autorizados sem offsets fixos, com resolução por metadata/API, fingerprint estrito e provider substituível.

## Fatos técnicos

IL2CPP converte assemblies IL em C++, compila código nativo e empacota runtime/metadata. Não existe Mono JIT/assembly managed original carregável como em P41. Stripping, Unity version e código gerado alteram símbolos/layouts.

Por isso:

- Mono e IL2CPP têm bootstrap/resolver diferentes;
- common behavior deve permanecer igual;
- build update invalida cache;
- address/offset nunca entra em perfil comunitário.

## Estratégia de providers

### IL2CPP SourceOwned

Lab/projeto próprio inclui bootstrap e native plugin durante build. É a primeira referência.

### BepInEx 6 IL2CPP Development Provider

Candidato para prototipar jogos offline:

- BepInEx 6 IL2CPP é pre-release na fotografia de planejamento;
- Il2CppInterop gera/usa proxies gerenciados;
- fixar versão/commit e hashes;
- não distribuir no canal stable até license/security/stability review;
- provider separado permite substituir tecnologia.

### Native Metadata Provider

Futuro provider oficial:

- carregado por um mecanismo permitido pelo jogo;
- resolve exports IL2CPP presentes;
- lê registration/metadata por família suportada;
- constrói bindings mínimos;
- evita depender de offsets publicados;
- maior custo de manutenção.

O plano não autoriza remote DLL injection. O mecanismo de carga é específico/permitido e transacional.

## Resolver

Resolução por:

- Unity version family;
- exports `il2cpp_*` quando disponíveis;
- assembly/image/class/method APIs;
- namespace/type/method signature;
- field/property metadata;
- validation de type/parameter count;
- runtime object type checks;
- cache preso ao fingerprint.

Não:

- endereços absolutos;
- pattern scan amplo sem descritor oficial;
- patch de instrução;
- ignorar mismatch.

## Segurança de runtime

- anexar thread ao domain antes de chamadas;
- usar handles/roots corretos para objetos persistentes;
- respeitar main thread Unity;
- exception boundary;
- validar object alive/scene revision;
- nenhuma referência após unload/shutdown;
- rate limit de inspection;
- watchdog do sidecar;
- fail closed.

## Bindings Unity

Criar facade mínima:

- GameObject/Component/Transform;
- Camera;
- Canvas;
- SceneManager;
- RenderTexture/Texture;
- Time/Application;
- input integration selecionada;
- animation/collider apenas via game adapter.

Gerar bindings por versão no build, não dinamicamente em cada frame.

## Render plugin

Mesmo native plugin/P12 de P40:

- D3D11 device;
- render thread event;
- shared textures;
- generation;
- poses/views;
- input.

IL2CPP provider apenas agenda/coordena chamadas common.

## Labs

Matriz inicial:

1. Unity 2021.3.35f1 IL2CPP Built-in D3D11 — replica MiSide;
2. Unity 2022.3 IL2CPP Built-in/URP;
3. Unity 6000.3 IL2CPP;
4. stripping Low/Medium/High;
5. development/non-development;
6. scene/camera/canvas/input/interactable.

Salvar builds controlados como artefatos, não source de terceiros.

## Cache

Local:

- fingerprint de exe/GameAssembly/metadata/UnityPlayer;
- provider version;
- generated proxy/binding version;
- resolved type catalog;
- validation result.

Cache incompatível é descartado. Nunca baixar offsets de perfil.

## Fases

1. Fazer o lab 2021.3.35 reproduzível.
2. SourceOwned IL2CPP provider.
3. P12/P13/common P40.
4. BepInEx development provider no lab.
5. Resolver/fingerprint/cache.
6. Inspection API.
7. install/remove transaction.
8. MiSide read-only probe.
9. MiSide L4-R.
10. production provider decision.

## Testes

- metadata missing/corrupt/stripped;
- wrong Unity version;
- wrong fingerprint;
- method overload/signature;
- object destroyed;
- scene change;
- GC pressure;
- exception;
- provider/core crash;
- install rollback;
- game update;
- anti-cheat/unknown verdict;
- no mods versus existing mod stack.

## Critérios de aceite

- Lab 2021.3.35 IL2CPP produz stereo/input por common.
- Resolver não contém offset absoluto por game.
- Wrong build recusa antes de alterar/iniciar adapter.
- Cache invalida corretamente.
- Install/remove é byte-identical.
- Provider failure deixa jogo abrir limpo/L1.
- MiSide só começa após esses gates.

## Riscos

- **Provider pre-release:** encapsular e manter source-owned reference.
- **Stripping/ofuscação:** adapter específico e fallback.
- **Update quebra binding:** fingerprint hard gate.
- **Licença/distribuição:** P66 antes de bundle.
- **Instabilidade no jogo:** labs, opt-in e rollback.

## Primeiros passos do próximo chat

1. Instalar Unity 2021.3.35f1 somente se autorizado.
2. Construir IL2CPP lab igual ao MiSide.
3. Implementar SourceOwned provider.
4. Fazer spike BepInEx/Il2CppInterop apenas no lab.
5. Documentar decisão e só então tocar MiSide.

## Referências oficiais/primárias

- [Unity: Introduction to IL2CPP](https://docs.unity3d.com/Manual/il2cpp-introduction.html)
- [BepInEx releases](https://github.com/BepInEx/BepInEx/releases)
- [Il2CppInterop](https://github.com/BepInEx/Il2CppInterop)

