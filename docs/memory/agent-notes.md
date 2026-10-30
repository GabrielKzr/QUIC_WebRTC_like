# Memória para futuras sessões

Consolidada em 2026-10-12. Detalhes e evidências:
[contexto do repositório](../repository-context.md). Comandos:
[guia Patchbay](../../sim/patchbay/README.md).
Estas memórias são arquivos locais; não garantem persistência fora do repo.

## Preferências do usuário

- Sempre adicionar e editar código bem formatado, com espaçamento, indentação e
  quebras de linha consistentes com o projeto, sem deixar ajustes manuais de
  formatação para o usuário.

- Comunicar em português brasileiro. Explicar escolhas e sua necessidade;
  evitar abstrações e regras adicionais sem justificativa.
- Nunca executar commits pelo usuário. Quando solicitado, fornecer apenas uma
  linha em bloco Bash: `git c "English message"`. Mensagem em inglês, até300
  caracteres; sem staging ou comandos git add/git commit.
- Preservar edições do usuário. Registrar problemas, separar hipóteses de fatos
  e não transformar tarefas pontuais em refatorações amplas.
- Respeitar pedidos de não validar. Na última interação sobre comentários de
  shutdown, o usuário preferiu escrevê-los pessoalmente; não sobrescrever.

## Decisões e estado atual verificado

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
