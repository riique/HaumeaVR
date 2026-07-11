# P15 — Menu VR, overlay e controle durante a sessão

**Status:** planejado  
**Prioridade:** alta  
**Dependências:** P10, P11, P13  
**Desbloqueia:** experiência utilizável sem retirar o headset

## Objetivo

Criar a parte da interface própria que vive dentro do headset, é renderizada pelo core e continua disponível independentemente da engine do jogo.

## Decisão arquitetural

- O menu pertence ao core e à mesma sessão OpenXR.
- Preferir `XrCompositionLayerQuad` com textura própria.
- Se layer budget/alpha não forem adequados, compor o painel na projection layer.
- Não abrir uma segunda sessão, não depender de overlay SteamVR e não renderizar UI dentro do jogo.
- Reutilizar uma biblioteca de UI 2D madura após spike e license review; não construir um toolkit completo do zero. Planejar RmlUi como primeira candidata e manter uma tela de emergência mínima em Direct2D/DirectWrite.

Na fotografia de julho de 2026, o repositório oficial do RmlUi fornece renderers de referência para DirectX 12, Vulkan e OpenGL, mas não um renderer D3D11 pronto. Como o core atual é D3D11, o spike precisa comparar objetivamente: implementar uma interface de render D3D11 pequena para RmlUi, usar Direct2D/DirectWrite no primeiro menu, ou aguardar o backend D3D12. Não introduzir D3D12 no core apenas para desenhar a UI.

## Conteúdo

### Quick panel

- continuar/fechar menu;
- recenter;
- tamanho/distância/altura;
- render scale;
- snap/smooth turn;
- modo atual e fallback;
- controller/hand status;
- performance compacta;
- parar HaumeaVR sem necessariamente fechar o jogo.

### Session panel

- jogo/build;
- runtime/headset/connection;
- modo L1–L4 e subcapabilities;
- adapter;
- warnings;
- último fallback;
- actions/bindings;
- HUD/câmera/conforto.

### Diagnostics

- frame time CPU/GPU;
- missed frames;
- producer latency;
- depth confidence;
- tracking confidence;
- resource generation;
- botão para marcar evento no log;
- exportação fica no desktop, não exige teclado VR.

### Safety

- “voltar para Virtual Screen”;
- “desativar adapter e manter jogo”;
- “encerrar sessão”;
- status de backup/rollback;
- confirmação clara para ações destrutivas.

## Input e foco

- long press configurável abre menu; não usar botão reservado do sistema;
- controller ray + select;
- hand pointer/pinch somente quando P13 comprovar;
- gaze fallback opcional, nunca obrigatório;
- navegação por thumbstick/d-pad;
- foco modal suspende gameplay semantic actions;
- release de foco em crash/fechamento;
- cursor e hover mostram tracking confidence.

## Rendering

- textura sRGB/alpha validada pelo runtime;
- resolução adaptativa com mínimo de legibilidade;
- atualização visual desacoplada quando painel está estático;
- poses world/head locked selecionáveis;
- painel a distância confortável e com curvatura opcional;
- evitar texto pequeno e transparência excessiva;
- emergency overlay deve funcionar mesmo se UI principal falhar.

## Acessibilidade

- escala 100–200%;
- alto contraste;
- redução de movimento;
- handedness;
- operação por um controle;
- alternativa a cor para estado;
- strings localizáveis PT-BR/EN;
- foco visível;
- confirmação por hold em emergência;
- áudio opcional, sem bloquear.

## Estrutura

```text
core/ui/
  VrMenuController
  VrUiModel
  IVrUiRenderer
  RmlUiRenderer (candidate)
  EmergencyPanelRenderer
  UiInputRouter
  QuadLayerPresenter
shared/protocol/
  menu-state schema
apps/launcher/
  shared localization/resources
```

O modelo de UI recebe snapshots; não chama diretamente OpenXR ou adapters.

## Fases

1. Quad layer com checkerboard/test panel.
2. Pointer controller e foco.
3. Emergency panel.
4. Quick panel ligado a recenter/screen settings.
5. Session state de P11.
6. performance/diagnostics.
7. hand pointer.
8. localization/accessibility.
9. visual polish e testes longos.

## Testes

- runtime com/sem alpha blend adequado;
- layer budget baixo;
- foco não vaza input;
- abrir/fechar repetidamente;
- controller desconecta;
- tracking de mão some;
- runtime sleep/wake;
- adapter falha com menu aberto;
- render scale e DPI;
- Quest e PICO;
- captura visual e legibilidade em resoluções recomendadas distintas.

## Critérios de aceite

- usuário inicia, ajusta, degrada e encerra sessão sem tirar headset.
- Menu funciona em Virtual Screen e engine native.
- Falha da UI principal ainda deixa emergency stop/recenter.
- Abrir menu não aciona ações no jogo.
- Não existe dependência SteamVR overlay/vendor.
- Frame overhead do menu aberto/fechado é medido e atende P35.

## Primeiros passos do próximo chat

1. Verificar `maxLayerCount` e formatos no capability snapshot.
2. Criar quad layer estática no core.
3. Implementar emergency panel antes da biblioteca completa.
4. Ligar somente recenter/stop.
5. Adicionar input focus e testes; depois expandir UI.

## Referências

- [OpenXR composition layers](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html)
- [RmlUi — repositório oficial](https://github.com/mikke89/RmlUi)
