# P03 — Método de execução por chat

**Status:** ativo  
**Prioridade:** obrigatório para handoff  
**Dependências:** P00–P02  
**Desbloqueia:** execução independente de qualquer plano

## Objetivo

Dar a um novo chat um procedimento repetível para transformar um único plano em código verificado sem depender da memória da conversa anterior e sem destruir alterações do usuário.

## Regra de escopo

Um chat deve assumir um plano primário e, no máximo, pré-requisitos pequenos indispensáveis. Se o plano depende de uma fundação grande ainda ausente, o chat implementa primeiro o plano bloqueador ou para com evidência; não cria um atalho incompatível.

## Preflight obrigatório

1. Ler:
   - `docs/planos-implementacao/README.md`;
   - P01 baseline;
   - P02 contratos;
   - plano escolhido;
   - planos listados em Dependências.
2. Inspecionar instruções locais como `AGENTS.md`, se existirem.
3. Executar:

```powershell
git status --short
rg --files
rg -n "TODO|FIXME|NotImplemented|stub" core shared graphics engines apps tests
```

4. Identificar arquivos já modificados pelo usuário e não sobrescrevê-los.
5. Revalidar dependências voláteis em documentação oficial.
6. Criar um plano de trabalho com no máximo uma etapa ativa.

## Antes de editar

Produzir uma nota curta no próprio plano ou em ADR contendo:

- estado observado;
- diferença para o baseline;
- decisão técnica;
- arquivos que serão tocados;
- testes planejados;
- risco e rollback.

Não criar um grande refactor “preparatório” sem teste que exija a mudança.

## Ciclo de implementação

1. Adicionar ou ajustar contrato/schema.
2. Escrever teste que falha pelo comportamento ausente.
3. Implementar o menor slice vertical completo.
4. Rodar teste focado.
5. Rodar suíte do módulo.
6. Integrar com o próximo processo/componente.
7. Testar falhas e rollback.
8. Rodar suíte ampla.
9. Atualizar documentação e evidências.

Um slice vertical é preferível a dez interfaces vazias. Exemplo para mãos: enumerar extensão, localizar joints, transportar uma mão no lab, renderizar debug skeleton e testar perda de tracking antes de criar gestos complexos.

## Evidência mínima por tipo

### Código sem hardware

- comando exato;
- resultado de build/test;
- golden file ou log redigido;
- teste de erro;
- lista de arquivos alterados.

### Hardware

- headset e firmware;
- conexão: USB, Wi-Fi, DisplayPort;
- software de conexão e versão;
- manifest/runtime OpenXR e versão;
- GPU/driver;
- refresh rate;
- modo HaumeaVR;
- duração do teste;
- frame timing e sintomas;
- captura/diagnóstico quando permitido.

### Jogo

- loja/AppID;
- versão/build ID;
- SHA-256 do executável e binários-chave;
- engine/backend;
- single-player/offline;
- mods existentes;
- cenas testadas;
- save usado e forma de restaurá-lo;
- resultado por render, cabeça, input, mãos, HUD, áudio e cutscene.

## Atualização do plano

Use os estados:

- **não iniciado**;
- **em implementação**;
- **bloqueado com evidência**;
- **implementado, hardware pendente**;
- **concluído**.

No fim, adicionar:

- data;
- commit ou árvore de trabalho;
- decisões;
- testes passados;
- testes não executados e motivo;
- limitações conhecidas;
- próximo plano desbloqueado.

Não alterar um checkbox para concluído quando somente o arquivo/classe existe.

## Portões de segurança

Parar antes de executar quando:

- anti-cheat detectado ou estado desconhecido em rota in-process;
- o título está incorreto ou o executável não foi identificado;
- o fingerprint do adapter não coincide;
- um instalador/update não tem assinatura/hash válido;
- backup não pode ser criado/verificado;
- a mudança exigiria driver, bypass, credenciais ou distribuição de arquivo do jogo;
- a única forma disponível viola licença confirmada.

Nesses casos, Virtual Screen externo continua sendo a rota permitida quando aplicável.

## Convenções de testes

Comandos-base:

```powershell
.\scripts\Configure.ps1
.\scripts\Build.ps1 -Preset windows-msvc-debug
.\scripts\Test.ps1 -Preset windows-msvc-debug
.\scripts\TestLauncher.ps1
dotnet list .\apps\launcher\UniversalXRBridge.Launcher.csproj package --vulnerable --include-transitive
```

Antes de release:

```powershell
.\scripts\Configure.ps1 -Fresh
.\scripts\Build.ps1 -Preset windows-msvc-release
.\scripts\Test.ps1 -Preset windows-msvc-release
.\scripts\Publish.ps1
.\scripts\Package.ps1
```

Adicionar sanitizers/validation layers quando o backend permitir, sem substituir teste real.

## Formato de handoff

O chat final deve responder:

1. resultado alcançado;
2. arquivos principais com links;
3. testes e hardware realmente executados;
4. limitação restante;
5. próximo plano recomendado.

Evitar despejar logs extensos. Manter logs completos em artefato local, com caminhos redigidos se forem compartilhados.

## Critérios de aceite

- Outro chat consegue reproduzir a mudança com os comandos registrados.
- O plano e o baseline não divergem silenciosamente.
- Falha e rollback foram testados.
- Nenhuma alteração externa não autorizada foi feita.
- A entrega distingue claramente “implementado” de “validado em hardware/jogo”.

## Primeiros passos do próximo chat

1. Escolher um único plano primário no índice.
2. Executar o preflight deste documento.
3. Registrar estado, arquivos, testes e rollback antes de editar.
4. Entregar um slice vertical e atualizar as evidências do plano.
