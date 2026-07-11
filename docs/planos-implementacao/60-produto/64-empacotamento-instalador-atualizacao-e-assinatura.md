# P64 — Empacotamento, instalador, atualização e assinatura

**Status:** ZIP portátil funcional; instalador/updater planejados  
**Prioridade:** alta para compartilhar com amigos  
**Dependências:** P61, P63, P66, build/test reprodutível  
**Desbloqueia:** distribuição sem source e P67

## Objetivo

Permitir enviar ao usuário final somente um instalador assinado ou um ZIP portátil, sem Visual Studio, compilador, source code, .NET global ou VC++ Redistributable.

## Estado atual

`scripts/Publish.ps1` já produz:

- launcher .NET self-contained;
- core e DLLs nativas;
- loader OpenXR;
- hashes;
- self-test do publicado.

Antes da renomeação P05, `scripts/Package.ps1` criou o pacote legado:

- `artifacts/distribution/UniversalXRBridge-0.1.0-win-x64.zip`;
- checksum;
- bundle sem arquivos de source/projeto;
- teste em extração limpa.

O empacotamento atual também já produz:

- `artifacts/distribution/HaumeaVR-0.1.0-win-x64.zip`;
- launcher público `HaumeaVR.Launcher.exe`;
- bundle autocontido sem source ou arquivos de projeto;
- checksums e teste em extração limpa.

O binário nativo ainda se chama internamente `uxrb_core.exe` durante a migração compatível definida em P05; ele não é uma segunda aplicação e é iniciado pelo launcher. O ZIP HaumeaVR já pode ser enviado a um amigo. O plano abaixo adiciona a experiência de instalador, assinatura e atualização.

## Não enviar um “compilador”

Compilador é necessário para transformar source em binário. O amigo deve receber binários publicados:

- instalador `.exe/.msi`;
- ou ZIP portátil;
- adapters oficiais;
- checksums/assinatura;
- instruções.

Nenhum source é necessário para executar.

## Formatos

### Portable ZIP

Manter permanentemente:

- útil para testes e usuários avançados;
- sem instalação;
- versão lado a lado;
- verificação SHA-256;
- instruções PT-BR/EN;
- updater opcional desligado por padrão em portable.

### Instalador principal

Decisão inicial: bundle EXE assinado + MSI por usuário, construído com WiX Toolset depois de verificar a versão estável/licença no momento da implementação.

Motivos:

- Win32/Avalonia/C++ self-contained;
- repair/uninstall;
- shortcuts/protocol/file associations opcionais;
- flexibilidade para adapter packs;
- elevação somente quando usuário escolher install machine-wide.

Default:

`%LOCALAPPDATA%\Programs\HaumeaVR`

Dados:

`%LOCALAPPDATA%\HaumeaVR`

Nunca instalar dentro da pasta de jogo.

### MSIX

Executar spike, não assumir. Avaliar:

- full-trust desktop;
- package identity;
- Store signing/update;
- install location imutável;
- interação com adapter deployment e updater;
- side-by-side/protocol handler.

Adotar apenas se os providers de mods e updates funcionarem sem hacks. MSI continua fallback.

## Conteúdo

Obrigatório:

- launcher;
- core backends estáveis;
- OpenXR loader;
- schemas/default profiles;
- notices/licenses;
- repair/uninstall;
- diagnostic self-test.

Opcional:

- experimental backends;
- engine adapter packs;
- game adapter packs;
- debug symbols separados;
- labs nunca no stable.

Não incluir:

- source;
- project files;
- compiler/SDK;
- game binaries/assets;
- unsigned community DLLs;
- secrets/signing keys.

## Installer UX

1. publisher/version/signature;
2. per-user default;
3. components;
4. install;
5. self-test;
6. detectar runtimes sem alterar global;
7. opção “abrir HaumeaVR”;
8. log redigido.

Não pedir admin se desnecessário. Não instalar serviço, driver ou startup automático.

## Repair/uninstall

