# Contexto do repositório

Estado consolidado em 2026-10-12. Resultados identificam as verificações
realizadas; conferir código e executar novamente após alterações.

## Objetivo e estrutura

Protótipo de pesquisa de comunicação P2P através de NAT, com CHOWNAT e túnel
TCP sobre UDP/QUIC. Integração QUIC via OpenSSL está implementada; compatibilidade
WebRTC e transporte QUIC próprio completo continuam como objetivos de pesquisa.

| Caminho | Responsabilidade |
| --- | --- |
| `source/common/utils/` | CLI, debug, endereços e temporização |
| `source/libs/protocols/udp_conn/` | API comum, sessões UDP/TCP e laço `select()` |
| `source/libs/protocols/chownat/` | Hole punching, controle e túnel CHOWNAT |
| `source/libs/protocols/ossl_quic/` | Backend QUIC OpenSSL |
| `source/libs/protocols/quic_like/` | Esboço de transporte próprio, majoritariamente comentado |
| `source/tests/` | Experimentos WebRTC/OpenSSL; mbedTLS histórico fora das classes atuais |
| `cmake_modules/` | Descoberta de bibliotecas, classes de teste e metadados `.meta.cmake` |
| `sim/patchbay/` | Runner Rust/Patchbay 0.7.0 e regressões em namespaces Linux |
| `sim/gns3/` | Topologias e referências do laboratório manual |
| `chownat/`, `deprecated/` | Referência original e material histórico |

`udp_conn_stream.h` propõe uma API de streams; sua existência não comprova
multiplexação implementada. Certificados de laboratório ficam em
`source/common/certs/`; os peers QUIC usam `source/` como diretório de trabalho.

## Build e testes

O preset `all` exige CMake >= 3.21, usa `build/all/`, compila as classes WebRTC e
OpenSSL e habilita todos os cenários CTest. Exemplos cliente/servidor também são
compilados, mas não são testes autônomos registrados.

```sh
cmake --preset all
cmake --build --preset all -j2
ctest --preset all
```

CTest não recompila. Dependências, filtros, topologia e logs estão no
[guia Patchbay](../sim/patchbay/README.md). O build direto por `source/` continua
suportado, sem a suíte de rede. OpenSSL é obrigatório no build C atual;
`BUILD_NETWORK_TESTS=ON` acrescenta Rust e dependências Linux.

`common_functions.cmake` expõe `check_deps(DEPENDENCIES ...)` para localizar
executáveis e exportar `<NOME>_EXECUTABLE`; aceita `HINTS` e `HELP_MESSAGE`
opcionais. Usada pelo Patchbay. Configuração com preset `all` e caminhos de
sucesso/erro da função verificados em 2026-10-11, sem reexecutar testes de rede.

O registro automático fica em `sim/patchbay/program_tests.cmake`, carregado
pelo CMake do Patchbay. Consulta propriedades `PATCHBAY_MODE` e `PATCHBAY_TIMEOUT`
nos targets já criados, declaradas em `source/tests/*/test.meta.cmake`; nome e
labels opcionais. Os cinco executáveis WebRTC têm essas propriedades.
A sondagem antiga de ambiente e sua pasta tests foram removidas a pedido
em2026-10-12; não participavam do runner/CTest. A função comum
`add_program_test` recebe `NAME`, `TARGET` e `RUNNER` obrigatórios e argumentos
opcionais `ARGS`, `LABELS`, `TIMEOUT`, `WORKING_DIRECTORY` e `RUN_SERIAL`.
O programa é o primeiro argumento do runner, seguido de `ARGS`. O runner é
passado explicitamente, sem variável global de caminho. Registro condicionado
a `BUILD_TESTING`. Em 2026-10-11: build e suíte `all` passaram 8/8 em 7,64 s;
verificados executor independente com espaços nos caminhos/argumentos e zero
testes com `BUILD_TESTING=OFF`. Label ativa: `program`; a label `environment` era usada pela sondagem removida.

