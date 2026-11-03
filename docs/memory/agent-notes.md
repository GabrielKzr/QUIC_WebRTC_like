# Memória para futuras sessões

Estado atual consolidado em 2026-11-01. Detalhes e evidências:
[contexto do repositório](../repository-context.md). Comandos:
[guia Patchbay](../../sim/patchbay/README.md).
Estas memórias são arquivos locais; não garantem persistência fora do repo.
O resumo abaixo prevalece sobre os relatos históricos posteriores quando houver
conflito. Conferir sempre Git e código antes de continuar.

## Ponto de retomada — 2026-11-01

### Restauração das regras NAT após experimento — 2026-11-02

- Usuário pediu retirar todas as substituições e testar NAT nativo. Bloco
  manual foi removido, runner compilou e ICE/STUN passou isolado em2,72s.
  Suíte completa:3/8 passaram em134,06s; nice_local, unit e ICE/NAT (3,00s)
  passaram, os cinco cenários diretos CHOWNAT/QUIC falharam por erro/timeout.
  Captura CHOWNAT mostrou porta externa31111 enquanto o peer mirava2222.
  No ICE observado, o NAT nativo preservou portas espontaneamente; não foi
  comprovada troca ICE com porta pública diferente da privada.
- Usuário solicitou em seguida voltar SOMENTE a alteração das regras, com
  urgência. Restaurado o bloco manual anterior completo, incluindo SNAT UDP
  preservando porta, mapas/guarda e SNAT TCP para ISP quando há hook. Import
  AsyncWriteExt restaurado. Hook, serviços e teste ICE/NAT mantidos.
  README corrigido para descrever esse estado. Não restaurar NAT nativo por
  inferência. Resultado8/8 em12,62s com esse bloco é anterior ao experimento;
  não afirmar nova execução da suíte após restauração sem conferir.
- Mensagem de commit solicitada pelo usuário; nenhum commit criado.

### Experimento: retirada da regra SNAT UDP — 2026-11-02

- A pedido explícito do usuário, removida somente a regra UDP
  snat to {public}:udp sport de topology.rs. Mantidos flush postrouting,
  atualizações dos mapas fullcone/opened_ports e SNAT TCP destinado ao ISP.
  Comentário e mensagem de configuração ajustados. Regra NÃO restaurada.
- Build patchbay_runner e diff-check passaram. Teste network_ice_udp_transport
  com coturn real temporário falhou em2,55s: peer B relatou ausência de
  candidato server-reflexive. Captura router-a.udp.log mostrou saída STUN
  ainda com origem privada192.168.80.20:48120 para10.0.0.1:3478, sem resposta.
  Snapshot router-a.nft confirmou ausência de SNAT UDP; só existe SNAT TCP.
- Estado atual é um experimento com tradução UDP ausente, pois o postrouting
  nativo continua sendo apagado. Isso NÃO demonstra falha de ICE com portas
  públicas diferentes: ainda falta um conjunto NAT coerente para remapeamento.
  Não alterado o restante das regras para consertar o experimento sem pedido.
  Suíte completa não repetida nesta retirada; resultado8/8 abaixo é anterior.

### Teste ICE/STUN integrado ao Patchbay/NAT — 2026-11-02

- Após usuário apontar que faltava ligação no .meta, integrado o teste
  test_ice_udp_transport ao runner: PATCHBAY_MODE udp, TIMEOUT20, nome
  network_ice_udp_transport, labels network/nice/ice/stun e ISP_SCRIPT isp.sh.
  Substitui o registro local anterior ice_udp_transport_local. Não há fork
  no executável atual: um peer por namespace, papel -c/-s na CLI do runner.
  Exemplo independente nice_local preservado. Não testa CHOWNAT sobre ICE.
- Runner fornece PATCHBAY_ISP_ADDRESS (downstream_gw do ISP) e
  PATCHBAY_LOCAL_ADDRESS (IPv4 do peer), somente quando há script. Config ICE
  usa esse endereço local, STUN no ISP:3478 e callback síncrona TCP para
  ISP:5555. Callback envia/recebe frames uint32 big-endian + SDP, com loops
  para I/O parcial e timeout3s. Exige ao menos um candidato local srflx.
- Teste exige READY, envio/recebimento de mensagens com conteúdo correto nos
  dois sentidos, par remoto selecionado srflx/prflx e close liberando recursos.
  Registra IPs/portas do par escolhido; progresso5s. README nice atualizado
  para build/preset all e CTest de rede, removendo instrução local anterior.
- Primeiro ensaio falhou no connect TCP: regras atuais faziam flush do
  postrouting e só recriavam SNAT UDP. Acrescentado SNAT TCP destinado ao
  gateway ISP somente quando há hook; demais regras/nós/enlaces preservados.
  Próximo ensaio trocou mensagens mas teste recusou par host: captura mostrou
  IPv6 roteado contornando NAT IPv4. Restringir local_address ao IPv4 do peer
  resolveu; não desabilitar IPv6 globalmente nem relaxar critério de NAT.
- Coturn não está instalado no sistema. Para validação real, apt-get download
  baixou coturn4.6.1-1build4, libhiredis1.1.0 e quatro libs libevent2.1 para
  /tmp/ice-coturn-runtime, extraídos com dpkg-deb em root/. Nenhum serviço
  instalado globalmente. CTest executado com PATH prefixado por
  /tmp/ice-coturn-runtime/root/usr/bin e LD_LIBRARY_PATH apontando para
  /tmp/ice-coturn-runtime/root/usr/lib/x86_64-linux-gnu. Uso normal requer
  turnserver e bibliotecas disponíveis; script agora informa dependência ausente.
- Configure/build all, C Wall/Wextra/Werror, sh -n e diff-check passaram.
  CTest ICE/NAT real fora do sandbox passou1/1 em2,97s: pares públicos
  10.0.0.2:51619 e10.0.0.3:59524, captura com STUN ISP:3478 e payloads UDP
  IPv4 entre WANs. Suíte final8/8 em12,62s com coturn temporário:
  nice_local0,04; unit2,02; CHOWNAT TCP1,38/UDP1,73/close1,52;
  QUIC TCP1,56/UDP1,31; ICE/NAT3,06. TURN e QUIC/CHOWNAT sobre ICE não validados.
  Artefatos em build/all/sim/patchbay/artifacts/network_ice_udp_transport/.
  Sem commit; alterações preexistentes do usuário preservadas.

### Hook opcional de script no ISP — 2026-11-02

- Atualização posterior aprovada: criados isp.sh e signaling.py em
  source/tests/nice/test_ice_udp_transport conforme esboço. Script inicia
  turnserver (coturn STUN-only UDP3478) e signaling Python TCP5555 no mesmo
  grupo, permanece em primeiro plano e sai com erro se um serviço encerrar.
  Python confirma resposta Binding STUN local antes de imprimir READY;
  troca mensagens opacas de dois clientes TCP, prefixo uint32 big-endian,
  payload1..65536 bytes, timeout5s de leitura. README nice atualizado.
- Nenhuma associação PATCHBAY_ISP_SCRIPT foi acrescentada ao teste local,
  nenhum teste NAT/CHOWNAT novo foi criado. Usuário perguntou se a futura
  validação usaria CHOWNAT; explicado que o teste atual é ICE pela HAL e que
  CHOWNAT sobre ICE exige adaptação de um executável de teste específico.
  Não migrar os testes atuais por inferência desse pedido de infraestrutura.
- sh -n, AST Python, framing válido/inválido/incompleto e diff-check passaram.
  Fixture temporária /tmp/isp-services-check executada fora do sandbox passou
  1/1 em0,41s: scripts reais com turnserver substituído por respondedor STUN
  mínimo de fixture, READY, troca bidirecional TCP de SDPs, cabeçalho dividido
  entre envios e saída1 do supervisor após frame inválido matar signaling.
  Grupo dos filhos encerrado pela fixture. Isso não valida coturn real nem
  ICE/NAT: turnserver não foi encontrado no PATH e não foi instalado nesta
  etapa. Suíte C/Patchbay não repetida, pois scripts ainda não são associados
  a testes registrados e não houve alteração de bibliotecas/runner.

- Usuário aprovou PATCHBAY_ISP_SCRIPT no .meta.cmake do teste. Registro CMake
  valida arquivo absoluto existente e repassa quinto argumento opcional ao
  runner. Main canonicaliza e passa à topologia; topologia mantém mesmos nós,
  enlaces e regras NAT. Nenhum teste atual declara script; não foi criado
  ainda teste NAT/STUN nem serviços STUN/signaling nesta etapa.
- Antes dos peers, start_isp_script executa sh no namespace ISP, cwd source,
  stdin fechado e stdout/stderr nos artefatos isp.stdout.log/isp.stderr.log.
  Aguarda linha READY no stdout, prazo5s. Script deve permanecer em primeiro
  plano e manter filhos no grupo original, sem daemonização/setsid.
- Script separado da lista de peers; wait_programs rejeita saída do script.
  IspScript guarda Child e grupo de processos; stop/Drop enviam SIGKILL ao
  grupo, cobrindo timeout, preparação parcial e fim do teste. Stop aguarda
  script. Dependência direta nix0.30 com signal/process adicionada, pacote
  já presente no lock/cache; Cargo.lock só acrescentou dependência da raiz.
  README Patchbay documenta propriedade e contrato.
- Cargo check offline, configure/build all e git diff --check passaram.
  CTest all fora do sandbox passou8/8 em9,48s: dois testes locais nice/ICE,
  unitário e cinco cenários de rede CHOWNAT/QUIC anteriores, sem hook.
- Fixture temporária /tmp/isp-hook-check usa registro CMake real e caminho de
  script com espaços. Quatro cenários passaram em7,84s: READY, saída antecipada,
  timeout5s e falha de peer; erros esperados conferidos em result.txt.
  Cenário adicional de falha ao montar comando do peer após READY passou
  em0,30s. Em todos, script registrou endereços do ISP (10.0.0.1/24) e PIDs;
  /proc confirmou ausência de processos vivos do script e filho sleep após
  limpeza (zumbis são processos terminados). Peers da fixture são stubs:
  isso valida hook/lifecycle, não serviços ou tráfego ICE através de NAT.
  Nenhum teste permanente adicional nem commit nesta etapa.

### Estruturas ICE e callback de signaling — 2026-11-02

- Teste permanente aprovado para o fluxo completo: criado
  source/tests/nice/test_ice_udp_transport, ligado ao target ice_udp_transport
  e registrado no CTest como ice_udp_transport_local, TIMEOUT10, classe nice.
  Um peer por processo via fork, sem threads; callback troca credenciais e
  candidatos SDP por socketpair Unix SOCK_DGRAM com SO_RCVTIMEO3s. Datagramas
  da aplicação usam ICE/UDP loopback, prazo de progresso5s. SDP fica no teste,
  sem impor formato de signaling à biblioteca. Configuração usa só127.0.0.1.
- Motivo dos dois processos: sondagem temporária /tmp/ice-open-order confirmou
  que sem STUN a callback de coleta/signaling ocorre dentro de open, antes de
  retornar. Dois opens sequenciais com signaling bloqueante no mesmo processo
  impediriam que o segundo peer iniciasse. Cada processo tem seu próprio ICE
  e loop chamador; isso preserva a abordagem threadless de cada transporte.
- Teste exige signaling e gathering concluídos, READY, envio e recebimento
  de conteúdo correto nos dois sentidos e close público com opened0 e
  agente/contexto NULL. Usa HAL pública para open/wait/send/recv/close; APIs
  libnice de serialização/parsing ficam na callback de signaling do teste.
- Configure all, build do novo target, Wall/Wextra/Werror e diff-check passaram.
  CTest filtrado fora do sandbox passou1/1 em0,05s (total0,06s). README nice
  atualizado com escopo/comandos. Sem validação NAT/TURN/QUIC ou reexecução da
  suíte Patchbay; bibliotecas preservadas nesta etapa. CMakePresets mantido.
- Ajuste anterior solicitado pelo usuário: removido helper ice_wait_fds;
  ice_wait chama g_poll diretamente. Build do target/diff-check passaram.

- Atualização posterior aprovada: ice_wait implementado com g_poll, reunindo
  descritores GLib de prepare/query, sockets obtidas por nice_agent_get_sockets
  e TCP emprestado quando solicitado. Escolhe menor prazo GLib/chamador,
  check/dispatch após espera e filtra prontidão conforme requested_events.
  Mantido ice_get_timeout para consulta da HAL; query no wait também coleta
  descritores e atualiza prazo após processamento. Nenhuma thread adicionada.
- Leitura explícita antes/depois da espera trata controle ICE e preserva um
  datagrama de aplicação em pending_packet[65536], pending_size/has_pending.
  Usuário considerou limite1500 e decidiu manter64KiB. Ice_recv agora consome
  esse pacote; capacity insuficiente descarta o datagrama com ERROR/received0.
  Isso supera os registros históricos abaixo de wait vazio e buffer local.
- Build all e git diff --check passaram. Harness temporário baseado no fonte
  aplicado compilou com Wall/Wextra/Werror e verificou consulta imediata,
  timeout20ms, timer GLib, prontidão de descritores, EOF, FD acima de FD_SETSIZE
  e liberação do contexto; referências em /tmp/ice-wait-sketches/check.c.
- Harness /tmp/ice-wait-applied-check/traffic.c, também Wall/Wextra/Werror,
  executado via CTest fora do sandbox, passou1/1 em0,04s: dois agentes ICE
  loopback chegaram a READY, trocaram payloads binários1/1200/8192 bytes nas
  duas direções, preservaram pending entre esperas, sinalizaram UDP/TCP juntos
  e descartaram datagrama maior que capacity sem deixar fragmento pendente.
  Fixture troca credenciais/candidatos diretamente, sem callback de signaling
  bloqueante. Não valida NAT/TURN, QUIC sobre ICE nem pressão de envio; slot
  cheio suspende novas leituras e escrita ainda pode retornar NOT_READY após
  prontidão. Nenhum teste permanente novo, commit ou alteração do preset.

- Atualização posterior: usuário removeu user_data do signaling e extraiu
  udp_transport_signaling_cb_t, com campo config->signaling_cb. Criou os
  esqueletos open/send/recv/get_timeout/wait/close e a tabela ice_ops.
- A pedido do usuário, aplicado ice_open: valida config/data/callback e portas,
  cria GMainContext próprio e NiceAgent não confiável RFC5245, configura papel,
  desliga ICE-TCP/UPnP, STUN opcional, endereço local opcional (NULL permite
  descoberta automática), uma stream/componente e TURN UDP opcional; conecta
  sinais e inicia coleta. Falha libera agente/contexto e zera data.
