# P34 — Backend Vulkan no Windows

**Status:** futuro; não implementado  
**Prioridade:** média após D3D12  
**Dependências:** P10 abstraction, P12, P35, P61  
**Desbloqueia:** engines/jogos Vulkan selecionados

## Objetivo

Adicionar um backend OpenXR Vulkan e transporte de recursos externo no Windows sem instalar implicit layers globais nem afetar jogos fora da sessão HaumeaVR.

## Escopo

- `XR_KHR_vulkan_enable2` preferencial;
- instance/device/queue recomendados pelo runtime;
- swapchains Vulkan;
- compositor/menu/mirror;
- external memory/semaphore Win32;
- lab producer/consumer;
- explicit layer por processo para observação experimental;
- Depth foundation.

## Core Vulkan

1. Enumerar extensão OpenXR e requirements.
2. Usar funções OpenXR para requisitos/extensions de instance/device.
3. selecionar physical device indicado;
4. criar queues necessárias;
5. criar session binding;
6. gerenciar images/layouts/render passes ou dynamic rendering;
7. sincronizar acquire/render/release;
8. tratar device lost.

Não assumir extensions Vulkan só porque o driver as expõe; respeitar interseção com runtime.

## Transporte

P12 Vulkan descriptor:

- external memory Win32 handle;
- allocation size/type;
- image create info relevante;
- format/extent/mips/layers/samples;
- tiling/usage;
- current/expected layout;
- external semaphore handle e timeline/value quando suportado;
- queue family ownership;
- generation/lifetime.

Validar compatibilidade de external handle type antes de importar.

## Observação segura

Preferir:

1. adapter/plugin da engine;
2. explicit Vulkan layer habilitada apenas no ambiente do processo lançado;
3. external-only fallback.

Não registrar implicit layer machine-wide. O launcher configura `VK_LAYER_PATH`/`VK_INSTANCE_LAYERS` apenas no child autorizado e remove no fim. Manifest/layer assinados e fingerprintados.

## Layer experimental

Interceptar inicialmente só metadata:

- instance/device/swapchain;
- queue present/submit;
- image/view creation;
- render pass/dynamic rendering;
- depth attachments;
- barriers/layouts.

Color/depth copy exige ponto seguro, layout transitions e queue ownership. Restaurar exatamente o layout e sincronização. Unsupported/protected images causam fallback.

## Fases

1. Vulkan OpenXR test scene.
2. resource sharing two-process lab.
3. menu/Virtual Screen parity.
4. explicit layer metadata.
5. color/depth lab.
6. Depth P31.
7. Unity/Unreal lab Vulkan.
8. jogos offline selecionados.

## Testes

- validation layers em dev;
- runtime extension mismatch;
- physical device mismatch;
- external memory unsupported;
- binary/timeline semaphore;
- layout/queue family;
- swapchain resize/out-of-date;
- device lost;
- layer environment não vaza ao sistema;
- Quest/PICO via runtimes testados;
- AMD/NVIDIA/Intel.

## Critérios de aceite

- Core Vulkan scene e menu passam validation sem erro.
- External image transport não usa CPU readback.
- Layer é ativada somente no child.
- Remover/fechar HaumeaVR deixa ambiente/registro inalterado.
- Depth lab tem confidence/fallback.
- Um jogo não suportado continua disponível em L1.

## Primeiros passos do próximo chat

1. Confirmar P33/graphics abstraction estável.
2. Criar Vulkan lab isolado.
3. Implementar OpenXR binding.
4. Testar external memory/semaphore.
5. Adiar layer até parity do core.

## Referências oficiais

- [Vulkan Specification](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)
- [OpenXR Vulkan enable2](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html)