## Base de rede adotada

- Dois peers; UDP default 2222, entrada TCP cliente 44444 e echo servidor 1234.
- Topologia única em `sim/patchbay/src/topology.rs`: peer_a192.168.80.20/24
  atrás de A e peer_b192.168.80.30/24 atrás de B. LANs isoladas compartilham
  CIDR192.168.80.0/24; ISP fornece10.0.0.0/24 para WANs automáticas A/B.
  IPs dos peers declarados em IfaceConfig. Papéis -c/-s preservados.
- NAT: endpoints públicos conhecidos; mapa começa vazio e é criado no envio,
  preservando a porta de origem UDP (`snat ... :udp sport`). Os peers usam 2222.
- Preset Moderate com APDF nativo e regras próprias de mapeamento, sem zonas.
  opened_ports/inbound_guard restaurados após experimento de remoção falhar,
  sem nova validação a pedido do usuário. Retorno DNAT.
- ICE e descoberta de endpoints/portas públicas diferentes ficam para o futuro.
- O script manual foi aposentado em `sim/patchbay/legacy/netns_virtual_env.sh`.

Essa base não valida descoberta de endpoints nem NAT residencial restritivo.
Os nomes de cenário `gns3`, `public` e `nat` foram removidos do runner.
A topologia é compartilhada pelos tipos de verificação `tcp` e `udp`. `environment` foi retirado do runner por enquanto. Laboratório GNS3 manual permanece em `sim/gns3/`.
Configuração atual não inclui teste sem NAT.

## Diagnósticos e correções confirmados

| Problema | Evidência e estado atual |
| --- | --- |
| NAT inicial alterava a porta | Captura mostrou saída em 12289. Laboratório corrigido para 2222; falhas antigas não demonstram defeito do protocolo. |
| CHOWNAT não encaminhava o TCP | strace/GDB confirmaram retorno antecipado em `chownat_udp_send()` com `tcp_session` e `buf` presentes. Guarda de envio removida; guarda de recepção mantida. Cinco cenários CHOWNAT passaram. |
| QUIC demorava ou deixava um bloco pendente | Observado `SSL_pending() > 0` com socket UDP vazio (`FIONREAD=0`). Laço esperava novos datagramas sem drenar dados internos. Callback opcional `process_events` agora processa eventos e drena dados antes do `select()`. |

O backend QUIC voltou a consultar `SSL_get_event_timeout()`. Seu timer não
incrementa o contador de keep-alive CHOWNAT. `get_timeout()` permanece uma
consulta; a drenagem ocorre no callback. Diminuir o timeout fixo isoladamente
não corrigia o buffer abandonado. Idle timeout negociado de 30 s era compatível
com PING em cerca de 15 s; não foi identificada a origem de cada timer observado.

## Contrato atual e validação

Suíte registrada: os cinco executáveis WebRTC (CHOWNAT TCP/UDP/UDP close e
QUIC TCP/UDP). Após tornar o QUIC UDP finito, registro automático e suíte:
5/5 em 13,37 s; QUIC UDP 1,11 s. O callback antes travava em `while(1)`; agora
valida três rodadas com framing newline, conteúdo e sequência, shutdown completo
e propagação de erros na saída. Harness temporário verificou fragmentação,
agrupamento de mensagens e sequência inválida; payload errado em execução NAT
foi rejeitado pelo runner. QUIC TCP passou em 6,24 s nesta mesma execução com
helper antigo de dupla consulta ativo; variava de ~1,3 a ~6,2 s. Causa depois
isolada no shutdown e ordem corrigida (ver seção abaixo).
Topologia única NAT, peers e
argumentos em `topology.rs`; echo e verificações em `sim/patchbay/src/patchbay.rs`.
Main coordena via coleção de devices, sem campos fixos client/server. Sondagem
antiga de ambiente removida a pedido, sem alterar a suíte ativa. Artefatos `topology.txt`, `test.txt`
e `result.txt` registram a rede e a execução.

