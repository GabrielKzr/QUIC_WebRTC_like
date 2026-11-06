> Correção posterior em 2026-11-06: Meson/glib-mkenums ausentes são preparados
> automaticamente em `_deps` durante o build. Não dependem dos wrappers de
> `/tmp` usados na primeira validação. Rebuild e18/18 passaram em31,85s.

> Atualização 2026-11-06: a libnice agora usa OpenSSL estático gerenciado pelo
> CMake; GnuTLS de desenvolvimento deixou de ser requisito. OpenSSL, coturn e
> libevent também estão em `_deps`, assim como tcpdump/libpcap opcionais.
> Suíte18/18 e PCAP4/4 passaram nesta alteração. Ver
> [dependências do build](../build-dependencies.md) e agent-notes para evidências.
> Referências abaixo ao backend GnuTLS e instalações manuais são históricas.

<!-- Captura atual: ver README do patchbay e agent-notes, atualização 2026-11-06.
As capturas textuais automáticas antigas foram substituídas por PCAP opcional
via presença de PATCHBAY_PCAP, em todas as interfaces do laboratório. -->

# Memória consolidada da integração libnice / ICE / CHOWNAT / QUIC

Consolidada em **2026-11-04**. Este é o ponto de retomada atual da integração.
As notas cronológicas em `agent-notes.md` e os relatórios datados permanecem
como histórico; afirmações antigas de “pendente”, “somente em /tmp” ou de
versão instalada não substituem o estado discriminado aqui.

### Validação longa completa após o patch — 2026-11-04

O usuário autorizou explicitamente uma nova execução, depois da interrupção
registrada abaixo. Esta atualização prevalece sobre as indicações históricas
de que ainda não havia aprovação longa após o patch.

- Branch udp_transport, HEAD17b4d0a; Git limpo antes da execução. Configure e
  build do preset long passaram, sem alterar implementação/configuração.
- `ctest --preset long --output-on-failure -V`: **1/1 passou em664,11s**
  (tempo total664,12s). `result.txt` confirma verificação e saídas dos processos.
- Ambos peers confirmaram1097728 bytes íntegros por sentido,67 blocos de16KiB
  na mesma sessão, handshake e fechamento completos. Par relay/relay.
- Coturn confirmou renovações de permissões aos241/482s e das duas allocations
  (lifetime600) e canais aos540s.
- Aos662s, cada pedido de remoção recebeu438 Stale nonce. Em ambos peers,
  o handler libnice registrou res2 seguido de res0/lifetime0. O coturn confirmou
  dois Refresh lifetime0 aos662s e duas remoções aos663s. Sem timeout de
  deallocate nos logs examinados; TURN_RENEWED allocations=2 channels=2 e
  TURN_RELEASED allocations=2 confirmados pelo signaling.
- Ambiente desta execução: Ubuntu24.04, coturn4.6.1, libnice0.1.21 corrigida
  incorporada estaticamente. Coturn/Meson temporários haviam desaparecido;
  recuperados por extração de pacotes em /tmp, sem instalação no sistema.
- Logs atuais completos:
  `build/long/sim/patchbay/artifacts/network_quic_ice_turn_long/`,
  `build/long/Testing/Temporary/LastTest.log` e `/tmp/quic-long-nonce-test.log`.
  O diretório de artefatos agora contém a execução APROVADA, substituindo
  a tentativa interrompida. Build/configure em `/tmp/quic-long-nonce-*.log`.
- Não foi repetida a suíte all nesta tarefa;18/18 em33,36s continua sendo
  o resultado anterior. GNS3, perdas/reordenação/restart/migração continuam
  fora desta validação. O teste longo segue separado de all.

### Retomada conferida após o commit final — 2026-11-04

- Git/código relidos: branch udp_transport, HEAD1a08804, working tree limpo
  antes desta atualização documental. O commit do usuário incorporou QUIC/ICE,
  streams separados, testes de falha/long e build libnice com patch de nonce.
  As indicações históricas de mudanças pendentes/f56c49e abaixo são da
  fotografia anterior ao commit e não representam código ainda sem commit.
- Conferidos no código: ExternalProject/Libnice::Nice, patch438, force_relay,
  consumo de respostas no close ICE, streams QUIC bidirecionais0/4 e uso do
  endpoint ICE selecionado no teste QUIC. Os12 hashes de fontes/patch/scripts
  ao final continuam iguais aos arquivos agora rastreados no commit.
- Resultados18/18 em33,36s e sessões longas são relatos registrados na
  validação anterior, não reexecuções desta retomada. Host consultado agora:
  Rocky8.10; os resultados recentes descritos adiante foram obtidos no Ubuntu.
  Não confundir o build incorporado0.1.21/GnuTLS com instalação local0.1.24.
- Implementações e testes curtos estão incorporados; teste longo completo
  após patch e reprodução GNS3 continuam sem aprovação. Pedido anterior de
  interromper/não repetir long permanece válido. Nenhum teste executado aqui.

## 1. Escopo, referências Git e estado desta consolidação

- Branch atual: `udp_transport`; HEAD conferido na retomada: `1a08804`.
- Base para o inventário das alterações preparatórias: `5b1f2f6`, anterior
  à primeira HAL deste ciclo. O inventário considera todos os commits dessa
  base até HEAD, o diff atual e os arquivos ainda não rastreados.
- A branch `external_socket_support` continua existente. O suporte intermediário
  à socket externa foi preservado historicamente em `9683dc5`; não é o contrato
  da implementação direct atual.
- O trabalho recente de QUIC/ICE, testes e patch da libnice foi incorporado pelo
  usuário em1a08804. Isso inclui suas edições em CHOWNAT/CMake raiz. Nenhum
  staging ou commit foi feito pelo agente; esta retomada só atualiza documentos.
- Esta tarefa de consolidação só altera documentação. Não executa novamente
  testes, não instala bibliotecas e não altera código C/Rust/CMake.
- A seção final inventaria cada arquivo alterado/adicionado no intervalo.
  O histórico Git preserva os diffs exatos dos commits; as seções seguintes
  explicam a função das mudanças, substituições e resultados experimentais.

## 2. Pesquisa e decisões que antecederam a integração atual

### Experimentos anteriores, sem confundir com a implementação atual

- Em outubro foi investigado o handoff de uma socket selecionada por ICE para
  CHOWNAT: libnice coleta/valida, o launcher duplica a socket e executa outro
  processo com FD herdado. Não se criava outra socket com os mesmos endereços.
- A cópia experimental adaptava CHOWNAT para aceitar FD já bindado e pular
  bind/hole punching nativos. UDP em três rodadas e TCP com pausa de 20 s
  funcionaram; o controle sem KA falhou na retomada. Houve experimento separado
  de remapeamento XOR `0x8000` e teste Patchbay com NAT nativo.
- Esse modelo não cobria TURN: `nice_agent_get_selected_socket()` não fornece
  handoff para candidato relayed. Liberar o agente também elimina manutenção
  ICE/TURN. Não transplantar esse protótipo para a arquitetura atual.
