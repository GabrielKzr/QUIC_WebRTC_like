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