UDP C valida três rodadas request/response (conteúdo e sequência), cliente
inicia disconnect e ambos retornam zero somente com troca completa e sessão
encerrada. Payload inválido em cópia temporária produziu código 1 no runner.
TCP exige 16 KiB íntegros, probe/echo concluídos e peers com saída zero.
O runner aguarda UDP por 10 s; TCP tem 55 s de transferência e 10 s para
encerramento. Timeout/sinal/código não zero reprovam o teste. A limpeza dos
restantes nunca é usada como critério de sucesso.

Fechamento QUIC corrigido com autorização em 2026-10-11:

- Diagnóstico em scratch confirmou `SSL_shutdown() == 0` ignorado pelo cliente
  antes de SSL_free; ao concluir esse shutdown, o servidor reconhecia fechamento
  remoto no OpenSSL, mas mantinha `data->closed == 0` e o TCP do echo aberto.
- `finish_shutdown` aguarda retorno 1 usando `select` (antes `poll`), timers OpenSSL e
  limite monotônico total de 5 s; trata WANT_READ/WRITE e erros reais, sem polling
  fixo de 10 ms. Consulta novamente após eventos antes de bloquear.
- `process_events` verifica close-info mesmo sem dados de aplicação pendentes.
  Código QUIC zero é considerado normal; código não zero registra erro.
- `close_status` conserva o resultado separado de `closed`. Deinit libera SSL e
  sockets mesmo em falha, zera ponteiros/invalida descritores e retorna o erro.
  O executável TCP QUIC propaga erro de conexão ou deinit como EXIT_FAILURE.
- Validação histórica com três testes: **3/3 em 3,88 s**, QUIC 1,31 s; payload,
  status dos peers e conclusão do echo validados. Build e diff-check passaram.
- Fixtures em `/tmp/quic-shutdown-validation/` confirmaram erro remoto real
  (SSL_shutdown_ex com código 7) propagado como saída 1, e API sem progresso
  simulada no cliente encerrada pelo limite de 5 s, com erro/limpeza propagados.

Isso supera as falhas anteriores de encerramento na configuração testada.
Alocação incremental de portas em colisão continua não implementada; os testes
assumem preservação da origem. Evidências de scratch são temporárias.

## Shutdown QUIC atual (2026-10-12)

Usuário aplicou ordem handle_events antes de shutdown e removeu laço de duas
attempts. A pedido, espera do helper trocada de poll para select, com read_fds,
write_fds somente se SSL_net_write_desired, timeval reconstruído a cada rodada
/ EINTR e validação fd < FD_SETSIZE. Limite monotônico5s e menor timer mantidos.
Build all passou; testes QUIC TCP1,34s eUDP1,24s passaram (2/2). Fixtures de
falha/timeout não reexecutadas nesta mudança. Isso supera o diagnóstico histórico abaixo. Comentários explicativos foram
escritos pelo usuário; preservados. Git limpo antes da atualização de memória
em2026-10-12; nenhum commit feito pelo agente.

## Variação de tempo QUIC: diagnóstico confirmado em scratchpad

Em 2026-10-11, instrumentação do helper reproduziu TCP em 1,234 / 1,321 /
6,224 s. Na execução lenta, aos 105 ms o último SSL_handle_events mudou o
shutdown para flags3; SSL_get_event_timeout passou a infinito. O helper não
consultou novamente SSL_shutdown, entrou em poll por4895ms (restante do limite)
e só aos5005ms recebeu retorno1. Portanto a espera extra é do helper, após a
transferência, e o teste passa porque consulta shutdown antes de verificar o
prazo vencido. Duas tentativas antes do poll reduzem a janela, mas não a eliminam.