- Também foram comparados CHOWNAT, o hole punching simétrico com esperas fixas,
  uma variante `select` com mensagens 01/03/04 e ICE real. Remover sleeps sem
  introduzir espera adequada falhou; RTO não é atraso obrigatório depois do
  sucesso. Perda de READY mostrou confirmação assimétrica e expôs falta de
  prazo no estabelecimento QUIC antigo, inclusive servidor preso até watchdog.
- Os protótipos/benchmarks e suas limitações estão nas entradas de 25/26 de
  outubro de `agent-notes.md`, com caminhos `docs/experiments/ice-handoff-*` e
  `docs/experiments/hole-punch-select-*`. São relatos de execuções anteriores;
  a existência de cada artefato local não foi revalidada nesta consolidação.

### Modelo escolhido posteriormente

- ICE permanece vivo durante toda a sessão e controla suas próprias sockets.
- Direct e ICE implementam a mesma interface datagrama; não se expõe FD UDP
  ao QUIC, CHOWNAT ou coordenador `udp_conn`.
- Foram consideradas libjuice, librice/sans-IO, Pion e PJNATH. O plano de
  outubro comparava FD externo obrigatório com libnice possuindo o I/O.
  O modelo adotado permite ownership pela implementação e usa libnice.
- A preferência atual é execução sem thread dedicada criada pelo adaptador:
  o chamador conduz GLib/libnice em `wait`. Isso não afirma que GLib/GIO nunca
  usem threads internas, como resolver DNS ou GTask.
- Signaling é responsabilidade da aplicação, por callback síncrona. A biblioteca
  não implementa autenticação/serviço de signaling nem interrompe uma callback
  que não retorna; os timeouts dessa operação pertencem à aplicação.

## 3. HAL `udp_transport`: contrato final e simplificações

Arquivos atuais: `source/libs/transports/udp_transport/inc/udp_transport.h`
e `src/udp_transport.c`.

- `udp_transport_t` é o handler fornecido pelo chamador: `ops`, `opened`,
  `tcp_fd`, `const void *config` e `void *data`. O polimorfismo está na vtable;
  configuração e dados específicos são opacos à HAL comum.
- Vtable: `open`, `send`, `recv`, `get_timeout`, `wait`, `close`.
  O wrapper `get_timeout` é privado; a operação existe na vtable, sem criar
  uma função pública `udp_transport_get_timeout()`.
- Status: `OK=0`, `NOT_READY=1`, `ERROR=-1`. Indisponibilidade/timeout não
  deve ser confundido com erro definitivo. `recv` zera `received` na entrada.
- Eventos: UDP readable=1, TCP readable=2, UDP writable=4, TCP writable=8.
  `wait` recebe `requested_events`, valida o timeout, consulta o timer interno,
  escolhe o menor prazo e filtra os eventos retornados pela implementação.
- `NULL` timeout não limita a espera do chamador; zero consulta sem bloquear;
  positivo impõe limite relativo. Não altera o timeval recebido.
- Uma chamada de `wait` faz uma espera. `OK/events=0` pode indicar timeout ou
  controle interno consumido sem payload; o laço e o prazo do protocolo são
  responsabilidade de `udp_conn`/chamador. EINTR de espera continua sendo erro,
  conforme a decisão de não acrescentar tratamento específico neste trabalho.
- O FD UDP foi removido da HAL, da configuração comum e de `udp_session_t`.
  O TCP continua em `tcp_session_t`, emprestado ao transporte em `tcp_fd`;
  `-1` significa ausente. A implementação de transporte não fecha esse TCP.
- `open` despacha a implementação e só marca aberto no sucesso. `close` é
  idempotente para transporte já fechado, invalida a associação TCP e preserva
  o armazenamento externo de config/data/ops.
- Foram retirados do modelo final: `prepare_wait`, `dispatch` público,
  fd_sets públicos, buffered/maintenance, setter de FD TCP, configuração comum
  com FDs duplicados e laço de espera interno da HAL. Essas formas aparecem
  nas notas históricas, mas não devem ser reintroduzidas ao retomar.
- `send` e `recv` passaram a receber timeout e a delegar sua espera à
  implementação; a HAL não faz select nem conhece OpenSSL/libnice.

### Direct

- `direct_udp_transport_data_t`: FD UDP privado e peer IPv4 fixo.
  Configuração: `reuse_address`, endereço local e peer.
- Direct sempre cria, aplica SO_REUSEADDR quando configurado, faz bind e fecha
  sua UDP. Não adota mais socket externa nem precisa de flag de ownership.
- `direct_wait` contém `select`, read_fds/write_fds locais e validação
  `FD_SETSIZE` exclusiva dessa implementação. Atende eventos UDP/TCP solicitados.
- `send` reutiliza a espera de escrita e chama `sendto(MSG_DONTWAIT)`;
  EAGAIN/EWOULDBLOCK retornam NOT_READY, envio parcial é erro.
- `recv` reutiliza a espera de leitura e usa `recvmsg(MSG_DONTWAIT)` com
  `msg_name`. Valida família, IP e porta do peer fixo. Origem diferente ou
  MSG_TRUNC consomem o pacote e retornam ERROR/received=0.
- Não foi criada API recvfrom pública nem metadados de origem/MTU da HAL.

## 4. Separação em bibliotecas e build inicial

- Interface e implementações saíram de `protocols/udp_conn` para três targets
  próprios em `source/libs/transports/{udp_transport,direct_udp_transport,ice_udp_transport}`,
  com diretórios `inc`/`src` e `.meta.cmake` quando necessário.
- `add_protocol_lib` foi renomeada para `add_lib`. O nome nativo `add_library`
  foi descartado após discussão para não sombrear o comando do CMake.
- O helper deixou de linkar toda biblioteca automaticamente a `udp_conn`.
  Dependências foram declaradas nos `.meta.cmake` de CHOWNAT, QUIC,
  quic_like, udp_conn, direct e ICE. O agregador `protocols` foi preservado.
- A classe `nice` foi registrada em `source/tests/CMakeLists.txt` e habilitada
  no preset `all`. Primeiro teste: dois NiceAgents locais e troca de datagramas.
- Inicialmente ICE e `test_nice_local` usavam `PkgConfig::LIBNICE` instalado
  no ambiente. Isso foi substituído pelo build corrigido descrito na seção 10.

## 5. Implementação ICE/libnice

Arquivos: `source/libs/transports/ice_udp_transport/{inc/ice_udp_transport.h,src/ice_udp_transport.c}`.

### Configuração e dados de um único peer

- Config: `controlling`, `local_address` opcional, STUN server/port,
  TURN server/port/username/password, `force_relay` e `signaling_cb`.
- Data: GMainContext, NiceAgent, stream ID, estado, `gathering_done`,
  `pending_packet[65536]`, tamanho e flag de pacote pendente.
