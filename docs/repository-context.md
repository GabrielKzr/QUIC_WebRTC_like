# Contexto do repositório

## Localização atual do TCC — 2026-11-10

O usuário substituiu `tem.tex` pelo projeto completo em `docs/text/`.
Entrada: `docs/text/main.tex`; implementação: `docs/text/src/5_implementation.tex`;
resultados: `docs/text/src/6_results.tex` (casos C1--C12 e figuras incorporadas).
Bibliografia e imagens acompanham o projeto. Os registros abaixo sobre o trecho
isolado e figuras pendentes são históricos; consultar os fontes atuais.
Revisão de 2026-11-10 alinhou resumo, objetivos e resultados à avaliação
funcional, preservando a comparação conceitual. C12 é o teste prolongado;
avaliação física e redação da conclusão ficam para depois. Decisões finais
em `docs/memory/agent-notes.md`, inclusive retirada dos percentuais UDP/TCP
do parágrafo comparativo. Sem compilação LaTeX ou novos testes de rede.

## Retomada atual e evidências do TCC — 2026-11-09

O ponto de retomada para continuar em casa é
[handoff-2026-11-09.md](memory/handoff-2026-11-09.md).
Git conferido: branch `udp_transport`, HEAD anterior ao commit do usuário
`f9df4a8`. O usuário fará a atualização no Git; o agente não fez stage,
commit ou push. Preservadas as alterações preexistentes de coturn e memória.

Última validação efetivamente executada, em Rocky Linux 8.10:
rebuild incremental de build/managed; **18/18 em 37,20 s** e teste prolongado
QUIC/ICE/TURN **1/1 em 663,814 s**, com 67 blocos/1097728 bytes por sentido,
renovações TURN e remoção das duas allocations após retry do erro 438.
221 PCAPs lidos sem erro. Detalhes no
[relatório funcional](analysis/functional-results-2026-11-09.md).
Não houve nova execução em Ubuntu/GNS3 nem avaliação de perdas/reordenação.

O usuário copiou as evidências para
[`functional-results-2026-11-09/`](../functional-results-2026-11-09/README.md),
fora de build, para levar pelo Git. Os 746 arquivos originais foram
conferidos por SHA-256; PCAPs/logs brutos preservados. Os índices e os guias
agora apontam para essa pasta. `build/` continua ignorado e não deve ser
incluído para transportar os resultados. Verificação de integridade:
`cd functional-results-2026-11-09 && sha256sum -c SHA256SUMS`.

`tem.tex` contém o ambiente experimental e a análise funcional com seis
placeholders de figuras. Próximo passo: produzir as imagens conforme o
[guia dos recortes](analysis/functional-pcap-figures-2026-11-09.md), incorporar
ao TCC e compilar no projeto LaTeX do usuário. Aqui houve apenas verificação
estática do LaTeX. Wireshark/tshark 4.6.9 foi instalado via Flatpak no usuário
desta máquina e abriu a captura remap com quadros 22/23; essa instalação
local não acompanha o Git. Para analisar as capturas em casa, basta Wireshark;
não é necessário executar os testes novamente.

As seções datadas abaixo mantêm o histórico; os resultados de 2026-11-09
acima prevalecem como validação mais recente. A mudança CMP0135 no coturn
e seu contexto estão em agent-notes. Nesta consolidação documental não
foram repetidos os testes de rede.

## Bootstrap de ferramentas corrigido — 2026-11-06

Meson e glib-mkenums ausentes agora são preparados pelo build em `_deps`, com
versões/hashes fixados. A validação inicial usou wrappers temporários e não
cobria esse caso do ambiente do usuário; o bootstrap elimina essa necessidade.
Configure do preset all passou; rebuild com ferramentas gerenciadas e18/18
passaram em31,85s. Ver agent-notes e docs/build-dependencies.md para requisitos.

## Build gerenciado — 2026-11-06

