\# Plano de desenvolvimento — Conversor universal de jogos para VR no Windows



\## 1. Visão do produto



Criar um aplicativo para Windows capaz de executar jogos originalmente feitos para monitor dentro de headsets VR, com:



\* Renderização estereoscópica.

\* Rastreamento da cabeça.

\* Suporte a movimento posicional e rotacional.

\* Mapeamento de controles VR para teclado, mouse e gamepad.

\* Interface de configuração utilizável dentro do headset.

\* Perfis automáticos para cada jogo.

\* Suporte prioritário para jogos Unity.

\* Compatibilidade futura com Unreal Engine e jogos DirectX genéricos.

\* Integração com headsets por meio do OpenXR.



O produto não deve depender de um headset ou fabricante específico. O OpenXR é um padrão aberto voltado justamente a abstrair diferentes runtimes e dispositivos XR, permitindo que o aplicativo converse com o runtime ativo do usuário por uma interface comum. A implementação deve utilizar o SDK e o loader oficiais da Khronos.



Nome provisório usado neste documento:



\*\*HaumeaVR\*\*



\---



\# 2. O que significa “funcionar em qualquer jogo”



Existem quatro níveis diferentes de conversão para VR.



\## Nível 1 — Virtual Screen



O jogo continua sendo renderizado normalmente, mas aparece em uma tela gigante dentro de um ambiente VR.



Compatibilidade esperada:



\* Praticamente qualquer jogo capturável.

\* Jogos 2D.

\* Jogos com interfaces complexas.

\* Jogos incompatíveis com injeção.



Recursos:



\* Tela curva ou plana.

\* Ajuste de tamanho e distância.

\* Ambientes virtuais opcionais.

\* Head tracking apenas para movimentar a visão da tela.

\* Modo passthrough, quando suportado pelo runtime.



Esse será o fallback universal.



\---



\## Nível 2 — Depth 3D



O aplicativo captura:



\* Imagem colorida.

\* Depth buffer.

\* Informações de projeção, quando disponíveis.



A partir disso, cria duas imagens ligeiramente deslocadas, uma para cada olho.



Vantagens:



\* Compatibilidade alta.

\* Custo de GPU relativamente baixo.

\* Não exige renderizar o jogo duas vezes.

\* Pode funcionar mesmo sem acesso direto à câmera da engine.



Problemas:



\* Objetos ocultos atrás de outros não existem na imagem original.

\* Podem surgir buracos nas bordas.

\* Transparências, partículas e reflexos podem ter profundidade incorreta.

\* A movimentação posicional da cabeça precisa ser limitada.



Esse modo deve utilizar reconstrução baseada em profundidade, preenchimento temporal e técnicas de disocclusion repair.



\---



\## Nível 3 — Geometry Stereo



O aplicativo intercepta o pipeline gráfico e faz determinados objetos serem renderizados novamente para o segundo olho.



Cada olho recebe:



\* Uma matriz de visão diferente.

\* Uma projeção assimétrica correspondente ao headset.

\* Um pequeno deslocamento equivalente à distância interpupilar.



Vantagens:



\* Profundidade geométrica real.

\* Melhor resposta ao movimento da cabeça.

\* Escala de mundo mais convincente.

\* Menos artefatos que o Depth 3D.



Problemas:



\* Pode quase duplicar o custo de renderização.

\* Exige identificar corretamente as matrizes de câmera.

\* Sombras, partículas e efeitos de tela podem ser executados duas vezes incorretamente.

\* Cada engine pode organizar seus constant buffers de maneira diferente.



\---



\## Nível 4 — Engine Native Bridge



O aplicativo identifica a engine e interage diretamente com:



\* Câmera principal.

\* Câmeras secundárias.

\* Pipeline de renderização.

\* Interface do jogo.

\* Sistema de entrada.

\* Objetos da cena.

\* Configurações de pós-processamento.



Esse será o melhor modo para Unity.



Em vez de apenas modificar comandos do DirectX, o adaptador entende conceitos como:



\* `Camera`.

\* `Canvas`.

\* `Transform`.

\* `RenderTexture`.

\* Built-in Render Pipeline.

\* URP.

\* HDRP.

\* Mono.

\* IL2CPP.



A Unity permite matrizes de projeção personalizadas e possui APIs específicas para matrizes estereoscópicas. Ela também expõe interfaces para plug-ins nativos acessarem eventos e recursos de renderização em baixo nível.



\---



\# 3. Estratégia principal



O projeto deve ser desenvolvido em camadas.



```text

Interface e launcher

&#x20;       ↓

Detector de jogo e engine

&#x20;       ↓

Gerenciador de perfis

&#x20;       ↓

Carregador seguro do bridge

&#x20;       ↓

Adaptador da engine ou adaptador gráfico genérico

&#x20;       ↓

Conversor estereoscópico

&#x20;       ↓

Compositor OpenXR

&#x20;       ↓

Runtime OpenXR

&#x20;       ↓

Headset

```



O aplicativo não deve possuir um único “hook universal”. Ele deve selecionar automaticamente a melhor estratégia disponível.



Ordem de preferência:



