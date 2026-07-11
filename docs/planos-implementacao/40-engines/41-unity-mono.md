# P41 — Adapter Unity Mono

**Status:** player controlado funcional; generalização planejada  
**Prioridade:** alta  
**Dependências:** P14, P40, P61  
**Desbloqueia:** L4 em jogos Unity Mono autorizados

## Objetivo

Transformar o bridge Mono controlado em um Official Engine Adapter versionado, com providers de deployment seguros, compatibilidade por família Unity e rollback.

## Estado inicial

- bridge gerenciado existe;
- native render plugin existe;
- launcher verifica presença dos dois;
- somente player Mono controlado é aceito;
- processo, mapping e texturas são autenticados;
- lab source existe;
- não há bootstrap em jogo comercial.

## Providers

### SourceOwned

Para projetos do usuário/desenvolvedor:

- pacote UPM/local;
- bootstrap em `RuntimeInitializeOnLoadMethod`;
- native plugin incluído no build;
- caminho preferido e estável.

### SupportedModLoader

Para jogo offline que permite mods:

- provider separado;
- BepInEx Mono é candidato de desenvolvimento;
- versão e licença revisadas;
- instalação transacional;
- plugin HaumeaVR assinado;
- nenhuma alteração de `Assembly-CSharp.dll`;
- recusar anti-cheat/multiplayer protegido.

### GameSpecific

Mecanismo oficial ou documentado do jogo. O provider fica no game adapter.

Não criar um injector Mono genérico.

## Bootstrap

Responsabilidades:

1. validar opt-in/session token;
2. anexar ao transport;
3. aguardar Unity main thread;
4. criar host persistente;
5. detectar scene/pipeline;
6. iniciar common P40;
7. publicar adapter health;
8. entrar idle se core some;
9. restaurar e descarregar quando possível.

Falha nunca impede o jogo limpo de prosseguir.

## Compatibilidade de assemblies

- build multi-target ou source package compatível com APIs mínimas;
- separar adapters por família quando API muda;
- evitar referência desnecessária a URP/HDRP em base;
- strategies carregadas por reflection segura/assembly detection;
- strong-name não é assinatura de distribuição;
- hash/signature no manifesto P14.

## Mono runtime

- detectar runtime/domain;
- não usar JIT injection própria;
- loader chama plugin pela API suportada;
- tratar domain unload quando aplicável;
- main-thread dispatcher;
- GC allocations medidas;
- exception boundary envia erro redigido.

## Render

- common stereo rig;
- `GL.IssuePluginEvent`/CommandBuffer no ponto correto;
- native plugin identifica D3D11 device;
- resources por generation;
- resize/recreate;
- keyed mutex;
- D3D12 strategy futura.

## Perfil e inspeção

- scene tree por IDs opacos;
- camera/canvas roles;
- action endpoints allowlisted;
- game adapter separado do engine adapter;
- profile overrides não chamam método arbitrário por string.

## Fases

1. Empacotar bridge atual como adapter P14 em SourceOwned.
2. Migrar P12/P13.
3. Labs por versão.
4. Inspection API.
5. Built-in/URP.
6. SupportedModLoader provider em lab externo.
7. Transaction/rollback.
8. primeiro jogo Mono offline com permissão.
9. HDRP/D3D12.

## Testes

- Unity versions/pipelines;
- missing native/managed module;
- wrong adapter version;
- core starts late/exits early;
- scene reload/domain behavior;
- exceptions;
- camera replacement;
- resize;
- input/hands lab;
- install/remove byte-identical;
- preexisting mod loader;
- anti-cheat block.

## Critérios de aceite

- SourceOwned é estável e documentado.
- Adapter manifest/capabilities/fingerprint funcionam.
- SupportedModLoader não sobrescreve assemblies do jogo.
- Remove restaura inventário.
- Mono adapter não contém regras de jogo.
- Pelo menos um jogo Mono offline autorizado atinge L4-R/I antes de claim.

## Primeiros passos do próximo chat

1. Converter bridge atual em manifesto P14 sem mudar deployment.
2. Migrar transport/input.
3. Criar labs 2021.3/2022.3.
4. Implementar SourceOwned completo.
5. Fazer license/security review antes de BepInEx.

## Referência

- [BepInEx — repositório oficial](https://github.com/BepInEx/BepInEx)
- [Unity native plug-ins](https://docs.unity3d.com/Manual/native-plugin-interface.html)