- Atualização posterior aprovada: ice_gathering_done recebe transport como
  user_data, marca gathering_done, obtém candidatos/credenciais locais, chama
  config->signaling_cb de forma síncrona e aplica credenciais/candidatos remotos.
  Valida retorno e saídas, libera listas/candidatos e strings locais/remotos
  inclusive em erro; falha grava state=FAILED. ice_state_changed permanece
  atualizando NiceComponentState para stream/componente1, sem outras ações.
- Usuário confirmou abordagem threadless e signaling bloqueante, com timeout
  sob responsabilidade da aplicação. Não substituir por signaling assíncrono
  nem criar threads por iniciativa própria. Durante a callback o contexto fica
  pausado. Open ainda retorna após iniciar coleta, não espera READY; progresso
  de contexto/recepção e wait continuam sem implementação; send/recv adicionados
  posteriormente conforme abaixo, ainda dependentes dessa espera.
- Aplicado ice_close conforme esboço aprovado: valida data/agente/contexto,
  desconecta handlers de coleta/estado, push_thread_default no contexto próprio,
  nice_agent_close_async com callback ice_closed e flag local; itera contexto
  com bloqueio até concluir, pop_thread_default, unref agente/contexto e zera
  data. Não adiciona thread nem prazo próprio de fechamento.
- Build ice_udp_transport e diff-check passaram após implementar close.
  Compilador avisa retorno implícito int nos quatro esqueletos ainda vazios,
  mantidos sem alteração. Harness temporário /tmp/ice-close-check verificou
  três fechamentos de agentes com stream criada, sem coleta nem TURN: callback
  conclui, contexto padrão anterior restaurado, data zerada e segundo close
  público retorna OK. Não comprova release TURN ou conexão ICE funcionando.
- Aplicado ice_get_timeout conforme esboço aprovado: inicializa saídas,
  valida contexto, acquire/prepare/query/release do GMainContext; trabalho
  pronto força prazo0, prazo não negativo vira timeval/has_timeout1 e ausência
  de timer mantém has_timeout0. Não chama check/dispatch/iteration.
  Build e diff-check passaram; avisos restantes são send/recv/wait vazios.
  Harness temporário /tmp/ice-timeout-check validou contexto sem timer, prazo
  futuro, fonte pronta com prazo0, liberação da posse do contexto e erro em
  contexto NULL. Consulta não despachou a callback; iteration posterior sim.
  Sem nova execução ICE/TURN ou suíte Patchbay nesta etapa.
- Usuário escolheu recepção explícita em vez de callback de payload. Aplicados
  ice_send/ice_recv conforme esboço: ambos chamam ice_wait pedindo somente o
  evento UDP correspondente, depois enviam/recebem uma mensagem com as APIs
  não bloqueantes da libnice, convertendo WOULD_BLOCK/contagem0 em NOT_READY e
  demais falhas em ERROR. API send do header instalado recebe GCancellable;
  passado NULL, assim como no recv. Pkg-config continua reportando nice0.1.21.
- Recv usa packet[65536] local para receber o datagrama inteiro, retorna ERROR
  se exceder capacity e copia somente quando couber. Sem fila de payload ou
  callback attach_recv. Limitação de datagramas vazios discutida: keep-alives
  CHOWNAT/HP usam byte0 com length1, sem envio vazio intencional identificado
  nos controles conferidos. Não foi alterado o contrato público de datagrama0.
- Build ice_udp_transport e diff-check passaram após send/recv. Apenas a
  declaração/assinatura de ice_wait foi tipada; corpo continua vazio como no
  esboço aprovado. Não executar send/recv ainda nem afirmar transporte pronto:
  implementação da espera e progresso dos checks com leitura explícita são
  pendentes. Nenhum teste de tráfego ICE realizado nesta etapa.

- Usuário aprovou criar ice_udp_transport_data_t/config_t no header ICE.
  Handler comum permanece udp_transport_t. Data contém contexto GLib, agente,
  stream_id, NiceComponentState e gathering_done, sem GMainLoop ou outro peer.
  Config contém papel controlador, endereço local, STUN e TURN (UDP inicial).
- Config originalmente incluía signaling_callback e signaling_user_data;
  nomes e ausência de user_data atuais registrados acima. A pedido posterior
  do usuário, a mesma callback faz envio/recebimento síncronos: recebe handler,
  ufrag, senha e GSList locais emprestados/somente leitura e devolve credenciais
  remotas por char** e candidatos por GSList** antes de retornar OK/ERROR.
  Saídas inicialmente NULL transferem posse ao transporte, inclusive parciais
  em erro: strings g_free, lista/candidatos g_slist_free_full/nice_candidate_free.
  Aplicação controla timeout, formato e transporte do signaling, sem SDP/JSON
  imposto. Não guardar endereços dos parâmetros de saída para concluir depois.
- Na etapa inicial só estruturas e declaração ice_ops foram adicionadas;
  open e chamada da callback implementados posteriormente conforme acima.
  Entrada inicial dos dados remotos ocorre pelas
  saídas da callback, sem função separada nessa proposta síncrona. Usuário
  escolheu posteriormente leitura não bloqueante explícita para payload.
- Build do target ice_udp_transport passou após editar o header. Sem novos
  testes de execução: esta etapa não implementa estabelecimento nem I/O ICE.

### Teste local da libnice

- A pedido do usuário, criado source/tests/nice/test_nice_local com classe
  nice habilitada por BUILD_NICE_TESTS=ON (ou TEST_NAME=test_nice_local).
  Usa diretamente PkgConfig::LIBNICE, sem HAL, QUIC ou Patchbay.
- Dois agentes RFC5245 no mesmo processo, UDP somente em 127.0.0.1 e portas
  automáticas. Credenciais/candidatos trocados em memória; espera READY nos
  dois lados e valida conteúdo de um datagrama em cada direção. Prazo interno
  de5s e limite CTest de10s. Não usa servidores STUN/TURN externos.
- Configure em build/nice com BUILD_NETWORK_TESTS=OFF e build do target
  test_nice_local passaram. CTest filtrado nice_local passou1/1 em0,05s fora
  do sandbox; dentro dele libnice não podia criar sockets (EPERM).
  Comandos de reprodução em source/tests/nice/README.md. Não valida NAT,
  relay, signaling externo ou backend ICE; suíte Patchbay não reexecutada.

### Atualização posterior: targets de transporte e libnice

- Usuário instalou o pacote de desenvolvimento; pkg-config agora encontra
  nice0.1.21. Libnice é a direção discutida para o backend ICE. Não há agente,
  signaling ou I/O ICE implementados; os arquivos ice_udp_transport.c/.h
  criados pelo usuário continuam somente com include/guarda.
- Usuário aprovou separar interface e implementações em source/libs/transports:
  udp_transport, direct_udp_transport e ice_udp_transport, cada uma com inc/src
  e target próprio. Udp_conn permanece em protocols. Arquivos movidos sem
  alteração de conteúdo, incluindo os dois arquivos iniciais ICE do usuário.
- Dependências específicas agora são explícitas nos .meta.cmake. Udp_conn e
  direct dependem de udp_transport; ICE depende de udp_transport e
  PkgConfig::LIBNICE. Chownat/quic_like dependem de udp_conn; ossl_quic depende
  de udp_conn e OpenSSL. COMMON_DEPS=utils mantido no helper.
- Meta ICE usa find_package(PkgConfig REQUIRED) e pkg_check_modules(LIBNICE
  REQUIRED IMPORTED_TARGET nice). ICE é configurado incondicionalmente nesta
  etapa; uma opção para build sem libnice não foi autorizada/adicionada.
- Executáveis WebRTC escolhem direct_udp_transport em TEST_EXTRA_LIBS; o
  unitário também liga direct. Agregador protocols mantém só protocolos e
  recebe a HAL transitivamente, sem selecionar implementação de transporte.
- Usuário escolheu add_lib para o helper antes chamado add_protocol_lib,
  preservando o comando nativo add_library. Helper e chamadas renomeados;
  reconfigure e build all passaram novamente após a renomeação.
- Reconfigure e build all, diff-check e CTest all passaram6/6 em8,94s:
  unit2,02; CHOWNAT TCP1,42/UDP1,43/close1,34; QUIC TCP1,48/UDP1,23.
  Flags geradas mostram nice/GLib somente no target ICE, não em udp_conn;
  link do executável QUIC direto não inclui libnice. Nenhum teste ICE de rede
  foi executado e nenhum teste novo registrado. Sem commit nesta etapa.

### Git verificado nesta consolidação

- Branch atual: `udp_transport`; HEAD e referência local `origin/udp_transport`
  em `ccabe92` (`refactor: simplify direct UDP ownership and waits, unify TCP/UDP
  data handlers and centralize monotonic clock utilities`). Git estava limpo
  antes desta atualização documental. Não houve fetch para conferir o remoto.
- `external_socket_support` aponta para `9683dc5`; `git show` confirmou que
  preserva `already_bound`, `take_ownership` e `owns_fd`, ou seja, a versão
  anterior com suporte a socket externa. Não remover essa branch como limpeza.
- O agente não criou commits nem branches. Os commits acima foram observados
  no Git após as ações do usuário; esta consolidação só altera documentação.

### Arquitetura e decisões aplicadas

- `udp_transport_t` é o handler comum: `ops`, `opened`, `tcp_fd`, configuração
  específica e dados específicos. Não contém FD UDP nem configuração comum
  duplicando FDs. `udp_session_t` também não tem mais `socket_fd` UDP.
- O FD UDP pertence a `direct_udp_transport_data_t`. Na branch atual, direct
  sempre cria/configura/binda/fecha sua UDP; removido o caminho de adoção de
  socket externa. Config específica contém apenas reuse, endereço local e peer.
  Dados contêm FD e peer. Falha de abertura fecha o FD recém-criado sem publicar
  estado parcial. Close invalida o FD e preserva a memória do chamador.
- Config/data são fornecidos pelo chamador, sem heap próprio na HAL. Nos cinco
  exemplos WebRTC, o main preenche local/peer/reuse antes de udp_conn_init;
  `direct_config={0}` é apenas a inicialização, não a configuração final.
- `udp_transport_wait` continua PÚBLICA com requested_events; delega a operação
  a `ops->wait`. A implementação `direct_wait` é privada e usa select. Não há
  select/fd_set/FD_SETSIZE na API nem no código comum do transporte.
- Eventos: UDP_READABLE=1, TCP_READABLE=2, UDP_WRITABLE=4, TCP_WRITABLE=8
  (nomes completos no header incluem TRANSPORT). Só eventos solicitados são
  retornados. Escrita nunca é observada incondicionalmente, para evitar giro.
- Send/recv públicos mantêm timeout no último parâmetro: zero consulta,
  positivo limita espera, NULL não impõe prazo do chamador. Delega aos callbacks
  específicos; direct_send/direct_recv reutilizam direct_wait pedindo só UDP,
  sem consultar TCP. EAGAIN/EWOULDBLOCK e falta de prontidão são NOT_READY=1;
  falha/truncamento são ERROR=-1. Wait sem eventos retorna OK/events0.
- Get_timeout comum é privado; operação opcional do backend permite escolher
  o menor prazo interno/chamador. Deadlines internos persistem nos dados da
  implementação; a espera específica atende os eventos/timers. Não restaurar
  prepare_wait/dispatch ou fd_sets públicos por causa de notas antigas.
- Direct recebe por recvmsg e exige IP/porta IPv4 do peer configurado. Pacote
  de outra origem é consumido e retorna ERROR, sem retry silencioso. Truncamento
  também é consumido e retorna erro; datagrama vazio continua válido.
- TCP continua em udp_conn: criação/configuração dos exemplos, bind/connect/
  accept, leitura/escrita e fechamento conforme fluxo existente. tcp_fd do
  transporte é emprestado, -1 se ausente. Sua reorganização foi adiada pelo
  usuário. Verificações específicas de FD_SETSIZE ficam no direct.
- Udp_connection continua dono do loop, drenagem pending_data, eventos do
  protocolo e keep-alive de5s. Handler privado handle_tcp_data é a antiga
  tcp_recv renomeada. Handle_udp_data unifica udp_recv, fechamento, entrega ao
  TCP e disconnect em erro; ambos os caminhos de recepção usam esse handler.
  Erro original não pode virar OK apenas porque a callback fechou a conexão.
- CHOWNAT usa a HAL para datagramas; CLOSED normal é separado de erro de
  recepção. Expiração sem confirmação no disconnect continua sendo erro.
- QUIC já usa ossl_quic_bio.c/.h no cliente e listener, sem SSL_set_fd ou
  connect UDP no backend. BIO próprio chama HAL com timeout0, sem FIFOs nem
  thread própria e sem fornecer FD ao OpenSSL. Peer IPv4 fixo configurado no
  BIO; limite atual na integração é1200 bytes de payload UDP. Não existem
  udp_transport_get_info nem origem opcional pública no recv.
- SSL opera não bloqueante; estabelecimento externo continua síncrono por
  loops de retry com prazo total5s. Cliente repete SSL_connect; servidor repete
  accept não bloqueante e conclui handshake antes de CONNECTED. Na OpenSSL
  local, accept NULL sem erro na fila é tratado como ausência de conexão;
  SSL_get_error(listener,0) isoladamente não distinguiu isso no experimento.
- Ossl_quic_wait privado consulta interesse de rede/timer e espera pela HAL
  somente UDP, limitando pelo deadline compartilhado. Shutdown reaproveita a
  espera com limite5s e preserva close_status. Usuário rejeitou expor FD ao
  OpenSSL para que ele bloqueasse internamente.
- BIO_METHOD e config do BIO vivem em ossl_quic_data_t. BIO_up_ref permite
  transferir as duas referências a set0_rbio/set0_wbio. SSL_free libera BIOs;
  o método é liberado depois de conn/listener. Transporte segue ativo durante
  shutdown/deinit QUIC e é fechado pela camada comum depois.
- Código morto QUIC removido: dois #if0, protótipo drain_socket_rx_queue e
  SSL_write após return do keep-alive. Limpeza dos três pontos concluída:
  espera compartilhada no direct, limites de select específicos do direct,
  recepção UDP centralizada. Não há outra refatoração implicitamente autorizada.
- Relógio compartilhado em utils.c/.h: monotonic_us(int64_t*) retorna0/-1;
  monotonic_ms(void) reutiliza a primeira e retorna valor/-1. Udp_conn usa us,
  QUIC usa ms; unidades, limites e tratamento de erro preservados. Relógio
  independente do unitário e debug_print_ts já em utils foram mantidos.

### Validação e limites

- Última execução após mover o relógio: build all, git diff --check e CTest
  all passaram6/6 em9,05s: unit_udp_transport2,02; CHOWNAT TCP1,39/UDP1,43/
  close1,44; QUIC TCP1,47/UDP1,30. Cinco testes de rede existentes + um unitário.
  Conteúdo, rounds/túnel e encerramento são verificados pelos testes existentes.
