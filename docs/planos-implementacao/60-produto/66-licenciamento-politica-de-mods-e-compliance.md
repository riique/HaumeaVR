# P66 — Licenciamento, política de mods e compliance

**Status:** contínuo; revisão necessária antes de distribuição  
**Prioridade:** bloqueador de adapters terceiros/release  
**Dependências:** P14, P61, P64–P65  
**Desbloqueia:** P67 e distribuição pública

## Objetivo

Garantir que o pacote respeite licenças de dependências, termos de modding dos jogos, propriedade intelectual e privacidade. Este plano organiza revisão técnica; questões duvidosas devem receber análise jurídica adequada antes de publicação ampla.

## Inventário de dependências

Gerar SBOM e registrar:

- OpenXR SDK/loader;
- Detours;
- Avalonia;
- .NET runtime;
- SQLite/native bundle;
- JSON libraries;
- Unity native headers/sample inputs;
- RmlUi se adotado;
- BepInEx/Il2CppInterop se adotados;
- WiX/installer tooling;
- updater/crash libraries;
- shaders/assets/fonts/icons.

Para cada:

- versão/commit;
- licença;
- source URL;
- notices;
- redistribuição binária/source requirements;
- modification notices;
- patents/trademarks;
- update cadence.

Não copiar texto de licença por memória; usar arquivo oficial da versão fixada.

## Third-party notices

`THIRD_PARTY_NOTICES.md` deve ser gerado/revisado no release e incluído no instalador/ZIP. Licenças que exigem arquivo separado entram completas.

## Jogos

Por game adapter:

- EULA e mod policy;
- mecanismo oficial;
- redistribuição permitida;
- uso de nome/logo;
- online/anti-cheat;
- assets/animações;
- reverse engineering restrictions por jurisdição;
- disclaimer de não afiliação.

Não distribuir:

- executáveis/DLLs/metadata do jogo;
- asset extraído;
- save do usuário;
- keys;
- patched binary.

Preferir adapter que resolve APIs em runtime e pacote com assets HaumeaVR próprios.

## Cyberpunk/REDmod

- usar página, documentação e licença oficial;
- separar o que REDmod permite do que um framework comunitário oferece;
- qualquer host comunitário recebe review próprio;
- pacote HaumeaVR não inclui conteúdo CDPR.

## Unity/BepInEx

- Unity docs/headers conforme licença;
- não distribuir Unity Editor/runtime além do que o jogo já inclui;
- revisar BepInEx 6 pre-release e Il2CppInterop antes de bundle;
- conservar notices/source obligations;
- provider substituível.

## Trademarks

- “Meta Quest”, “PICO”, “SteamVR”, “Unity”, “Unreal”, “Cyberpunk 2077” usados descritivamente;
- não usar logos oficiais sem permissão;
- não sugerir certificação/parceria;
- compatibility list inclui disclaimer.

## Privacidade

P63:

- privacy policy legível;
- local-first;
- consentimento para support bundle/dump;
- retention/delete;
- terceiros/endpoints declarados;
- menor coleta;
- idade/conta não necessária.

## Segurança e responsible disclosure

- SECURITY.md;
- canal de reporte;
- supported versions;
- embargo razoável;
- CVE/dependency response;
- revoked keys/adapters;
- não publicar exploit/anti-cheat bypass.

## Export/crypto/signing

Documentar uso de criptografia de assinatura/HTTPS e requisitos aplicáveis ao canal de distribuição. Não armazenar private key no repo ou pacote.

## Checklist por release

- SBOM;
- notices;
- licenses;
- source obligations;
- game policy;
- trademarks;
- privacy;
- security contact;
- signatures;
- no proprietary files;
- attribution UI;
- repository profile licenses.

## Critérios de aceite

- Todo binário tem origem/licença.
- Bundle contém notices exigidos.
- Adapters têm game policy record.
- Nenhum arquivo do jogo é empacotado.
- Claims não sugerem afiliação.
- Privacy/security docs existem.
- Dependência sem licença clara bloqueia release.

## Primeiros passos do próximo chat

1. Gerar dependency inventory real.
2. Conferir arquivos LICENSE oficiais.
3. Atualizar notices.
4. Criar compliance record MiSide/Cyberpunk.
5. Revisar cada nova dependência antes de adicioná-la.
