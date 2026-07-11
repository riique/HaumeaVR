# P32 — Geometry Stereo genérico em D3D11

**Status:** pesquisa/planejamento; não funcional  
**Prioridade:** média-alta  
**Dependências:** P12, P14, P31, P35, P61  
**Desbloqueia:** L3 para builds D3D11 validadas

## Objetivo

Produzir duas vistas geométricas em jogos D3D11 quando não há adapter de engine, usando classificação por shader/draw/matriz e replay seletivo. Este é um modo por perfil, não um hook universal.

## Por que é difícil

Um `Present` não contém a lista reexecutável do frame. Draw calls dependem de:

- constant/vertex/index buffers que mudam;
- render targets/depth;
- UAVs e compute;
- queries/predication;
- command lists;
- recursos temporários;
- ordem e efeitos colaterais;
- TAA/history/post-processing.

Reexecutar tudo depois do frame pode estar incorreto. O design precisa observar/capturar no momento certo, limitar o escopo e cair para Depth.

## Estratégias por ordem

1. Adapter de engine L4, se disponível.
2. Stereo pass integrado por host autorizado.
3. Geometry seletivo para mundo opaco + Depth/HUD híbrido.
4. Depth 3D.

L3 genérico nunca deve competir com um adapter nativo validado.

## Observer/recorder D3D11

Expandir módulo current-process:

- device/context/swapchain lifecycle;
- OM render targets/depth;
- viewports/scissors;
- shaders e hashes;
- input layouts;
- VB/IB/topology;
- constant buffer bindings e updates;
- SRV/samplers/blend/raster/depth state;
- Draw/DrawIndexed/Instanced;
- command lists;
- markers quando houver.

Não capturar conteúdo de buffer para disco por padrão. Diagnostics usam hashes/metadados redigidos.

## Detector de câmera/matriz

Candidates:

- matrices 4×4 e combinações;
- propriedades de projection perspective;
- inverse/transposes;
- atualização por frame;
- correlação com mouse/camera;
- shader/draw coverage;
- near/far/FOV;
- consistência temporal.

Profile Studio permite:

- candidate list;
- visual perturbation somente no lab ou com confirmação;
- classificação world/sky/weapon/HUD/reflection/shadow/post;
- offsets/transpose/layout;
- save por shader hash+slot+signature.

Nunca modificar todos os constant buffers.

## Pipeline híbrido recomendado

1. Jogo produz frame/base eye.
2. Recorder identifica draw ranges do mundo.
3. Para a segunda vista, reemite somente draw calls declarados seguros em target/depth separado.
4. Substitui view/projection copies em buffers privados; não corrompe original.
5. Céu/arma usam regras próprias.
6. Pós-processamento compatível roda por eye; incompatível usa pass compartilhado ou é desativado por perfil.
7. HUD é camada mono.
8. Elementos não reexecutáveis usam Depth 3D.

## Estado e sincronização

- snapshot mínimo de COM refs e small state;
- lifetime por frame;
- limites de memória/draw count;
- command list/thread safety;
- restore comprovado;
- no readback;
- abortar frame em unsupported operation;
- GPU markers/timestamps;
- device removed recovery.

## Perfil L3

- executable/build hash;
- swapchain selection;
- camera candidate(s);
- shader groups;
- draw ranges/rules;
- eye mode;
- weapon/world scale;
- culling strategy;
- HUD/post;
- exclusions;
- performance preset;
- known scenes;
- fallback threshold.

Perfis comunitários só referenciam hashes/slots/roles permitidos pelo adapter oficial D3D11; não contêm código/offsets.

## Fases

1. Lab D3D11 simples com geometry ground truth.
2. Metadata recorder e replay de triângulo/cena opaca.
3. Matrix detector.
4. Profile Studio classification.
5. asymmetric eye projections.
6. state restore/command lists.
7. hybrid Depth/HUD.
8. performance.
9. três jogos offline controlados.
10. beta opt-in.

## Testes

- deferred/immediate contexts;
- dynamic buffers;
- instancing;
- multiple cameras;
- shadows/reflections;
- transparent/particles;
- compute/post;
- TAA;
- resize/device loss;
- shader reload;
- unsupported call causes fallback;
- state diff before/after;
- GPU validation/debug layer no errors;
- long session.

## Critérios de aceite

- Ground truth lab coincide com stereo engine reference dentro da tolerância.
- State restore não altera frame original.
- Classificador explica cada grupo reexecutado.
- Unsupported path cai para Depth/L1.
- Nenhum perfil por pointer/offset.
- Três builds D3D11 offline passam matriz definida.
- UI mantém selo Experimental até hardware/game gates.

## Riscos

- **Replay semântico impossível:** priorizar híbrido e adapter.
- **Custo quase dobra:** qualidade adaptativa/grupos.
- **Update muda shaders:** fingerprint invalida perfil.
- **Anti-cheat:** hard block P61.
- **Crash no jogo:** opt-in, backups e telemetry local.

## Primeiros passos do próximo chat

1. Não começar em jogo comercial.
2. Criar lab D3D11 determinístico com referência stereo.
3. Expandir observer só para metadata e testes.
4. Reemitir um subset simples com restore.
5. Implementar matrix tooling antes de heurística automática.