1\. Adaptador Unity completo.

2\. Geometry Stereo por Direct3D.

3\. Depth 3D.

4\. Virtual Screen.



Caso um modo falhe, o aplicativo reduz automaticamente para o seguinte, em vez de simplesmente fechar o jogo.



\---



\# 4. Arquitetura geral



\## 4.1 Launcher



Aplicativo responsável por:



\* Detectar jogos instalados.

\* Adicionar executáveis manualmente.

\* Identificar a engine.

\* Escolher o runtime OpenXR ativo.

\* Instalar ou remover o bridge.

\* Aplicar argumentos de inicialização.

\* Gerenciar perfis.

\* Exibir diagnósticos.

\* Restaurar configurações originais.

\* Iniciar o jogo em modo seguro.



Tecnologias recomendadas:



\* \*\*C# com .NET 10\*\* para o launcher.

\* \*\*WinUI 3\*\* ou \*\*Avalonia\*\* para a interface.

\* \*\*SQLite\*\* para banco local de perfis.

\* Comunicação com o núcleo por named pipes ou memória compartilhada.



O launcher deve funcionar mesmo sem o headset conectado, permitindo preparar perfis previamente.



\---



\## 4.2 Core nativo



Biblioteca central em:



\* C++20 ou C++23.

\* CMake.

\* Visual Studio/MSVC.

\* Direct3D 11 inicialmente.

\* OpenXR.

\* Windows API.



Responsabilidades:



\* Inicializar o OpenXR.

\* Receber poses do headset.

\* Criar swapchains para os olhos.

\* Interagir com Direct3D.

\* Controlar sincronização.

\* Fazer cópias GPU–GPU.

\* Submeter os frames ao runtime.

\* Gerenciar overlay e mirror window.

\* Monitorar falhas e reinicializações.



O OpenXR fornece bindings específicos para Direct3D 11, incluindo requisitos do dispositivo e imagens de swapchain baseadas em `ID3D11Texture2D`.



\---



\## 4.3 Interceptador gráfico



Responsável por observar:



\* Criação do dispositivo Direct3D.

\* Criação da swapchain.

\* `Present`.

\* Back buffers.

\* Depth-stencil views.

\* Constant buffers.

\* Draw calls.

\* Mudanças de viewport.

\* Shaders ativos.



A swapchain contém as superfícies utilizadas para apresentar os frames, e o método `Present` realiza a apresentação da imagem. Esses pontos são adequados para identificar o fim de um frame e transferi-lo ao pipeline VR.



Para interceptação em user mode, uma opção inicial é o Microsoft Detours, biblioteca oficial destinada à instrumentação e interceptação de funções em aplicações Windows.



O interceptador deve ser isolado por backend:



```text

GraphicsBackend

├── D3D11Backend

├── D3D12Backend

├── VulkanBackend

└── LegacyBackend

```



Somente o D3D11 deve ser implementado inicialmente.



\---



\## 4.4 Compositor OpenXR



O compositor interno deverá:



1\. Inicializar uma instância OpenXR.

2\. Detectar o sistema XR.

3\. Consultar as configurações de visão.

4\. Criar uma sessão.

5\. Criar swapchains para os olhos.

6\. Aguardar o instante correto de renderização.

7\. Obter as poses previstas dos olhos.

8\. Renderizar ou copiar as imagens.

9\. Liberar as swapchains.

10\. Enviar uma camada de projeção.



Recursos posteriores:



\* Camadas separadas para HUD.

\* Camada quad para menus.

\* Camada cilíndrica para interfaces.

\* Envio de depth buffer ao runtime.

\* Foveated rendering.

\* Quad views.

\* Reprojeção por profundidade.



A extensão `XR\_KHR\_composition\_layer\_depth` permite associar informações de profundidade às imagens das camadas de projeção, o que pode ajudar runtimes compatíveis a realizar reprojeção baseada em profundidade.



\---



\# 5. Estratégia Unity-first



\## 5.1 Detector de jogos Unity



O launcher deve procurar sinais como:



\* `UnityPlayer.dll`.

\* Diretório `NomeDoJogo\_Data`.

\* `globalgamemanagers`.

\* `GameAssembly.dll`.

\* `MonoBleedingEdge`.

\* `Managed/Assembly-CSharp.dll`.

\* Metadados de versão.

\* Bibliotecas URP ou HDRP.

\* Uso de Direct3D 11 ou Direct3D 12.



Classificação automática:



```text

Engine: Unity

Unity version: detectada

Scripting backend: Mono ou IL2CPP

Graphics API: D3D11 ou D3D12

Render pipeline: Built-in, URP, HDRP ou customizado

Architecture: x64 ou x86

Anti-cheat: não detectado, detectado ou desconhecido

```



Unity utiliza diferentes backends de scripting. Mono executa código gerenciado por JIT, enquanto o IL2CPP converte o código intermediário para C++ e produz código nativo antecipadamente. Isso exige adaptadores separados.



\---



\## 5.2 Adaptador Unity Mono



Será o primeiro modo completo.



Objetivos:



\* Localizar o runtime Mono.

\* Carregar um assembly bridge.

