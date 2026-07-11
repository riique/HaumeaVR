# P31 — Depth 3D genérico

**Status:** fundação compute existente; produto não implementado  
**Prioridade:** alta  
**Dependências:** P12, P14 ou observer autorizado, P16 tooling, P35  
**Desbloqueia:** L2 em jogos D3D11/D3D12

## Objetivo

Entregar estéreo por reprojeção de cor+profundidade com confidence, HUD correto, reparo temporal e fallback. É a ponte entre tela plana e geometry/native, não uma simulação completa de 6DoF.

## Estado inicial

- `depth_reconstruction.*` possui visualização/reconstrução compute;
- observer D3D11 encontra back buffer e DSV no `Present` dentro do processo atual;
- não existe transporte color/depth de jogo comercial até o core;
- GUI ainda marca Depth indisponível.

## Requisitos de entrada

- color da cena antes de HUD quando possível;
- depth associado à câmera principal;
- viewport;
- formato/sample count;
- convenção normal/reverse-Z;
- near/far ou parâmetros para inferência;
- matriz/projeção quando disponível;
- frame/timestamps;
- máscara HUD/transparências opcional.

Sem depth confiável por frame, não anunciar L2.

## Aquisição D3D11

1. observar criação de textures/DSVs e bindings;
2. catalogar candidatos sem lerback pesado por padrão;
3. identificar recursos shareable/readable;
4. quando depth original não puder ser SRV/shared, criar caminho de cópia compatível no momento seguro ou instrumentar criação em host autorizado;
5. resolver MSAA quando a API/formato permitirem;
6. copiar cor/depth para recursos nomeados;
7. sinalizar por keyed mutex/generation via P12.

Não alterar flags de todos os depth resources nem tentar copiar combinações proibidas. Falha vira candidate unavailable.

## Pontuação de depth

Features:

- resolução/aspect/viewport próximos ao color;
- uso recente antes de Present;
- draw count/cobertura;
- frequência por frame;
- distribuição/histograma;
- continuidade temporal;
- correlação com câmera/matriz;
- rejeição de shadow maps, minimap, reflection e UI;
- estabilidade de near/far;
- intervenção manual no Profile Studio.

Modelo inicial determinístico e explicável. ML só depois de dataset, teste e fallback.

## Linearização

Suportar:

- D16, D24S8, D32 e typeless equivalentes;
- normal Z;
- reverse Z;
- finite/infinite far plane;
- perspective/orthographic;
- ranges API;
- jitter TAA.

Guardar parâmetros no perfil por fingerprint. Heurística nunca sobrescreve configuração validada sem registrar.

## Reprojeção estéreo

1. reconstruir posição view-space;
2. deslocar para poses dos olhos;
3. splat/warp para cada olho;
4. z conflict resolve;
5. detectar disocclusion;
6. preencher por vizinhança/temporal;
7. compor HUD separado;
8. limitar head translation;
9. enviar depth composition opcional ao runtime.

Quality modes:

- fast;
- balanced;
- quality temporal;
- hybrid distant depth para L3/L4.

## HUD, transparência e efeitos

- capture pré-HUD ideal;
- shader/hash/draw range mask como perfil;
- fallback HUD monoscópico em quad layer;
- transparências/particles marcadas como low confidence;
- sky/infinite depth;
- weapon depth scale separada;
- cutscene mode rotation-only.

## Confidence e fallback

Score por frame:

- candidate stability;
- valid pixel ratio;
- temporal discontinuity;
- hole ratio;
- depth range sanity;
- reprojection error proxy.

Estados: Warmup → Reliable → Degraded → Lost. P11 recebe eventos. Não oscilar entre L2/L1; usar hysteresis e cooldown.

## Tooling Profile Studio

- thumbnails de candidatos;
- histograma;
- normal/reverse toggle;
- near/far calibration;
- HUD mask preview;
- stereo anaglyph/side-by-side preview desktop;
- hole/confidence heatmap;
- translation limit;
- salvar decisão por shader/resource signature, nunca pointer.

## Fases

1. Lab D3D11 controlado gera color/depth conhecido.
2. Transporte P12.
3. Candidate catalog/scorer.
4. Linearization suite.
5. Stereo reconstruction correctness.
6. HUD separation.
7. temporal/disocclusion.
8. confidence/fallback.
9. Profile Studio.
10. D3D12 port P33.

## Testes

- cenas sintéticas com ground truth;
- normal/reverse Z, finite/infinite;
- formatos e MSAA;
- múltiplos DSVs;
- camera cut;
- particles/transparency;
- HUD/subtitles;
- dynamic resolution/TAA/DLSS-like input;
- loss/recreation;
- performance em RX 6600 e segunda GPU;
- Quest/PICO.

Métricas visuais: hole ratio, depth edge error, temporal flicker e comparação ground truth no lab.

## Critérios de aceite

- Lab reproduz profundidade correta com erro definido.
- Candidate errado não é ativado automaticamente.
- HUD permanece legível.
- Translation limit impede revelar artefatos extremos.
- Confidence provoca fallback previsível.
- Overhead atende P35.
- Pelo menos três jogos offline de engines diferentes têm perfil beta antes de claim amplo.

## Limitações comunicadas

- não revela geometria oculta;
- não cria colisão/interação;
- translação é limitada;
- efeitos sem depth podem parecer planos/incorretos;
- mãos exigem L4.

## Primeiros passos do próximo chat

1. Criar lab com ground truth e depth variants.
2. Completar transporte color/depth.
3. Testar formatos/linearização antes de candidate heuristics.
4. Implementar confidence.
5. Não ligar botão do launcher até DoD visual/fallback.