- QUIC desses testes já passa pelo BIO próprio e HAL, na topologia NAT do repo.
  Isso não valida ICE, TURN, descoberta de endpoints ou migração de sessão.
- Ensaios temporários anteriores verificaram filtros de origem, máscaras dos
  quatro eventos, pressão de escrita TCP e estabelecimento QUIC sem resposta
  por cerca de5s, com limpeza. Fontes em /tmp não são artefatos permanentes;
  podem estar desatualizadas após mudanças de contrato e exigem recompilação.
- Não foi adicionado teste permanente nesta sequência. Não houve nova execução
  de testes para esta tarefa documental. Falha de clock_gettime não foi injetada.
- Limitação conhecida mantida: envio de aplicação QUIC ainda trata falha de
  SSL_write_ex como erro; política de retry sob pressão/WANT_WRITE não foi
  alterada nem validada especificamente. Não considerar isso resolvido pelos
  testes pequenos ou pela máquina de handshake já tratar WANT_READ/WRITE.
- BIO atual presume peer fixo garantido pelo transporte; não generalizar sua
  atribuição de origem nem o limite1200 para ICE/mudança de caminho sem discutir
  contrato de metadados/MTU. Biblioteca ICE, signaling e manutenção não existem.

### Próxima etapa discutida, ainda não implementada

- Retomar [plano ICE/QUIC](../plans/ice-quic-integration.md): avaliar/escolher
  biblioteca ICE e mapear open/send/recv/wait/close para a HAL existente.
  Avaliar ownership, progresso de eventos/timers e fechamento antes de escolher.
- Primeiro provar datagramas opacos entre dois peers, caminho direto e TURN
  forçado com signaling de laboratório; só então usar QUIC pelo BIO sobre ICE.
- Preferências antigas por FD criado pela infraestrutura, par BIO e select
  comum foram superadas na base atual: FD é da implementação, BIO próprio e
  wait específico. Não escolher biblioteca só por capacidade de adotar FD.
- Esta é a próxima proposta de trabalho, não autorização para instalar uma
  biblioteca, adicionar helpers/filas/threads ou implementar ICE sem discussão.

## Preferências do usuário

- Sempre adicionar e editar código bem formatado, com espaçamento, indentação e
  quebras de linha consistentes com o projeto, sem deixar ajustes manuais de
  formatação para o usuário.

- Comunicar em português brasileiro. Explicar escolhas e sua necessidade;
  evitar abstrações e regras adicionais sem justificativa.
- Mensagem de commit: uma única linha com MENOS de300 caracteres, em bloco
  `text`, sem comando git e sem realce. Mensagens desta sequência foram em inglês.
  Não executar staging/commit por iniciativa própria.
- Esboços: blocos `text`, + para adições, - para remoções, contexto real antes/
  depois sem marcador, arquivo e função identificados, estilo/indentação originais.
  Resumir e omitir comentários dispensáveis. Esboço é proposta, não aplicação.
- Escopo estrito: fazer somente o pedido. Discutir e aguardar concordância para
  acréscimos de helpers, abstrações, mudanças de comportamento ou refatorações.
- Preservar edições do usuário. Registrar problemas, separar hipóteses de fatos
  e não transformar tarefas pontuais em refatorações amplas.
- Respeitar pedidos de não validar. Na última interação sobre comentários de
  shutdown, o usuário preferiu escrevê-los pessoalmente; não sobrescrever.

## Histórico: base verificada em 2026-10-12

- Build raiz e preset configure/build/test `all`; CTest não recompila.
- Integração Patchbay em `sim/patchbay/`: runner, topologia, verificação e CMake.
  Executáveis/metadados em `source/tests/`; registro automático por propriedades
  dos targets. Cinco testes WebRTC registrados. Sondagem antiga de ambiente removida a pedido em2026-10-12, junto com
  sim/patchbay/tests/CMakeLists.txt; não participava do runner/CTest.
- Topologia Rust explícita e única: peer_a192.168.80.20 epeer_b192.168.80.30
  em LANs isoladas192.168.80.0/24 atrás de A/B. ISP Public fornece10.0.0.0/24
  para WANs automáticas. Papéis atuais -c/-s preservados; ISP sem serviço. WAN descoberta por uplink_ip/interfaces(), sem
  assumir ix/wan. Tráfego entre filhos usa bridge do ISP, não sua WAN.
- Topologia declara rede, peers e argumentos; echo/probe em src/patchbay.rs.
  Main coordena coleção de devices. Nomes antigos gns3/public/nat removidos.
- NAT atual: Moderate com APDF nativo; sem zonas conntrack. Regras próprias
  mantêm SNAT por porta de origem e mapa DNAT dinâmico. Peers usam2222.
  opened_ports/inbound_guard restaurados a pedido; guarda descarta antes do
  conntrack entradas para portas não abertas. Não equivale a APDF completo.
- ICE/STUN e descoberta de endpoints fora do escopo atual. Incremento de porta
  em colisão discutido, não implementado; não confundir com zonas conntrack.
- Testes exigem saída zero dos peers; TCP exige também payload16KiB íntegro e
  probe/echo concluídos. Kill/timeout é falha, nunca critério de sucesso.
  CHOWNAT UDP eQUIC UDP são finitos, com3rodadas validadas; QUIC trata framing.
  UDP close teve usleep removido pelo usuário (~3,44s para~1,39s).
- QUIC: eventos/dados pendentes processados antes de esperar. Shutdown atual
  handle_events antes de uma consulta SSL_shutdown por rodada, sem dupla
  tentativa. Espera com select (pedido do usuário), menor entre timer OpenSSL
  e restante do limite monotônico5s; infinite não elimina esse limite.
  Escrita monitorada só com SSL_net_write_desired; fd validado contraFD_SETSIZE.
  close_status propaga erros locais/remotos; deinit libera recursos mesmo em erro.

## Evidências e limites ao retomar

- Moderate sem zonas, com guarda: suíte5/5 em6,75s e15/15 repetições em20,29s;
  capturas finais preservaram2222. Remover guarda/conjunto falhou5/5. Foram
  restaurados; último build/teste após troca para select passou QUIC2/2
  (TCP1,34s, UDP1,24s). Não afirmar suíte completa reexecutada após essa troca.
- Hipótese pendente: sem guarda, primeiro pacote sem mapa DNAT segue input e
  cria estado conntrack prejudicial. Não confirmado por nft trace/conntrack.
- Variação QUIC antiga~1s/~6s foi reproduzida com instrumentação: último
  processamento concluía shutdown, mas helper dormia até deadline sem consultar
  novamente. Ordem atual incorpora correção; não atribuir apenas a build velho.
- Fixtures anteriores rejeitaram payload inválido, erro remoto QUIC7 e shutdown
  sem progresso. Não foram reexecutadas após substituição poll/select.
- UDP close ainda ignora alguns retornos e pode sair zero após erro: pendência.
- Rede e nomes atualizados com autorização: build all e suíte5/5 em6,71s
  (2026-10-12). Artefato confirmou WANs10.0.0.2/.3; não são fixadas no código.
  Usuário aceita IPs WAN automáticos, pois topologia fornece endpoints aos peers.
- Git estava limpo em2026-10-12 antes desta atualização de memória; código e
  comentários do usuário conferidos. Nenhum commit executado pelo agente.
- Conferir Git/código sempre; notas não são especificação definitiva. Patchbay
  pode exigir saída do sandbox para namespaces; não alterar políticas do host.
  Scratchpads em /tmp são temporários. Relatório deprecated/STATIC_ANALYSIS_BUGS
  não revalidado; não confiar sem conferir código.

## Entrega de payload centralizada (2026-10-22)

- Aplicado a pedido do usuário o patch validado em `/tmp/udp-delivery-prototype`.
  `udp_conn` encaminha payload ao TCP e chama callback de recepção também no
  túnel. `recv_reason` na API preserva os códigos de eventos dos backends.
- CHOWNAT retorna payload/tamanho apenas para dados na sequência; controle
  retorna zero bytes. Evento LOST_DATA permanece no backend. QUIC retorna dados
  via SSL_read_ex e expõe pending_data; udp_conn drena antes do select.
- Verificação: configure/build all no temporário passaram; CTest fora do sandbox
  passou 5/5 em 7,13 s. Dentro do sandbox, namespace falhou com EPERM antes dos
  peers. Aplicação no repositório passou git diff --check e comparação byte a
  byte dos quatro arquivos com o temporário. Não houve nova execução da suíte
  no repositório original nem commit.
- Limites: escrita TCP segue bloqueante, com tratamento de parcial/EINTR.
  udp_conn_recv explícito retorna dados sem callback; callback de recepção é
  chamada pelo laço de entrega. Zero bytes não dispara callback. Sem novos
  testes de perda/reordenação, backpressure ou callback reentrante.

### Ajuste solicitado em 2026-10-22

Removido recv_reason. Callbacks de recepção voltaram ao final da leitura de
payload em chownat_udp_recv e ossl_quic_udp_recv, inclusive em modo túnel.
Encaminhamento TCP continua na udp_conn, após retorno do backend; portanto a
callback agora ocorre antes do envio TCP. udp_conn_recv explícito também chama
a callback novamente. Isto substitui a descrição anterior de callback no laço.

Verificação após ajuste: build all, git diff --check e CTest all passaram
(5/5 em 7,21 s no repositório original, fora do sandbox para namespaces).

## Pesquisa de ICE e transferência do socket (2026-10-25)

- Verificado em documentação/API upstream, sem implementação nem teste de rede:
  libnice oferece `nice_agent_get_selected_socket()` explicitamente para aplicações
  existentes assumirem o socket após duplicá-lo e liberar o agente. Retorna NULL
  para candidato relayed; proposta inicial limitada a UDP direto.
  Fonte: https://libnice.freedesktop.org/libnice/NiceAgent.html#nice-agent-get-selected-socket
- Header público da libjuice consultado: permite obter endereços/candidatos
  selecionados, mas não expõe transferência/injeção do socket. Para o modelo
  solicitado, libnice é candidata mais adequada que libjuice.
  Fonte: https://github.com/paullouisageneau/libjuice/blob/master/include/juice/juice.h
- Alternativa Rust: librice/rice-proto é sans-IO e oferece API C; aplicação
  controla I/O. README upstream ainda aponta melhorias pendentes na camada ICE.
  Pion/ICE (Go) aceita PacketConn via UDPMux, mas mux mantém leitor próprio e
  seu Close fecha a conexão subjacente: injeção não equivale a transferência.
  Fontes: https://github.com/ystreet/librice e https://github.com/pion/ice/blob/master/udp_mux.go
- Código local conferido com alterações do usuário preservadas: chownat_init
  sempre faz bind; udp_connection sempre chama hole_punching; teste CHOWNAT UDP
  cria socket e usa remoteport também como porta local. Integração proposta
  precisa aceitar FD já vinculado, destino remoto independente da porta local
  e caminho previamente estabelecido. Launcher em outro processo precisa passar
  o socket (herança de FD/SCM_RIGHTS), além dos endereços. Somente iniciar outro
  socket com endereços descobertos não preserva o caminho validado pelo ICE.
- Proposta, ainda não decisão de implementação: launcher ICE por peer, troca
  de candidatos/credenciais por sinalização, conclusão nos dois lados, repasse
  de socket UDP direto ao CHOWNAT; manutenção posterior pelo protocolo próprio.

## Prova funcional ICE → CHOWNAT preservada (2026-10-25)

Usuário pediu registrar resultados e preservar todos os arquivos para retomar
futuramente. Cópia integral de `/tmp/ice-handoff-test` preservada em
[`docs/experiments/ice-handoff-2026-10-25/`](../experiments/ice-handoff-2026-10-25/README-PRESERVATION.md),
incluindo fontes, patch, launcher C, serviço STUN/sinalização Python, runner
Patchbay Rust, headers/pacote libnice, binários, cache de build e logs.
Cópia comparada por SHA-256 com a origem; manifesto `PRESERVED-SHA256.json`.
`docs/` é ignorado pelo Git: preservação local, sem commit/envio remoto.
Não depender de `/tmp`; consultar instruções de restauração na cópia.

Fatos verificados por execução real, detalhados no `VALIDATION.md` preservado:

- libnice 0.1.21 chega a READY nos dois peers; launcher duplica socket,
  libera GSocket/NiceAgent e faz exec do CHOWNAT com FD herdado. Novo processo
  não carrega ICE. Não foi usado socket novo apenas com endereços descobertos.
- Adaptações só na cópia: aceitar FD herdado, pular segundo bind e retornar
  sucesso de hole_punching nativo no caminho ICE. Dados, encaminhamento TCP,
  keep-alives de 5 s e fechamento CHOWNAT preservados.
- UDP: três rodadas, conteúdo/sequência e fechamento completos; peers exit 0.
- TCP: 16 KiB antes e 16 KiB depois de pausa de 20 s, conteúdo íntegro, echo
  com 32768 bytes/EOF; peers exit 0. NAT UDP/stream timeout 8 s. Controle sem
  keep-alives falhou na retomada, conforme esperado.
- Laboratório separado EIM/APDF com portas traduzidas: 2222→34990 e
  3333→36101, mapa de retorno criado no envio. Tradução por XOR 0x8000,
  sem alocador aleatório geral nem cobertura de colisões/múltiplos hosts.
- Cópia Patchbay: UDP e TCP passaram com Moderate nativo e serviços no ISP,
  removido TODO o bloco próprio (SNAT por udp sport, opened_ports/inbound_guard).
  Nessa execução NAT preservou portas naturalmente; mapa nativo quando houver
  remapeamento/colisão não validado. Regras no projeto original não alteradas.
- Primeiro teste Patchbay transmitiu UDP mas falhou no encerramento do auxiliar;
  fechamento explícito dos writers corrigiu fixture. Resultados finais PASS
  em `logs/patchbay-native-{udp,tcp}-pass/result.txt`.
- SHA-256 de 265 arquivos source/topologia originais confirmou zero mudanças;
  edições prévias do usuário preservadas e nenhuma implementação aplicada.

Limites: QUIC/OpenSSL, TURN, IPv6, perda/reordenação, sessões longas e reinício
ICE não testados. Não afirmar travessia universal: perfil inicial de NAT mudou
porta por destino e ICE direto falhou sem TURN. Integração/API ainda não decididas.

Limpeza pendente: algumas capturas tcpdump recusaram sinais pelo perfil AppArmor
`tcpdump`; sudo exige senha. `cleanup.py` preservado identifica somente capturas
com stdout no diretório temporário do experimento. Não assumir que já encerraram.
Retomar quando solicitado; não integrar/refatorar implicitamente.

## Comparação isolada de hole punching (2026-10-26)

