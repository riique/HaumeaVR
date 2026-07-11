# P00 — Visão, níveis de compatibilidade e promessa do produto

**Status:** planejado e normativo  
**Prioridade:** permanente  
**Dependências:** nenhuma  
**Desbloqueia:** todos os demais planos

## Objetivo

Definir o que o HaumeaVR promete, como essa promessa será medida e onde termina a palavra “universal”. Este documento é normativo: decisões posteriores podem expandir capacidade, mas não podem substituir transparência por uma promessa tecnicamente impossível.

## Produto

**HaumeaVR** é o nome oficial do produto. É um aplicativo Windows de PCVR que seleciona a melhor forma disponível de apresentar e controlar um jogo originalmente criado para monitor. Ele combina:

- launcher e interface própria;
- detecção de jogo, engine, backend gráfico, build e risco;
- core OpenXR independente de fabricante;
- backends de captura/reconstrução/renderização;
- adaptadores de engine;
- adaptadores e perfis específicos de jogo;
- menu dentro do headset;
- diagnóstico, backup, rollback, atualização e distribuição sem código-fonte.

O programa não é um “emulador mágico de mãos”. OpenXR resolve o lado do hardware: poses, botões, ações, háptica e, quando exposto, juntas das mãos. A interpretação do jogo é uma segunda camada. Transformar um gesto físico em “pegar a chave correta e acionar a animação correta” exige semântica que só uma integração com a engine ou o jogo pode fornecer.

## Escada de compatibilidade

### L1 — Virtual Screen

O jogo permanece intacto. A janela é capturada e exibida em uma camada ou malha dentro do VR.

Entrega mínima:

- tela plana e curva;
- tamanho, distância, altura, curvatura e recenter;
- ponteiro do controle para o menu HaumeaVR;
- entrada de teclado/mouse segura para jogos offline, quando explicitamente ativada;
- nenhum módulo dentro do processo do jogo.

É o fallback amplo. Não deve ser chamado de “VR nativo”.

### L2 — Depth 3D

O sistema obtém cor e profundidade e reprojeta o frame para dois olhos.

Entrega mínima:

- profundidade estereoscópica coerente;
- seleção e linearização de depth;
- tratamento de reverse-Z;
- máscara de HUD;
- reparo de disocclusion;
- translação limitada e comunicada ao usuário;
- fallback automático quando depth deixa de ser confiável.

Não cria geometria oculta, colisão nova nem interação por mãos.

### L3 — Geometry Stereo

O sistema identifica câmera/matrizes/draw calls e produz duas vistas geométricas.

Entrega mínima:

- projeções assimétricas derivadas do OpenXR;
- classificação explícita de mundo, céu, arma, HUD, sombra e pós-processamento;
- reexecução seletiva e restauração de estado;
- perfil por build;
- orçamento de desempenho e botão de emergência para fallback.

É uma capacidade experimental por backend e perfil, nunca uma modificação indiscriminada de constant buffers.

### L4 — Engine/Game Native

Um adaptador entende câmeras, UI, entrada, objetos, estados e eventos do jogo.

Subníveis:

- **L4-R:** renderização e câmera nativas;
- **L4-I:** input semântico e háptica;
- **L4-H:** mãos representadas e interações selecionadas;
- **L4-F:** experiência completa validada no conjunto de cenas definido pelo perfil.

Um jogo só pode ser anunciado como “mãos completas” depois de atingir L4-H ou L4-F com matriz de interações publicada.

## Promessas públicas permitidas

- “Funciona em modo tela VR com a maioria dos jogos capturáveis.”
- “Oferece 3D por profundidade ou estéreo geométrico em builds validadas.”
- “Oferece experiência nativa em jogos com adaptador dedicado.”
- “Suporta Meta Quest e PICO em PCVR por runtimes OpenXR compatíveis e testados.”
- “Escolhe fallback seguro quando uma integração não está disponível.”

## Promessas públicas proibidas

- “Transforma qualquer jogo em VR nativo.”
- “Mãos funcionam automaticamente em todo jogo.”
- “Compatível com todos os anti-cheats.”
- “Zero artefatos” ou “zero latência”.
- “Suporte a um headset” sem mencionar a rota de conexão/runtime testada.

## Métricas principais

1. **Cobertura L1:** porcentagem de jogos offline testados que iniciam e exibem frame estável.
2. **Cobertura L2/L3:** porcentagem por API e engine, nunca agregada de forma enganosa.
3. **Cobertura L4:** número de builds exatos com adaptador validado.
4. **Confiabilidade:** sessões sem crash, fallback bem-sucedido e restauração limpa.
5. **Desempenho:** overhead CPU/GPU, frame misses e latência por modo.
6. **Conforto:** frame pacing, opções de snap turn, vignette, altura e recenter.
7. **Segurança:** zero execução em processo protegido, zero modificação sem inventário e zero update não assinado.

## Decisões de produto

- PCVR Windows x64 primeiro.
- OpenXR é a fronteira de hardware; SDKs de fabricante são opcionais e encapsulados.
- D3D11 continua sendo o primeiro backend funcional; D3D12 é obrigatório para Cyberpunk 2077; Vulkan vem depois.
- A GUI própria é parte central do produto: desktop para biblioteca/diagnóstico/edição e overlay OpenXR para ajustes durante a sessão.
- O projeto não recriará runtime OpenXR, driver de headset, API gráfica nem toolkit de UI.

## Critérios de aceite deste plano

- O índice mestre e a interface usam a mesma taxonomia L1–L4.
- Perfis armazenam nível suportado por build e não um simples booleano “VR”.
- A GUI mostra separadamente visual, câmera, input, mãos e háptica.
- Materiais de release deixam claro o runtime e a conexão testados.
- Toda solicitação futura de “qualquer jogo” é decomposta por nível.

## Primeiros passos para o chat executor

1. Ler este documento e procurar no código textos que anunciem capacidade genérica demais.
2. Introduzir a taxonomia como modelo de domínio somente quando P60 estiver em execução.
3. Não editar código neste plano isoladamente; usar suas regras nas implementações dependentes.
4. Registrar qualquer mudança de promessa no changelog e no índice mestre.
