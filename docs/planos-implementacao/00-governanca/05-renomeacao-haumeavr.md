# P05 — Renomeação integral para HaumeaVR

**Status:** nome oficial definido; migração interna planejada  
**Prioridade:** imediata para interface e distribuição  
**Dependências:** P01–P03  
**Desbloqueia:** identidade consistente em todos os releases

## Objetivo

Tornar **HaumeaVR** o único nome público do produto, migrando gradualmente os identificadores internos herdados de “UniversalXR Bridge” e “UXRB” sem perder perfis, backups, compatibilidade de protocolo ou capacidade de atualizar instalações antigas.

## Decisão normativa

- Nome público: **HaumeaVR**.
- Publisher provisório: **Haumea**.
- Nome curto na interface: **HaumeaVR**.
- “UniversalXR Bridge” deixa de ser nome de produto e passa a ser apenas o nome histórico do protótipo 0.1.0.
- Novos documentos, telas, pacotes e claims usam HaumeaVR.

## Inventário legado

Identificadores existentes:

- executável `UniversalXRBridge.Launcher.exe`;
- core `uxrb_core.exe`;
- assembly/namespaces `UniversalXRBridge.*`;
- namespaces C++/includes `uxrb`;
- magic IPC `UXRB`;
- flags `-uxrb-*`;
- mappings `Local\UXRB-*`;
- AppData `%LOCALAPPDATA%\UniversalXRBridge`;
- artefatos `UniversalXRBridge-<versão>-win-x64.zip`.

Não renomear todos simultaneamente sem compatibilidade.

## Nomes alvo

| Tipo | Nome alvo |
| --- | --- |
| Launcher | `HaumeaVR.Launcher.exe` |
| Core | `haumeavr_core.exe` |
| Pacote | `HaumeaVR-<versão>-win-x64.zip` |
| Instalador | `HaumeaVR-Setup-<versão>.exe` |
| AppData | `%LOCALAPPDATA%\HaumeaVR` |
| Assembly/root namespace | `HaumeaVR.*` |
| Namespace C++ | `haumeavr` |
| Includes | `haumeavr/*` |
| IPC v2 | magic/version documentada por P12; não alterar magic v1 |
| Flags novas | `-haumeavr-*`, aceitando aliases `-uxrb-*` durante migração |
| Named objects novos | `Local\HaumeaVR-*`, aceitando legado apenas quando sessão v1 negociar |

## Fases

### R1 — Marca pública

- README, título da janela, cabeçalho e mensagens;
- nome OpenXR application/engine;
- mirror window;
- Unity lab/logs;
- instruções;
- nome do ZIP e launcher publicado.

Identificadores internos permanecem compatíveis.

### R2 — Executáveis e dados

- assembly/output `HaumeaVR.Launcher.exe`;
- package validation;
- scripts e atalhos;
- novo AppData;
- na primeira abertura, detectar pasta antiga;
- copiar/migrar DB/config com journal e backup;
- logs antigos permanecem legíveis;
- rollback para build antiga não perde dados.

### R3 — Código e protocolo

- namespaces C#;
- targets/solution/project filenames;
- namespace/include C++;
- core executable;
- environment variables novas;
- aliases para flags/mappings v1;
- protocol v2 usa os contratos P12;
- adapter manifests declaram aliases/compatibility.

Executar em slices com build/test a cada família. Não fazer busca/substituição cega em magic, schema IDs ou hashes.

### R4 — Remoção do legado

Somente depois de:

- duas releases com migração;
- updater reconhece instalações antigas;
- profiles/adapters v1 têm mensagem de upgrade;
- matriz de rollback;
- documentação de EOL.

## Compatibilidade

- Magic IPC `UXRB` v1 permanece imutável.
- Schema/profile IDs antigos são migrados, não reinterpretados.
- `UXRB_CORE_PATH` continua alias temporário de `HAUMEAVR_CORE_PATH`.
- `-uxrb-enable` continua aceito nos labs/adapters v1.
- Named mapping v1 só é aceito pelo transport v1.
- Logs mostram “legacy identifier” em debug, nunca na marca principal.

## Testes

- pacote contém apenas nomes públicos HaumeaVR;
- launcher encontra core antigo/novo durante janela de migração;
- perfil/DB AppData antigo migra uma única vez;
- rollback abre dados antigos;
- flags aliases;
- protocol v1 golden vectors não mudam;
- v2 usa nomes novos;
- clean install não cria pasta antiga;
- uninstall remove somente dados escolhidos;
- nenhum texto público “UniversalXR Bridge/UXRB” fora de histórico/licenças.

## Critérios de aceite

- Todas as superfícies visíveis exibem HaumeaVR.
- Novos pacotes/instaladores começam com HaumeaVR.
- Dados existentes são preservados.
- Protocol v1 continua interoperável.
- Identificadores legados estão marcados e têm prazo de remoção.
- P64 e P67 usam exclusivamente o nome público.

## Primeiros passos do próximo chat

1. Concluir R1 e validar launcher/pacote.
2. Implementar migração AppData antes de mudar o diretório.
3. Renomear assembly/executável e ajustar scripts/testes.
4. Adiar namespace C++ e magic para P12/v2.
5. Registrar a versão em que cada alias poderá ser removido.