Proposta testada somente em cópias: processar SSL_handle_events **antes** da
consulta SSL_shutdown e decidir a espera pelo resultado atualizado. Sem logs
extras, três TCP passaram em1,240/1,348/1,334s e três UDP em1,033/1,099/1,076s.
Também passaram cinco repetições instrumentadas de cada modo. O UDP usa o mesmo
helper, compatível com os6,03s relatados pelo usuário; oito reproduções UDP
instrumentadas ficaram rápidas, sem reproduzir a espera longa nesse modo.
Hashes dos fontes ativos inalterados. Fontes/logs/patch revisável em
`/tmp/quic-timing-analysis/`; ordem posteriormente aplicada pelo usuário e
espera substituída por select, conforme estado atual acima.

## Pendências para tarefas futuras

Topologia atualizada em2026-10-12: nomes peer_a/peer_b em devices, processos e
verificação; ISP downstream10.0.0.0/24, LANs192.168.80.0/24. Build e suíte5/5
em6,71s; topology.txt confirmou WAN A10.0.0.2/B10.0.0.3. ISP continua roteador
Public sem serviço signaling/STUN. Imagem topology.png antiga não atualizada.

Experimento de remoção (2026-10-12), solicitado pelo usuário: removidos opened_ports,
inbound_guard e atualização do conjunto, mantendo mapeamento e APDF Moderate.
Build passou; suíte falhou5/5 em129,83s (CHOWNAT TCP reset41,63s, três UDP
expiraram ~10,8s, QUIC TCP transferência55,77s). Capturas TCP mostram2222
preservada, mas handshake incompleto. Guarda e conjunto restaurados posteriormente a pedido, sem build/testes
novos. Código ativo voltou à base anterior; programas C não alterados.
Correção da explicação anterior: opened_ports não implementa APDF completo;
guarda raw atua antes do conntrack, APDF nativo em forward apósDNAT. Com mapa
vazio, pacote para IP WAN pode seguir input, sem passar pelo APDF forward.
Hipótese: registro conntrack/NAT antes de haver mapa mantém caminho incorreto;
ainda não confirmada por captura de conntrack/nft trace. Não afirmar causa
exata nem redundância da guarda com base apenas no resultado da suíte.

NAT Moderate validado em 2026-10-12: removidas somente as duas regras de
zonas conntrack, mantendo SNAT por porta de origem, mapa DNAT, opened_ports e
APDF nativo. Antes, UDP falhava em10,67s com zonas1/2. Após build all, suíte5/5
em6,75s; repetição `ctest --preset all --repeat until-fail:3` passou15/15
execuções em20,29s. Capturas finais dos cinco testes mostram somente endpoints
198.18.1.2:2222 e198.18.1.3:2222, sem alteração de porta observada. Isso valida
a topologia atual, não exclui colisões em outras topologias ou múltiplos peers.

ISP adicionado pelo usuário, corrigido em 2026-10-12: a interface WAN é
resolvida por `wan_interface()` usando `uplink_ip()` e `interfaces()`, tanto
nas regras NAT quanto no tcpdump. ISP incluído em `Topology.routers` para resumo
/capturas; NAT próprio aplicado somente a A/B. Build `all` concluído e suíte
5/5 em 6,79 s; CHOWNAT UDP close 1,39 s após o usuário remover `usleep`.
Artefatos confirmam A/B em 198.18.1.2/.3, regras em `wan`, ISP em 198.18.0.10.
ISP Public não acrescenta NAT; A/B compartilham sua bridge downstream. Captura
na WAN `ix` do ISP não observa esse tráfego entre filhos (zero pacotes nesta
execução); para inspecioná-lo, usar bridge downstream ou interface `any`.

CHOWNAT UDP close (~3,44 s): inspeção do teste confirmou `usleep(100000)`
em cada callback de recepção dos dois peers; cliente desconecta ao receber
20 mensagens. Captura existente em `build/all/sim/patchbay/artifacts/` mostrou
~2,10 s entre handshake/dados iniciais e último pacote de fechamento. O restante
do tempo CTest inclui preparação/limpeza do laboratório. Não é evidência de
timeout de 3 s no protocolo. Análise de código e artefatos, sem nova execução.
Esse teste ainda imprime erro de `udp_connection`, ignora retorno de deinit e
retorna zero; reforçar validação de sucesso em tarefa própria.

