# P44 — Adapter REDengine 4

**Status:** futuro; orientado inicialmente a Cyberpunk 2077  
**Prioridade:** alta depois de D3D12  
**Dependências:** P14, P33, P35, P61, P66  
**Desbloqueia:** P51

## Objetivo

Criar uma integração segura e versionada para REDengine 4, reconhecendo que o primeiro alvo real é Cyberpunk 2077 e que REDmod oficial não fornece sozinho um compositor VR completo.

## Rotas

### Oficial REDmod

Pode entregar:

- pacote/mod descriptor;
- scripts/tweaks/assets compatíveis;
- configuração e animações permitidas;
- instalação/ativação pela infraestrutura oficial.

Usar quando suficiente e respeitar EULA.

### Adapter binário específico

Necessário potencialmente para câmera/render/input de baixo nível. Só pode existir após:

- mecanismo de carregamento documentado/permitido;
- license/security review;
- assinatura oficial;
- fingerprint exato;
- single-player;
- transação/rollback.

REDmod não deve ser descrito como autorização implícita para qualquer hook nativo.

### Graphics fallback

- L1 imediatamente;
- L2 D3D12 após P31/P33;
- L3 experimental;
- mantém valor enquanto L4 amadurece.

## Detecção/fingerprint

- storefront e executable;
- game version/build;
- exe + REDengine DLLs/chaves;
- D3D12;
- Phantom Liberty/base;
- REDmod instalado/version;
- mods existentes;
- launcher arguments;
- language/config;
- save backup separado.

Não coletar ou distribuir assets do jogo.

## Domínios do adapter

### Camera

- base player camera;
- first-person offsets;
- FOV/asymmetric projection;
- head delta;
- camera shake;
- cinematic/photo/vehicle/dialog contexts;
- clipping/near plane;
- body/head visibility;
- weapon/arms.

### Rendering

- D3D12 stereo targets;
- TAA/upscaling/dynamic resolution;
- ray tracing modes;
- HUD layer;
- depth/motion vectors;
- frame timing;
- mirrors/screens/effects.

### Input

- semantic actions;
- locomotion/turn/aim;
- controller fallback;
- UI contexts;
- vehicle;
- haptics.

### Hands

Progressão:

1. controller poses;
2. visual hands retargeted;
3. aim/weapon pose;
4. selected interactions;
5. collision/grab only where game adapter supports;
6. no promessa de manipular todo objeto.

Cyberpunk tem sistemas complexos de weapon animation, body IK, vehicles e cinematics; mãos completas são um projeto game-specific, não uma feature automática REDengine.

## Inspection

Adapter allowlist:

- camera/context states;
- UI layers;
- input actions;
- player/weapon anchors;
- interactable categories escolhidas;
- performance signals.

Não expor arbitrary script execution no Profile Studio.

## Fases

1. L1 profile e fingerprint Cyberpunk.
2. D3D12 color/depth lab aplicado read-only.
3. L2 profile.
4. REDmod packaging/config spike.
5. decisão documentada de adapter host.
6. camera read-only diagnostics.
7. L4-R stereo/head.
8. HUD/cutscenes/vehicles.
9. L4-I controls/haptics.
10. L4-H subset de mãos.

## Testes

- base/expansion;
- versions e update invalidation;
- existing mods clean/stack;
- launcher/store;
- menus/gameplay/dialog/cutscene/vehicle/combat;
- save/load;
- D3D12 quality modes;
- ray tracing/upscalers;
- controller/mãos fallback;
- install/remove/verify files;
- 60 min/2 h.

## Critérios de aceite

- REDmod route segue documentação/licença.
- L4 adapter só carrega em fingerprint suportado.
- Update recusa até revalidação.
- HUD/cinematics/vehicle não causam 6DoF descontrolado.
- Mãos são anunciadas por interaction subset.
- Remove deixa instalação limpa/verificável.

## Primeiros passos do próximo chat

1. Não iniciar por hook no Cyberpunk.
2. Fechar P33 lab.
3. Inspecionar uma instalação legal e fingerprint.
4. Criar L1/L2 profile.
5. Fazer REDmod/license spike e ADR do host.

## Referência oficial

- [Cyberpunk 2077 — REDmod](https://www.cyberpunk.net/pt-br/modding-support)
- [Patch 1.6 e suporte REDmod](https://www.cyberpunk.net/en/news/45280/edgerunners-update-patch-1-6-list-of-changes)