- Verificado no código atual: ossl_quic_hole_punching tem dez msleep(50) após
  descoberta e não interrompe o laço após confirmação; CHOWNAT faz 01/03/03
  sem espera fixa no caminho bem-sucedido. Timeout recv de 1 s não é atraso
  obrigatório. Descoberta simétrica não tem limite total de tentativas.
- Benchmark com funções extraídas literalmente, dois peers UDP loopback e
  barreira, 30 pares por método, sem logs: média do maior tempo por par
  CHOWNAT 0,361 ms; simétrico 502,207 ms. Todos os pares retornaram sucesso.
- Não mede NAT, rede real, perda, início defasado ou handshake QUIC. Não usar
  esses números como média de conexão na Internet. Código ativo preservado.
- Reprodução, fontes extraídas, CSV e limites em
  `docs/experiments/hole-punch-timing-2026-10-26/README.md`. Execução exigiu
  saída do sandbox porque socket retornou EPERM. Nenhum commit.

## Análise ampliada de hole punching (2026-10-26)

- Cópia Git temporária com working tree em /tmp/hole-punch-analysis-cp1wmjx3/repo;
  análise, script, traces e resumo em ANALYSIS.md, analyze.py, results.json e
  summary.json na pasta pai. Removido msleep somente na cópia/variante experimental.
- Relay UDP local sem NAT, 10 pares por método/cenário: CHOWNAT e simétrica
  original passaram baseline, 30 ms por sentido, servidor 250 ms atrasado e perda
  do primeiro pacote cliente. Sem sleep: 0/10 em cada cenário, por sondagens
  MSG_DONTWAIT terminarem antes de confirmação. Não extrapolar taxa para Internet.
- Com 30 ms/sentido, médias até ambos terminarem: CHOWNAT 95,52 ms, original
  535,33 ms. Incluem processos/relay; não comparar baseline com benchmark sem relay.
- Tempo lógico ideal: CHOWNAT 3 mensagens/1,5 RTT para ambos; simétrica original
  22 mensagens/~d+500 ms quando confirmação chega na janela. Memória O(1),
  caminho ideal O(1) em ambos; variação O(D+K) para D descobertas e K=10 repetições.
- Não há limite global na descoberta simétrica; servidor CHOWNAT também pode
  permanecer em laços com mensagens recebidas, pois attempts não aumenta em todos
  os caminhos e contador interno sombreia externo. Cliente tem limite A.
- Perda do ACK final CHOWNAT: cliente sucesso e servidor ainda esperando no limite
  externo 3,2 s em 3/3 sondas; recuperação no restante do backend não avaliada.
- Hash de 264 fontes ativos antes/depois: nenhuma mudança. Não validado NAT,
  handshake QUIC completo ou payload nesta comparação; nenhum commit.

## Select otimizado e comparação com libnice (2026-10-26)

Fatos verificados, substituindo os resultados intermediários de select com janela
fixa de 300 ms:

- Trabalho isolado em `/tmp/hole-punch-analysis-cp1wmjx3/repo`. Fontes ativos não
  alterados: 264 hashes conferidos. Nenhum commit. Protótipo/patch, fontes, scripts,
  traces, tabelas e testes NAT preservados em
  `docs/experiments/hole-punch-select-2026-10-26/FINAL-REPORT.md` e pasta associada.
- Variante select final usa 01 probe / 03 response / 04 ready; RTO inicial500ms,
  backoff, prazo total3s, uma checagem disparada quando READY chega antes de
  confirmar a própria sondagem. Sem msleep nem janela fixa após confirmação.
  Drenagem pré-QUIC preserva datagramas diferentes desses controles.
- Métrica final C: max(fim dos peers)-min(início), CLOCK_MONOTONIC dentro das
  funções, sem criação dos processos. Com RTT60ms, 10 pares: CHOWNAT92,3ms,
  original532,6ms, select93,0ms. Select confirma resposta em ambos~62,2ms;
  não confundir essa validação com conclusão/saída. Controlador observa95,5ms.
- RTT200ms: CHOWNAT302,4ms, original602,9ms, select303,0ms. Com RTT60ms e perda
  da primeira sondagem, 5 pares: CHOWNAT1147,7ms, original564,4ms, select186,0ms.
  Todos os pares desses grupos concluíram. Nominal sem perda:3/22/8 mensagens.
- ICE real: libnice10 Ubuntu0.1.21-2build3 já instalada; headers preservados do
  experimento anterior, sem instalar pacotes. Dois NiceAgents RFC5245, UDP IPv4,
  um componente, credenciais geradas/trocadas, candidatos HOST via mesmo relay.
  Mede disponibilização dos candidatos remotos até ambos READY; payload5bytes
  em ambos os sentidos validado depois. Coleta HOST~1,1–1,4ms separada.
- libnice padrão usa AGGRESSIVE, confirmado nomination-mode=1; modo regular
  explícito usa0. Runtime defaults: pacing20ms, initial-timeout500ms, max-retries3.
  RTT60: padrão82,4ms/10STUN; regular178,3ms/12STUN (10pares cada). RTT200:
  222,6/454,7ms. RTT60+perda inicial:123,4/225,3ms (5pares). Todos passaram.
- RTO é prazo para retransmitir sem resposta, não espera mínima após sucesso.
  RFC8445§14.3 mínimo500ms; RFC8489§6.2.1 detalha transações/retransmissões.
  Não sustentar200ms como mínimo ICE. libnice padrão testado é compatibilidade
  RFC5245/agressiva; não apresentar como única política de ICE atual.
- Caminho ideal constante/memóriaO(1) nos dois protocolos simples. Novo acordo
  de saída~1,5RTT, igual CHOWNAT para ambos; primeira confirmação~1RTT. Custo
  extra de mensagens pequenas não dominou latência no laboratório, mas os
  recursos de autenticação/seleção do ICE não existem nas mensagens3bytes.
- Build WebRTC e runner Patchbay com QUIC UDP/TCP finais passaram: payload,
  encerramento e exit0 de ambos nos dois NATs. Logs nat-final-{udp,tcp}.
  Números da tabela são do relay sem NAT, não desses testes de integração.
- Limites verificados: sem resposta retorna erro~3s. Perda de respostas por200ms
  recuperada3/3; perda de TODOS os READY do cliente falhou3/3, cliente pode sair
  sucesso/servidor expirar. Para produção, responder a retransmissões após
  sinalizar sucesso precisa integrar dispatcher, ou usar ICE real. Não aplicar
  este protótipo como substituto completo/robusto da libnice.
- Benchmark de ICE não inclui STUN discovery público, signaling de rede, TURN,
  múltiplos candidatos ou NAT real; C usa2processos, libnice2agentes/1processo.
  Tratar diferenças sub-ms como ruído, sem afirmar superioridade universal.

## Falha ponta a ponta HP → QUIC validada (2026-10-26)

- Usuário pediu validar a hipótese de que, com confirmação assimétrica, o peer
  restante abortaria no timeout do QUIC. A ideia de HP continua plausível, mas
  **essa garantia não é atendida pelo backend atual**, conforme testes abaixo.
- Fixture baseada no teste QUIC UDP real, wrappers de API com timestamps, portas
  locais independentes e relay UDP; protocolo/backend/budgets da cópia mantidos.
  Artefatos: `docs/experiments/hole-punch-select-2026-10-26/e2e-validation/VALIDATION.md`.
- Cinco cenários (uma execução cada). Sem perda: peers exit0, três rodadas e
  encerramento ~0,136s. Perda total: ambos erro/exit1~3s. READY do cliente perdido:
  servidor HP erro~3s; cliente QUIC erro/exit1~30s. READY do servidor perdido:
  cliente HP erro~3s; servidor ainda esperando aos45s, morto pelo watchdog.
  Todos os pacotes QUIC perdidos após HP: cliente erro~30s; servidor ainda
  esperando aos45s, morto pelo watchdog. Kill NÃO é sucesso/limpeza normal.
- Nos logs, servidores bloqueados chegaram a `QUIC server listening` e não
  saíram do callback connect nem chegaram ao deinit. Código usa listener
  bloqueante e SSL_accept_connection sem deadline do projeto. Cliente usa
  SSL_connect bloqueante antes da mudança para não bloqueante. Limite5s
  existente cobre shutdown, não estabelecimento. ~30s do cliente é observação
  desta build, sem identificar o timer interno, não garantia geral.
- Nenhum cenário de perda emitiu callback CONNECTED. Peers que retornaram erro
  liberaram socket/SSL/ctx/listener e zeraram init; servidores mortos não
  executaram limpeza do projeto. Requisito pendente: deadline de conexão
  cobrindo accept, handshake e hello, com erro/limpeza, antes de afirmar timeout
  ponta a ponta. Não foi aplicada correção neste turno de validação.
- Relatório principal atualizado com aviso de validação reprovada; 264 hashes
  de fontes ativos conferidos e preservados. Sem commit. Não repetir a antiga
  afirmação de que basta confiar no timeout QUIC para ambos saírem.

## Variante select aplicada ao backend ativo (2026-10-26)

- Usuário solicitou explicitamente substituir ossl_quic_hole_punching pela
  variante select implementada no experimento. Aplicado SELECT-PROTOTYPE.patch;
  arquivo ativo comparado byte a byte com a cópia validada: igual. Edição prévia
  do usuário (linha em branco na init) preservada. Nenhum commit.
- Agora ativo: 01 probe/03 response/04 ready, select, relógio monotônico, RTO
  inicial500ms/backoff, prazo HP3s e checagem disparada pelo READY remoto quando
  falta confirmação local. Drenagem pré-QUIC só descarta controles01/03/04 e
  propaga erros, preservando datagramas QUIC já recebidos. Ambos os peers devem
  usar o novo protocolo; não é STUN/ICE completo.
- Build preset all passou; CTest openssl_quic no repositório original passou
  TCP/UDP2/2, com namespaces NAT fora do sandbox. git diff --check passou.
- Isto supera as notas anteriores de que a variante só existia na cópia. O
  timeout do estabelecimento QUIC NÃO foi corrigido nesta mudança: validação
  anterior continua relevante (cliente~30s; servidor sem saída até watchdog45s
  nas falhas descritas). Limite HP3s e shutdown5s não cobrem accept/handshake.

## IntelliSense local (2026-10-27)

- `.vscode/settings.json` apontava para `source/` e para a base antiga
  `source/build/compile_commands.json` (5 entradas). Alinhado com a raiz,
  presets CMake e `build/all/compile_commands.json` (12 entradas). Includes de
  fallback corrigidos para utils, protocolos e OpenSSL local. `.vscode/` é
  ignorado pelo Git; ajuste só local.
- JSON validado; gcc -fsyntax-only usando includes da base atual passou para
  ossl_quic.c e udp_conn.c. As entradas desses dois arquivos têm os mesmos
  includes nas duas bases; a divergência de configuração foi confirmada, mas
  não há confirmação de que ela explique todo sintoma relatado. A interface do
  VS Code não foi inspecionada; recarregar a janela se o diagnóstico persistir.

## Toolchain e captura opcional (2026-10-27)

- Instalados sem sudo: OpenSSL 4.1.0 em `~/.local/openssl-4.1.0`, ativo via
  symlink `~/.local/openssl`; anterior preservado em
  `~/.local/openssl-4.0.0-before-20261027`. SHA-256 oficial conferido; testes
  OpenSSL quicapi/multistream passaram fora do sandbox.
- Rust 1.99.1/Cargo locais: `~/.local/share/rustup` e `~/.local/cargo`.
  Ativação em `~/.bashrc.d/quic-local-toolchain.sh`. Nenhum módulo carregado;
  sistema já tinha Rust 1.92, GCC 8.5, CMake 3.26 e ferramentas de rede.
- Build completo passou após corrigir caminhos fixos /usr/local no exemplo
  OpenSSL. CMake cache usa /usr/bin/gcc e Cargo local. Cinco testes falharam
  inicialmente no diagnóstico tcpdump, antes da verificação dos protocolos.
- strace em namespace temporário com loopback confirmou: socket AF_PACKET e
  bind funcionam; setgroups(1, [0]) retorna EPERM no tcpdump 4.9.3 com -Z root.
  Patchbay define setgroups=deny no user namespace. Não é prova de bloqueio
  geral de captura. Nenhuma alteração de permissões/políticas do host.
- Usuário pediu captura opcional: falhas de inicialização da captura viram
  aviso e capture-status.txt; nft/dados/encerramento permanecem obrigatórios.
  tcpdump removido das dependências obrigatórias do CMake.
- Validação da captura opcional: CTest all passou 5/5 em 7,80 s; todos os
  capture-status.txt registram indisponibilidade, e result.txt confirma dados
  e saídas dos peers. Nenhum critério de protocolo foi removido.
- Build incremental original encontrou rustup tentando baixar rust-src e,
  depois, E0463 para matchers com artefatos existentes. Sem reinstalar Rust:
  compilação offline com binários cargo/rustc diretos em
  /tmp/quic-optional-capture-target passou; runner copiado para o caminho do
  CTest. Não afirmar resolvida a sincronização/cache Rust do build incremental.

## Recepção e encaminhamento TCP (2026-10-28)

- Usuário extraiu tcp_send e deixou udp_recv nos dois caminhos do laço;
  UDP_CONN_OK_TRUNCATED agora é -9. Ajuste solicitado preserva erro de leitura
  e de envio TCP após disconnect, sem encaminhar payload após falha.
- Encerramento já sinalizado pelo backend interrompe o processamento; CHOWNAT
  retorna erro também ao receber fechamento normal. Não converter esse caso
  automaticamente em falha do teste. Guarda após tcp_recv evita ler UDP
  depois de disconnect; guarda após callback evita enviar em socket fechado.
- Cinco executáveis recompilados com CCACHE_DISABLE=1 (cache fora do sandbox).
  CTest all passou 5/5 em 6,62 s fora do sandbox para namespaces. Checagem gcc
  de sintaxe com Wall/Wextra passou antes da compilação. Sem commit.

## QUIC iniciado pelo cliente (2026-10-28)

- A pedido do usuário, hole punching QUIC ativo agora distingue modos: cliente
  envia 01 antes de receber; servidor espera 01 e envia apenas keep-alive UDP
  bruto de 1 byte nos timeouts de 1 s. Após o trigger, segue fase transiente
  existente. Não usa SSL_write para essa manutenção pré-QUIC.
- Build dos dois peers QUIC passou; CTest openssl_quic TCP/UDP 2/2 em 3,87 s.
  Harness temporário /tmp/quic-trigger-check confirmou dois keep-alives antes
  do trigger e primeira resposta 01 somente após trigger; função terminou OK.
  Somente loopback para harness e namespaces temporários para CTest, sem sudo.
- Implementação ativa é a antiga com fase transiente de 10x50 ms; variante
  select está em #if 0 por edição do usuário. Preservada essa escolha.
## IntelliSense local verificado em 2026-10-29