Itens observados, sem auditoria completa nem correção implícita:

- Cobrir perda/reordenação, atraso, reconexão, falha de peer, sessões prolongadas
  e encerramento gracioso; revisar backpressure, escritas parciais e `WANT_WRITE`.
- Revisar retornos ignorados de envio/keep-alive e `exit()` em caminhos da biblioteca.
- Definir autenticação dos peers: cliente QUIC atualmente usa `SSL_VERIFY_NONE`.
- Rever `utils`: portas com `atoi`, interrupções em `nanosleep`, semântica de
  `DEBUG_PRINT` com `NDEBUG` e portabilidade de macros/feature defines.
- Rever proteção de inclusão e completude de `udp_conn_stream.h` ao trabalhar streams.

Evidências de diagnóstico locais, ignoradas pelo Git, podem existir em
`sim/patchbay/artifacts/focused-{chownat,quic}/`. Scratchpads em `/tmp` são
temporários; não são necessários para reproduzir a suíte atual.

## Entrega de payload centralizada (2026-10-22)

A pedido do usuário, backends CHOWNAT e OpenSSL QUIC agora retornam payload e
quantidade por udp_recv. A udp_conn encaminha ao TCP quando há túnel e chama a
callback de recepção em ambos os modos, usando recv_reason para preservar os
códigos existentes. CHOWNAT mantém controle/retransmissão e callback LOST_DATA;
QUIC mantém eventos/fechamento e expõe pending_data para drenagem pela udp_conn
antes do select. O helper comum trata escritas TCP parciais/EINTR; segue bloqueante.

Configure/build all e regressões no temporário passaram: 5/5 em 7,13 s, com
namespaces fora do sandbox (sandbox retornou EPERM antes dos peers). Patch
aplicado ao original e quatro arquivos comparados byte a byte com os validados;
git diff --check passou. Sem nova execução de rede no original. udp_conn_recv
explícito agora apenas retorna dados; callback ocorre no laço de entrega e zero
bytes não gera callback. Perda/reordenação, backpressure e reentrância não foram
validados adicionalmente. Patch/logs temporários em /tmp/udp-delivery-prototype.patch,
/tmp/udp-delivery-tests.log e /tmp/udp-delivery-prototype/VALIDATION.md.

### Ajuste solicitado em 2026-10-22

Removido recv_reason. Callbacks de recepção voltaram ao final da leitura de
payload em chownat_udp_recv e ossl_quic_udp_recv, inclusive em modo túnel.
Encaminhamento TCP continua na udp_conn, após retorno do backend; portanto a
callback agora ocorre antes do envio TCP. udp_conn_recv explícito também chama
a callback novamente. Isto substitui a descrição anterior de callback no laço.

Verificação após ajuste: build all, git diff --check e CTest all passaram
(5/5 em 7,21 s no repositório original, fora do sandbox para namespaces).

## Prova ICE → CHOWNAT (2026-10-25; sem integração no código ativo)

Experimento isolado com libnice 0.1.21 executado e preservado integralmente em
[docs/experiments/ice-handoff-2026-10-25/](experiments/ice-handoff-2026-10-25/README-PRESERVATION.md),
a pedido do usuário para revisão futura. Inclui relatório `VALIDATION.md`,
patch, fontes, dependências extraídas, binários, cache e logs; cópia comparada
por SHA-256. Pasta `docs/` é ignorada pelo Git, sem commit. Não depender só de
`/tmp/ice-handoff-test`; scripts mantêm esse caminho, ver restauração na cópia.