- A estrutura do adaptador representa um peer; as estruturas `test_t`/`peer_t`
  do exemplo de dois agentes não foram copiadas como dependência do transporte.
- `user_data` da configuração de signaling foi removido pelo usuário.
  A callback recebe o próprio transporte, credenciais/candidatos locais e
  devolve credenciais/candidatos remotos por referência.
- Ufrag/password ICE são gerados pela libnice e autenticam checks ICE; não
  autenticam a troca de candidatos em si. Credenciais TURN são outra configuração,
  fornecida pela aplicação. Credenciais de um serviço de signaling são externas.

### Open, gathering e estado

- `ice_open` valida config/data/callback, portas STUN/TURN, credenciais e
  consistência de `force_relay`; cria contexto privado e NiceAgent RFC5245.
- Aplica controlling/force-relay, desabilita ICE TCP e UPnP, configura STUN,
  restringe endereço local quando fornecido, cria um stream com um componente
  e configura relay TURN UDP. Local address ausente deixa a coleta à libnice.
- Conecta `candidate-gathering-done` e `component-state-changed`, inicia gathering
  e retorna sem aguardar READY. Falhas liberam agente/contexto e zeram data.
- Gathering marca conclusão, obtém ufrag/password/candidatos, chama signaling,
  valida os retornos e aplica remote credentials/candidates. Libera strings e
  listas locais/remotas; erro marca FAILED. A callback de estado só acompanha
  o stream/componente desta instância.
- Não foi implementado trickle ICE, recebimento assíncrono de signaling,
  cancelamento forçado da callback nem thread própria para negociação.

### Send, recv, timeout e wait

- Envio por `nice_agent_send_messages_nonblocking`, após `ice_wait` solicitar
  writable. Um datagrama por mensagem; sucesso de um envio retorna OK,
  indisponibilidade/WOULD_BLOCK retorna NOT_READY.
- Leitura explícita por `nice_agent_recv_messages_nonblocking`; não é usado
  callback de payload durante operação. `ice_read_pending` preserva no máximo
  um datagrama para que wait informe readable sem descartá-lo.
- Foi mantido buffer de 65536 bytes por decisão do usuário, em vez de fixar
  1500 ou fragmentar no adaptador. Datagramas maiores que o buffer do chamador
  são consumidos com ERROR/received=0; não há entrega de fragmento pendente.
- Limitação discutida: a API libnice usada não permite distinguir de forma
  equivalente todos os casos de datagrama vazio; a aplicação usa KA `\0`
  com um byte, não datagrama de tamanho zero.
- `ice_get_timeout` adquire o contexto, faz prepare/query, converte o prazo
  GLib de ms para timeval e libera o contexto. Sources prontas pedem prazo zero.
- `ice_wait` tenta leitura pendente, adquire o contexto, faz prepare/query
  dimensionando GPollFDs, acrescenta sockets do agente e TCP solicitado,
  calcula o menor prazo, chama `g_poll` diretamente, faz check/dispatch e
  tenta leitura novamente. Libera arrays, referências de sockets e contexto.
- Writable UDP depende de READY; readable depende de payload pendente.
  HUP/ERR TCP acordam o chamador, NVAL é erro. Controle/timer pode retornar
  sem payload. Não há helper `ice_wait_fds` no código final.
- Foi discutida implementação com select, mas o modelo adotado usa GPollFD/g_poll
  para integrar os múltiplos descritores e sources da GLib.

### Close e correção de recepção durante teardown

- Desconecta callbacks de gathering/estado, define thread-default correto,
  chama `nice_agent_close_async`, itera até a callback, restaura o contexto,
  solta referências e zera os dados.
- A versão inicial apenas iterava GLib, sem continuar consumindo respostas
  TURN no modo de leitura explícita. Havia retransmissões e timeout de cerca
  de dois segundos, embora o servidor já tivesse removido as allocations.
- Correção autorizada: `nice_agent_attach_recv` com `ice_discard_recv` somente
  durante o fechamento, antes de close_async. As respostas passam a ser
  processadas; operação normal permanece com leitura explícita.
- Isso resolveu a falta de consumo da resposta, mas não o desafio 438 no
  handler da libnice. São dois problemas distintos, corrigidos separadamente.

## 6. Integração com udp_conn e CHOWNAT

- `udp_conn_t` recebe o transporte do chamador. `udp_conn_init` abre o
  transporte antes do backend e fecha se init falhar. Deinit encerra o backend
  antes do transporte, desassocia TCP, fecha sockets TCP sem fechamento duplo
  e depois fecha a implementação UDP/ICE.
- `udp_session_t` deixou de guardar FD UDP/reuse; mantém endereços e limiar KA.
  TCP bind/listen/accept/connect continuam no coordenador e atualizam o FD
  emprestado. Não foi feita uma abstração de transporte TCP própria.
- O laço continua em `udp_connection`; `select` foi substituído por
  `udp_transport_wait(UDP_READABLE | TCP_READABLE, ...)`.
- `handle_tcp_data` e `handle_udp_data` centralizam o encaminhamento; o segundo
  serve tanto a dados já pendentes no backend quanto à leitura sinalizada.
  Fechamento normal é distinguido de erro; retornar erro não deve virar OK
  por um break genérico ao detectar is_closed.
- Mantidos processamento de eventos/pending_data do backend QUIC e deadline
  monotônico de inatividade de cinco segundos. Timer interno do backend/ICE
  não conta como KA perdido nem deve sozinho disparar KA.
- `monotonic_us`/`monotonic_ms` foram movidos das cópias locais para utils,
  preservando propagação de falha de clock_gettime.
- CHOWNAT removeu bind/timeout/close UDP próprios e usa send/recv da HAL,
  mantendo envio KA de um byte zero com timeout zero e handshake 01/03/03.
- Recepção de controle foi inicialmente discutida sem helper. Depois, a
  integração real demonstrou que um timer ICE acordava recv sem payload e
  consumia as cinco tentativas em aproximadamente 80 ms.
- A decisão final autorizada foi `chownat_recv_control` no chamador: deadline
  único, wait pelo restante, recv não bloqueante, controles/KA curtos sem
  renovar prazo nem esgotar tentativas. Aplicado às cinco recepções de controle
  de estabelecimento/fechamento. Não foi alterado ice_recv para mascarar isso.
- No working tree atual, o usuário comentou as atribuições de tv_sec/tv_usec
  em `chownat_get_timeout`; a função ainda retorna OK. Como o chamador inicializa
  o timeval com zero, esse detalhe não deve ser descrito como timeout de 5 s
  devolvido pelo CHOWNAT. Foi preservado, sem correção nesta consolidação.

## 7. QUIC/OpenSSL através do BIO

### BIO próprio e estabelecimento não bloqueante

- Criados `ossl_quic_bio.c/.h`, inicialmente sem ligação ao backend; depois
  conectados ao cliente e listener. A alternativa de par BIO com FIFOs foi
  discutida, mas o código adotou BIO próprio com callbacks.
