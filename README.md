# HaumeaVR

Leve jogos de tela plana para um ambiente OpenXR no Windows, começando por uma tela virtual segura e evoluindo apenas quando a integração do jogo permite.

O HaumeaVR é uma plataforma experimental para PCVR com launcher próprio, core nativo OpenXR e uma escada de compatibilidade honesta. O objetivo não é prometer “VR nativo em qualquer jogo”, mas oferecer um fallback amplo e preservar limites de segurança em integrações mais profundas.

## O que está implementado

### Virtual Screen

É o caminho principal para o usuário:

- captura de desktop ou janela por DXGI Desktop Duplication;
- tela plana ou curva renderizada no headset;
- ajuste de tamanho, distância e curvatura;
- rastreamento da cabeça, janela espelho e recentralização;
- ações de controles OpenXR;
- perfis locais para jogos e runtimes.

### Core e launcher

- loader oficial da Khronos e seleção de runtime por processo;
- correspondência do adaptador D3D11 pelo LUID informado pelo OpenXR;
- swapchains estereoscópicos, poses previstas e diagnóstico;
- launcher Avalonia com descoberta Steam, Epic e executáveis manuais;
- detecção de engine, backend de scripting, API gráfica e anti-cheat;
- perfis SQLite e IPC local autenticado.

### Integrações controladas

- bridge Unity 6000.3 Mono com câmeras por olho e plug-in nativo;
- transporte de poses e texturas entre processos com nome aleatório e versão;
- laboratório de compatibilidade Unity reproduzível para Mono e IL2CPP;
- observador D3D11 somente no processo atual e com ativação explícita;
- fundações de reconstrução Depth 3D.

## Limites atuais

Virtual Screen é o modo de uso geral. Os demais caminhos não devem ser interpretados como compatibilidade automática:

- **Unity Stereo** exige um player Mono controlado, já compilado com os dois módulos do HaumeaVR;
- **Depth 3D** e o observador D3D11 são fundações técnicas, não recursos comerciais habilitados para qualquer jogo;
- **Geometry Stereo**, integração Unreal, IL2CPP comercial e adapters específicos ainda exigem trabalho;
- o projeto não adiciona mãos, colisão ou interação física a um jogo que não conheça esses conceitos;
- jogos com anti-cheat não recebem integração dentro do processo; use apenas a captura externa;
- o repositório não contém um injetor genérico nem bypass de integridade.

O plano de produto completo está em [Plano.md](Plano.md). A sequência de implementação e os contratos técnicos ficam em [docs/planos-implementacao/README.md](docs/planos-implementacao/README.md).

## Segurança por arquitetura

O HaumeaVR não altera o runtime OpenXR global. A seleção do launcher é transmitida somente ao processo filho por `XR_RUNTIME_JSON`.

Outras proteções:

- IPC aceita clientes locais e exige token aleatório de 256 bits;
- o token da sessão não é persistido em perfil ou SQLite;
- o transporte Unity valida PID, nome, formato e dimensões das texturas;
- recursos D3D11 compartilhados usam keyed mutex;
- o observador exige `UXRB_D3D11_HOOK_ENABLE=1`;
- módulos de anti-cheat conhecidos causam bloqueio imediato;
- nenhum driver, serviço de kernel ou helper de injeção é instalado.

Falhar em um adapter não deve impedir o jogo limpo de abrir. Virtual Screen e execução sem HaumeaVR são os caminhos de retorno.

## Usar um pacote portátil

1. Extraia todo o ZIP para uma pasta local.
2. Conecte o headset e ative o runtime PCVR desejado.
3. Execute `HaumeaVR.Launcher.exe`.
4. Selecione um manifesto OpenXR válido.
5. Adicione o jogo, clique em **Analisar** e use **Virtual Screen**.

Não execute o launcher de dentro do ZIP. O pacote publicado é autocontido e inclui launcher, core e loader OpenXR; não exige .NET ou Visual C++ Redistributable instalados globalmente.

Os logs ficam em:

```text
%LOCALAPPDATA%\HaumeaVR\logs
```

Dados antigos na pasta `UniversalXRBridge` são copiados na primeira execução para preservar perfis.

## Requisitos para desenvolvimento

- Windows x64;
- Visual Studio Build Tools com MSVC e SDK do Windows;
- CMake 3.28 ou superior;
- Ninja;
- PowerShell;
- .NET SDK `10.0.301`, definido em `global.json`;
- acesso à internet na primeira configuração das dependências CMake.

O laboratório Unity requer exatamente Unity `6000.3.19f1`.

## Compilar e testar

```powershell
.\scripts\Configure.ps1 -Fresh
.\scripts\Build.ps1 -Preset windows-msvc-debug
.\scripts\Test.ps1 -Preset windows-msvc-debug
.\scripts\TestLauncher.ps1
```

Repita com `windows-msvc-release` antes de publicar.

Verificações nativas úteis:

```powershell
.\build\windows-msvc-debug\bin\uxrb_core.exe --self-test
.\build\windows-msvc-debug\bin\uxrb_core.exe --capture-self-test
.\build\windows-msvc-debug\bin\uxrb_core.exe --diagnose
```

Para compilar o laboratório:

```powershell
.\scripts\BuildUnityLab.ps1 -Backend Mono
.\scripts\BuildUnityLab.ps1 -Backend IL2CPP
```

## Publicar

```powershell
.\scripts\Publish.ps1
.\scripts\Package.ps1
```

Saídas:

```text
artifacts\launcher\win-x64\HaumeaVR.Launcher.exe
artifacts\distribution\HaumeaVR-0.1.0-win-x64.zip
```

`Publish.ps1` executa o self-test do core já publicado e gera `SHA256SUMS.txt`. `Package.ps1` valida a presença dos arquivos essenciais, cria o ZIP portátil e grava seu SHA-256.

## Estado da validação

O relatório versionado em [docs/validation-2026-07-10.md](docs/validation-2026-07-10.md) registra builds Debug/Release, testes nativos, testes do launcher e checks do pacote daquela revisão.

Naquela validação:

- os testes automatizados e os self-tests publicados passaram;
- o runtime Virtual Desktop carregou por override local;
- não havia headset ativo para concluir observação física;
- o Unity Editor exigido não estava instalado;
- a rota final em headset e o laboratório Unity permaneceram pendentes.

Esse relatório é evidência histórica de uma revisão específica. Mudanças posteriores ou outra combinação de headset, runtime, GPU e driver precisam de nova validação.

## Arquitetura

```text
Launcher Avalonia
  └── IPC local autenticado
      └── uxrb_core
          ├── OpenXR, D3D11, poses e swapchains
          ├── Virtual Screen por Desktop Duplication
          ├── transporte Unity controlado
          └── fundação Depth 3D
```

Detalhes de processos e transportes estão em [docs/architecture.md](docs/architecture.md). O protocolo compartilhado está em [shared/protocol/protocol.md](shared/protocol/protocol.md).

## Contribuição

Antes de propor código:

1. leia os contratos no pacote de planos;
2. mantenha a seleção de runtime limitada ao processo;
3. não enfraqueça bloqueios de anti-cheat;
4. preserve o fallback para Virtual Screen;
5. inclua testes reproduzíveis e descreva quais verificações físicas continuam pendentes.

Não publique DLLs de terceiros sem revisar licença, procedência e integridade.

## Licença

O código do HaumeaVR é distribuído sob a [Licença MIT](LICENSE). Dependências e componentes mantêm suas próprias licenças; consulte [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