\* Executar o componente principal na thread correta.

\* Localizar todas as câmeras.

\* Identificar a câmera do jogador.

\* Criar câmeras para olho esquerdo e direito.

\* Aplicar poses OpenXR.

\* Renderizar em texturas compartilhadas.

\* Separar HUD da imagem 3D.

\* Mapear controles VR.



Componente conceitual:



```text

UnityVRBridge

├── CameraManager

├── StereoRenderer

├── CanvasManager

├── InputMapper

├── HeadTrackingController

├── CutsceneDetector

├── PostProcessingController

└── OpenXRSharedTransport

```



O bridge gerenciado não deve controlar diretamente a sessão OpenXR principal. O OpenXR ficará no core C++, enquanto o bridge Unity trocará poses, texturas e comandos com ele.



\---



\## 5.3 Adaptador Unity IL2CPP



O IL2CPP será tratado em uma segunda etapa porque:



\* Métodos estão compilados em código nativo.

\* Metadados podem ter stripping.

\* Nomes e layouts mudam entre versões.

\* Jogos podem usar ofuscação.

\* Atualizações podem alterar offsets.



A arquitetura não deve depender de offsets fixos publicados em perfis.



Preferências:



1\. Usar exports e metadados identificáveis.

2\. Resolver métodos por assinatura e estrutura.

3\. Validar cada endereço antes do uso.

4\. Manter descritores por família de versões da Unity.

5\. Cair para o modo gráfico genérico quando a resolução falhar.



O adaptador IL2CPP deve ser um módulo opcional. Uma falha nele não pode impedir o jogo de ser iniciado normalmente.



\---



\## 5.4 Controle da câmera



O sistema deve oferecer diferentes modos.



\### Head tracking acoplado



A cabeça controla completamente a câmera.



Indicado para:



\* Jogos em primeira pessoa.

\* Simuladores.

\* Jogos sem animações rígidas de câmera.



\### Head tracking desacoplado



O usuário olha ao redor, mas a mira permanece controlada pelo mouse ou controle.



Indicado para:



\* Jogos de tiro.

\* Jogos com movimentação rápida.

\* Usuários sensíveis a enjoo.



\### Movimento híbrido



\* Movimento amplo do mouse gira o personagem.

\* Movimento pequeno da cabeça apenas altera a visão.

\* Ao ultrapassar determinado ângulo, a câmera recentraliza gradualmente.



\### Rotação apenas



Ignora o movimento posicional do headset.



Indicado para:



\* Cutscenes.

\* Jogos em terceira pessoa.

\* Depth 3D.

\* Situações em que mover a câmera revelaria partes não renderizadas.



\### Posicionamento completo



Aplica os seis graus de liberdade:



\* X.

\* Y.

\* Z.

\* Pitch.

\* Yaw.

\* Roll.



Deve incluir:



\* Limites de deslocamento.

\* Colisão opcional.

\* Neck model.

\* Recentramento.

\* Ajuste de altura.

\* Ajuste de escala do mundo.



\---



\## 5.5 Renderização estereoscópica Unity



A implementação deve receber do OpenXR:



\* Pose de cada olho.

\* Campo de visão de cada olho.

\* Dimensões recomendadas da swapchain.

\* Posição prevista para o instante de exibição.



Em seguida, o bridge deverá:



1\. Encontrar a câmera principal.

2\. Copiar suas configurações.

3\. Criar ou reutilizar duas câmeras.

4\. Aplicar a matriz de visão de cada olho.

5\. Aplicar projeções assimétricas.

6\. Renderizar para duas `RenderTexture`.

7\. Compartilhar as texturas com o núcleo nativo.

8\. Restaurar o estado da câmera original.



Cuidados:



\* Não usar apenas o `fieldOfView` tradicional.

\* Respeitar projeções assimétricas.

\* Corrigir jitter de TAA.

\* Evitar executar lógica de gameplay duas vezes.

\* Não duplicar áudio.

\* Não duplicar scripts de câmera.

\* Controlar sombras separadamente.

\* Evitar duas atualizações de partículas.



\---



\## 5.6 Pipelines Unity



\### Built-in Render Pipeline



Primeiro alvo.



É o mais adequado para validar:



\* Câmeras duplicadas.

\* Render textures.

\* Depth buffer.

\* Pós-processamento.

\* Interfaces.



\### URP



Segundo alvo.



Exigirá tratamento para:



\* Camera stacking.

\* Renderer Features.

\* Scriptable Render Passes.

\* Post-processing.

\* Render scale.

\* Forward e deferred.



\### HDRP



Terceiro alvo.



Exigirá tratamento especial para:



\* TAA.

\* Dynamic resolution.

\* DLSS e outros upscalers.

\* Custom Passes.

\* Exposure.

\* Volumetrics.

\* Ray tracing.

\* History buffers por câmera.



\### Pipelines personalizados



Devem começar em:



\* Geometry Stereo.

\* Depth 3D.

\* Virtual Screen.



Um perfil da comunidade poderá adicionar correções específicas posteriormente.



\---



\## 5.7 Interfaces e HUD



A interface 2D é um dos maiores problemas de conversões para VR.