- Config do BIO guarda transporte emprestado, peer IPv4 fixo e limite de
  datagrama. O backend usa payload UDP de 1200 bytes, excluindo headers IP/UDP.
  Isso não é MTU Ethernet 1200 nem impede fragmentação fora desse contrato.
- `BIO_METHOD` configura create/destroy/ctrl/sendmmsg/recvmmsg. Processa até
  um datagrama por chamada, com timeout zero na HAL; converte NOT_READY em
  erro BIO não fatal e outros erros em falha de transferência.
- Valida argumentos, endereço remoto, comprimento e ausência de endereço local
  não suportado; reporta peer/MTU/capacidades, recusa truncamento, retorna
  FD/poll descriptor indisponíveis. Não há FD entregue ao OpenSSL.
- BIOs são entregues ao SSL com referências corretas para rbio/wbio; SSL os
  libera. BIO_METHOD permanece em `ossl_quic_data_t` até depois de liberar
  conexão/listener. Não se presume que liberar o método libere os BIOs.
- `SSL_set_fd` e acesso direto UDP saíram do caminho ativo. Config/dados do
  QUIC não gerenciam nem fecham FD UDP; socket externa permanece só na branch
  histórica já mencionada.
- Handshake/listener/shutdown usam modo não bloqueante e deadlines do projeto
  de 5 s. `ossl_quic_wait` combina prazo do SSL e deadline, consulta desejo de
  leitura/escrita e espera por eventos na HAL. WANT_READ/WANT_WRITE são tratados
  nos laços de estabelecimento/shutdown, sem reintroduzir FD no BIO.
- O hole punching QUIC passou a usar HAL; envia KA antes de aguardar para
  abrir o mapeamento. O log `Should not be here` para KAs durante descoberta
  foi substituído por comentário. A fase transiente de 10 x 50 ms permanece.
- No teste ICE, após READY, o endereço remoto do par selecionado preenche
  `udp_session.dst`, atendendo ao BIO de peer fixo. Não há suporte validado
  a migração transparente, metadados de origem variáveis ou PMTU dinâmico.

### Keep-alive QUIC e streams explícitos

- O envio de KA estava inativo. O usuário o ativou com SSL_write_ex, primeiro
  com ponteiro de quantidade escrita NULL: o teste longo detectou SIGSEGV.
- Depois de corrigir esse ponteiro, o byte zero seguia no mesmo stream da
  aplicação; o teste longo detectou payload mismatch na segunda rodada.
- Correção autorizada: dois streams **bidirecionais**, um de dados e um de
  controle. Não foram mantidos streams unidirecionais separados de send/recv.
- Cliente cria IDs 0 e 4; servidor aceita validando ID/tipo/duplicidade.
  Default stream fica NONE e incoming policy ACCEPT. Eventos são processados
  antes de novas tentativas para progredir limites/capacidade de streams.
- Ambos os peers anunciam e consomem marcadores D/C nos dois streams antes
  de CONNECTED. Isso permite servidor enviar primeiro, sem depender de payload
  TCP para materializar o stream.
- `udp_send`, `udp_recv` e `pending_data` usam o stream de dados. KA de um byte
  zero usa controle; `process_events` drena controle e não o encaminha ao TCP.
  Retornos/tamanho escrito são conferidos. Os streams são liberados antes da
  conexão no deinit, seguido de listener/método/contexto conforme ownership.
- O agendamento KA continua pertencendo ao loop udp_conn. Não foi implementada
  uma fila geral de retry da aplicação QUIC sob backpressure/WANT_WRITE.

## 8. Infraestrutura de teste: ISP, signaling e regras NAT

- A topologia mantém ISP público, dois roteadores Home/Nat::Moderate e dois
  peers em LANs separadas, ainda que compartilhem o prefixo 192.168.80.0/24.
  Endereços públicos e interfaces WAN vêm dos objetos da topologia.
- Hook opcional `PATCHBAY_ISP_SCRIPT` no target CMake leva um `.sh` ao runner.
  Rust inicia o serviço dentro do namespace ISP, espera linha READY em até
  5 s, acompanha morte prematura e encerra o grupo de processos na limpeza.
  Cargo passou a habilitar sinal/processo via nix para killpg.
- Peers recebem `PATCHBAY_ISP_ADDRESS` e `PATCHBAY_LOCAL_ADDRESS`. O script
  serve STUN ou TURN em UDP/3478 e signaling TCP/5555. Esse TCP é negociação
  ICE; não substitui echo/SSH nem transporta payload do túnel.
- Signaling Python verifica STUN real antes de READY; troca dois SDPs com
  tamanho uint32 em ordem de rede e mensagem de 1..65536 bytes. Peers usam
  timeout TCP de 3 s; serviço usa 5 s para a troca inicial.
- No TURN, o serviço espera CLOSED dos dois peers, exige no log do coturn
  Refresh lifetime zero e fechamento de todas as allocations (pelo menos duas),
  então responde OK e registra TURN_RELEASED. Matar coturn não conta como release.
- No modo longo, espera CLOSED por 720 s e exige também Refresh positivo por
  allocation e pelo menos dois CHANNEL_BIND bem-sucedidos por allocation,
  registrando TURN_RENEWED. O serviço curto mantém seus limites.
- Dependências turnserver/python3 são checadas no configure quando o cenário
  correspondente está habilitado, com instrução de diagnóstico.
- Inicialmente existia regra global de mapeamento espelhado. Remover apenas
  SNAT UDP mantendo flush postrouting deixou tráfego STUN com origem privada:
  falha do experimento não demonstrava incapacidade ICE de remapear portas.
- Remover todo o bloco manual permitiu ICE no NAT nativo, mas quebrou os cinco
  testes diretos que dependem de portas previsíveis; o bloco foi restaurado.
- A opção booleana intermediária PATCHBAY_MIRRORED_NAT/--mirrored-nat foi
  substituída pela preferência final do usuário: lista PATCHBAY_NAT_EXTRA_RULES.
- Lista contém comandos **nftables**, não comandos shell arbitrários. CMake
  gera arquivo `.nat.nft`, o runner lê por `--nat-rules` e aplica nos dois
  roteadores com substituição de @PUBLIC_IP@/@WAN@/@ISP_IP@. Sem lista, só NAT
  nativo. Semicolons internos são escapados no CMake, sem escape no nft final.
- Os cinco testes WebRTC diretos declaram suas regras espelhadas em seus
  próprios `.meta.cmake`; ICE original não. Remapeamento ICE usa XOR `0x4000`
  e mapa DNAT coerente, incluindo SNAT TCP de signaling na própria lista.
- Registro CMake segue propriedades dos targets, incluindo subdiretórios
  customizados; mantém execução serial e modos tcp, udp e o novo tcp-long.

## 9. Testes criados/adaptados e correções das fixtures

### Matriz funcional

- `nice_local`: dois NiceAgents locais, troca em memória de credenciais e
  candidatos, ambos READY e datagramas corretos nos dois sentidos.
