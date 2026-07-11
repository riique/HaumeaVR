# P20 — Compatibilidade Meta Quest em PCVR

**Status:** planejado; hardware pendente  
**Prioridade:** crítica  
**Dependências:** P10, P13 inicial, P62 harness  
**Desbloqueia:** suporte declarado a Meta Quest

## Objetivo

Validar e suportar Meta Quest como headset PCVR do HaumeaVR usando OpenXR no Windows, sem transformar o core em produto dependente da Meta.

## Rotas suportadas

### Rota A — Meta Quest Link por USB

`Quest → USB Link → Meta Quest Link no Windows → runtime OpenXR Meta → HaumeaVR`

É a primeira rota de referência por reduzir variáveis de rede.

### Rota B — Air Link

`Quest → Wi-Fi → Meta Quest Link no Windows → runtime OpenXR Meta → HaumeaVR`

Usa o mesmo contrato OpenXR, mas métricas precisam separar encode/network/decode do overhead HaumeaVR.

### Rota C — runtime terceiro detectado

Virtual Desktop/VDXR ou outra rota só pode entrar na matriz com nome/versão próprios. Nunca agrupar seu resultado como “Meta runtime”. O launcher já encontrou um manifest Virtual Desktop em validação anterior, mas isso não prova hardware conectado nem compatibilidade futura.

## Dispositivos alvo

Prioridade:

1. Quest 3;
2. Quest 3S;
3. Quest 2;
4. Quest Pro quando disponível.

Não hardcodar feature por modelo. A lista apenas define compra/empréstimo e cobertura física.

## Preflight do launcher

1. Localizar manifests Meta registrados e validar que o arquivo existe.
2. Não escrever `ActiveRuntime`.
3. Aplicar o manifest selecionado apenas ao `uxrb_core.exe`.
4. Verificar que o aplicativo Meta Quest Link está executando.
5. Orientar o usuário a iniciar Link/Air Link no headset.
6. Rodar probe: loader → instance → system → D3D11 requirements.
7. Exibir erro distinto para:
   - app ausente;
   - manifest quebrado;
   - headset desconectado/adormecido;
   - runtime ocupado;
   - GPU/driver incompatível.
8. Salvar capability snapshot.

O launcher pode abrir uma página de instruções, mas não deve trocar runtime global nem opções Beta/Developer sem ação do usuário.

## Recursos obrigatórios

- OpenXR + `XR_KHR_D3D11_enable` para a fase atual;
- primary stereo;
- orientation + position tracking;
- grip/aim poses;
- trigger/squeeze/thumbstick/buttons;
- haptic simples;
- reference space local ou stage;
- projection e quad layer dentro do limite do runtime.

## Recursos opcionais

- `XR_EXT_hand_tracking`;
- `XR_EXT_hand_interaction`;
- simultaneidade mãos/controles;
- refresh rate;
- eye gaze no hardware aplicável;
- passthrough;
- foveation;
- composition layer depth;
- recursos Meta.

Presença na especificação ou no headset standalone não é suficiente. O snapshot do runtime Windows na rota ativa precisa confirmar e um probe funcional precisa passar.

## Bindings

- incluir o interaction profile Oculus Touch padronizado quando disponível;
- incluir generic controller para OpenXR 1.1 quando suportado;
- tratar Touch, Touch Plus e Touch Pro pelos profiles/extensions registrados no SDK fixado;
- ouvir mudança de interaction profile;
- exibir profile real no diagnóstico;
- não depender de nomes de dispositivo do SteamVR.

## Mãos

A Meta mantém amostras OpenXR que podem ser executadas no Windows via Quest Link e menciona “Developer Runtime Features”. Isso torna a rota testável, não garantida em toda versão/configuração.

Plano:

1. executar primeiro sample oficial de mãos com a mesma rota;
2. capturar extensões/system properties;
3. executar debug skeleton HaumeaVR;
4. testar mãos sem controle, controle sem mãos e troca;
5. registrar se o recurso exige developer setting;
6. se ausente, mostrar “mãos não expostas por esta rota” e usar controles;
7. não bloquear renderização/câmera por falta de mãos.

## Fases de validação

### Q1 — Diagnóstico

- manifest e runtime;
- system/headset;
- views/formats;
- spaces;
- profiles;
- extensões;
- sleep/wake.

### Q2 — Core visual

- cena de teste;
- Virtual Screen;
- quad menu;
- mirror;
- 30 minutos USB.

### Q3 — Input

- todos os botões/eixos/poses;
- háptica;
- recenter;
- foco do menu.

### Q4 — Air Link

- repetir Q1–Q3;
- medir jitter, drops, encode/network quando expostos;
- testar perda/reconexão de Wi-Fi.

### Q5 — Mãos opcionais

- sample oficial;
- debug skeleton;
- gestures P13;
- fallback.

### Q6 — Engine labs

- Unity Mono e IL2CPP labs;
- menus/cutscene/HUD;
- 60 minutos;
- frame pacing por refresh.

## Evidência

Para cada execução registrar:

- modelo/firmware;
- Meta Quest Link version;
- USB/Air Link;
- runtime manifest/hash/version;
- developer runtime features on/off;
- GPU/driver;
- resolution/refresh;
- capabilities;
- teste/resultados.

## Critérios de aceite

- pelo menos Quest 3/3S e um modelo adicional passam core visual + controles.
- USB e Air Link são linhas distintas na matriz.
- Nenhuma mudança no runtime global.
- Sleep/wake e desconexão não deixam processo travado.
- Hand tracking é anunciado somente na combinação testada.
- Virtual Screen, menu e um lab engine funcionam.
- Support bundle permite reproduzir o diagnóstico.

## Riscos

- **Atualização Meta quebra manifest/feature:** detectar versão e manter workaround table restrita.
- **Hand tracking muda com developer features:** capability + probe, controller fallback.
- **Air Link mascara overhead:** métricas separadas por rota.
- **Runtime selecionado mas app não conectado:** mensagens por estágio.

## Primeiros passos do próximo chat

1. Reparar/instalar Meta Quest Link somente com autorização do usuário.
2. Reinspecionar manifests e rodar `--diagnose --json`.
3. Conectar via USB e fechar Q1/Q2.
4. Adicionar controller profile tests.
5. Testar mãos apenas depois do sample oficial.

## Referências oficiais

- [Meta OpenXR SDK / Samples](https://github.com/meta-quest/Meta-OpenXR-SDK)
- [OpenXR interaction profiles](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html)