Devem existir quatro estratégias:



\### HUD preso à cabeça



O HUD permanece sempre à frente do usuário.



\### HUD no mundo



A interface fica posicionada como uma tela flutuante no cenário.



\### HUD curvo



A interface é projetada sobre uma superfície curva.



\### HUD original



A interface continua integrada à imagem de cada olho.



O adaptador Unity deverá identificar `Canvas` configurados como:



\* Screen Space Overlay.

\* Screen Space Camera.

\* World Space.



Sempre que possível, o HUD será renderizado separadamente e enviado ao OpenXR como uma camada própria. Isso evita que a interface seja distorcida pelo efeito estereoscópico.



Configurações:



\* Distância.

\* Escala.

\* Curvatura.

\* Opacidade.

\* Altura.

\* Profundidade.

\* Ocultação automática.

\* Compatibilidade com legendas.



\---



\# 6. Caminho genérico Direct3D 11



\## 6.1 Captura do frame



No modo genérico, o core interceptará a swapchain e obterá:



\* Back buffer.

\* Formato.

\* Resolução.

\* Viewport.

\* Depth-stencil ativo.

\* Estado do dispositivo.

\* Lista resumida de draw calls.



A imagem será transferida diretamente entre texturas da GPU, evitando cópia para RAM.



\---



\## 6.2 Detector de depth buffer



O jogo pode criar múltiplos buffers de profundidade:



\* Cena principal.

\* Sombras.

\* Reflexos.

\* Minimapa.

\* Interface.

\* Efeitos.

\* Câmeras secundárias.



O sistema deve pontuar cada candidato usando:



\* Resolução semelhante ao back buffer.

\* Quantidade de draw calls.

\* Frequência de uso.

\* Momento em que foi vinculado.

\* Cobertura da imagem.

\* Relação com o viewport principal.

\* Distribuição dos valores de profundidade.



O usuário poderá abrir um visualizador de depth buffer dentro do headset e selecionar manualmente o correto quando a detecção falhar.



\---



\## 6.3 Detector de matrizes



Para Geometry Stereo, o sistema precisa reconhecer constant buffers que contenham:



\* View matrix.

\* Projection matrix.

\* View-projection matrix.

\* Inversas dessas matrizes.

\* Posição da câmera.

\* Near plane.

\* Far plane.



Estratégias:



\* Reconhecimento estrutural de matrizes 4×4.

\* Validação de propriedades matemáticas.

\* Comparação entre frames.

\* Observação das mudanças causadas pelo mouse.

\* Correlação com draw calls.

\* Classificação por shader.

\* Perfil manual como fallback.



Nunca modificar todos os constant buffers indiscriminadamente.



O sistema deve registrar quais draw calls utilizam determinada matriz e permitir classificar:



\* Mundo 3D.

\* Céu.

\* Armas.

\* HUD.

\* Sombras.

\* Reflexos.

\* Pós-processamento.



\---



\## 6.4 Geometry Stereo



Pipeline proposto:



1\. Capturar o estado do draw call.

2\. Verificar se ele pertence à cena 3D.

3\. Renderizar normalmente para o olho esquerdo.

4\. Substituir as matrizes relevantes.

5\. Renderizar novamente para o olho direito.

6\. Restaurar os buffers originais.

7\. Executar pós-processamento de maneira controlada.

8\. Enviar os dois olhos ao OpenXR.



Modos de desempenho:



\* Stereo completo.

\* Stereo somente em objetos próximos.

\* Stereo por grupos de draw calls.

\* Alternate-eye rendering.

\* Depth 3D para elementos distantes.

\* Híbrido Geometry + Depth.



\---



\## 6.5 Reconstrução por profundidade



O Depth 3D será implementado com compute shaders.



Etapas:



1\. Linearizar o depth buffer.

2\. Reconstruir posições em view space.

3\. Projetar cada pixel para o olho correspondente.

4\. Detectar regiões sem informação.

5\. Preencher pequenos buracos.

6\. Usar dados de frames anteriores.

7\. Tratar bordas com filtros adaptativos.

8\. Compor transparências e HUD.

9\. Aplicar sharpening moderado.



Parâmetros configuráveis:



\* Intensidade 3D.

\* Escala de profundidade.

\* Distância de convergência.

\* Near e far plane.

\* Limite de movimento posicional.

\* Qualidade do preenchimento.

\* Quantidade de histórico temporal.



\---



\# 7. Entrada e controles



\## 7.1 Sistema de ações



Internamente, os controles VR devem ser representados por ações abstratas:



```text

Move

Look

PrimaryAction

SecondaryAction

Jump

Crouch

Sprint

Interact

Reload

Menu

Inventory

Recentre

ToggleVRMenu

```



Cada ação poderá ser vinculada a:



\* Teclado.

\* Mouse.

\* XInput.

\* Movimento do controle.

\* Gesto.

\* Botão do controle VR.

\* Combinação de botões.



\---



\## 7.2 Emulação de controle tradicional



Primeira versão:



\* Controles VR simulam um gamepad XInput.

\* Analógico esquerdo controla movimento.