- `unit_udp_transport`: valida HAL/direct, configuração, ciclo de vida,
  espera UDP/TCP de leitura/escrita, timeouts/NOT_READY, argumentos, truncamento,
  origem fixa e reabertura/fechamento, sem depender de ICE para sua evidência.
- Os cinco peers WebRTC existentes foram migrados para handler/config/data
  direct, sem criar socket UDP externamente; scripts NAT continuam por teste.
- `network_ice_udp_transport`: peers separados no Patchbay, STUN/signaling reais,
  candidatos reflexivos, READY, par selecionado e mensagens bidirecionais.
- Variante remap recompila esse fonte, exige porta pública diferente da base
  nos dois peers. Controle sem regras foi rejeitado; portas históricas medidas:
  A 39241→55625 e B 50811→34427. Não é alocador NAT geral nem prova universal.
- Variante TURN recompila esse fonte com ICE_REQUIRE_RELAY; exige relay/relay,
  dados e confirmação no servidor. Controle STUN-only não aprovou fallback.
- `test_chownat_ice` e variante TURN: espera READY, consome KA antecipado,
  valida KA explícito e handshake original com cinco tentativas; runner exige
  16 KiB binários por sentido/EOF. Teste exige CONNECTED único, ausência de
  LOST_DATA, closed, deinit e FDs/contexto invalidados. TURN exige release.
- `test_quic_ice` e variante TURN: configura destino selecionado para o BIO,
  espera READY aceitando controles esperados, valida CONNECTED único, 16384
  bytes por sentido, conteúdo/EOF pelo runner, fechamento/deinit e release.
- `test_ice_failures`, fonte único com quatro argumentos/CTest: signaling TCP
  recusado, TURN com senha errada, candidato remoto ausente e nonce TURN expirado
  no fechamento. Warm-up GLib/GTask antecede contagem de FDs. Timers curtos
  são exclusivos da fixture; timeout/watchdog não conta como aprovação.
- A regressão `turn_stale_nonce` usa senha correta/force_relay, exige um relay,
  falha esperada de conectividade ao peer ausente, espera 2,2 s com nonce de
  um segundo e fecha. Confere 438 + Refresh lifetime zero + remoção no servidor,
  além de limpeza local. Timeout CTest 15 s; casos não usam Patchbay.
- `test_chownat_udp_close` deixou de aprovar erro silenciosamente: confere
  status de conexão/envio/desconexão/deinit, quantidades, closed e LOST_DATA;
  contadores próprios exigem 20 mensagens no cliente e 21 no servidor.
  Impressão mudou de `%s` para `%.*s`, respeitando o comprimento recebido.

### Sessão longa separada de all

- Nova classe `long`, BUILD_LONG_TESTS OFF por padrão e explicitamente OFF em
  all. Preset long tem build/long, compila peer/runner e filtra label long.
- Reutiliza fonte QUIC/ICE com ICE_REQUIRE_RELAY e ICE_LONG_SESSION. Novo modo
  Rust tcp-long troca 67 blocos de 16384 bytes, com 66 pausas de dez segundos,
  mesma conexão TCP/QUIC e 1.097.728 bytes por sentido. Limites: probe 710 s,
  CTest 740 s. Wrapper ISP exporta ICE_TEST_REQUIRE_RENEWALS=1.
- Execuções anteriores comprovaram dados/renovações durante cerca de 660 s,
  mas a aprovação completa falhou: primeiro timeout CLOSED de 5 s, corrigido
  para 720 s só nesse cenário; depois remoção TURN com nonce expirado.
- Após integrar o patch, o agente iniciou outra execução. O usuário pediu
  interromper e não repetir o teste de 11 minutos. CTest foi interrompido com
  SIGINT, saiu 130 e não deixou CTest/runner/peers long em execução na checagem.
  **Não há execução longa completa aprovada com o patch. Não repetir sem pedido.**

## 10. Investigação do nonce e patch integrado ao build

- Nonce TURN é desafio temporário do servidor para autenticação; não é a senha
  ICE nem a senha TURN. 438 fornece nonce novo para repetir a requisição.
- Em libnice0.1.21, renovação normal reconhece 438, atualiza a resposta e repete.
  `priv_map_reply_to_relay_remove` liberava o CandidateRefresh para todo retorno
  diferente de INVALID, incluindo ERROR de 438, e a callback completava.
- A reprodução direta sem HAL/QUIC/signaling/Patchbay confirmou a falha com
  credenciais corretas e contextos padrão/privado. Controles com nonce válido
  removiam. Fonte upstream0.1.21 recompilada reproduziu, excluindo hipótese de
  problema exclusivo do pacote da distribuição.
- Cliente TURN Python independente repetiu a remoção com nonce novo: sucesso
  e release confirmado. MESSAGE-INTEGRITY foi validado nas respostas de sucesso.
- A 0.1.24 foi compilada/testada: não resolveu nonce expirado e mostrou, no
  exemplo gathered-only, outro problema de streams removidos antes da resposta,
  levando a retransmissões/timeout até no controle válido. Não generalizar esse
  segundo achado a toda integração; não recomendar upgrade como correção provada.
- A condição problemática já aparece no commit upstream0556ec49 (2019-02-19).
  Pesquisas no tracker encontraram relatos relacionados, sem identidade
  confirmada. Comentários da API retornaram401; não afirmar que nunca reportaram.
  MR333 trata consistência/valgrind; MR283 final trata DNS/close-forced.
  Não foi aberto issue nem enviada mensagem externa.
- Primeira correção existiu apenas na cópia `/tmp/nice-nonce-deep`: repetir
  lifetime zero com novo nonce resolveu quatro cenários experimentais.
- Depois de autorização explícita, a correção foi incorporada em
  `patches/libnice/0001-retry-turn-deallocation-on-stale-nonce.patch`.
  Ela acrescenta `remove_nonce_retried` privado, valida NONCE/REALM não vazios,
  limita a cópia à capacidade e permite uma repetição do desafio. Reutiliza o
  construtor/retransmissão existente com disposing/lifetime zero; inicializa
  lifetime do log, que respostas de erro não preenchem. Outros erros/novo 438
  após a repetição seguem o cleanup existente. API pública não mudou.
- `cmake_modules/libnice.cmake` usa ExternalProject_Add: baixa tag0.1.21 do
  espelho oficial GitHub com TLS e SHA-256 fixo, extrai, aplica GNU patch,
  configura Meson, compila/instala via Ninja dentro do build.
- SHA-256 do archive:
  `0f0d9a6b21cfb512593eacbba2a1981a3ba493341dd7377298085d47954b4ba4`.
- Prefixo `_deps/libnice-0.1.21-<12 caracteres do SHA256 do patch>`;
  download em `_deps/downloads`. Mudanças no patch são dependência de
  reconfigure e selecionam outra árvore; build incremental reutiliza a pronta.
- Compila static/PIC, debugoptimized, backend crypto GnuTLS. Desabilita GUPnP,
  GStreamer, introspection, docs, exemplos e testes internos dessa dependência;
  wrap-mode nodownload. Os testes do projeto continuam ativos.