OpenSSL, libnice corrigida, coturn e libevent são compilados pelo CMake em
`_deps`; tcpdump/libpcap entram com `BUILD_PCAP_TOOLS=ON`. Libnice usa OpenSSL
estático, sem GnuTLS de desenvolvimento. Runner Patchbay segue via Cargo e é
pré-requisito dos targets de rede. GLib e ferramentas básicas continuam externas.
Ver [dependências do build](build-dependencies.md) e a atualização inicial de
[memory/agent-notes.md](memory/agent-notes.md) para requisitos e descobertas.
Validado em build/managed:18/18 em31,79s; captura opcional4/4 em11,58s,
68 PCAPs legíveis; incremental sem reconstruir dependências. Long apenas
configurado nesta tarefa; resultado longo anterior abaixo é histórico.

## Capturas opcionais — 2026-11-06

Implementado no runner Rust: presença de PATCHBAY_PCAP habilita PCAP por interface
em todos os nós do laboratório, antes de ISP/peers. Ausência não inicia tcpdump.
Metadados de interfaces/endereços, stderr por captura e capture-status.txt são
salvos; snapshots nft continuam obrigatórios. SIGINT e espera fazem flush;
falhas de captura habilitada são erros do teste. PCAP anterior é substituído
somente ao habilitar captura. Detalhes e comando no README do patchbay.

Verificado com build Rust offline e binários C já existentes em build/all:
ICE direto e TURN forçado passaram, 17 PCAPs por execução. Parser conferiu
cabeçalhos/records completos, ausência de truncamento, sinalização TCP inicial,
STUN, mensagens de aplicação e TURN ChannelData. Validaram-se variável ausente
(sem executar tcpdump), variável vazia (habilita), ferramenta falhando e ISP
falhando após iniciar capturas, com limpeza sem capturadores órfãos. Artefatos
em /tmp/patchbay-pcap-{ice,turn,disabled,tool-failure,isp-failure}-check;
validação em /tmp/patchbay-pcap-{validate,inspect}.py. Não foi repetida a suíte
longa nem reconstruída a suíte C completa nesta tarefa.

Descoberta: tcpdump do sistema falha ao trocar UID/GID dentro do namespace.
Instalado ~/.local/bin/tcpdump upstream4.99.5, compilado sem WITH_USER,
cap-ng ou Capsicum; versão validada nos casos acima. Modo imediato e100ms de
drenagem antes de SIGINT foram necessários para preservar os últimos datagramas.
Não confundir resultados históricos da suíte18/18 abaixo com esta verificação.

Validação final via CTest com PATCHBAY_PCAP=1 e tcpdump instalado: ICE direto,
ICE TURN e CHOWNAT ICE TCP passaram3/3 em10,94s. Todos os51 arquivos foram
lidos com tcpdump/libpcap; loopback dos dois peers TCP contém SYN, payload e FIN.
Artefatos persistidos em build/all/sim/patchbay/artifacts/<nome-do-teste>/pcap/,
com capture-status.txt COMPLETE nos três casos.


## Ponto de retomada atual — 2026-11-04

Git e fontes conferidos na retomada: branch udp_transport, HEAD1a08804,
working tree limpo antes desta atualização documental. As implementações da
noite anterior estão incorporadas nesse commit do usuário. Nenhum teste novo
executado na retomada; os resultados abaixo são registros da validação anterior.

A [memória consolidada da integração libnice](memory/libnice-integration.md)
registra todas as etapas, arquivos e decisões deste ciclo e prevalece sobre os
retratos datados abaixo. HAL/direct/ICE, BIO QUIC, CHOWNAT e QUIC sobre ICE
direto/TURN estão implementados. O CMake já compila libnice0.1.21 com o patch
de remoção TURN após438; consumidores usam `Libnice::Nice` estática em `_deps`.
Suíte atual18/18 passou em33,36s, incluindo regressão nonce que exige remoção
no servidor; controle negativo sem patch foi um executável separado em /tmp.
Após novo pedido explícito, o teste longo completo passou1/1 em664,11s no
HEAD17b4d0a:67 blocos/1097728 bytes por sentido, permissões/allocations/canais
renovados,438 recebido no close e remoção das duas allocations confirmada
após retry com nonce novo. A interrupção anterior é histórica. O teste segue
separado de all; GNS3 continua não executado.
Os detalhes de ambientes e resultados históricos estão separados na memória.