\* Analógico direito controla câmera.

\* Gatilhos simulam botões.

\* Head tracking controla visão.



Modos adicionais:



\* Controle VR como mouse.

\* Apontamento por raycast.

\* Giro físico do controle como analógico.

\* Gestos configuráveis.

\* Controle de mira separado da cabeça.



\---



\## 7.3 Controles de movimento completos



Não devem fazer parte do primeiro MVP universal.



Para oferecer mãos reais dentro do mundo seria necessário:



\* Identificar armas e objetos.

\* Substituir animações.

\* Alterar sistemas de interação.

\* Criar colisões.

\* Interceptar raycasts.

\* Modificar lógica de gameplay.



Isso deverá ser implementado por adaptadores específicos de jogo ou por uma API pública de mods.



\---



\# 8. Menu dentro do VR



O usuário deve conseguir configurar tudo sem remover o headset.



Menu acionado por combinação configurável:



\* Recentrar.

\* Altura.

\* Escala do mundo.

\* Intensidade 3D.

\* Separação dos olhos.

\* Convergência.

\* Modo de renderização.

\* Modo de câmera.

\* HUD.

\* Mapeamento de controle.

\* Qualidade.

\* Desempenho.

\* Visualizador de depth.

\* Diagnóstico de matrizes.

\* Perfil ativo.

\* Modo seguro.



O menu pode ser enviado como uma camada quad separada do OpenXR, mantendo boa legibilidade.



\---



\# 9. Sistema de perfis



Cada jogo deverá possuir um perfil declarativo.



Exemplo conceitual:



```json

{

&#x20; "game": {

&#x20;   "name": "Example Game",

&#x20;   "executable": "ExampleGame.exe",

&#x20;   "engine": "unity",

&#x20;   "engineVersion": "2022.3",

&#x20;   "backend": "il2cpp",

&#x20;   "graphicsApi": "d3d11"

&#x20; },

&#x20; "vr": {

&#x20;   "preferredMode": "unity-native-stereo",

&#x20;   "fallback": \["geometry", "depth", "screen"],

&#x20;   "worldScale": 1.0,

&#x20;   "headTracking": "hybrid",

&#x20;   "positionalTracking": true

&#x20; },

&#x20; "camera": {

&#x20;   "selector": "tag:MainCamera",

&#x20;   "nearClip": 0.05,

&#x20;   "cutsceneMode": "rotation-only"

&#x20; },

&#x20; "hud": {

&#x20;   "mode": "openxr-layer",

&#x20;   "distance": 1.5,

&#x20;   "scale": 0.8,

&#x20;   "curvature": 0.15

&#x20; }

}

```



O perfil não deve conter código executável.



Perfis com scripts ou patches especiais devem utilizar módulos separados, assinados e explicitamente autorizados pelo usuário.



\---



\## 9.1 Perfis automáticos



Ao abrir um jogo sem perfil:



1\. Detectar engine.

2\. Detectar backend.

3\. Detectar API gráfica.

4\. Iniciar modo seguro.

5\. Encontrar depth buffer.

6\. Testar rastreamento.

7\. Avaliar desempenho.

8\. Sugerir modo recomendado.

9\. Salvar configuração local.



O aplicativo poderá produzir um relatório:



```text

Compatibilidade: boa

Modo recomendado: Unity Stereo

Fallback: Depth 3D

HUD separado: disponível

Head tracking: completo

Controles de movimento: não configurados

Problemas detectados: TAA incompatível

Correção aplicada: TAA desativado durante stereo

```



\---



\## 9.2 Perfis comunitários



Requisitos:



\* Versionamento.

\* Assinatura digital.

\* Avaliações.

\* Compatibilidade por versão do jogo.

\* Histórico de alterações.

\* Rollback.

\* Comparação antes da instalação.

\* Proibição de executáveis embutidos em perfis comuns.

\* Denúncia de perfis maliciosos.

\* Verificação automática após atualizações.



Um perfil deve poder ser exportado como arquivo portátil.



\---



\# 10. Desempenho



\## 10.1 Técnicas essenciais



\* Render scale independente.

\* Upscaling após renderização.

\* Foveated rendering quando disponível.

\* Fixed foveated rendering como fallback.

\* Reutilização de depth.

\* Renderização de sombras apenas uma vez.

\* Geometry Stereo seletivo.

\* Alternate-eye rendering.

\* Occlusion mask do headset.

\* Dynamic resolution.

\* Frame pacing baseado em `xrWaitFrame`.

\* Evitar cópias GPU–CPU.

\* Texturas compartilhadas.

\* Filas assíncronas quando seguras.

\* Limitação de pós-processamento por olho.



\---



\## 10.2 Métricas exibidas



Overlay de desempenho:



\* FPS do jogo.

\* FPS enviado ao headset.

\* Tempo de CPU.

\* Tempo de GPU.

\* Tempo do bridge.

\* Frames perdidos.

\* Frames reprojetados.

\* Latência estimada.

\* Resolução por olho.

\* Modo estereoscópico.

\* Uso de VRAM.

\* Quantidade de draw calls duplicados.



\---



\# 11. Segurança e compatibilidade