- Target global importado `Libnice::Nice` expõe arquivo estático/headers e
  dependências pkg-config GLib/GIO/GThread/GnuTLS; depende da instalação externa
  antes de compilar consumidores. Tanto ICE quanto nice_local usam esse target.
- Dependências de build: Meson, Ninja, patch, pkg-config, compilador C,
  GLib/GIO/GThread >=2.54 e GnuTLS >=2.12. Não requer libnice de desenvolvimento
  instalada no sistema. Não altera bibliotecas do sistema.
- Conferidos link.txt usando `libnice.a` de `_deps` e ldd sem libnice.so.
  Assim LD_LIBRARY_PATH não troca silenciosamente a libnice dos testes por uma
  cópia sem patch; dependências dinâmicas GLib/GnuTLS continuam no ambiente.
- O controle negativo foi outro executável **somente em /tmp**, ligado à libnice
  original. Ele falhou na confirmação de remoção, como esperado. Não desativou
  o patch no projeto. O projeto permaneceu usando a biblioteca corrigida.

## 11. Verificações registradas e limites do que foi provado

Não foram executados testes novos para escrever esta memória. Resultados lidos
dos logs das execuções anteriores desta sessão:

| Verificação | Resultado registrado |
| --- | --- |
| Configure/build all com download e patch reais | Passou; dois arquivos upstream corrigidos antes de compilar |
| Segundo build all | Incremental, sem repetir download/patch da dependência |
| Configure/build long | Passou com a mesma dependência corrigida |
| Nova regressão nonce pelo CTest | Passou, remoção no servidor confirmada; 4,21 s |
| Mesmo teste ligado à libnice original em /tmp | Saiu 1, sem remoção; falha esperada do controle |
| Suíte all após patch | **18/18 passaram, 33,36 s** |
| Nova execução long após patch | Interrompida por solicitação do usuário; não aprovada nem repetida |
| Git diff-check documental | Conferido na consolidação; whitespace preexistente CHOWNAT não é corrigido aqui |

### Matriz exata da última execução all

Todos os casos abaixo passaram na mesma execução de33,36s, com o patch ativo.

| Teste CTest | Tempo |
| --- | --- |
| `nice_local` | 0.03 s |
| `unit_udp_transport` | 2.02 s |
| `ice_failure_signaling_unavailable` | 0.01 s |
| `ice_failure_turn_bad_credentials` | 0.11 s |
| `ice_failure_remote_absent` | 0.88 s |
| `ice_failure_turn_stale_nonce` | 4.21 s |
| `network_chownat_tcp` | 1.75 s |
| `network_chownat_udp` | 1.58 s |
| `network_chownat_udp_close` | 1.61 s |
| `network_openssl_quic_tcp` | 1.45 s |
| `network_openssl_quic_udp` | 1.20 s |
| `network_chownat_ice_tcp` | 2.95 s |
| `network_ice_udp_transport` | 2.87 s |
| `network_quic_ice_tcp` | 3.54 s |
| `network_chownat_ice_turn_tcp` | 1.97 s |
| `network_ice_udp_transport_remap` | 2.85 s |
| `network_ice_udp_transport_turn` | 1.75 s |
| `network_quic_ice_turn_tcp` | 2.56 s |

Suítes intermediárias: 8/8 após regras NAT opcionais, 12/12 após CHOWNAT/ICE,
17/17 após QUIC/ICE e três falhas locais. Esses números pertencem a matrizes
diferentes; não substituir o resultado final18/18 por um deles.

### Achados adicionais e pendências preservadas

- Aviso libnice `Integrity check failed - bad request` foi decodificado em
  Binding Indication de28 bytes gerado pela própria libnice, sem credenciais
  e só FINGERPRINT. O validador0.1.21 exige credencial curta nesse caminho;
  RFC8445§11 permite/requer o keepalive sem autenticação. Não foi incluída
  correção desse outro comportamento no patch de nonce.
- Falhas de pares host-host entre LANs isoladas são esperadas quando outro par
  reflexivo/relay funciona. Não confundir esses logs com falha final do ICE.
- Logs coturn contêm aviso de banco SQLite ausente; os usuários estáticos dos
  testes funcionaram. Não foi corrigida configuração global do servidor.
- Algumas capturas ISP/ix ficaram vazias ou iniciaram tarde; em execuções
  posteriores houve evidência WAN dos roteadores. Não afirmar captura completa
  de todos os estágios/testes; preservar as limitações dos relatórios originais.
- Permanecem fora da validação: perda/reordenação sistemáticas, filas cheias,
  datagramas zero em ICE, IPv6, TURN TCP/TLS, mudanças de interface/NAT em sessão,
  trickle/restart, fallback dinâmico, reconexão e migração QUIC transparente.
- GNS3 é a etapa final ainda não executada desta integração. `sim/gns3/README.md`
  descreve dois NATs/ISP, direto/remapeado/relay, CHOWNAT/QUIC, falhas e evidências.
- Não repetir o teste longo sem novo pedido. A decisão do usuário encerrou essa
  validação por ora; não deixar tarefa automática de 11 minutos pendente.

## 12. Ambientes, artefatos e instruções de retomada

- Histórico inclui outro host Rocky8.10 com instalação0.1.24/coturn4.6.3 em
  ~/.local e setup de GLib/PCRE/OpenSSL local. Isso não é a versão incorporada
  ao build atual e não deve substituir as versões verificadas nesta sessão.
- Ambiente desta sessão: Ubuntu, GLib/GIO2.80.0, OpenSSL4.1.0-dev em /usr/local,
  libnice do sistema0.1.21 usada no controle, coturn4.6.1 extraído em
  `/tmp/quic-coturn`. Meson de diagnóstico extraído em `/tmp/nice-nonce-deep/meson`.
- Foi autorizado acrescentar PATH/LD_LIBRARY_PATH do coturn temporário ao
  `.zshrc` na sessão anterior. São caminhos temporários, não dependências
  vendorizadas no repositório. A proposta de bashrc foi corrigida para zshrc.
- Logs atuais curtos: `build/all/Testing/Temporary/LastTest.log` e
  `build/all/sim/patchbay/artifacts/<nome-do-teste>/`.
- Logs de build/validação da aplicação do patch:
  `/tmp/libnice-patched-build.log`, `*-build-2.log`, `*-suite.log`,
  `/tmp/libnice-stale-nonce-regression.log`, `*-unpatched.log` e
  `/tmp/libnice-patched-long-{configure,build,test}.log`.
- Reprodução independente, fontes0.1.21/0.1.24, matrizes, cliente Python,
  patch experimental e respostas do tracker: `/tmp/nice-nonce-deep/`.
- Artefatos da última execução longa **completa anterior ao patch** foram
  copiados para `/tmp/quic-turn-long-before-nonce-patch` antes da tentativa nova.
  O diretório normal em build/long foi reutilizado pela execução interrompida;
  não tratá-lo como evidência completa da execução anterior ou do patch.