- Estado observado difere da nota de 27/10: `.vscode/settings.json` continha
  somente cmake.sourceDirectory apontando para source/. Git estava limpo.
- Log exthost da sessão 20261029T195822 registra ativação de ms-vscode.cpptools
  por onLanguage:c, sem erro de ativação dessa extensão no log consultado.
- Base build/all/compile_commands.json tem 12 entradas; comandos dessa base
  com -fsyntax-only passaram para utils.c e test_chownat_udp_close.c.
- Ajuste local: raiz CMake no workspace, presets habilitados e compileCommands
  explícito para build/all. Não foi confirmada a resposta de autocomplete na UI;
  ativação e sintaxe válidas não comprovam funcionamento completo do IntelliSense.

## Prazo de keep-alive independente dos eventos (2026-10-29)

Histórico intermediário: a política periódica abaixo foi substituída pela
política de inatividade descrita na subseção seguinte.

- Aplicado a pedido em udp_connection: prazo CLOCK_MONOTONIC de 5 s, select
  espera o menor tempo entre evento do backend e próximo KA; chamada de
  udp_send_ka no final da iteração, inclusive após dados disponíveis ou EINTR.
  process_events permanece no início. Erros do envio são propagados e não se
  chama KA após fechamento. Contador threshold continua restrito aos backends
  sem process_events; agora avança somente no prazo de KA, não por timer.
- Escopo aplicado: agendamento comum. ossl_quic_udp_send_ka continua sem envio
  efetivo; não foi habilitado SSL_write de um byte nem inventado framing de
  controle. Portanto não afirmar que há pacote QUIC/KA periódico na rede.
- Build preset all passou; CTest all 5/5 em 8,61 s, fora do sandbox para
  namespaces. Tentativa no sandbox falhou antes dos peers por EPERM.
- Harness temporário /tmp/udp-ka-deadline-check liga o udp_conn.c ativo com
  backend de teste e socketpair: eventos a cada 20 ms, tráfego a cada 10 ms
  e cenário com timeout não aplicável/sinais a cada 7 ms. Três cenários finais
  passaram com chamadas KA aos 5/10 s (última em 10,001 s sob tráfego).
  Tráfego exigiu execução fora do sandbox, que bloqueava send com EPERM.
  Verifica agendamento, não QUIC real ocioso ou validade do NAT prolongada.
- Compilação do harness com Wall/Wextra/Werror exigiu despromover somente
  enum-int-mismatch preexistente (udp_connection declarado int, definido enum).
  Não corrigido neste escopo. Notas locais anteriores preservadas; sem commit.

### Correção para inatividade, solicitada no mesmo dia

- Substitui a política periódica descrita acima: ready > 0 renova next_ka_us
  após tratar sockets; dados positivos drenados por pending_data também renovam
  o prazo antes do select. Sem variável socket_activity. KA somente no ramo
  de timeout (ready == 0) e se o prazo de 5 s venceu; evento antecipado/EINTR
  não dispara KA nem renova o prazo. tcp_recv propaga retorno de udp_send.
- Usuário excluiu expressamente observar tráfego gerado internamente pelo
  QUIC. Não adicionados contadores BIO, callbacks nem instrumentação de socket.
  process_events sozinho não renova prazo. ossl_quic_udp_send_ka continua
  sem envio efetivo, conforme limite já registrado; nenhum commit.
- Versão final: build all e CTest all 5/5 em 8,52 s passaram (namespaces fora
  do sandbox). Harness recompilado com fonte ativo passou: timers de 20 ms
  sem tráfego chamam KA aos 5/10 s; recepção contínua por ~6 s não chama KA
  durante tráfego, primeiro aos 10,997 s e segundo aos 15,998 s, sempre 5 s
  após a última atividade/KA; timeout não aplicável com sinais de 7 ms também
  passou aos 5/10 s. Harness usa backend de teste, não prova envio QUIC/NAT.

### Contexto consolidado e preferências da discussão

- Intenção do usuário: manter eventos e KA juntos, mas KA somente após
  inatividade observada pelo laço comum. Não usar o simples timeout de evento
  como sinal de 5 s sem tráfego. Preservar ramo de timeout visível e simples;
  dispensar variável socket_activity quando os próprios ramos bastam.
- Estado relido após os testes: usuário simplificou o ramo para
  `else { // ready == 0 (timeout) }` após ready < 0 e ready > 0, e ajustou
  formatação. Semântica equivalente à versão testada; edições preservadas.
  Não reexecutados build/testes nesta atualização exclusivamente documental.
- Escopo de observação: atividade nos caminhos de sockets do laço e payload
  drenado. Não há rastreamento universal de todo send externo à função nem
  dos datagramas internos do QUIC. Não apresentar isso como medição exata da
  última atividade de rede. As funções síncronas ainda podem atrasar o laço.
- Verificado por leitura de ossl_quic.c e udp_conn.c: hole_punching e connect
  executam antes de iniciar next_ka_us/laço de 5 s. No HP QUIC, servidor
  aguarda recv com SO_RCVTIMEO de 1 s e envia um byte UDP bruto ao destino ao
  expirar; cliente repete 01. Esse envio pré-QUIC abre o caminho NAT sem
  depender do KA de manutenção de 5 s. Primeiras sondagens podem ser perdidas.
- Verificado por leitura de chownat/perl/chownat.pl (sem executar Perl/rede):
  servidor entra em can_read(5) antes de qualquer conexão; ao expirar, envia
  send(CHOWNAT, "", 0, $remoteport), datagrama UDP de payload vazio. Primeiro
  envio pode ocorrer após ~5 s ociosos e serve para abrir o mapa NAT. Cliente,
  após accept TCP, repete 01 com espera recv de até 1 s e pausa de 250 ms;
  quando o caminho permite, completa 01 -> 03 -> 03. Bind sozinho não é o
  envio que cria o mapa. Isso descreve o código e o mecanismo esperado, não
  uma validação nova nem uma garantia para todos os NATs.
- Diagrama PlantUML solicitado deve permanecer resumido, como o original:
  process_events/drenagem -> menor prazo -> select -> I/O renova prazo ou
  timeout verifica prazo KA e renova após chamada. Diagrama representa o
  laço de manutenção; não inclui a abertura inicial/hole punching. Não
  expandir com todos os ramos de erro/contadores se pedido for só o resumo.
- Último commit continua 15f9449 (28/10); mudanças desta sessão não commitadas.
  Pendência preservada: ossl_quic_udp_send_ka retorna antes do SSL_write,
  portanto agendar/chamar KA não comprova envio efetivo de pacote QUIC.

## Análise ICE persistente + QUIC (2026-10-29)

- Estado conferido por Git e fontes: working tree estava limpo; usuário criou
  commit 5b1f2f6 (KA por inatividade). Isso supera a indicação histórica acima
  de que o último commit ainda era 15f9449 e alterações estavam sem commit.
- Usuário solicitou análise aprofundada, autorizou agentes para pesquisa e
  plano inicial. Prefere manter ICE vivo para candidatos/relay; esclareceu que
  ideal é FD criado pela infraestrutura, mesmo socket para ICE e QUIC e demux
  central. Não pressupor que escolher portas/interfaces equivale a esse pedido.
- Relatório: [ice-quic-integration-2026-10-29.md](../analysis/ice-quic-integration-2026-10-29.md).
  Plano revisável: [ice-quic-integration.md](../plans/ice-quic-integration.md).
  Arquivos documentais novos; nenhuma integração/instalação aplicada e nenhum
  teste novo de rede. Pesquisa primária e revisão por três agentes, com URLs
  e limites no relatório. Sem commit feito pelo agente.
- Achado API libnice: configura endereços/portas e expõe sockets criados pelo
  agente, mas não foi encontrada adoção pública de FD externo. Mesmo socket
  ICE+QUIC é possível com libnice dona do I/O e payload QUIC por BIO datagrama;
  não usar dois leitores concorrentes nem dup como demux. get_selected_socket
  não retorna socket para candidato local relay/reliable/ICE-TCP; relay apenas
  remoto é diferente. Experimento de 25/10 rejeitava relays dos dois lados.
- Recomendação condicional: libnice se puder possuir os sockets; se FD próprio
  for obrigatório, provar PJNATH pj_ice_sess + STUN/TURN de baixo nível, APIs
  independentes de transporte. Maior integração, licenciamento e consent/
  restart precisam avaliação. Não recomendar fork libnice como caminho padrão.
- Libnice 0.1.24 listada como release (01/09/2026), experimento antigo 0.1.21;
  fontes de implementação consultadas em master sem hash fixado, reconfirmar
  tag antes de ensaio. Headers OpenSSL locais dizem 4.1.0-dev. Libjuice v1.7.4
  rejeita restart; libdatachannel adiciona pilha além da ICE isolada desejada.
- ICE vivo não garante regather automático por nova interface nem failover
  transparente para TURN. Restart não é recriar candidatos locais. Precisar
  signaling alcançável fora do QUIC que caiu, política de recuperação e testes.
- Mesmo QUIC após troca de par é hipótese distinta de reconexão. Provar direto
  e TURN fixos com BIO/listener, depois recuperação, endpoint lógico estável ou
  migração real. SSL_set1_initial_peer_addr não atualiza sessão estabelecida.
- Manutenção ICE/consent/TURN não reinicia idle timeout QUIC. Demux deve seguir
  RFC9443, incluindo origem TURN para resolver sobreposição ChannelData/QUIC;
  não negociar grease_quic_bit se usar esse esquema. Plano exige provas de
  payload, NAT/refresh prolongado, falha, prazos e limpeza sem watchdog=sucesso.
- Discussão posterior: aplicação também precisa observar TCP; não interpretar
  transporte ICE como remover espera TCP. Usuário perguntou por threadless.
  GMainContext pode integrar ao loop existente por prepare/query/check/dispatch
  (descritores + timeout), ou TCP/QUIC podem virar fontes no contexto GLib.
  Thread dedicada + fila/eventfd foi apenas uma alternativa, não requisito.
  Para threadless, get_poll_fd único não é contrato geral suficiente; prever
  vários descritores/eventos e prazo. Plano prioriza prova sem thread dedicada.
  Consultada documentação oficial docs.gtk.org/glib/main-loop.html; sem ensaio.
- A pedido, criado source/libs/protocols/udp_conn/inc/udp_transport.h como
  esboço revisável somente em comentários. Propõe start, send/recv datagrama,
  prepare_wait/dispatch, estado, close/destroy e política de manutenção; reserva
  integração ICE sem dependência libnice e conserva select/TCP sem thread
  dedicada. Nenhum include, mudança de backend, build ou implementação. Script
  confirmou que o arquivo inteiro é um único comentário; sem testes de rede.

## Transporte datagrama básico implementado (2026-10-29)

- Usuário rejeitou o esboço só em comentários e solicitou implementação básica,
  autorizando também udp_transport.c. Substituído header por API C compilável
  e acrescentado source/libs/protocols/udp_conn/src/udp_transport.c.
- API independente de udp_conn_t/libnice: vtable send/recv/prepare_wait/dispatch/
  close, estado, política de manutenção e indicação de HP necessário. UDP direto
  IPv4 aceita FD emprestado/próprio já bound ou cria/binda socket. Transferência
  de posse externa ocorre no sucesso; close idempotente libera estado privado.
- I/O usa MSG_DONTWAIT por chamada sem mudar O_NONBLOCK do FD externo; trata
  EINTR/EAGAIN, datagrama vazio e truncamento explícito por recvmsg/MSG_TRUNC.
  prepare_wait acrescenta leitura/escrita aos conjuntos existentes do select,
  preservando TCP e timeout do chamador. Não há thread dedicada ou polling fixo.
- Só o transporte direto está implementado. Não há libnice, BIO, signaling ou
  integração dos backends. READY não comprova conectividade/HP/handshake. UDP
  direto não filtra/autentica origem. Estados futuros não são prova de ICE.
- CMake passa a compilar o novo .c pela descoberta existente; em BUILD_TESTING
  registra unit_udp_transport com teste próprio em source/tests/unit/.
- Verificação: C11 Wall/Wextra/Werror sem diagnósticos para implementação e
  teste; configure/build all passaram. CTest all 6/6 em 8,50 s fora do sandbox:
  novo teste e cinco regressões NAT existentes. Novo teste cobre UDP binário
  bidirecional, TCP+UDP no mesmo select, vazio vs AGAIN, truncamento/limites de
  datagrama, timeout sem leitura, FD emprestado/adotado/criado e falha de init
  sem tomar posse. Não testa travessia NAT da nova camada, ICE ou backpressure
  por EAGAIN de envio. Código dos backends preservado; sem commit.

### Armazenamento do transporte fornecido pelo chamador

- Usuário exigiu ausência de alocação dinâmica nesta camada: dados internos
  devem ter tipo udp_transport_data_t e região static definida acima. Exposto
  esse tipo no header e tipado transport.data como ponteiro para ele. Exemplo:
  static udp_transport_data_t data; udp_transport_t t = {.data = &data}.
- Removidos calloc/free e stdlib da implementação. init exige data não nulo,
  prepara estado temporário automático e só grava na região externa no sucesso.
  close reinicializa o conteúdo sem liberar/desassociar a memória; repetição e
  reinit continuam funcionando. Posse do FD é independente da posse da memória.
- Teste usa duas regiões static e verifica ausência de storage, close antes
  de init, preservação do ponteiro após close/falha e reutilização. C11 com
  Wall/Wextra/Werror passou; nm -u no objeto confirmou ausência de referências
  a malloc/calloc/realloc/free. Build all e CTest 6/6 em 8,64 s passaram fora
  do sandbox para sockets/namespaces. Sem implementação ICE/BIO; sem commit.

### Erros do transporte somente por status

- Usuário pediu remover manutenção de errno, após já remover atribuições em
  alguns wrappers. Edições e formatação do usuário preservadas. Eliminadas
  atribuições restantes e save/restore de errno; header não manda consultar
  errno. API informa OK/AGAIN/TRUNCATED/ERROR. Testes não exigem errno da API.
- Mantidas somente leituras internas de errno para EINTR/EAGAIN/EWOULDBLOCK
  em sendto/recvmsg; necessárias para implementar retries e AGAIN. Teste ainda
  consulta EBADF da chamada POSIX fcntl para verificar fechamento de FD.
- Build all e sintaxe C11 Wall/Wextra/Werror passaram. Teste unit_udp_transport
  1/1 passou fora do sandbox para sockets loopback. Suíte NAT não repetida:
  backends não foram alterados nem usam a camada nova. Sem commit.

### Truncamento tratado como erro

- A pedido do usuário, removido UDP_TRANSPORT_TRUNCATED. MSG_TRUNC retorna
  UDP_TRANSPORT_ERROR antes de preencher received; wrapper mantém received=0.
  Datagrama é consumido, mas prefixo não é apresentado como payload válido.