\## 11.1 Jogos com anti-cheat



O produto não deverá:



\* Contornar anti-cheat.

\* Ocultar o bridge de sistemas de integridade.

\* Modificar drivers ou kernel.

\* Injetar automaticamente em jogos protegidos.

\* Prometer compatibilidade com multiplayer competitivo.



Comportamento recomendado:



1\. Detectar módulos de anti-cheat conhecidos.

2\. Bloquear o modo de injeção por padrão.

3\. Oferecer somente Virtual Screen ou captura externa.

4\. Informar claramente o motivo.

5\. Manter uma lista local de bloqueio.

6\. Permitir que desenvolvedores autorizem oficialmente o bridge.



Isso reduz riscos de banimento e evita que o projeto seja percebido como ferramenta de evasão.



\---



\## 11.2 Isolamento de falhas



Cada execução deve possuir:



\* Watchdog externo.

\* Timeout de inicialização.

\* Modo seguro.

\* Rollback automático.

\* Restauração de arquivos.

\* Logs separados por jogo.

\* Minidumps opcionais.

\* Desativação do módulo que causou a falha.

\* Atalho para iniciar o jogo sem VR.



O programa não deve substituir permanentemente DLLs originais sem manter backup e método de restauração.



\---



\## 11.3 Princípio de menor privilégio



\* Sem serviço permanente, salvo necessidade futura comprovada.

\* Sem driver de kernel no MVP.

\* Sem execução como administrador por padrão.

\* Diretório de perfis separado do diretório dos jogos.

\* Atualizações assinadas.

\* Hash dos módulos.

\* Comunicação IPC autenticada.

\* Nenhum código comunitário carregado automaticamente.



\---



\# 12. Estrutura sugerida do repositório



```text

HaumeaVR/

├── apps/

│   ├── launcher/

│   ├── profile-editor/

│   └── diagnostics/

│

├── core/

│   ├── openxr/

│   ├── compositor/

│   ├── frame-timing/

│   ├── ipc/

│   ├── logging/

│   └── crash-handler/

│

├── graphics/

│   ├── common/

│   ├── d3d11/

│   ├── d3d12/

│   └── vulkan/

│

├── engines/

│   ├── unity/

│   │   ├── common/

│   │   ├── mono/

│   │   ├── il2cpp/

│   │   ├── builtin/

│   │   ├── urp/

│   │   └── hdrp/

│   └── unreal/

│

├── stereo/

│   ├── geometry/

│   ├── depth/

│   ├── temporal/

│   └── cinema/

│

├── input/

│   ├── openxr-actions/

│   ├── xinput/

│   ├── keyboard-mouse/

│   └── bindings/

│

├── overlay/

│   ├── vr-menu/

│   ├── hud/

│   └── diagnostics/

│

├── profiles/

│   ├── schema/

│   ├── validator/

│   └── repository-client/

│

├── tests/

│   ├── synthetic-games/

│   ├── unity-scenes/

│   ├── graphics-tests/

│   └── openxr-tests/

│

└── docs/

```



\---



\# 13. Etapas de desenvolvimento



\## Marco 0 — Base OpenXR



Entregáveis:



\* Aplicativo OpenXR mínimo.

\* Detecção do headset.

\* Renderização de uma cena de teste.

\* Swapchains D3D11.

\* Head tracking.

\* Mirror window.

\* Controle de frame timing.

\* Logs e diagnóstico.



Usar o exemplo oficial `hello\_xr` como referência arquitetural, não como base monolítica do produto. O repositório oficial contém loader, camadas de validação e exemplos.



Critério de conclusão:



\* Aplicação apresenta duas imagens estáveis no headset.

\* Funciona com o runtime OpenXR ativo.

\* Não possui dependência direta de uma API específica de fabricante.



\---



\## Marco 1 — Virtual Screen universal



Entregáveis:



\* Captura de jogos em janela e tela cheia compatível.

\* Tela plana e curva.

\* Head tracking.

\* Mapeamento básico de controles.

\* Menu VR.

\* Perfis simples.

\* Recentramento.

\* Mirror window.



Esse marco cria um produto utilizável antes da conversão estereoscópica completa.



\---



\## Marco 2 — D3D11 Depth 3D



Entregáveis:



\* Interceptação D3D11.

\* Identificação da swapchain.

\* Captura do back buffer.

\* Detector de depth buffer.

\* Visualizador de profundidade.

\* Reconstrução estéreo por compute shader.

\* Preenchimento de bordas.

\* Ajuste de convergência.

\* Fallback automático.



Critério de conclusão:



\* Uma seleção de jogos D3D11 apresenta profundidade utilizável sem perfis manuais complexos.



\---



\## Marco 3 — Unity Mono completo



Entregáveis:



\* Detector de Unity.

\* Detector de Mono.

\* Bridge gerenciado.

\* Localização da câmera principal.

\* Projeção por olho.

\* Render textures compartilhadas.

\* Separação de HUD.

\* Head tracking híbrido.

\* Mapeamento OpenXR para XInput.

\* Tratamento de cutscenes.

\* Suporte inicial ao Built-in Render Pipeline.