- Há diagnósticos anteriores de fechamento, STUN e SIGSEGV nos caminhos
  `/tmp/quic-close-*`, `/tmp/quic-stun-inspect`, `/tmp/quic-long-gdb-artifacts`,
  detalhados nas notas cronológicas. Arquivos /tmp não garantem persistência.
- Referências duráveis: este arquivo, `agent-notes.md`, `repository-context.md`,
  plano ICE/QUIC, relatórios de timeout e nonce, READMEs nice/long/patches,
  guias Patchbay/GNS3 e os próprios testes/commits.
- Ao retomar: conferir Git, ler esta memória, confirmar ferramentas e caminhos,
  usar build/CTest do preset apropriado. Testes de rede precisam das permissões
  de namespaces; CTest não recompila. Não executar long por inferência.

## 13. Linha do tempo dos commits da integração

Os commits abaixo já existem no Git. O bloco de streams explícitos, falhas/long,
correção do teste de close e libnice corrigida foi incorporado em1a08804.

<!-- Inventário Git gerado ao consolidar: não inclui binários/cache ignorados. -->

| Commit | Alteração registrada |
| --- | --- |
| `d926924` | Add a UDP transport HAL with internal select and optional TCP readiness events |
| `ad6d21e` | Add a UDP transport HAL with internal select and optional TCP readiness events |
| `f23d8d6` | Separate direct UDP transport from the HAL and centralize socket waiting |
| `a05e315` | refactor: integrate UDP transport into udp_conn and CHOWNAT, centralize socket lifecycle and event waiting, and adapt QUIC socket ownership |
| `705497d` | fix: distinguish CHOWNAT closure from receive errors and simplify udp_conn state checks |
| `9683dc5` | refactor: integrate QUIC with custom BIO, isolate UDP socket handling in direct transport and delegate event waits |
| `ccabe92` | refactor: simplify direct UDP ownership and waits, unify TCP/UDP data handlers and centralize monotonic clock utilities |
| `c9e6f2c` | refactor: split UDP transports into dedicated targets, rename CMake helper to add_lib, declare libnice dependencies and add a local ICE datagram test |
| `47c2a39` | feat: implement libnice ICE transport and NAT integration test with ISP STUN/signaling services |
| `07f7d91` | feat: configure per-test NAT rule lists and document ICE integration validation through GNS3 |
| `f56c49e` | Add ICE remapping, forced TURN and CHOWNAT TCP tests; preserve CHOWNAT control receive deadlines |
| `1a08804` | QUIC sobre ICE, streams separados, falhas/sessão longa e libnice compilada com patch de nonce TURN |

## 14. Inventário histórico de arquivos do ciclo

Fotografia anterior ao commit1a08804, tendo f56c49e como HEAD. A/M são
adição/modificação em relação à base5b1f2f6; “novo”/“pendente” descrevem aquela
fotografia. Todos esses arquivos agora estão rastreados e incorporados em
1a08804. O commit também acrescentou a explicação do patch em
patches/libnice/0001-retry-turn-deallocation-on-stale-nonce.md.
O inventário inclui documentação/formatação sem atribuir toda edição ao agente.

| Situação desde a base | Estado em HEAD/working tree | Arquivo |
| --- | --- | --- |
| M | Pendente | `CMakeLists.txt` |
| M | Pendente | `CMakePresets.json` |
| M | Commitado | `cmake_modules/common_functions.cmake` |
| novo | Novo, pendente | `cmake_modules/libnice.cmake` |
| A | Commitado | `docs/analysis/chownat-ice-recv-timeout-2026-11-03.md` |
| A | Commitado | `docs/analysis/ice-quic-integration-2026-10-29.md` |
| novo | Novo, pendente | `docs/analysis/libnice-turn-close-nonce-2026-11-03.md` |
| M | Pendente | `docs/memory/agent-notes.md` |
| novo | Novo, pendente | `docs/memory/libnice-integration.md` |
| A | Pendente | `docs/plans/ice-quic-integration.md` |
| M | Pendente | `docs/repository-context.md` |
| novo | Novo, pendente | `patches/libnice/0001-retry-turn-deallocation-on-stale-nonce.patch` |
| novo | Novo, pendente | `patches/libnice/README.md` |
| A | Commitado | `sim/gns3/README.md` |
| M | Commitado | `sim/patchbay/Cargo.lock` |
| M | Commitado | `sim/patchbay/Cargo.toml` |
| M | Commitado | `sim/patchbay/README.md` |
| M | Pendente | `sim/patchbay/program_tests.cmake` |
| M | Pendente | `sim/patchbay/src/main.rs` |
| M | Pendente | `sim/patchbay/src/patchbay.rs` |
| M | Commitado | `sim/patchbay/src/topology.rs` |
| M | Commitado | `source/common/utils/inc/utils.h` |
| M | Commitado | `source/common/utils/src/utils.c` |
| M | Commitado | `source/libs/CMakeLists.txt` |
| A | Commitado | `source/libs/protocols/chownat/chownat.meta.cmake` |
| M | Pendente | `source/libs/protocols/chownat/src/chownat.c` |
| M | Pendente | `source/libs/protocols/ossl_quic/inc/ossl_quic.h` |
| A | Commitado | `source/libs/protocols/ossl_quic/inc/ossl_quic_bio.h` |
| M | Commitado | `source/libs/protocols/ossl_quic/ossl_quic.meta.cmake` |
| M | Pendente | `source/libs/protocols/ossl_quic/src/ossl_quic.c` |
| A | Commitado | `source/libs/protocols/ossl_quic/src/ossl_quic_bio.c` |
| A | Commitado | `source/libs/protocols/quic_like/quic_like.meta.cmake` |
| M | Commitado | `source/libs/protocols/udp_conn/inc/udp_conn.h` |
| M | Commitado | `source/libs/protocols/udp_conn/src/udp_conn.c` |
| A | Commitado | `source/libs/protocols/udp_conn/udp_conn.meta.cmake` |
| A | Commitado | `source/libs/transports/direct_udp_transport/direct_udp_transport.meta.cmake` |
| A | Commitado | `source/libs/transports/direct_udp_transport/inc/direct_udp_transport.h` |
| A | Commitado | `source/libs/transports/direct_udp_transport/src/direct_udp_transport.c` |
| A | Pendente | `source/libs/transports/ice_udp_transport/ice_udp_transport.meta.cmake` |
| A | Commitado | `source/libs/transports/ice_udp_transport/inc/ice_udp_transport.h` |
| A | Pendente | `source/libs/transports/ice_udp_transport/src/ice_udp_transport.c` |
| A | Commitado | `source/libs/transports/udp_transport/inc/udp_transport.h` |
| A | Commitado | `source/libs/transports/udp_transport/src/udp_transport.c` |
| M | Pendente | `source/tests/CMakeLists.txt` |
| M | Commitado | `source/tests/WebRTC/test_chownat/test.meta.cmake` |
| M | Commitado | `source/tests/WebRTC/test_chownat/test_chownat.c` |
| M | Commitado | `source/tests/WebRTC/test_chownat_udp/test.meta.cmake` |
| M | Commitado | `source/tests/WebRTC/test_chownat_udp/test_chownat_udp.c` |
| M | Commitado | `source/tests/WebRTC/test_chownat_udp_close/test.meta.cmake` |
| M | Pendente | `source/tests/WebRTC/test_chownat_udp_close/test_chownat_udp_close.c` |
| M | Commitado | `source/tests/WebRTC/test_openssl_quic/test.meta.cmake` |
| M | Commitado | `source/tests/WebRTC/test_openssl_quic/test_openssl_quic.c` |
| M | Commitado | `source/tests/WebRTC/test_openssl_quic_udp/test.meta.cmake` |
| M | Commitado | `source/tests/WebRTC/test_openssl_quic_udp/test_openssl_quic_udp.c` |
| novo | Novo, pendente | `source/tests/long/README.md` |
| novo | Novo, pendente | `source/tests/long/test_quic_ice_turn_long/CMakeLists.txt` |
| novo | Novo, pendente | `source/tests/long/test_quic_ice_turn_long/isp.sh` |
| novo | Novo, pendente | `source/tests/long/test_quic_ice_turn_long/test.meta.cmake` |
| A | Pendente | `source/tests/nice/README.md` |
| A | Commitado | `source/tests/nice/test_chownat_ice/test.meta.cmake` |
| A | Commitado | `source/tests/nice/test_chownat_ice/test_chownat_ice.c` |
| A | Commitado | `source/tests/nice/test_chownat_ice_turn/CMakeLists.txt` |
| A | Commitado | `source/tests/nice/test_chownat_ice_turn/test.meta.cmake` |
| novo | Novo, pendente | `source/tests/nice/test_ice_failures/CMakeLists.txt` |
| novo | Novo, pendente | `source/tests/nice/test_ice_failures/test.meta.cmake` |
| novo | Novo, pendente | `source/tests/nice/test_ice_failures/test_ice_failures.c` |
| A | Commitado | `source/tests/nice/test_ice_udp_transport/isp.sh` |
| A | Pendente | `source/tests/nice/test_ice_udp_transport/signaling.py` |
| A | Commitado | `source/tests/nice/test_ice_udp_transport/test.meta.cmake` |
| A | Commitado | `source/tests/nice/test_ice_udp_transport/test_ice_udp_transport.c` |
| A | Commitado | `source/tests/nice/test_ice_udp_transport_remap/CMakeLists.txt` |
| A | Commitado | `source/tests/nice/test_ice_udp_transport_remap/test.meta.cmake` |
| A | Commitado | `source/tests/nice/test_ice_udp_transport_turn/CMakeLists.txt` |
| A | Commitado | `source/tests/nice/test_ice_udp_transport_turn/isp.sh` |
| A | Commitado | `source/tests/nice/test_ice_udp_transport_turn/test.meta.cmake` |
| A | Pendente | `source/tests/nice/test_nice_local/test.meta.cmake` |
| A | Commitado | `source/tests/nice/test_nice_local/test_nice_local.c` |
| novo | Novo, pendente | `source/tests/nice/test_quic_ice/test.meta.cmake` |
| novo | Novo, pendente | `source/tests/nice/test_quic_ice/test_quic_ice.c` |
| novo | Novo, pendente | `source/tests/nice/test_quic_ice_turn/CMakeLists.txt` |
| novo | Novo, pendente | `source/tests/nice/test_quic_ice_turn/test.meta.cmake` |
| A | Commitado | `source/tests/unit/test_udp_transport.c` |