Histórico de retomada em 2026-11-03: o ponto de entrada era o
[estado atual consolidado](memory/agent-notes.md#ponto-de-retomada--2026-11-03).
A memória de2026-11-04 vinculada acima prevalece sobre esse retrato anterior.
Base deste documento consolidada em2026-10-12, com atualização de keep-alive/
hole punching em2026-10-29. Conferir Git/código antes de usar as notas.

## Validação atual — 2026-11-03

ICE/libnice já foi validado pela HAL com signaling TCP e STUN reais no ISP do
Patchbay, dois NATs IPv4, par reflexivo selecionado e dados nos dois sentidos.
Regras extras são declaradas por teste em PATCHBAY_NAT_EXTRA_RULES; os cinco
cenários diretos mantêm mapeamento espelhado e ICE original usa NAT nativo.
Novo network_ice_udp_transport_remap usa regras do próprio .meta para XOR
da porta UDP e mapa DNAT correspondente. Exige diferença entre porta privada
e pública do candidato local selecionado em ambos os peers. Dois testes ICE
passaram2/2 em5,41s: A39241→55625 e B50811→34427, dados e fechamento corretos.
Controle sem regras reprovou portas preservadas. Captura WAN indisponível;
evidências em logs dos peers e snapshots nft. Coturn agora instalado em ~/.local.
Última suíte completa:12/12 em26,50s após aplicar a espera de controle CHOWNAT.

TURN UDP obrigatório também validado em network_ice_udp_transport_turn:
configuração force_relay, autenticação no coturn e par relay/relay nos dois
peers. Signaling só aprova após Refresh lifetime zero e fechamento de todas
as allocations no log do servidor, além de dados íntegros e HAL fechada.
Três cenários ICE passaram3/3 em8,13s (TURN2,70s); suíte final10/10 em16,63s.
STUN-only foi rejeitado pelo
novo executável, sem fallback direto. Detalhes das regressões na memória.

CHOWNAT TCP sobre ICE validado em network_chownat_ice_tcp e
network_chownat_ice_turn_tcp, reutilizando serviços ISP e runner TCP existente.
Peers usam ice_ops, aguardam READY, verificam KA explícito e mantêm handshake
CHOWNAT01→03→03. Exigem CONNECTED único,16KiB enviados/recebidos, EOF do
echo, sessão encerrada e deinit sem erro; TURN exige liberação no servidor.
Novos testes2/2 em6,04s e seis execuções consecutivas em17,43s. Não valida
sessão prolongada/agendamento de KA, perda/reordenação ou reconexão.
Configure/build dos novos targets e suíte completa12/12 em22,95s passaram.

Diagnóstico confirmou retorno antecipado de ice_recv por timer interno sem
payload, consumindo cinco tentativas CHOWNAT em~80ms. A pedido do usuário,
controle de prazo aplicado no chamador: chownat_recv_control usa deadline
monotônico, wait com o restante e recv sem bloquear, ignorando KAs curtos.
Usada nas cinco recepções de controle de handshake/fechamento, cliente e
servidor. Ice_recv e HAL não foram alterados nessa correção. Testes ICE agora
usam cinco tentativas; seis execuções passaram em17,08s e suíte12/12 em26,50s.
Teste também consome
KA recebido durante espera por READY, preservando progresso ICE. Histórico e
evidências na [análise](analysis/chownat-ice-recv-timeout-2026-11-03.md).

Pendências, em ordem: QUIC sobre ICE;
falhas/manutenção/limpeza; e, como etapa final,
reproduzir os casos no GNS3. Critérios e limites estão no
[plano atualizado](plans/ice-quic-integration.md). Nenhuma etapa futura foi
executada só por estar documentada. Os retratos datados abaixo são históricos.

## Estado atual em 2026-11-01

- Branch `udp_transport`, commit `ccabe92`; `external_socket_support` preserva
  suporte a socket externa no commit `9683dc5`. Referências conferidas localmente.
- FD UDP exclusivo dos dados do direct, que sempre cria/binda/fecha sua socket.
  HAL pública delega wait/send/recv à implementação; select fica só no direct.
  TCP permanece sob o ciclo de vida existente em udp_conn, emprestado à espera.
- Interface udp_transport e implementações direct/ice agora têm targets próprios
  em source/libs/transports, com inc/src próprios. Dependências em .meta.cmake;
  libnice0.1.21 encontrada por pkg-config no target ICE, ainda somente um stub.
- QUIC cliente/listener usam BIO próprio sobre a HAL, sem FD exposto ao OpenSSL.
  SSL não bloqueante, loops de estabelecimento e shutdown com prazo5s. BIO usa
  peer IPv4 fixo e limite atual de payload1200. CHOWNAT também usa a HAL.
- Handlers de dados TCP/UDP centralizados em udp_conn; relógio monotônico
  compartilhado em utils. Limpeza aprovada concluída, sem alterar política TCP.
- Último build/diff-check/CTest passaram6/6 em9,05s após centralizar relógio:
  um unitário do transporte e cinco cenários de rede CHOWNAT/QUIC existentes.
  Sucesso é da topologia NAT atual, não de ICE/TURN ainda não implementados.
- Próximo trabalho discutido: avaliar biblioteca e execução de ICE para a HAL,
  provar datagramas opacos com caminho direto/TURN e depois QUIC sobre ICE.
  Decisões, limites e detalhes de verificação estão na memória vinculada acima.

## Objetivo e estrutura

Protótipo de pesquisa de comunicação P2P através de NAT, com CHOWNAT e túnel
TCP sobre UDP/QUIC. Integração QUIC via OpenSSL está implementada; compatibilidade
WebRTC e transporte QUIC próprio completo continuam como objetivos de pesquisa.

| Caminho | Responsabilidade |
| --- | --- |
| `source/common/utils/` | CLI, debug, endereços e temporização |
| `source/libs/protocols/udp_conn/` | Coordenação da conexão, sessões e túnel TCP |
| `source/libs/transports/` | HAL UDP, direct com `select()` e início do adaptador ICE |
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

## Keep-alive por inatividade e abertura NAT (2026-10-29)

Estado conferido por código/Git: último commit 15f9449 (28/10), com mudanças
posteriores em udp_conn.c ainda sem commit. Variante select de HP QUIC está
em #if 0; implementação ativa aguarda iniciativa do cliente e mantém a fase
transiente de 10x50 ms, conforme escolha do usuário.

No laço comum, next_ka_us usa CLOCK_MONOTONIC e intervalo de 5 s. select espera
o menor prazo entre eventos do backend e KA. Após tratar sockets (ready > 0)
ou drenar payload positivo, renova o prazo; somente o ramo ready == 0 chama
udp_send_ka se o prazo venceu. Timer de evento antecipado e EINTR não renovam
o prazo nem disparam KA. process_events permanece no início; threshold continua
restrito a backends sem esse callback. tcp_recv agora propaga retorno de udp_send.
Por instrução explícita do usuário, não se rastreiam datagramas internos QUIC;
isso não é uma medição universal de toda atividade no socket.

Limite atual: ossl_quic_udp_send_ka ainda retorna sem enviar, com SSL_write
inalcançável abaixo. Agendamento corrigido não prova KA QUIC na rede nem
manutenção prolongada do NAT. Envio efetivo continua pendente.

Abertura inicial é anterior ao laço: no HP QUIC, servidor envia um byte UDP
após timeout recv de 1 s enquanto espera 01 do cliente, abrindo o caminho NAT.
No Perl original, servidor já executa can_read(5) antes de conexão e envia
datagrama de payload vazio ao expirar; sondagens 01 do cliente são repetidas
até obter 03 e confirmar com 03. Leitura de fontes, sem nova execução do Perl.
O diagrama resumido de manutenção não representa essa abertura inicial.

Validação da implementação antes dos ajustes posteriores de formatação do
usuário: build all e CTest all 5/5 em 8,52 s, namespaces fora do sandbox.
Harness temporário com fonte ativo/backend de teste confirmou KA aos 5/10 s
sem tráfego, e primeiro KA ~11 s após ~6 s de recepção contínua (5 s ociosos),
inclusive com timers de 20 ms; sinais de 7 ms também testados. Não valida
inatividade longa QUIC real/NAT. Detalhes e histórico em memory/agent-notes.md.

## Planejamento ICE persistente (2026-10-29; sem implementação)

Usuário commitou a mudança anterior como 5b1f2f6; essa é a base relida para a
[análise ICE/QUIC](analysis/ice-quic-integration-2026-10-29.md) e o
[plano inicial](plans/ice-quic-integration.md). Supera o estado sem commit da
seção anterior. Nenhum teste novo nesta análise documental.

Preferência: ICE permanece ativo para candidatos/relay, idealmente com FD criado
pela infraestrutura e demux único de ICE/QUIC no mesmo socket. Libnice pública
não oferece adoção de FD encontrada na revisão: pode criar/configurar sockets
e transportar QUIC como payload via BIO datagrama. Para FD próprio obrigatório,
plano prevê prova com PJNATH de baixo nível (I/O por callbacks). Biblioteca ainda
não foi escolhida definitivamente para implementação.

ICE persistente demanda adaptar transporte, handshake/listener, eventos e
shutdown, não apenas hole punching. Não implica fallback automático, regather
de interfaces ou continuidade da mesma sessão QUIC. NAT/ICE/TURN vivos e QUIC
idle timeout são responsabilidades distintas. Detalhes, fontes, alternativas,
versões e critérios de validação no relatório e plano; código ativo preservado.

### Base UDP para a futura camada (2026-10-29)

A pedido posterior, udp_transport.h deixou de ser somente comentários e ganhou
implementação em udp_transport.c, dentro de udp_conn. API com operações de
datagrama, integração select/TCP sem thread dedicada, estado/manutenção e
ownership. Implementação atual é somente UDP direto IPv4 com I/O não bloqueante
por chamada; aceita FD externo ou cria/binda socket. CHOWNAT/QUIC ainda não
utilizam essa camada. Nenhuma libnice ou ponte BIO implementada.

Configure/build all e CTest 6/6 em 8,50 s passaram. Novo unit_udp_transport
verifica UDP loopback e TCP/select, limites de datagrama, truncamento, ausência
de dados e ownership; cinco regressões existentes continuam passando. Sintaxe
C11 Wall/Wextra/Werror passou. Execução de sockets/namespaces fora do sandbox.
Isso não valida ICE nem NAT na camada nova. Detalhes em memory/agent-notes.md.

Atualização no mesmo dia: por exigência do usuário, a camada de transporte não
aloca memória dinâmica. udp_transport_data_t é definido no header; chamador
fornece região static via transport.data. init preserva dados em falha e close
reinicializa sem liberar/desassociar essa memória. Testes com storage static,
build all e CTest 6/6 em 8,64 s passaram; nm no objeto não referencia funções
de heap. Posse de memória e posse de socket permanecem contratos separados.

Erros dessa API agora são informados somente por status, a pedido do usuário:
sem atribuições ou preservação de errno. Apenas a implementação POSIX consulta
errno internamente para EINTR/EAGAIN/EWOULDBLOCK. Build e teste unitário 1/1
passaram após essa mudança; não houve nova execução da suíte NAT.

Truncamento agora é ERROR com received=0 (UDP_TRANSPORT_TRUNCATED removido).
Edição posterior do usuário faz recv sem dados retornar ERROR também; AGAIN
permanece no envio. Build e teste unitário 1/1 passaram com o contrato atualizado.

Direção posterior: HAL comum de transporte com implementações UDP POSIX e ICE.
Removido campo/enum maintenance; manutenção interna cabe à implementação por
prepare_wait/dispatch. Temporização do protocolo continua separada. Build e
teste unitário 1/1 passaram; ligação da HAL aos backends/udp_conn segue pendente.

Simplificação posterior por instrução do usuário: HAL observa somente read_fds.
Removidos write_fds, wait_write, WRITABLE e AGAIN. Falha de envio retorna ERROR,
sem agendar retry por escrita. Teste TCP+UDP/select e build passaram (unit 1/1).
Integração futura ICE/GLib ainda precisa ser validada com esse contrato.

API atual simplificada a pedido do usuário: udp_transport_wait encapsula select
para UDP e TCP opcional (FD -1 desabilita TCP), recebe timeout const e retorna
somente flags UDP_TRANSPORT_READABLE/TCP_TRANSPORT_READABLE. Eliminados
prepare_wait/dispatch, estrutura pública de espera, estados e flag de HP.
Controles internos de um transporte futuro não devem produzir leitura de
payload. Memória continua fornecida pelo chamador; TCP apenas observado e nunca
fechado pelo transporte. Adicionado set_tcp_fd; send/recv falham por status,
sem errno/repetição EINTR. Código espaçado para revisão. Build e teste unitário
1/1 passaram; não houve integração aos backends ou nova validação ICE/NAT.