- repair verifica hashes e repõe somente app;
- uninstall bloqueia/encerra HaumeaVR, não mata jogos sem consentimento;
- oferece remover ou manter profiles/logs/backups;
- remove shortcuts/protocol handlers;
- não remove runtime Meta/PICO/SteamVR;
- não remove mods de terceiros;
- P61 restaura adapters HaumeaVR antes de remover inventory;
- journal permite retomar.

## Code signing

Produção:

- Authenticode para EXE/MSI/DLLs oficiais;
- certificado de publisher confiável ou Azure Artifact Signing/Store;
- timestamp RFC3161;
- build server/CI acessa signer sem exportar private key;
- verificar assinatura depois do upload;
- dev certificate separado e nunca aceito pelo stable.

Assinatura reduz avisos evitáveis, mas reputação SmartScreen também depende de distribuição consistente.

## Update model

### Canais

- stable;
- beta;
- developer local.

### Metadata

- channel/version;
- package URL;
- size/SHA-256;
- Authenticode publisher/thumbprint policy;
- protocol/profile/adapter compatibility;
- minimum OS;
- release notes;
- rollout/expiry;
- signature.

Não inventar criptografia. Usar primitives/plataforma ou framework de atualização auditado após review.

### Fluxo

1. check HTTPS + metadata signature;
2. download para staging;
3. hash/signature/SBOM policy;
4. pedir fechamento;
5. updater separado;
6. atomic switch/installer transaction;
7. health check;
8. rollback se falhar;
9. manter uma versão anterior;
10. adapter/profile updates compatíveis e separados.

Nunca atualizar no meio de sessão/jogo.

## Supply chain

- dependencies pinned/locked;
- vulnerable package scan;
- SBOM CycloneDX/SPDX;
- third-party notices;
- reproducible build target;
- SHA256SUMS;
- provenance/CI run;
- malware scan;
- symbols privados/publicados separadamente;
- signing after tests.

## Build pipeline

```powershell
.\scripts\Configure.ps1 -Fresh
.\scripts\Build.ps1 -Preset windows-msvc-release
.\scripts\Test.ps1 -Preset windows-msvc-release
.\scripts\TestLauncher.ps1
.\scripts\Publish.ps1
.\scripts\Package.ps1
# Build installer
# Sign binaries and installer
# Verify signatures/checksums
# Install/repair/update/uninstall in clean VM
```

Adicionar scripts dedicados com parâmetros, nunca credenciais.

## Test matrix

- clean Windows 10/11 suportados;
- user sem admin;
- non-ASCII username/path;
- install/repair/uninstall;
- previous version update;
- failed download/hash/signature;
- disk full/locked files/reboot;
- portable side-by-side;
- no .NET/VC runtime/SDK;
- Meta/PICO/SteamVR detection;
- antivirus/SmartScreen observations;
- source absence;
- rollback.

## Critérios de aceite

- Amigo instala/abre sem compilador/source.
- Publisher/assinatura são verificáveis.
- Nenhuma mudança global OpenXR.
- Uninstall não apaga dados/mods de terceiros.
- Update falho volta à versão anterior.
- Pacote contém somente binários/licenças/perfis permitidos.
- Clean VM passa self-test e launcher.

## Passos que o amigo receberá

1. Instalar Meta Quest Link ou PICO Connect + SteamVR conforme headset.
2. Baixar `HaumeaVR-Setup-x.y.z.exe` do canal oficial.
3. Conferir publisher; executar.
4. Conectar/ativar PCVR.
5. Abrir HaumeaVR, rodar diagnóstico e adicionar o jogo.
6. Instalar adapter oficial pelo próprio launcher apenas para builds suportados.

## Primeiros passos do próximo chat

1. Manter ZIP e seus testes.
2. Criar installer spike per-user em branch/dir próprio.
3. Testar clean VM.
4. Integrar P61 uninstall/repair.
5. Só então adicionar assinatura/updater.

## Referências oficiais

- [Microsoft MSIX](https://learn.microsoft.com/en-us/windows/msix/)
- [MSIX signing guide](https://learn.microsoft.com/en-us/windows/msix/package/sign-msix-package-guide)
- [WiX Toolset documentation](https://docs.firegiant.com/wix/)
- [Windows Package Manager submission](https://learn.microsoft.com/en-us/windows/package-manager/package/)