- Na releitura, usuário já havia removido o tratamento EAGAIN/EWOULDBLOCK de
  direct_recv: ausência de dados também retorna ERROR. Preservada essa edição,
  atualizados comentário e expectativas do teste. AGAIN continua para envio.
- Build all, C11 Wall/Wextra/Werror e unit_udp_transport 1/1 passaram; teste
  confirma received zerado em truncamento e integridade do datagrama seguinte.
  Teste loopback fora do sandbox; suíte NAT não repetida. Sem commit.

### HAL de transporte sem campo maintenance

- Usuário definiu a direção: udp_conn usa HAL de transporte, com implementações
  UDP POSIX e ICE da mesma interface. Removidos udp_transport_maintenance_t,
  constantes, campo maintenance e sua inicialização. Manutenção interna de uma
  implementação futura ICE pertence a prepare_wait/dispatch; não ao chamador
  selecionando política por campo. Isso não remove timers/controles de protocolo.
- udp_transport_ops_t já fornece o despacho da HAL. Implementação disponível
  segue somente UDP direto; integração dos backends/udp_conn e ICE permanece
  pendente. Campo needs_hole_punching não alterado neste pedido.
- Build all, sintaxe C11 Wall/Wextra/Werror e unit_udp_transport 1/1 passaram.
  Teste fora do sandbox por sockets loopback; suíte NAT não repetida. Sem commit.

### HAL somente com espera de leitura

- Usuário pediu remover todos os pontos de espera de escrita, inclusive do
  contrato HAL. Removidos write_fds, wait_write, UDP_TRANSPORT_WRITABLE e
  UDP_TRANSPORT_AGAIN. send retorna ERROR para falhas POSIX ou envio incompleto;
  mantém somente repetição por EINTR. recv segue ERROR sem dados/truncado.
- prepare_wait acrescenta somente read_fds; dispatch informa READABLE; teste
  passa NULL como conjunto de escrita no select, preservando espera TCP+UDP.
  Essa decisão supera a sugestão anterior de manter escrita para ICE futuro.
  Não afirmar validada a integração GLib/ICE com o contrato simplificado.
- Build all, C11 Wall/Wextra/Werror e unit_udp_transport 1/1 passaram. Checagem
  nos três arquivos confirmou ausência dos símbolos removidos. Sem rede NAT
  nova e sem commit; implementação ICE/BIO permanece pendente.

### API simplificada: transporte controla a espera UDP/TCP

- Após discutir isolamento, usuário solicitou implementação só de UDP puro,
  API mínima e formatação espaçada. Removidos prepare_wait/dispatch, tipo de
  espera exposto, max_fd/has_timeout públicos, get_state/enum de estados e
  needs_hole_punching. udp_conn/backends ainda não foram migrados.
- API atual: init_udp, send, recv, wait, set_tcp_fd e close. Vtable tem
  send/recv/wait/close. wait encapsula select, copia o timeout do chamador e
  retorna somente UDP_TRANSPORT_READABLE e TCP_TRANSPORT_READABLE, combináveis.
  NULL timeout espera sem prazo; OK/events=0 indica timeout. Detalhes do
  transporte não aparecem como eventos públicos; futuro transporte deve consumir
  controles internamente e só sinalizar datagramas do protocolo superior.
- TCP é emprestado, opcional e configurado como -1 quando ausente. set_tcp_fd
  atualiza observação após accept/connect ou a desabilita antes de fechar. close
  nunca fecha TCP. UDP continua com ownership próprio/emprestado e memória
  udp_transport_data_t fornecida pelo chamador, sem heap.
- Preservado send simples editado pelo usuário. recv também passou a uma única
  chamada, retornando ERROR em qualquer falha; sem errno, laço EINTR ou AGAIN.
  MSG_DONTWAIT e detecção de truncamento via recvmsg permanecem. Truncado/sem
  dados é ERROR com received=0. .ops=NULL marca instância inativa; sem estado ICE.
- Build all, C11 Wall/Wextra/Werror e unit_udp_transport 1/1 passaram. Teste
  loopback cobre eventos UDP+TCP juntos, só TCP, desabilitar/reabilitar TCP,
  timeout sem eventos, timeout de entrada preservado, limites de FD, datagramas
  e ownership. Suíte NAT não repetida; backends intactos. Sem implementação ICE.
- Usuário pediu mensagem de commit durante a conclusão; sugerida:
  Add a UDP transport HAL with internal select and optional TCP readiness events.
  Nenhum commit feito pelo agente.
- Ao concluir as notas, Git mostrou que o usuário já criou d926924 com essa
  mensagem; arquivos de implementação/teste estão commitados. Restaram apenas
  estas atualizações documentais no working tree. Não atribuir commit ao agente.

## Separação HAL/direto (2026-10-30)

- Usuário esclareceu que udp_transport_t é handler seguindo udp_conn_t, não
  base embutida em outro handler. Config comum via transport_config, config/data
  opacos para implementação, ops e FDs UDP/TCP no handler. Sem heap.
- direct_udp_transport.h/.c concentram config/data específicos e init direto;
  udp_transport.c mantém wrappers comuns. Não existe direct_udp_transport_t.
- Configure/build all e diff-check passaram; unit_udp_transport 1/1 passou fora
  do sandbox para sockets loopback. NAT não reexecutado; backends não migrados.

### Abertura pela vtable (2026-10-30)

- A pedido, direct_ops agora expõe open (antigo init específico, agora static
  direct_open). Handler seleciona .ops = &direct_ops; udp_transport_open chama
  a operação com config/data do handler. Não há init direto público.
- opened distingue associação da vtable de abertura efetiva. close preserva
  ops/config/data, mantém idempotência e permite reabrir; operações de I/O
  rejeitam handler fechado. Init duplicado/falha não transfere posse.
- Build all, sintaxe C11 Wall/Wextra/Werror e diff-check passaram. Unitário
  1/1 passou fora do sandbox para loopback, incluindo reabertura e posse.
  Sem nova execução NAT ou migração dos backends.

### Espera compartilhada e despacho específico (2026-10-30)

- A pedido, removido ops->wait; prepare_wait acrescenta descritores de leitura
  e prazo interno em udp_transport_wait_t, dispatch consome eventos/controles
  após select e sinaliza somente payload UDP. Sem espera de escrita.
- udp_transport_wait centraliza select/TCP, combina payload enfileirado com
  prontidão TCP, usa deadline CLOCK_MONOTONIC para prazo externo e continua
  após controles/timers sem renovar esse prazo. EINTR continua sendo ERROR.
  Configuração/desassociação do TCP também migrou para open/close comuns.
- Direto apenas registra FD UDP e traduz legibilidade em payload disponível;
  criação/bind/send/recv/posse/fechamento UDP continuam específicos.
- Build all, sintaxe C11 Wall/Wextra/Werror e diff-check passaram. Unitário
  1/1 em 0,04 s fora do sandbox: regressões UDP/TCP/posse e backend simulado
  com dois FDs de controle, timers repetidos, payload enfileirado, timeout total
  e erro de dispatch. Não comprova integração ICE/GLib, TURN ou NAT novo.

### Contrato com um UDP e TCP opcional (2026-10-30)

- Usuário reafirmou um FD UDP obrigatório e um TCP opcional (-1 desabilita).
  Removido prepare_wait: camada comum monta integralmente os fd_sets/max_fd.
  Nenhum conjunto/descritor de espera é exposto à implementação.
- dispatch opcional recebe flags UDP, processa controles/timers e devolve
  disponibilidade de payload/prazo interno. Chamado antes da espera sem evento
  bruto e após select; payload enfileirado é preservado separadamente. Direto
  dispensa dispatch: socket legível já significa datagrama disponível.
- Isto substitui o contrato de múltiplos descritores da nota anterior; não
  comprova viabilidade de libnice/GLib com esse contrato escolhido.
- Build all, sintaxe C11 Wall/Wextra/Werror e diff-check passaram; unitário
  1/1 em 0,04 s fora do sandbox. Simulação usa controles no único FD UDP,
  timers, payload enfileirado+TCP, deadline e rejeição de FD UDP inválido.

### Dispatch explícito no direto (2026-10-30)

- Usuário pediu restaurar direct_dispatch na direct_ops. Callback explícito
  preserva evento UDP de payload e informa ausência de timeout interno.
  Isso supera a escolha anterior de omitir dispatch no transporte direto.
- Build all e unit_udp_transport 1/1 passaram (loopback fora do sandbox).

### Responsabilidade exata de dispatch (2026-10-30)

- Correção explícita do usuário: dispatch deve consultar read_fds com FD_ISSET
  e preencher flags readable UDP/TCP, não receber flags já calculadas.
- Contexto agora inclui read_fds; camada comum monta os dois FDs e executa
  select, dispatch obrigatório interpreta o resultado. Antes de esperar recebe
  conjunto vazio para timers/filas. Direto mapeia UDP e TCP válidos em flags.
  Isso substitui descrições anteriores de callback opcional ou pass-through.
- Build all, C11 Wall/Wextra/Werror, diff-check e unitário 1/1 passaram;
  testes de rede loopback fora do sandbox. Nenhuma integração ICE validada.

### get_timeout separado de dispatch (2026-10-30)

- A pedido, acrescentados ops->get_timeout e wrapper udp_transport_get_timeout
  (timeval relativo + has_timeout; flag zero/operacao ausente = sem timer).
  Direto implementa consulta sem timer. Wrapper valida prazo e propaga erros.
- wait consulta antes de cada select, inclusive o primeiro e em novas chamadas,
  combina prazo interno com deadline total do chamador. dispatch ocorre só após
  select, interpreta read_fds e preenche eventos; contexto não contém timeout.
  Sem buffered, callback antecipado ou prepare_wait. Formatação do usuário
  preservada. API/backend ativos de udp_conn ainda não migrados.
- Build all, sintaxe C11 Wall/Wextra/Werror e diff-check passaram. Unitário
  1/1 em 0,05 s fora do sandbox: timer antes de tráfego, renovação da consulta
  entre chamadas, limite externo, ausência de timer no direto, erro/prazo
  inválido de get_timeout, controles e eventos UDP/TCP. Não valida ICE/NAT novo.

### Consulta de timeout privada (2026-10-30)

- Usuário esclareceu que get_timeout não deve ser API pública. Removida
  declaração/exportação udp_transport_get_timeout; helper static get_timeout
  permanece usado só por udp_transport_wait, delegando à operação da vtable.
- Build all, C11 Wall/Wextra/Werror, diff-check e unitário 1/1 passaram.
  nm -g --defined-only confirmou ausência de símbolo get_timeout exportado.
- Usuário também perguntou se uma função sobra, sem nome/seleção no contexto
  IDE; solicitada identificação, sem remover outra função por suposição.

### Setter TCP removido (2026-10-30)

- Usuário identificou udp_transport_set_tcp_fd como função sobrando. Removida
  declaração/implementação; chamador atualiza tcp_fd no handler diretamente,
  usando -1 para ausência/desassociação. Validação permanece na espera.
- Teste usa atribuição direta; descritores inválidos agora são verificados em
  udp_transport_wait, mantendo cobertura de ativar/desativar TCP sem fechá-lo.
- Build all, C11 Wall/Wextra/Werror, diff-check e unitário 1/1 passaram;
  sockets loopback fora do sandbox, sem nova execução NAT.

### Uma espera por chamada; loop pertence a udp_conn (2026-10-30)

- Usuário definiu udp_transport_wait como substituto do select no laço udp_conn.
  Removidos loop interno, deadline total e clock monotônico da camada comum.
  Cada chamada consulta get_timeout, escolhe menor prazo, executa select uma
  vez e dispatch uma vez após sucesso, inclusive com conjuntos vazios/timeout.
- OK/events=0 pode representar timer interno ou controle consumido, além de
  timeout do chamador. Chamador decide repetir e mantém seus próprios prazos.
- Deadlines internos pertencem aos dados da implementação: get_timeout calcula
  restante sem reiniciar; dispatch atende vencimento e agenda próxima ação.
  Evento TCP/UDP antecipado não deve renovar esses deadlines por si só.
- Build all, C11 Wall/Wextra/Werror, diff-check e unitário 1/1 em 0,02 s
  passaram. Teste verifica uma consulta/dispatch por chamada, retorno sem payload
  após controle/timer e ambos os casos de menor timeout. Loopback fora do sandbox.
  udp_conn ativo ainda usa select diretamente; migração permanece pendente.

## recv com timeout na interface de transporte (2026-10-30)

- Usuário autorizou somente udp_transport, mantendo CHOWNAT/udp_conn intactos.
  recv público ganhou último argumento const timeval *timeout: zero consulta,
  positivo limita espera, NULL sem limite imposto pelo chamador. Prazo de 1 s
  é passado como {.tv_sec=1}, não fixado pela biblioteca.
- Wrapper desabilita observação TCP temporariamente, chama wait uma vez,
  restaura tcp_fd em sucesso/erro e chama ops->recv somente com evento UDP.
  Vtable recv e implementação direta não mudaram. Timeout/controle/timer sem
  payload retornam ERROR/received=0; continuação após controle ICE não implementada.
- Build all, C11 Wall/Wextra/Werror e diff-check passaram. Unitário 1/1 em
  2,02 s fora do sandbox: timeout de 1 s sem UDP, TCP pendente sem UDP,
  preservação do timeout/FD TCP, consulta zero, NULL com datagrama já pronto,
  truncamento, vazio e posse. Suíte NAT não repetida: backends não usam HAL.
- Esboços de integração discutidos anteriormente continuam sem aplicação.

### recv com read_fds local (2026-10-30)

- Usuário corrigiu a direção da simetria send/recv: recv deve ter fd_set local,
  não usar udp_transport_wait. Aplicado select local somente no FD UDP, seguido
  de ops->recv; não consulta/altera tcp_fd nem chama get_timeout/dispatch.
- Contrato de timeout permanece zero/imediato, positivo/limite, NULL/sem prazo.
  Send permanece sem timeout; o esboço de escrita não foi aplicado. CHOWNAT,
  udp_conn e implementação direta não alterados nesta etapa.
- Build all, C11 Wall/Wextra/Werror, diff-check e unitário 1/1 passaram;
  regressões existentes incluem duas esperas de 1 s, TCP pendente, payloads,
  vazio/truncamento e posse. Loopback fora do sandbox; sem nova suíte NAT.

### send com timeout e write_fds local (2026-10-30)

- A pedido, send público ganhou último argumento const timeval *timeout;
  select local observa apenas escrita UDP antes de ops->send. Zero consulta,
  positivo limita espera, NULL sem prazo. Timeout/erro retornam ERROR.
- Não usa wait/get_timeout/dispatch nem consulta/altera TCP. Vtable send e
  implementação direta preservadas; CHOWNAT/udp_conn continuam sem integração.
