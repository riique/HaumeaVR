# P30 — Virtual Screen universal

**Status:** funcional em 0.1.0; evolução planejada  
**Prioridade:** alta e permanente  
**Dependências:** P10, P11, P13, P15  
**Desbloqueia:** fallback confiável para todo jogo

## Objetivo

Tornar L1 uma experiência polida, rápida e previsível em Quest/PICO, inclusive quando qualquer adapter falhar. Virtual Screen não é um protótipo descartável; é o modo universal e o caminho de recuperação.

## Estado inicial

- `DesktopCapture` usa DXGI Desktop Duplication.
- A janela selecionada é recortada da imagem do monitor.
- `D3D11Renderer` desenha malha plana/curva em swapchains OpenXR.
- Ajustes de tamanho/distância/curvatura e recenter existem.
- Há self-test de captura em GPU.

## Escopo

- captura robusta por janela e monitor;
- tela plana/curva/cinema;
- resolução, HDR/SDR e color management;
- cursor e pointer;
- baixa latência e frame pacing;
- áudio/foco como integração de produto;
- fallback instantâneo;
- Quest/PICO.

Não inclui profundidade, câmera do jogo ou mãos no gameplay.

## Backends de captura

### DXGI Desktop Duplication

Manter como fallback:

- captura GPU-resident;
- funciona por monitor;
- recorte de janela;
- cursor metadata;
- recovery em mode change/access lost.

Limitações: oclusão, múltiplas janelas, HDR e identificação precisa de janela.

### Windows Graphics Capture

Adicionar como opção preferida por janela quando suportada:

- picker/consentimento quando exigido;
- captura de window item;
- resize;
- closed event;
- frame pool;
- interop D3D11.

Nunca usar técnica oculta para capturar conteúdo protegido. Quando a API retorna preto/protegido, informar e não tentar contornar.

### Seleção

Profile define preferência, mas probe decide:

1. WGC window;
2. Desktop Duplication;
3. monitor/manual;
4. erro explicável.

## Pipeline

1. adquirir frame com timestamp;
2. detectar resize/format/color space;
3. crop/scale em GPU;
4. converter HDR→headset target quando necessário;
5. compor cursor opcional;
6. renderizar malha/camada;
7. aplicar environment/lighting mínimo;
8. submeter no frame OpenXR mais próximo sem bloquear.

Usar último frame apenas dentro de limite; jogo parado pode continuar exibido sem consumir GPU excessiva.

## Modos visuais

- flat;
- curved com arco e subdivisão configuráveis;
- cinema com fundo escuro;
- head-locked temporário para ajuste;
- world-locked;
- passthrough background somente se capability e política permitirem;
- aspect fit/fill/native pixels;
- supersampling controlado.

## Input

- menu HaumeaVR por controles/mãos;
- pointer opcional para mouse do jogo somente com consentimento;
- click/scroll/keyboard mappings foreground-only;
- botão de emergência nunca enviado ao jogo;
- capturar/liberar cursor e foco de forma transacional;
- gamepad continua direto no jogo quando usuário escolher.

## Áudio

Fase opcional:

- deixar jogo usar dispositivo padrão inicialmente;
- permitir seleção de áudio do PC/Quest/PICO sem instalar driver;
- não capturar áudio protegido;
- diagnóstico de device change;
- sincronização apenas por métricas, sem prometer lip-sync perfeito.

## Configuração por perfil

- capture backend;
- window matching strategy;
- crop/aspect;
- screen width/distance/height;
- curvature;
- environment;
- cursor/input;
- refresh/quality;
- fallback behavior.

## Implementação

1. Congelar Desktop Duplication em testes.
2. Abstrair `ICaptureSource`.
3. Implementar WGC source.
4. GPU crop/scale/color converter comum.
5. resize/HDR/device-loss.
6. integrar input focus P13.
7. fallback pelo P11.
8. UI desktop/VR.
9. testes Quest/PICO.

## Testes

- windowed/borderless/fullscreen;
- janela muda monitor/DPI/tamanho;
- monitor desconecta;
- minimize/restore;
- game alt-tab;
- HDR desktop/jogo;
- protected content;
- cursor;
- 30/60/120+ fps game versus 72/80/90/120 headset;
- Quest USB/Air Link e PICO/SteamVR;
- fallback de adapter em sessão;
- 2 horas sem leak.

## Métricas

- capture-to-submit latency;
- frames adquiridos/repetidos/descartados;
- GPU copy/scale time;
- resize/recovery count;
- VRAM;
- OpenXR frame misses.

## Critérios de aceite

- qualquer jogo capturável selecionado abre sem módulo in-process.
- Resize/move/alt-tab recuperam sem reiniciar HaumeaVR.
- Protected content é recusado sem bypass.
- Overhead atende P35.
- Fallback L4/L2→L1 ocorre com mensagem e sem fechar jogo.
- UI deixa claro que cabeça move a tela, não a câmera do jogo.

## Primeiros passos do próximo chat

1. Medir captura atual e adicionar testes de `ICaptureSource`.
2. Extrair Desktop Duplication sem regressão.
3. Criar spike WGC D3D11 em app de teste.
4. Integrar seleção/fallback.
5. Só depois adicionar HDR/input.

## Referências oficiais

- [Desktop Duplication API](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/desktop-dup-api)
- [Windows Graphics Capture](https://learn.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture)
