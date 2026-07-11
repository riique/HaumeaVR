# P02 — Contratos arquiteturais globais

**Status:** normativo  
**Prioridade:** bloqueador de arquitetura  
**Dependências:** P00, P01  
**Desbloqueia:** P10–P67

## Objetivo

Fixar fronteiras, formatos e invariantes que permitem desenvolver módulos em chats separados sem criar sessões OpenXR concorrentes, protocolos incompatíveis ou caminhos inseguros.

## Componentes e propriedade

| Componente | Processo | Responsabilidade exclusiva |
| --- | --- | --- |
| Launcher/Profile Studio | HaumeaVR desktop | descoberta, configuração, transações, UX e orquestração |
| Core | `uxrb_core.exe` | sessão OpenXR, timing, poses, swapchains, composição e menu VR |
| Adapter host | processo controlado do jogo ou sidecar | traduzir conceitos da engine/jogo para contratos HaumeaVR |
| Graphics observer | processo do jogo, opt-in | observar recursos/pipeline da API suportada |
| Profile repository | launcher | persistência, migração, assinatura e seleção por fingerprint |
| Updater | processo separado e assinado | atualização atômica quando HaumeaVR não estiver em uso |

O core é sempre o único dono de `XrInstance`/`XrSession` da sessão HaumeaVR. Um adaptador recebe poses e produz frames; não cria sessão concorrente.

## Plano de controle versus plano de dados

### Controle

Named pipe autenticado e local:

- comandos de iniciar/parar/recenter/configurar;
- status, warnings, fallback e erros;
- transações de adapter;
- payloads JSON validados por schema;
- mensagens raras, não uma pose por frame.

### Dados de alta frequência

Memória compartilhada versionada e recursos GPU:

- frame state previsto;
- views e FOV;
- controles e hand joints;
- descritores de textura/depth/motion vectors;
- métricas por frame;
- solicitações de háptica em ring buffer limitado.

Nenhuma textura percorre JSON ou RAM quando existir interop GPU compatível.

## Contrato de capacidades

Todo início de sessão produz um `CapabilitySnapshot` imutável com:

- identidade e versão do runtime;
- nome do sistema/headset;
- API OpenXR negociada;
- extensões disponíveis e habilitadas;
- view configurations, dimensões, samples e formatos;
- blend modes e reference spaces;
- interaction profiles ativos por mão;
- orientação/posição;
- controller pose/action/haptic;
- hand tracking, eye gaze e recursos opcionais;
- backend gráfico e LUID/adapter;
- rota de conexão declarada pelo usuário e inferida pelo diagnóstico;
- timestamp e hashes dos manifests.

Código de recurso consulta esse snapshot. Marca/modelo serve para exibição e regras de workaround versionadas, nunca como substituto da enumeração.

## Contrato de frame

O core publica, por frame:

- `frameId` monotônico;
- `predictedDisplayTime` em unidade OpenXR/representação transportável;
- `shouldRender`;
- base space e transform de recenter;
- pose e FOV por view;
- dimensão recomendada/ativa;
- poses grip/aim e estados semânticos;
- joints + flags de validade por mão;
- deadline estimado para o produtor.

O produtor devolve:

- mesmo `frameId` ou razão explícita de drop;
- generation id do conjunto de recursos;
- color left/right;
- depth left/right opcional;
- motion vectors opcionais;
- metadata de cor, alpha, origem, array slice e layout;
- timestamps CPU/GPU e status.

Frames atrasados nunca podem bloquear indefinidamente o loop OpenXR. O core usa último frame seguro, fallback visual ou camada de erro.

## ABI de adaptador

Todo adapter oficial possui:

- manifest JSON assinado;
- ID e versão semântica;
- engines/builds/arquiteturas suportadas;
- hash do binário;
- versão mínima/máxima do host;
- capacidades fornecidas;
- permissões necessárias;
- entry points C estáveis, sem tipos STL atravessando DLL;
- `GetApiVersion`, `Probe`, `Start`, `Tick`, `ApplyConfig`, `Stop` e `GetDiagnostics`;
- lifecycle idempotente e timeouts;
- fail-closed em mismatch.