- Build all, C11 Wall/Wextra/Werror e diff-check passaram. Unitário 1/1 em
  2,02 s fora do sandbox cobre envio com prazo positivo/zero/NULL, prazo
  inválido, preservação do argumento/TCP e integridade de datagramas reais.
- Harness /tmp/udp-send-timeout-check.c com socketpair AF_UNIX datagrama e
  backend de teste confirmou fila cheia: prazo de 50 ms e zero retornam ERROR
  sem invocar envio; após drenar fila, envio conclui com payload íntegro.
  Verifica espera por escrita sob pressão simulada, não backpressure UDP/NAT.

## CHOWNAT usa transporte direto (2026-10-30)

- Usuário aprovou o esboço CHOWNAT com envios de timeout zero e recepções de
  HP/fechamento com udp_recv_timeout_sec (1 s nos exemplos). Todos os envios,
  controles/retransmissões e leituras agora passam pela HAL; dados do laço
  usam recv com timeout zero. Não criado helper recv_control.
- Bind, SO_REUSEADDR, SO_RCVTIMEO e close UDP removidos do backend CHOWNAT.
  Adicionado udp_transport_t *transport em udp_conn_t; init CHOWNAT exige
  transporte aberto. Os três exemplos CHOWNAT fornecem armazenamento static,
  configuram HAL direta com FD externo próprio, abrem antes de udp_conn_init
  e fecham depois de udp_conn_deinit (também limpam se init falhar).
- Integração deliberadamente parcial: udp_conn.c mantém select/laço original;
  wrappers udp_conn_init/deinit não abrem/fecham HAL. QUIC não modificado.
  FD UDP em udp_session permanece alias usado pelo laço atual. Migração da
  espera comum e ownership comum permanece para passo posterior.
- Build all e diff-check passaram. CTest all fora do sandbox passou 6/6 em
  10,57 s: CHOWNAT TCP 1,50 s (16 KiB/echo), UDP 1,40 s (3 rodadas), UDP close
  1,43 s; QUIC TCP 2,13 s/UDP 2,08 s e unitário 2,02 s. Valida os cenários
  atuais NAT e fechamento conforme critérios existentes; UDP close ainda tem
  critérios internos mais fracos já registrados. Perda, retransmissão e peer
  ausente não testados de novo. Tamanhos/framing de retransmissão preservados.
- Falhas de envio HAL passam a retornar UDP_CONN_ERR, inclusive keep-alive,
  em vez do exit anterior no envio KA. Sem commit pelo agente.

## Duração QUIC ~2 s investigada (2026-10-30)

- Usuário comparou tempos atuais acima de 2 s com histórico médio ~1,40 s.
  Sem editar código de produção, três repetições CTest de cada QUIC passaram:
  TCP 2,02/2,11/2,09 s; UDP 2,06/2,05/2,17 s.
- Baseline do HEAD f23d8d6, antes das mudanças não commitadas, extraído por
  git archive em /tmp/quic-timing-baseline-6rugj9ij e compilado source-only.
  Mesmo runner/topologia/working directory atuais: TCP 1,950/2,049/2,070 s e
  UDP 2,097/2,051/2,038 s, todas as seis execuções zero. Baseline também é ~2 s;
  não há aumento ~0,7 s explicado pelas alterações atuais CHOWNAT/HAL.
- Logs/captura: servidor registra timeout na descoberta antes do primeiro
  keep-alive bruto de 1 byte. TCP atual: primeira sonda cliente 22:27:33.143023,
  KA servidor 33.771047 (~628 ms depois), descoberta e transiente até ~34.27;
  dados/shutdown em poucos ms seguintes. Baseline tem gap semelhante ~633 ms.
  Isso localiza espera na abertura do caminho anterior ao QUIC; transiente
  existente tem 10x50 ms. Não confundir com a antiga espera de shutdown de 5 s.
- Blame atribui servidor passivo/KA após timeout ao commit 15f9449 (28/10).
  Não foi comparado binário anterior a esse commit; portanto origem exata da
  referência histórica ~1,40 s não comprovada por este ensaio. Baseline e
  resultados/logs/capturas preservados no temporário acima. Sem correção aplicada.

### Comparação com HEAD~5 confirma origem do aumento QUIC (2026-10-30)

- Pedido explícito: comparar cinco commits atrás. HEAD f23d8d6, HEAD~5 b0c39ed.
  Extração/compilação source-only em /tmp/quic-five-commits-y0v42sto; mesmo
  runner/topologia/bibliotecas/cwd atuais. Código de produção não alterado.
- Três execuções intercaladas por modo/variante, todas as 12 passaram:
  b0c39ed TCP 1,338/1,414/1,473 s (média1,408), UDP1,202/1,235/1,212 (1,216).
  HEAD com somente a função hole punching antiga TCP1,426/1,414/1,450 (1,430),
  UDP1,216/1,228/1,184 (1,209). Atual normal medido antes: médias2,073/2,093.
- Substituir só HP recupera tempos antigos, isolando a mudança 15f9449 na
  topologia atual. Antes os dois peers enviavam 01; servidor atual espera
  trigger e só abre caminho pelo KA bruto após primeiro timeout de 1 s.
  Captura atual tem ~628 ms entre sonda cliente e envio servidor; antiga
  responde em ~0,3 ms após sonda capturada. Transiente10x50ms existia em ambos.
- Isso supera a incerteza histórica sobre a referência ~1,40 s na nota anterior.
  Não é prova de desempenho em todos os NATs. REPORT.md, timings.json, fontes,
  logs e capturas no temporário acima. Sem correção aplicada ou commit.

### KA pré-recepção QUIC aplicado (2026-10-30)

- Usuário aprovou mover KA bruto do servidor para antes de recv em cada
  tentativa da descoberta, expressamente sem do/while. Envio único de 1 byte,
  erro se retorno !=1. Cliente continua enviando 01; servidor só envia 01
  na fase transiente após receber trigger válido. Timeout1s/transiente mantidos.
- Build all e diff-check passaram. Três execuções de cada QUIC no CTest,
  fora do sandbox, passaram: TCP1,38/1,52/1,51 s (média1,47), UDP1,26/1,24/1,26 s
  (média1,25). Recupera tempos anteriores na topologia testada. Demais cenários
  não repetidos nesta mudança específica; sem commit pelo agente.

## Integração comum udp_conn/transporte aplicada (2026-10-30)

- Usuário aprovou o esboço completo; EINTR sem tratamento especial. O laço
  continua em udp_conn e usa udp_transport_wait/eventos UDP/TCP no lugar de
  select/FD_ISSET próprios. TCP aceito/conectado é associado a transport->tcp_fd.
- udp_conn_init abre a HAL antes do protocolo e fecha se init do protocolo
  falhar. udp_conn_deinit finaliza o protocolo antes de fechar TCP e HAL UDP;
  deduplica accepted_sock/socket_fd e invalida descritores. CHOWNAT não fecha
  mais TCP nem depende de udp_session/tcp_session para operações do protocolo.
- Cinco exemplos usam o ciclo comum; QUIC recebeu somente a adaptação de
  bind/reuse/ownership à HAL direta. Seu I/O OpenSSL/hole punching ainda usa
  FD UDP via alias em udp_session; BIO próprio/ICE não implementados.
  Esta nota supera o estado de integração parcial descrito acima.
- Build all e git diff --check passaram. CTest all fora do sandbox: 6/6 em
  8,93 s; unit 2,02 s, CHOWNAT TCP 1,34 s/UDP 1,43 s/close 1,45 s,
  QUIC TCP 1,47 s/UDP 1,21 s. Critérios e limitações dos testes preservados.
- Harness temporário /tmp/udp-conn-lifecycle-check.c, com transporte/protocolo
  simulados e pipes, compilado com -Wall -Wextra e executado com sucesso:
  erro de init fecha transporte; init duplicado preserva FD; deinit ocorre
  com FDs abertos; limpeza após sucesso/erro preserva status e fecha TCP
  compartilhado uma vez. Não adicionado teste permanente ou helper de produção.

## Retornos de fechamento CHOWNAT corrigidos (2026-10-31)

- Aplicados somente os dois ajustes do esboço aprovado: disconnect_recv retorna
  ERR ao esgotar tentativas de recepção; udp_recv ao receber 02 propaga o
  resultado do handshake, convertendo OK em CLOSED. Estado terminal preservado.
- Alterações do usuário no laço udp_conn preservadas; versão conferida já
  trata CLOSED antes de erro nos dois caminhos de recepção.
- Build all passou; CTest network_chownat fora do sandbox passou 3/3 em4,33s
  (TCP1,51s, UDP1,42s, close1,39s). Falha por ausência de confirmação não foi
  exercitada por teste específico. diff-check aponta whitespace preexistente
  na alteração do usuário em udp_conn.c:198, deixado intacto. Sem commit.

## Hole punching QUIC pela HAL (2026-10-31)

- Usuário aprovou o esboço: função ativa ossl_quic_hole_punching usa somente
  udp_transport_send/recv para datagramas. Envios e leitura transiente têm
  timeout zero; descoberta espera1s por chamada. Removidos SO_RCVTIMEO e
  dependência de udp_session nesta função; sequência e10x50ms preservados.
- Limitação discutida e aprovada: HAL não distingue timeout de falha real;
  ERROR de recepção repete descoberta ou ignora leitura transiente. Truncamento
  também retorna ERROR, em vez da antiga aceitação de prefixo por recv de3bytes.
  Sem deadline total novo ou alteração de contrato da HAL.
- Build all e diff-check passaram. CTest all fora do sandbox passou6/6 em9,05s:
  unit2,02s; CHOWNAT TCP1,50s/UDP1,45s/close1,40s; QUIC TCP1,48s/UDP1,20s.
  Valida cenários existentes; falhas persistentes/perda não reensaiadas.
- Próxima etapa ainda proposta: BIO datagrama para cliente/listener substitui
  SSL_set_fd; adaptar progresso de handshake e shutdown ao transporte.
  OpenSSL ainda faz I/O direto no FD após hole punching. ICE não implementado.

## NOT_READY no transporte (2026-10-31)

- Esboço aprovado/aplicado: UDP_TRANSPORT_NOT_READY=1 em send/recv para
  select sem prontidão e EAGAIN/EWOULDBLOCK da implementação direta.
  Falha real, EINTR e truncamento permanecem ERROR; received continua0 em falha.
  wait permanece OK/events0 em timeout, sem mudança de assinatura.
- Quatro expectativas existentes de recepção sem dados/timeout atualizadas no
  unitário, incluindo UDP vazio com TCP pendente. Documentação do header alinhada.
  CHOWNAT e chamadores QUIC não precisaram mudar: comparam retorno com OK.
- Build all, diff-check e CTest all fora do sandbox passaram:6/6 em8,92s.
  Unit2,02s; CHOWNAT TCP1,34s/UDP1,43s/close1,44s; QUIC TCP1,45s/UDP1,22s.
  Pressão de escrita/EAGAIN não exercitada especificamente nesta rodada.
- Discussão posterior propôs BIO próprio como alternativa ao par com filas;
  nenhuma implementação BIO autorizada/aplicada. Metadados de endereço/MTU e
  espera opcional por escrita ainda precisam de esboço/decisão. Sem commit.

## Origem fixa validada no transporte direto (2026-10-31)

- Usuário aprovou o esboço: direct_recv preenche msg_name/msg_namelen em
  recvmsg e exige tamanho sockaddr_in, família AF_INET, IP e porta iguais
  ao peer configurado. Origem diferente é consumida e retorna ERROR/received0,
  sem continuar esperando. Consequência CHOWNAT discutida: pode encerrar sessão.
  Contrato documentado no header; sem saída pública de origem nem BIO novo.
- Build all e diff-check passaram. Primeira suíte: cinco testes de rede passaram
  (CHOWNAT TCP1,33s/UDP1,42s/close1,40s; QUIC TCP1,43s/UDP1,21s). Unit falhou
  no cenário que recriava A em porta aleatória, mantendo o peer antigo em B.
  Ajuste mínimo da fixture: criar A na porta original já liberada. Após rebuild,
  unit1/1 passou em2,02s; testes de rede não repetidos, sem mudança de produção.
- Harness temporário /tmp/udp-peer-filter-check.c compilado com Wall/Wextra/
  Werror e executado fora do sandbox passou: mesmo IP/porta diferente rejeitado,
  IP127.0.0.2/mesma porta rejeitado, datagramas consumidos sem nova leitura e
  peer correto aceito. Harness não incorporado à suíte. Sem commit.

## FDs somente no handler do transporte (2026-10-31)

- Esboço aprovado/aplicado: removidos udp_transport_config_t e o ponteiro
  transport_config. udp_fd/tcp_fd são fornecidos diretamente no handler;
  udp_fd=-1 solicita criação e tcp_fd=-1 indica ausência. Config específica
  direct preservada. direct_open guarda initial_fd local para ownership e
  limpeza em falha, só grava o novo FD no handler após sucesso.
- Cinco exemplos WebRTC e unitário migrados. Falha de abertura com FD externo
  preserva esse FD no handler e não toma posse; expectativa do unit ajustada.
  Reabertura após close exige repor os FDs desejados; não há config comum
  guardando os anteriores. Busca rg confirmou ausência das referências antigas
  em fontes/headers C de source.
- Build all e diff-check passaram. CTest all fora do sandbox passou6/6 em9,01s:
  unit2,02s; CHOWNAT TCP1,33s/UDP1,42s/close1,48s; QUIC TCP1,49s/UDP1,27s.
  Sem implementação BIO ou metadados novos; mudanças anteriores preservadas.

## BIO próprio isolado implementado (2026-10-31)

- Usuário pediu implementação em ossl_quic_bio.c/.h sem ligação ao backend.
  Como get_info/origem opcional ainda não existem na HAL, foi feita pergunta
  explícita; usuário escolheu configurar o BIO para o peer fixo atual, mantendo
  HAL intacta. Essa decisão supera o esboço dependente de get_info/recv_from.
- Config emprestada contém transport, peer IPv4 e max_datagram_size de payload
  UDP (1200..65507). Deve coincidir com o peer filtrado pelo transporte; os
  datagramas entregues com sucesso recebem esse endereço no BIO. Não serve
  como fonte real para transporte que não garanta peer fixo. Sem heap adicional
  próprio, filas ou thread; BIO e BIO_METHOD são alocados pelo OpenSSL.
- API do módulo: ossl_quic_bio_method_new e ossl_quic_bio_new. Callbacks create,
  destroy, ctrl, sendmmsg/recvmmsg privados. Até um datagrama por chamada,
  timeout0, NOT_READY gera erro BIO_R_NON_FATAL, falha gera TRANSFER_ERROR.
  Destino diferente, endereço local explícito, flags/stride inválidos e envio
  acima do limite são rejeitados. Truncamento continua fatal. Ctrl fornece peer,
  limite de payload, overhead0, capacidades e ausência de descritor de espera.