Critério de conclusão:



\* Jogos Unity Mono selecionados funcionam com geometria estéreo real, HUD legível e rastreamento de cabeça.



\---



\## Marco 4 — URP e HDRP



Entregáveis:



\* Adaptadores para camera stacking.

\* Correções para TAA.

\* Correções para post-processing.

\* Compatibilidade com dynamic resolution.

\* Tratamento de efeitos de tela.

\* Perfis por pipeline.

\* Diagnóstico de passes problemáticos.



\---



\## Marco 5 — Unity IL2CPP



Entregáveis:



\* Identificação de versões.

\* Resolução segura de estruturas.

\* Adaptador IL2CPP.

\* Compatibilidade com stripping.

\* Validação de métodos.

\* Fallback automático.

\* Atualizações independentes por adaptador.



\---



\## Marco 6 — Geometry Stereo genérico



Entregáveis:



\* Analisador de constant buffers.

\* Detector de matrizes.

\* Classificador de draw calls.

\* Duplicação seletiva.

\* Editor visual de shaders e passes.

\* Perfis de matrizes.

\* Modo híbrido Geometry + Depth.



Esse será o marco tecnicamente mais difícil.



\---



\## Marco 7 — Direct3D 12



Não deve começar antes de o D3D11 estar estável.



Desafios adicionais:



\* Command queues.

\* Command lists.

\* Resource barriers.

\* Descritores.

\* Sincronização explícita.

\* Múltiplas filas.

\* Estados de recursos.

\* Pipelines assíncronos.



Unity permite que jogadores Windows sejam iniciados explicitamente com D3D11 ou D3D12 por argumentos de linha de comando. Durante a fase inicial, o launcher poderá tentar `-force-d3d11` em jogos Unity que suportem essa opção, mantendo D3D12 como etapa posterior.



\---



\## Marco 8 — Unreal Engine



Somente após a arquitetura Unity e o modo genérico estarem maduros.



Separar:



\* Unreal Engine 4.

\* Unreal Engine 5.

\* Jogos com forward rendering.

\* Jogos com deferred rendering.

\* Jogos com TSR.

\* Jogos com Lumen e Nanite.



\---



\# 14. Aplicativos de desenvolvimento auxiliares



\## 14.1 Unity Compatibility Lab



Criar projetos próprios de teste com:



\* Unity Mono e IL2CPP.

\* Built-in, URP e HDRP.

\* Primeira e terceira pessoa.

\* Múltiplas câmeras.

\* TAA.

\* Motion blur.

\* Transparências.

\* Partículas.

\* Água.

\* Espelhos.

\* Portais.

\* Cutscenes.

\* HUDs variados.

\* Dynamic resolution.

\* Diferentes versões da Unity.



Esses projetos permitirão testar o bridge sem depender inicialmente de jogos comerciais.



\---



\## 14.2 Graphics Inspector interno



Ferramenta para visualizar:



\* Render targets.

\* Depth buffers.

\* Draw calls.

\* Shaders.

\* Constant buffers.

\* Matrizes candidatas.

\* Viewports.

\* Recursos por frame.



Não precisa competir com ferramentas completas de debugging gráfico. Deve mostrar apenas o necessário para criar perfis VR.



\---



\## 14.3 Profile Studio



Editor visual para:



\* Selecionar câmera.

\* Selecionar depth buffer.

\* Marcar draw calls como HUD, mundo ou céu.

\* Ajustar escala.

\* Configurar controles.

\* Testar diferentes modos.

\* Comparar frame esquerdo e direito.

\* Exportar perfil.



\---



\# 15. Critérios do primeiro MVP público



O primeiro MVP não deve prometer “qualquer jogo”.



Escopo recomendado:



\* Windows 10 e 11 x64.

\* OpenXR.

\* Direct3D 11.

\* Jogos Unity.

\* Backend Mono inicialmente.

\* Built-in Render Pipeline.

\* Virtual Screen.

\* Depth 3D.

\* Stereo nativo pelo adaptador Unity.

\* Rotação da cabeça.

\* Movimento posicional configurável.

\* Controle por gamepad.

\* Controles VR emulados como XInput.

\* Menu dentro do headset.

\* Perfis locais.

\* Modo seguro.

\* Sem suporte a jogos com anti-cheat.



Mensagem comercial adequada:



> Converta jogos Unity selecionados em experiências VR com rastreamento de cabeça, estereoscopia real e perfis configuráveis, com modos de compatibilidade para outros jogos DirectX 11.



Não anunciar:



> Todos os jogos funcionam perfeitamente em VR.



\---



\# 16. Como superar soluções existentes



O projeto deve ser objetivamente melhor nos seguintes pontos.



\## Detecção automática



\* Identificação da engine.

\* Identificação do pipeline.

\* Identificação do backend.

\* Escolha automática do modo.

\* Diagnóstico automático de problemas.



\## Transparência



O usuário consegue visualizar:



\* Depth buffer escolhido.

\* Matrizes escolhidas.

\* Passes duplicados.

\* Correções aplicadas.

\* Motivo de um fallback.

\* Custo de cada recurso.