Launcher por peer conclui ICE, duplica FD selecionado, libera agente e inicia
CHOWNAT por exec com o mesmo socket. Cópia aceita socket já bound e pula hole
punching próprio. UDP validou três rodadas e fechamento com peers zero. TCP
validou 32 KiB divididos antes/depois de 20 s sem payload, NAT UDP timeout 8 s
e keep-alives originais de 5 s; controle sem keep-alives falhou após pausa.

Dois cenários: laboratório EIM/APDF com tradução 2222→34990 e 3333→36101 por
XOR e mapa dinâmico; cópia do runner Patchbay com Moderate nativo, sem bloco
personalizado de NAT/guarda, passou UDP e TCP. No Patchbay portas naturalmente
preservadas: mapa nativo com remapeamento não validado. Regras originais não
alteradas; hashes de 265 arquivos source/topologia confirmaram zero mudanças.

QUIC, TURN, IPv6, colisões, múltiplos hosts, perda/reordenação e sessões longas
não testados. Não é implementação integrada nem decisão definitiva de API.
Algumas capturas recusaram limpeza por AppArmor; script e detalhes preservados.
Conferir pendência ao retomar, sem assumir processos encerrados.

## Comparação de hole punching/ICE (2026-10-26; somente cópia temporária)

Experimento e patch revisável preservados em
[hole-punch-select-2026-10-26/FINAL-REPORT.md](experiments/hole-punch-select-2026-10-26/FINAL-REPORT.md).
Fontes ativos preservados por SHA-256 (264 arquivos); nenhum commit. Variante
select experimental usa controles01/03/04, sem espera fixa, RTO inicial500ms e
prazo3s. Com RTT60ms em relay, tempo interno até ambos terminarem: CHOWNAT92,3ms,
original532,6ms, variante93,0ms (10pares cada). Confirmação da variante~62,2ms
é medida diferente de saída. Libnice real0.1.21: READY em82,4ms padrão/agressivo
RFC5245 ou178,3ms regular, com coleta HOST/signaling excluídos. Comparação não
mede NAT, discovery público ou TURN. Integração QUIC UDP/TCP da cópia passou
separadamente com dois NATs, dados/fechamento/exit0. Perda persistente de READY
expôs sucesso assimétrico: protótipo não é ICE completo e não deve ser aplicado
como substituto robusto sem tratamento de controles após sucesso no dispatcher.
Detalhes, outros RTTs, perdas e reprodução no relatório; notas de memória
registram limites e metodologia corrigida. Resultados intermediários com janela
fixa300ms são históricos e não representam a variante final.

### Validação de timeout ponta a ponta (2026-10-26)

A hipótese de que o QUIC encerraria a confirmação assimétrica foi testada na
cópia temporária e **não se confirmou para o servidor dentro de45s**. Cliente
QUIC retornou erro~30s; peers retidos no HP retornaram erro~3s. Servidor que
avançou ao QUIC sem cliente ficou na espera de conexão até watchdog45s. O mesmo
ocorreu com HP completo e todos os pacotes QUIC bloqueados. Não houve callback
CONNECTED nas falhas. Limite5s é só shutdown; accept/handshake inicial ainda
não têm deadline explícito do projeto. Correção não aplicada neste turno.
[Relatório e logs](experiments/hole-punch-select-2026-10-26/e2e-validation/VALIDATION.md).

### Variante select integrada a pedido do usuário (2026-10-26)

Aplicado ao backend ativo o patch do experimento de hole punching: controles
01/03/04, select, RTO inicial500ms com backoff, prazo monotônico HP3s e checagem
por progresso remoto. Drenagem pré-QUIC preserva dados que não são controles.
Arquivo comparado byte a byte com a cópia validada; alteração prévia do usuário
preservada. Build all e CTest QUIC TCP/UDP2/2 passaram no original, com NAT;
git diff --check passou. Sem commit. Não foi corrigida a pendência de timeout
no estabelecimento QUIC documentada acima; a integração não equivale a ICE/STUN.
