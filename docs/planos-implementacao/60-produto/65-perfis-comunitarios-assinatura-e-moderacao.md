# P65 — Perfis comunitários, assinatura e moderação

**Status:** futuro  
**Prioridade:** média após perfil v2 estável  
**Dependências:** P14, P22, P60–P64, P66  
**Desbloqueia:** escala comunitária segura

## Objetivo

Permitir que a comunidade compartilhe ajustes e bindings sem transformar um profile repository em distribuição de código/injeção.

## Unidade de publicação

`.uxrbprofile` é um ZIP canônico contendo:

- manifest;
- profile JSON;
- assets próprios permitidos e pequenos;
- test coverage;
- known issues;
- changelog;
- signature;
- license.

Proibido:

- DLL/exe/script;
- game asset;
- dump/save;
- memory address/byte patch;
- shell command;
- URL de payload executável;
- arquivo fora de allowlist.

## Trust levels

- **Local:** criado pelo usuário, unsigned, só máquina local.
- **Community unreviewed:** import manual com warning; não auto-update nem L4.
- **Reviewed:** schema/security/content review.
- **Official:** publisher HaumeaVR, evidence e adapter compatibility.
- **Revoked:** não ativar; oferecer fallback.

Stable repository distribui Reviewed/Official. Unreviewed não aparece por padrão.

## Review pipeline

1. schema/lint;
2. canonicalization/signature;
3. archive/path/size scan;
4. safety monotonicity;
5. adapter endpoint allowlist;
6. fingerprint;
7. evidence/test matrix;
8. license/assets;
9. human review;
10. sign/publish.

Mudança de fingerprint ou capability claim reinicia revisão relevante.

## Repository metadata

- package ID/version;
- game/build;
- author key/display;
- trust;
- capabilities/status;
- dependencies;
- compatible app/adapter/protocol;
- hashes/signature;
- moderation/revocation;
- download size;
- release date.

## Community workflow

- export pelo Profile Studio;
- remover paths/user overrides;
- rodar tests/lint;
- preview;
- submit;
- review;
- feedback;
- publish.

O usuário nunca precisa fornecer game files.

## Conflicts

- profile layering P60;
- diff antes de aplicar;
- safety ganha;
- two profiles with same role require choice;
- local override preserved;
- rollback to previous profile;
- adapter mismatch blocks endpoints.

## Moderação

Razões:

- malware/code payload;
- copyrighted assets;
- false claims;
- dangerous settings;
- harassment/naming;
- broken build;
- license violation.

Manter audit trail, appeal e revocation list. Não expor dados pessoais do autor.

## Atualização

- signed metadata;
- pin por game build;
- no auto-enable after major capability change;
- rollback;
- offline cache;
- repository outage não bloqueia local profiles.

## Testes

- malicious archives;
- zip bomb/path traversal;
- signature/revocation;
- huge assets;
- unsafe merge;
- fake evidence;
- adapter endpoint not allowed;
- update/downgrade;
- offline.

## Critérios de aceite

- Nenhum profile público executa código.
- L4 depende de adapter oficial instalado.
- Review é reproduzível.
- Revocation funciona offline com last-known list.
- User local data é removido no export.
- Claims apontam à matriz.

## Primeiros passos do próximo chat

1. Fechar P60 canonical format.
2. Criar pack/unpack/lint local.
3. Implementar malicious fixtures.
4. Definir trust UI.
5. Só criar serviço remoto depois do pipeline local.