Adapters gerenciados usam um shim C estável ou mensagens versionadas; não acoplar o core ao CLR/Mono/IL2CPP.

## Perfis

Um perfil é declarativo e separado em:

1. identificação/fingerprint;
2. capacidades verificadas;
3. render/câmera/HUD;
4. ações semânticas;
5. grafo de interação;
6. overrides por runtime/headset;
7. segurança e fallback;
8. proveniência/assinatura.

Perfis não contêm shell command, endereço de memória absoluto, byte patch, DLL embutida ou script arbitrário. Resolução de símbolos fica em adapter oficial e versionado.

## Seleção e fallback

Ordem padrão:

1. L4 compatível com fingerprint exato;
2. L3 compatível com API/build e perfil validado;
3. L2 com depth comprovado;
4. L1.

Cada etapa executa preflight. Em falha:

- registrar código estruturado;
- parar somente o módulo;
- restaurar estado;
- tentar o próximo modo permitido;
- explicar a mudança no desktop e no headset.

O jogo deve continuar normal quando for seguro; se houve alteração incompleta, parar antes de abrir e oferecer rollback.

## Segurança invariável

- conexão IPC local e autenticada;
- nomes de objetos aleatórios, ACLs mínimas e PID esperado;
- validação de tamanho, formato, geração e owner de todo recurso;
- anti-cheat detectado ou desconhecido bloqueia adapter/in-process por padrão;
- multiplayer protegido não recebe exceção de perfil;
- nenhum driver, serviço persistente ou injeção genérica;
- binários oficiais assinados e hashes conferidos;
- update metadata assinada e proteção contra downgrade;
- backup transacional antes de tocar na pasta do jogo;
- botão “restaurar jogo” independente do core.

## Estrutura de diretórios alvo

```text
core/
  openxr/              capability, session, input, composition
  runtime/             orchestration and fallback
shared/
  protocol_v2/
  adapter_api/
  diagnostics/
graphics/
  d3d11/
  d3d12/
  vulkan/
engines/
  unity/
  unreal/
  redengine/
apps/launcher/
  Features/
    Library/
    Runtime/
    ProfileStudio/
    Sessions/
    Diagnostics/
distribution/
  installer/
  update/
```

A migração deve ser incremental; não mover tudo antes de existir teste protegendo o comportamento.

## Compatibilidade de versão

- Major diferente: recusar.
- Minor novo no produtor: aceitar somente campos conhecidos se `structSize` permitir.
- Toda struct binária começa com `type`, `version` e `size`.
- Reservados devem ser zero e ignorados de forma definida.
- Schema JSON usa `schemaVersion` e migradores determinísticos.
- Adapter declara intervalo de host e protocolo.

## Testes contratuais obrigatórios

- golden vectors para framing e schemas;
- producer/consumer com minor versions distintas;
- struct truncada, tamanho gigante, geração antiga e PID incorreto;
- queda do adapter no meio do frame;
- runtime loss e headset sleep;
- texture format inválido e timeout de mutex/fence;
- fallback sem encerrar o jogo;
- rollback depois de falha simulada em cada passo;
- carga de perfil não assinado/corrompido recusada.

## Critérios de aceite

- P12, P14 e P60 apontam para os mesmos tipos e regras.
- Não há segundo owner de OpenXR.
- Não há caminho de alta frequência por JSON.
- Não há perfil comunitário executável.
- Falhas têm códigos e fallback previsíveis.
- Os testes contratuais executam sem headset.

## Primeiros passos para o chat executor

1. Revalidar os contratos contra os headers atuais antes de criar v2.
2. Criar um ADR quando uma decisão precisar mudar; não editar silenciosamente a invariável.
3. Implementar tipos compartilhados e testes antes dos consumers.
4. Manter v1 funcional até que launcher, core e lab tenham migrado.
