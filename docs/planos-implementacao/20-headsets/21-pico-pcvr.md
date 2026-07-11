# P21 — Compatibilidade PICO em PCVR

**Status:** planejado; hardware pendente  
**Prioridade:** crítica  
**Dependências:** P10, P13 inicial, P62 harness  
**Desbloqueia:** suporte declarado a PICO

## Objetivo

Suportar PICO no produto Windows pela rota PCVR que o software oficial realmente oferece, mantendo o core OpenXR cross-vendor.

## Decisão de runtime

A página oficial do PICO Connect descreve streaming de jogos SteamVR. Portanto, a rota inicial é:

`PICO → PICO Connect → SteamVR → runtime OpenXR SteamVR → HaumeaVR`

Alternativa oficial:

`PICO → Steam Link → SteamVR → runtime OpenXR SteamVR → HaumeaVR`

Não assumir que o PICO Connect instala um runtime OpenXR Windows independente. Se uma versão futura expuser um manifest próprio, ele entra como nova rota depois de probe e teste, não por nome.

## Headsets alvo

Ordem sugerida:

1. PICO 4 Ultra;
2. PICO 4;
3. PICO Neo3 Link;
4. PICO 4 Pro/Neo3 variantes quando acessíveis.

A lista oficial de dispositivos e requisitos muda. Reconsultar PICO Connect/Store antes de cada release.

## Rotas físicas

- wireless PICO Connect;
- wired USB streaming quando oferecido pela versão/dispositivo;
- Neo3 Link por DisplayPort quando disponível;
- Steam Link wireless.

Cada uma é uma linha separada na matriz. DisplayPort não deve herdar resultado de streaming comprimido.

## Preflight do launcher

1. Detectar Steam e SteamVR.
2. Localizar manifest OpenXR do SteamVR e validar hash.
3. Não tornar SteamVR global; passar manifest ao core.
4. Detectar PICO Connect/Steam Link somente para orientação.
5. Confirmar que o headset está conectado ao SteamVR.
6. Probe OpenXR/D3D11.
7. Registrar system name, runtime version e interaction profiles.
8. Distinguir:
   - SteamVR ausente;
   - runtime manifest ausente;
   - PICO driver/streaming ausente;
   - headset não visto pelo SteamVR;
   - compositor ocupado;
   - GPU/driver não compatível.

## Recursos obrigatórios

- primary stereo;
- position/orientation;
- D3D11 binding;
- grip/aim;
- trigger/squeeze/thumbstick/botões;
- haptic;
- local/stage reference space;
- projection/quad layer.

O SteamVR pode mapear controladores PICO para profiles diferentes. Usar o profile retornado pelo runtime e generic semantic actions; não presumir path vendor.

## Mãos

PICO OS e a página do app podem citar hand tracking, mas isso não prova que PICO Connect/SteamVR entregue `XR_EXT_hand_tracking` a uma aplicação PC. O teste é:

1. enumerar extensão no runtime SteamVR;
2. consultar system property;
3. tentar tracker;
4. validar joints no frame;
5. repetir por rota;
6. registrar profile/source;
7. usar controles quando ausente.

Não criar uma ponte proprietária PICO-Android→PC nesta fase. Isso seria outro subproduto de streaming/input e exigiria aplicativo no headset.

## Bindings

- generic controller quando suportado;
- simple controller fallback;
- profiles relatados pelo SteamVR;
- tabela de nomes somente para diagnóstico;
- ações semânticas P13;
- offsets de grip por profile em calibration, não no código do jogo.

## Fases de validação

### P1 — SteamVR/OpenXR

- manifest por processo;
- system/adapter/views/formats;
- session state;
- sleep/wake;
- SteamVR restart.

### P2 — Visual

- cena de teste;
- Virtual Screen;
- menu quad;
- mirror;
- 30 minutos.

### P3 — Controles

- poses;
- todos os eixos/botões;
- háptica;
- menu focus;
- recenter.

### P4 — Rotas

- PICO Connect wireless;
- USB/DP quando aplicável;
- Steam Link;
- medir cada uma separadamente.

### P5 — Mãos opcionais

- capability/probe;
- skeleton;
- gestures;
- controller fallback.

### P6 — Engine lab

- Unity lab;
- transporte v2;
- 60 minutos;
- performance.

## Diagnóstico específico

Registrar sem dados pessoais:

- PICO OS/device;
- PICO Connect ou Steam Link version;
- SteamVR build/runtime version;
- conexão;
- driver names do runtime;
- interaction profile por mão;
- render resolution/refresh;
- network stats quando disponíveis;
- OpenXR extension list.

## Critérios de aceite

- pelo menos PICO 4/4 Ultra passa visual+controllers.
- Uma segunda variante/rota passa smoke test.
- SteamVR é selecionado apenas no processo.
- Rotas são diferenciadas na UI/matriz.
- PICO sem hand extension continua totalmente utilizável por controles.
- Falha/restart de SteamVR não deixa handles/processos presos.
- Um Unity lab completa sessão.

## Riscos

- **Dupla camada streaming+SteamVR:** medir e otimizar, não atribuir tudo ao core.
- **Profile de controle varia:** semantic mapping e diagnóstico real.
- **Documentação consumer não garante OpenXR direto:** manter SteamVR como contrato inicial.
- **Mãos standalone apenas:** fallback obrigatório.

## Primeiros passos do próximo chat

1. Instalar/atualizar PICO Connect e SteamVR somente com autorização.
2. Selecionar o manifest SteamVR por processo.
3. Fechar P1/P2 com um PICO conectado.
4. Capturar profiles de controles reais.
5. Só então investigar hand extension.

## Referências oficiais

- [PICO Connect](https://www.picoxr.com/software/pico-link)
- [PICO Connect na PICO Store](https://store-global.picoxr.com/global/detail/1/7257472870996230207)
- [Steam Link para Meta, HTC e PICO](https://help.steampowered.com/en/faqs/view/0E2C-406B-9135-38A4)
- [OpenXR Loader](https://registry.khronos.org/OpenXR/specs/1.1/loader.html)