### Caminhos intermediários removidos pela separação de targets

Estes caminhos existiram no histórico, mas deram lugar às bibliotecas em
`source/libs/transports/`; não representam fontes adicionais a compilar:

- `source/libs/protocols/udp_conn/inc/direct_udp_transport.h`
- `source/libs/protocols/udp_conn/inc/udp_transport.h`
- `source/libs/protocols/udp_conn/src/direct_udp_transport.c`
- `source/libs/protocols/udp_conn/src/udp_transport.c`

### Identificação dos fontes novos na fotografia anterior

SHA-256 registrado nesta consolidação para arquivos C/CMake/scripts/patch
então não rastreados e agora incorporados em1a08804. Os12 hashes foram
reconferidos na retomada e continuam iguais. Documentos não recebem hash aqui, para evitar
autorreferência. O hash identifica o conteúdo da fotografia atual, não uma
garantia de imutabilidade depois de novas edições.

| Arquivo novo | SHA-256 |
| --- | --- |
| `cmake_modules/libnice.cmake` | `ce5132e5bbc808ddd7a6793bddc29c0271f3c3cadaf17fbcbdf28742932ac4ea` |
| `patches/libnice/0001-retry-turn-deallocation-on-stale-nonce.patch` | `181009c33855b7a8398ff231f5694117f087b692aa5b7760b30c2448d1300ef3` |
| `source/tests/long/test_quic_ice_turn_long/CMakeLists.txt` | `eea08531c858dbd2fc0484dbbc3489574c86cf1824a9d48bf258db303aab70e5` |
| `source/tests/long/test_quic_ice_turn_long/isp.sh` | `09a4dd3ab7c3c00960f15f1471e8ea1ce6d9d78b1f2e2070448690f1bfbe982a` |
| `source/tests/long/test_quic_ice_turn_long/test.meta.cmake` | `eab62c705b8d67e54371701ada8f3aeca221c9279def18f02ad2a79a5b761961` |
| `source/tests/nice/test_ice_failures/CMakeLists.txt` | `5775b98cb0be8f7250439441c706d50894d8170ec6c55329f3cfda0cf42329e9` |
| `source/tests/nice/test_ice_failures/test.meta.cmake` | `c0b02977d774a1dfa8030c50ca139eb3e2d128d34f026dd1d7a15d9168d49583` |
| `source/tests/nice/test_ice_failures/test_ice_failures.c` | `5df61ba5bee09001e9aba67a289823ae3cded60abba84d37c5e5e53eea4be6fa` |
| `source/tests/nice/test_quic_ice/test.meta.cmake` | `9ef3d78fe353006dca7c729e0a66ba5c5265bc6038f2853fa40f5680fabab127` |
| `source/tests/nice/test_quic_ice/test_quic_ice.c` | `8dc4a3278c5e88e39b3c21b1a67c1bef4c5af3bed22b9ecaa60c28718497a20d` |
| `source/tests/nice/test_quic_ice_turn/CMakeLists.txt` | `7fa25ed014f51f66210132808c361ae67348132d38359977e3335096709deb12` |
| `source/tests/nice/test_quic_ice_turn/test.meta.cmake` | `460f20d8e4b454aa50bedc275715689dab8e5a8ef50acc63afe965ad56e11d36` |