- Liberação do BIO não fecha transporte/socket nem libera config. Método deve
  sobreviver às instâncias. Teste temporário verificou também referências rbio/
  wbio liberadas por SSL_free, com transporte preservado e blocking desabilitado.
- Reconfigure/build all incluiu o novo .c pelo glob existente. Wall/Wextra/Werror
  e diff-check passaram. Harness /tmp/ossl-quic-bio-check.c executado fora do
  sandbox passou: payload binário, peer, processamento parcial de lote, vazio,
  ausência de dados, truncamento, origem indevida, limites, flags, erros de envio
  simulados, controles e ownership. Não foi registrado novo teste permanente.
- CTest existente passou6/6 em9,09s: unit2,02; CHOWNAT TCP1,32/UDP1,49/close1,45;
  QUIC TCP1,55/UDP1,25. Esses QUIC ainda usam SSL_set_fd; nenhum handshake/dado
  QUIC real pelo novo BIO foi testado. Nenhuma ligação nova em ossl_quic.c/.h,
  nem alteração HAL nesta etapa. Integração/handshake/shutdown ficam pendentes.

## Espera por leitura/escrita UDP/TCP (2026-10-31)

- Usuário aprovou aplicar a máscara requested_events em udp_transport_wait,
  incluindo UDP_TRANSPORT_WRITABLE=4 e TCP_TRANSPORT_WRITABLE=8. Mantidos os
  valores de leitura anteriores. wait_t contém read_fds/write_fds; a camada
  comum monta os conjuntos solicitados, executa select e o direct_dispatch
  traduz prontidão dos dois conjuntos. Saída filtrada pela máscara solicitada.
  Seleção de menor timeout interno/chamador e dispatch após timeout preservados.
- udp_conn solicita somente UDP_TRANSPORT_READABLE|TCP_TRANSPORT_READABLE,
  preservando sua espera anterior. Chamadas do unitário existente migradas
  mecanicamente à mesma máscara; nenhum teste novo registrado na suíte.
  Não foi implementado ossl_quic_wait nem ligação do BIO com ossl_quic.
- Build all e git diff --check passaram. CTest all fora do sandbox passou6/6
  em8,94s: unit2,02; CHOWNAT TCP1,32/UDP1,42/close1,47;
  QUIC TCP1,47/UDP1,23. Cenários QUIC ainda usam SSL_set_fd.
- Harness temporário /tmp/udp-wait-events-check.c compilado com Wall/Wextra/
  Werror e executado fora do sandbox passou: máscaras isoladas e quatro flags
  simultâneas, TCP pendente não acorda espera só UDP, TCP ausente, espera só
  timer com máscara0, argumento timeout preservado e escrita TCP não sinalizada
  quando seu buffer está cheio. TCP exercitado por socketpair SOCK_STREAM;
  UDP por sockets IPv4 loopback. Sem commit.
- Decisão de arquitetura discutida antes da aplicação: usuário rejeitou expor
  udp_fd ao OpenSSL para bloqueio interno. Pesquisa nos headers locais e na
  documentação/código oficial não encontrou callback pública para delegar essa
  espera ao transporte. Proposta ainda não aplicada: SSL não bloqueante com
  loop síncrono no estabelecimento, usando HAL e prazo total persistente.

## QUIC ligado ao BIO e espera pela HAL (2026-10-31)

- Usuário autorizou aplicar a integração no ossl_quic após a extensão de wait.
  ossl_quic_data_t agora mantém BIO_METHOD e configuração emprestada do BIO.
  Init cria o método; pre_connect cria o BIO para cliente/listener, duplica a
  referência para set0_rbio/set0_wbio e configura SSL não bloqueante. Peer vem
  de udp_session->dst; limite de payload UDP fixado em1200 nesta integração.
  Deinit libera conn/listener antes do método e contexto, preservando HAL.
- Removidos SSL_set_fd e connect UDP do pre_connect. Cliente configura peer
  explicitamente por SSL_set1_initial_peer_addr. Inicialmente a tentativa de
  inferir peer pelo ctrl GET_PEER do BIO próprio não funcionou: logs dos dois
  cenários registraram remote peer address not set; chamada explícita corrigiu.
- Cliente repete SSL_connect em WANT_READ/WRITE. Servidor repete accept não
  bloqueante e conclui handshake com SSL_do_handshake antes de CONNECTED.
  ERR_clear_error antes de cada operação permite distinguir falha atual.
  Na OpenSSL local, SSL_accept_connection NULL sem conexão retornou
  SSL_get_error(listener,0)==SSL_ERROR_SSL, mas fila ERR vazia; por isso a
  aceitação verifica ERR_peek_error após a chamada e repete quando vazia.
  Evidência: teste real + conferência do código oficial quic_impl.c.
- Novo helper privado ossl_quic_wait consulta net_read/write_desired e timer,
  limita pelo deadline monotônico compartilhado entre chamadas e usa somente
  udp_transport_wait com eventos UDP. Estabelecimento tem prazo total5s;
  shutdown usa o mesmo helper conservando seu prazo5s. Não há select/FD no
  fluxo QUIC ativo; referências remanescentes pertencem a código #if0.
  Callback de envio de aplicação preservada nesta etapa; comportamento de
  pressão de stream/WANT_WRITE nela não foi alterado nem ensaiado.
- Build all e diff-check passaram. Após corrigir estabelecimento, testes QUIC
  passaram2/2 em2,63s; suíte final6/6 em9,06s: unit2,02;
  CHOWNAT TCP1,33/UDP1,45/close1,46; QUIC TCP1,52/UDP1,28.
  Agora os testes QUIC validam handshake, dados e shutdown pelo BIO próprio
  usando a HAL, na topologia NAT existente. ICE ainda não implementado.
- Harness temporário /tmp/quic-bio-timeout-check.c, compilado Wall/Wextra/Werror
  e executado fora do sandbox, verificou peer loopback que não responde:
  cliente expirou em5003ms, servidor5005ms, sem callback CONNECTED. Ambos
  tinham legacy udp_session.socket_fd=-1 durante connect, BIO sem FD exposto
  e SSL não bloqueante. Deinit liberou SSLs/contexto/método e UDP próprio,
  preservando socket do peer. Nenhum teste permanente acrescentado. Sem commit.

## FD UDP e espera específicos da implementação (2026-10-31)

- Usuário escolheu mover a espera para ops->wait e esclareceu que
  udp_transport_wait continua pública. Essa decisão supera a anterior de manter
  select/dispatch na camada comum. TCP continua emprestado no handler e com
  ciclo de vida em udp_conn; nenhuma reorganização do TCP foi feita.
- Removido udp_fd do handler e socket_fd de udp_session_t, incluindo cópias
  de init/deinit. FD único fica em direct_udp_transport_data_t. Inicializar
  udp_fd=-1 solicita criação; FD externo configurado nos dados específicos
  preserva already_bound/take_ownership e só é tomado no sucesso de open.
  Close invalida o campo para-1 e preserva memória externa como antes.
- Ops send/recv agora recebem o timeout; os select locais dessas funções
  foram movidos para direct_send/direct_recv. A API pública mantém assinaturas.
  Wait público valida argumentos/TCP, limita pelo menor prazo interno/chamador,
  chama ops->wait e filtra eventos solicitados. Get_timeout continua opcional.
  Removidos udp_transport_wait_t/dispatch da API e direct_get_timeout vazio.
  Direct_wait privado monta read/write_fds, executa select e sinaliza eventos.
- Cinco exemplos WebRTC deixam direct_open criar UDP e apenas inicializam os
  dados específicos com udp_fd=-1; configuração de endereços/reuse preservada.
  Teste unitário migrou FD aos dados do backend e sua implementação de espera
  aos callbacks wait, mantendo cenários de timers/controle/lifecycle anteriores.
  Nenhum teste permanente novo. Código histórico #if0 no ossl_quic preservado.
- Build all e git diff --check passaram. CTest all fora do sandbox6/6 em8,94s:
  unit2,02; CHOWNAT TCP1,38/UDP1,42/close1,42; QUIC TCP1,43/UDP1,27.
  Harness temporário udp-wait-events-check recompilado Wall/Wextra/Werror para
  o novo contrato passou fora do sandbox: quatro flags, máscaras isoladas,
  timer sem interesses, TCP ausente e buffer de escrita TCP cheio.
  Busca rg confirmou ausência de udp_fd/select/fd_set na interface/código comum
  do transporte e ausência do alias UDP em udp_conn.h. Sem commit.

## Limpeza de código morto QUIC (2026-10-31)

- Usuário autorizou somente o item1 da revisão de código inflado. Removidos
  os dois blocos #if0 (hole punching experimental antigo e drain_socket_rx_queue),
  o protótipo de drain_socket_rx_queue e o SSL_write inalcançável após return
  de ossl_quic_udp_send_ka. Total147 linhas removidas de ossl_quic.c.
- Comparação com cópia anterior em /tmp confirmou somente essas exclusões.
  Build all e git diff --check passaram. Testes de rede não repetidos nesta
  limpeza de código desativado/inalcançável. Demais itens não aplicados.

## Direct sempre cria e fecha sua UDP (2026-10-31)

- Usuário aprovou/aplicou o esboço do item2: removidos already_bound,
  take_ownership e owns_fd. direct_open sempre cria AF_INET/SOCK_DGRAM,
  configura reuse quando solicitado, faz bind e publica FD/peer só no sucesso.
  Removidos getsockopt/getsockname e validações para FD externo. Em falha,
  fecha o FD que criou. direct_close sempre fecha UDP e invalida dados.
- Cinco exemplos usam direct_config={0}, preenchida no main com local/peer/
  reuse como antes. API pública do transporte, BIO e TCP não alterados.
  A branch atual conferida antes da edição era udp_transport, Git limpo.
  Não foi criada branch nem commit pelo agente nesta etapa.
- Unitário ajustado: sockets iniciais servem apenas para escolher duas portas
  distintas, são fechadas e as sockets testadas são criadas por direct_open.
  Fechamento exige EBADF, reabertura mantém porta/peer, cenário de adoção em
  falha foi substituído por falha de bind em porta ocupada com dados preservados.
  Cenários de dados/espera/timers/truncamento/TCP preservados. Sem testes novos
  registrados. Busca rg não encontrou campos de posse antigos nos C/headers.
- Build all e git diff --check passaram. CTest all fora do sandbox6/6 em9,21s:
  unit2,02; CHOWNAT TCP1,40/UDP1,49/close1,46; QUIC TCP1,52/UDP1,31.

## Direct reutiliza a espera em send/recv (2026-10-31)

- Esboço aprovado/aplicado: direct_send chama direct_wait com UDP_WRITABLE e
  direct_recv com UDP_READABLE, recebendo eventos em variável zerada. Falha
  propagada; OK sem evento solicitado vira NOT_READY. EAGAIN/EWOULDBLOCK dos
  syscalls continuam NOT_READY. Checks de dados/FD ficam em direct_wait.
- Direct_wait só consulta transport->tcp_fd se a máscara pedir leitura/escrita
  TCP; send/recv continuam independentes de TCP. Único select restante no
  arquivo é o da espera compartilhada. API pública não mudou. Nenhum teste
  novo incorporado e nenhum outro item da revisão aplicado.
- Build all e git diff --check passaram. CTest all fora do sandbox6/6 em9,65s:
  unit2,02; CHOWNAT TCP1,45/UDP1,57/close1,56; QUIC TCP1,58/UDP1,45.
  Sem commit; remoção anterior de suporte a FD externo preservada.

## Limite do select somente no direct (2026-10-31)

- Esboço do item2 aprovado/aplicado: retirados include sys/select.h e checks
  tcp_fd>=FD_SETSIZE de udp_transport.c. Validação genérica tcp_fd<-1 mantida.
  Direct_open rejeita TCP acima do limite; direct_wait valida o FD TCP local
  antes dos conjuntos, após selecionar TCP apenas quando solicitado na máscara.
- Busca rg confirmou ausência de select/fd_set/FD_SETSIZE/sys/select na API
  e no código comuns de udp_transport. TCP continua emprestado no handler;
  nenhuma outra mudança de ciclo de vida ou unificação de udp_connection.
- Build all e diff-check passaram. Suíte existente fora do sandbox6/6 em9,60s:
  unit2,02; CHOWNAT TCP1,46/UDP1,58/close1,53; QUIC TCP1,64/UDP1,36.
  Nenhum teste novo nem commit nesta etapa.

## Handlers de dados TCP/UDP (2026-11-01)

- Esboço aprovado/aplicado: tcp_recv renomeada handle_tcp_data, mantendo corpo.
  Novo handle_udp_data privado concentra udp_recv, distinção CLOSED, checagem
  de encerramento pela callback, tcp_send e disconnect em erro. Retorna erro
  original, sem trocar por OK quando a conexão já está fechada.
- Drenagem pending_data e evento UDP chamam o mesmo handler. Loops, timers,
  keep-alive e tratamento TCP fora da renomeação preservados. Nenhum teste novo.
- Build all e diff-check passaram. CTest all fora do sandbox6/6 em9,39s:
  unit2,02; CHOWNAT TCP1,56/UDP1,49/close1,48; QUIC TCP1,51/UDP1,32.
- Pergunta do usuário sobre relógio: rg confirmou monotonic_us privada só em
  udp_conn.c, ossl_quic_monotonic_ms privada no QUIC, clock_gettime MONOTONIC
  em debug_print_ts no utils.h e test_now_us no unitário. Centralizar relógio
  em utils foi apenas sugerido, não autorizado/aplicado nesta etapa. Sem commit.

## Relógio compartilhado em utils (2026-11-01)

- Usuário autorizou mover a leitura para utils. Função monotonic_us movida
  de udp_conn.c para utils.c, declarada em utils.h com stdint.h. Contrato
  preservado: int status0/-1 e resultado por ponteiro, CLOCK_MONOTONIC.
- Monotonic_ms pública em utils reutiliza monotonic_us e divide por1000,
  preservando retorno-1 na falha e a resolução/unidade anterior do QUIC.
  Removidos helper/protótipo ossl_quic_monotonic_ms e migradas suas4 chamadas.
  Udp_conn continua em microssegundos, QUIC em milissegundos; prazos inalterados.
  Debug_print_ts já em utils e relógio independente do unitário preservados.
- Build all e git diff --check passaram. Suíte existente fora do sandbox6/6
  em9,05s: unit2,02; CHOWNAT TCP1,39/UDP1,43/close1,44; QUIC TCP1,47/UDP1,30.
  Não foi injetada falha de clock_gettime nesta mudança; tratamento de erro
  preservado pelo código. Nenhum teste permanente novo nem commit.