\## Interface moderna



\* Launcher claro.

\* Configuração dentro do headset.

\* Busca por jogos.

\* Perfis sincronizáveis.

\* Comparação antes/depois.

\* Restaurar padrões com um clique.



\## Segurança



\* Sem kernel.

\* Sem bypass de anti-cheat.

\* Perfis declarativos.

\* Módulos assinados.

\* Rollback.

\* Início sem VR sempre disponível.



\## Arquitetura extensível



\* Adaptadores separados por engine.

\* Backends gráficos separados.

\* OpenXR como padrão.

\* API pública para perfis e extensões.

\* Testes automatizados.

\* Módulos atualizáveis independentemente.



\## Qualidade Unity



\* Acesso real à câmera.

\* HUD separado.

\* Tratamento de cutscenes.

\* Mapeamento por objeto.

\* Compatibilidade por pipeline.

\* Menor dependência de ajustes manuais.



\---



\# 17. Riscos principais



| Risco                            | Consequência                    | Mitigação                                  |

| -------------------------------- | ------------------------------- | ------------------------------------------ |

| Jogo atualiza                    | Perfil deixa de funcionar       | Perfis versionados e fallback              |

| Matriz errada                    | Imagem deformada                | Validação matemática e modo seguro         |

| Depth buffer incorreto           | Profundidade invertida          | Detector, preview e seleção manual         |

| TAA incompatível                 | Ghosting diferente em cada olho | Histórico separado ou desativação          |

| Câmera duplicada executa scripts | Gameplay roda duas vezes        | Câmeras passivas e callbacks controlados   |

| Desempenho insuficiente          | Enjoo e frames perdidos         | Depth mode, render scale e stereo seletivo |

| HUD fica profundo demais         | Interface ilegível              | Camada OpenXR separada                     |

| Anti-cheat bloqueia              | Banimento ou fechamento         | Bloqueio preventivo de injeção             |

| IL2CPP muda entre versões        | Falhas ou crashes               | Adaptadores versionados e validação        |

| Mods maliciosos                  | Comprometimento do computador   | Perfis sem código e módulos assinados      |

| Head tracking conflita com mouse | Movimento desconfortável        | Modos desacoplado e híbrido                |



\---



\# 18. Equipe técnica ideal



Competências necessárias:



\* C++ e Windows internals.

\* Direct3D 11 e posteriormente Direct3D 12.

\* OpenXR.

\* HLSL e compute shaders.

\* Unity internals.

\* C# e .NET.

\* Matemática de computação gráfica.

\* Matrizes de visão e projeção.

\* Renderização estereoscópica.

\* Sincronização CPU/GPU.

\* UX para VR.

\* Testes de compatibilidade.

\* Segurança de software.



Divisão sugerida:



```text

Graphics/OpenXR

Unity integration

Launcher/Profile system

VR UX/Input

Testing/Compatibility

```



Uma pessoa pode criar o protótipo, mas a compatibilidade ampla exigirá especialização em várias dessas áreas.



\---



\# 19. Ordem exata recomendada para começar



1\. Criar o repositório CMake.

2\. Compilar o loader oficial do OpenXR.

3\. Adaptar o `hello\_xr` para D3D11.

4\. Renderizar duas texturas simples no headset.

5\. Implementar mirror window.

6\. Criar o launcher em C#.

7\. Criar comunicação launcher–core.

8\. Implementar captura de uma janela.

9\. Criar Virtual Screen.

10\. Implementar menu VR.

11\. Criar um projeto Unity próprio em Mono e D3D11.

12\. Criar um plug-in nativo Unity controlado.

13\. Compartilhar texturas Unity–OpenXR.

14\. Aplicar poses às câmeras.

15\. Renderizar olho esquerdo e direito.

16\. Separar um Canvas.

17\. Mapear controles VR para XInput.

18\. Criar um jogo Unity de testes automatizados.

19\. Implementar detector de jogos Unity.

20\. Somente depois iniciar injeção em jogos externos.

21\. Implementar Depth 3D.

22\. Criar sistema de perfis.

23\. Adicionar URP.

24\. Adicionar IL2CPP.

25\. Iniciar Geometry Stereo genérico.

26\. Adicionar HDRP.

27\. Iniciar D3D12.

28\. Criar distribuição comunitária de perfis.



\---



\# 20. Resultado final esperado



O produto completo terá três componentes principais:



\## UniversalXR Launcher



Gerencia jogos, perfis, diagnósticos e atualizações.



\## UniversalXR Runtime Bridge



Conecta o jogo ao OpenXR, controla renderização, poses e submissão dos frames.



\## UniversalXR Engine Adapters



Fornece integrações especializadas:



```text

Unity Mono

Unity IL2CPP

Unity Built-in

Unity URP

Unity HDRP

Direct3D 11 genérico

Direct3D 12 genérico

Unreal Engine

```



A vantagem decisiva não será apenas “injetar VR” em mais jogos. Será oferecer um sistema que entende gradualmente cada jogo, escolhe a melhor técnica disponível, explica o que está acontecendo e continua funcionando em um modo inferior quando a conversão completa não for possível.



