## Estado final da revisão do manuscrito — 2026-11-10

Esta entrada prevalece sobre as versões intermediárias abaixo. Conferidos os
fontes atuais e git diff/status; apenas documentação alterada nesta revisão.

- Escopo experimental funcional: resumo/abstract, objetivos e resultados sem
  promessa de medir latência/vazão. Análise comparativa permanece conceitual.
- Casos C1--C12, sendo C12 prolongado; antigo teste dedicado de encerramento
  Chownat excluído da apresentação, sem excluir testes/artefatos do projeto.
- Implementação distingue topologia compartilhada de regras NAT por cenário.
  Resultados identificam Rocky8.10 como ambiente da coleta e Ubuntu como parte
  do desenvolvimento. Frase generalizando parâmetros configuráveis removida.
- Resultados explicitam payload binário, relação com modularidade e alcance
  Moderate/EIM/APDF. Expectativa para filtragem menos restritiva é inferência;
  NAT simétrico limita reutilização do endpoint STUN, TURN é alternativa;
  não apresentar esses perfis adicionais como experimentalmente avaliados.
- Corrigidos handshakes TCP/TLS, abertura simultânea, dependência do servidor,
  confidencialidade perante intermediário e critério de encerramento com
  liberação mesmo após438 tratado. Oito labels de tabela após captions.
- Após conferir Ford2005, usuário preferiu RETIRAR os percentuais comparativos
  do último parágrafo dos resultados, mantendo só complexidade/sincronização
  TCP hole punching com referência à seção teórica. Fonte atual confirma isso.
  A citação Ford permanece em outro parágrafo sobre suporte a hole punching.
- Pesquisa encontrou Trautwein et al., Large-Scale Measurement of NAT Traversal
  for the Decentralized Web: A Case Study of DCUtR in IPFS (IMC2026):
  https://arxiv.org/abs/2604.12484 e https://arxiv.org/html/2604.12484v1.
  Autores relatam TCP/QUIC estatisticamente semelhantes no DCUtR, ~70% na etapa
  condicionada ao sucesso das reservas relay e descoberta de endereço público.
  Não equivale a UDP básico versus TCP nem a teste ICE/libnice. Consultados
  resumo e seção5.2.3. Estudo NÃO adicionado ao BibTeX nem ao manuscrito.
- Conclusão ainda por redigir: seguir notas específicas abaixo, considerando
  numeração C1--C12 e ausência de comparação experimental de desempenho.
  Avaliação física fica para depois, por decisão explícita do usuário.
- git diff --check e verificações estáticas de referências/labels/casos feitos
  durante a sessão; sem compilação LaTeX ou novos testes de rede. Nenhum commit,
  stage ou push pelo agente. Usuário pediu apenas mensagem de commit.

## Revisão final de coerência dos resultados — 2026-11-10

Aplicados a pedido do usuário em docs/text/src/6_results.tex: distinguir ICE
direto de casos sem ICE, critério de encerramento com liberação (compatível com
438 tratado), retirar instrução antiga de inserir figuras, corrigir duração
aproximada11min e descrição de processamento no intermediário, distinguir
cifragem em trânsito de confidencialidade perante o servidor que termina sessões.
Conferido o PDF do link ford2005peer em ref.bib:
https://pdos.csail.mit.edu/papers/p2pnat.pdf, seção6.2/Tabela1: UDP310/380 (82%),
TCP184/286 (64%), relatos voluntários não aleatórios e amostras distintas.
Texto substitui20% menos chance por percentuais reportados e diferença18 pontos
percentuais, restritos ao levantamento de2005, sem generalizar às redes atuais.
Avaliação física permanece para depois por decisão do usuário. Sem compilação.

## Caso dedicado de encerramento Chownat retirado do TCC — 2026-11-10

A pedido do usuário, removidos dos resultados a linha e o parágrafo do antigo
C2 (Chownat sem ICE, encerramento dedicado). Casos seguintes renumerados:
C1--C11 regulares e C12 prolongado (antes C13); texto e tabelas ajustados.
Os demais casos Chownat foram preservados. Não foram removidos testes ou
artefatos do repositório; mudança apenas no escopo apresentado no manuscrito.
Verificados 12 identificadores consecutivos e ausência de C13 nos resultados.
Esta numeração prevalece sobre registros anteriores C1--C13, inclusive os
pontos anotados para a conclusão. Sem compilação LaTeX ou testes de rede.

## Pontos para redigir a conclusão do TCC — 2026-11-10

Registro solicitado pelo usuário; não corresponde a texto já inserido na conclusão.
Fonte atual: docs/text/src/7_conclusion.tex. Decisões desta conversa:

- A contribuição central é implementar e validar funcionalmente a integração
  QUIC/ICE para transportar dados de aplicações TCP locais entre gateways.
- Relacionar os resultados aos objetivos revisados: estabelecimento por caminho
  direto e TURN obrigatório, preservação dos dados, manutenção da mesma sessão
  e encerramento/liberação dos recursos. Apoiar cada conclusão nos casos C1--C13.
- Distinguir as vantagens previstas pelos protocolos e discutidas na análise
  lógica dos resultados experimentais da implementação. Menos etapas/RTTs em
  condições específicas não demonstram menor tempo total da infraestrutura,
  que também inclui sinalização, ICE, controles do backend e abertura dos streams.
- Não concluir superioridade medida sobre TCP/TLS, menor latência ou maior vazão:
  não foi realizada avaliação comparativa de desempenho. As promessas dessas
  medições foram retiradas do resumo/abstract, introdução e resultados.
- Como trabalho futuro, avaliar desempenho com pergunta e referência explícitas,
  cargas equivalentes, marcos de medição, repetições e dispersão. Possibilidade
  discutida: comparar QUIC/ICE direto e via TURN; comparação com TCP/TLS exigiria
  experimento apropriado. Não tratar essa sugestão como experimento autorizado.
- Limites a respeitar: hosts de propósito geral representam gateways; não houve
  validação de sensores, microcontroladores, consumo energético ou sistema IoT
  completo. TURN forçado não demonstra fallback automático; não afirmar validação
  de identidade remota com SSL_VERIFY_NONE. Não atribuir garantia geral de
  disponibilidade, recuperação/migração ou comportamento sob perdas à coleta.
- O usuário excluiu avaliação de falhas do foco experimental. O erro438 permanece
  como tratamento esperado de nonce expirado no encerramento TURN, com patch da
  libnice e remoção confirmada; não descrevê-lo como certificado expirado.
- Avaliação física e outros trabalhos futuros dependem do estado efetivo do
  manuscrito/experimentos na hora da redação; não afirmar que foram realizados.

Verificação deste registro: decisões explícitas da conversa e contexto já lido;
nenhuma nova execução ou edição da conclusão realizada nesta tarefa.

## Escopo experimental do TCC ajustado — 2026-11-10

Usuário autorizou retirar promessas de avaliação de desempenho. Alterados
resumo/abstract em docs/text/main.tex, objetivos/organização na introdução e
abertura/metodologia/remissão à carga nos resultados (nove ajustes discutidos).
Removido o objetivo específico de medir estabelecimento, resposta e vazão.
Discussões teóricas e análise conceitual preservadas. Usuário quer tratar na
conclusão a distinção entre vantagens dos protocolos e desempenho medido,
com comparação experimental como trabalho futuro; conclusão ainda não editada.
Verificadas remoções no texto ativo e git diff --check; sem compilação LaTeX.

## Manuscrito completo incorporado ao repositório — 2026-11-10

Usuário removeu tem.tex e passou a manter o projeto LaTeX em docs/text/.
Conferido no filesystem e Git (HEAD8514de0): main.tex inclui introdução,
trabalhos relacionados, fundamentos, QUIC, implementação, resultados e conclusão.
Resultados atuais: docs/text/src/6_results.tex, com C1--C13 e imagens das seis
figuras já presentes em docs/text/fig/results/tests/. Referências em ref.bib;
classe PUCRS e demais recursos estão junto ao main.tex. Trabalhos relacionados
está vazio; conclusão contém apenas título/label; avaliação física tem títulos
e anotação pendente. Introdução mantém objetivos de desempenho, sem assumir
que já foram medidos. Leitura de main, introdução, implementação, resultados e
estrutura dos fundamentos/QUIC; nenhuma compilação ou execução de teste feita.
Referências anteriores a tem.tex e à ausência do manuscrito completo são
históricas; usar docs/text como fonte atual. Preservadas edições do usuário.

## Identificadores dos casos no TCC — 2026-11-09

Por solicitação do usuário, tem.tex introduz tabela C1--C13 no início dos
resultados funcionais: 12 casos de rede regulares e um prolongado, sem incluir
fixtures de falha. Texto e tabelas de resultados usam os mesmos códigos.
O usuário retirou a avaliação de falhas do escopo do capítulo; o tratamento
438 permanece como evidência do encerramento da sessão prolongada.
Conferidos os 12 diretórios de artefatos curtos, 13 códigos únicos, labels e
balanceamento dos ambientes LaTeX. Sem compilação LaTeX nem novos testes de rede.

## Retomada em casa e preservação para o Git — 2026-11-09

Memória consolidada em [handoff-2026-11-09.md](handoff-2026-11-09.md),
com tarefas pendentes, comandos e distinção entre fatos verificados e
resultados históricos. repository-context.md atualizado como ponto de entrada.
O usuário informou que já copiou a coleta; localizada na raiz em
functional-results-2026-11-09/, sem criar cópia adicional. Os746 arquivos
originais (221PCAPs) foram comparados por SHA256 com a cópia de build:
nenhum ausente/diferente. Depois foram ajustados caminhos em índices/script,
tem.tex e guias; PCAPs/logs brutos mantidos iguais. README e SHA256SUMS
permitem conferir a coleta em casa. Tamanho original≈61MiB de conteúdo;
build inteiro ocupa≈1,8GiB no disco e continua ignorado.

Estado anterior ao commit do usuário: branch udp_transport, HEAD f9df4a8;
sem stage/commit/push pelo agente. Capturas levam o resultado da coleta,
não a instalação Flatpak nem os binários/dependências de build. Próximo passo
é gerar as seis imagens de comprovação e incorporar/compilar o trecho do TCC.
Não repetir testes só para abrir PCAPs: uma nova execução muda portas,
identificadores, tempos e números de quadro, embora deva manter o comportamento
esperado. PATCHBAY_PCAP habilita captura por presença; novas execuções
substituem artefatos dentro de build, não a cópia preservada da raiz.
Não houve nova execução de rede nesta consolidação; resultados18/18 e1/1
do teste longo estão documentados abaixo e no relatório específico.

## Wireshark local instalado — 2026-11-09

Instalado a pedido do usuário: Wireshark4.6.9 via Flatpak/Flathub, instalação
user em ~/.local/share/flatpak; sem pacotes ou privilégios de captura do sistema.
Abrir com `flatpak run org.wireshark.Wireshark`. Inclui tshark, executável por
`flatpak run --command=tshark org.wireshark.Wireshark ...`.
Verificados versão, leitura do PCAP remap/peer_a/eth0.pcap e quadros22/23:
Binding Request e Success com XOR-MAPPED-ADDRESS10.0.0.2:38576.
GUI aberta no DISPLAY:1 com essa captura e filtro frame.number22/23;
xwininfo confirmou janela eth0.pcap (wireshark/Wireshark).

Importação GPG inicial falhou com fchown Invalid argument no home NFS;
mesmo remote-add oficial funcionou com FLATPAK_USER_DIR em /tmp.
Copiada a chave pública importada desse repo temporário para o repo do usuário,
mantendo gpg-verify=true e gpg-verify-summary=true. Instalação concluiu sem
desabilitar assinaturas. Edição Flathub serve para análise de arquivos, sem
captura ao vivo. As notas anteriores de ausência de Wireshark/tshark descrevem
o estado anterior a esta instalação, não o estado atual.

## Evidências em pacotes e figuras do TCC — 2026-11-09

A pedido do usuário, a seção funcional em tem.tex foi reescrita com análise
de transações e seis placeholders de figuras. Guia de arquivos originais,
filtros por frame.number, quadros e campos em
docs/analysis/functional-pcap-figures-2026-11-09.md. Conferidos nos bytes dos
PCAPs: tradução54960→38576 correlacionada por Transaction ID; checks STUN
e QUIC no mesmo par; Allocate/ChannelBind/ChannelData; renovações; Refresh
3371→erro4383372→novo Refresh3373→sucesso3374 na captura longa do peer A.
Nonce do novo pedido coincide com o do erro e difere do pedido inicial.

Reconstrução dos quatro fluxos TCP unidirecionais dos loopbacks QUIC/ICE
direto:16384 bytes sem lacunas, todos iguais ao padrão i%251. SHA256 comum
4348e3b98e8a327b34ced39c1da9e67cdb4cd5e48e4d7960607a3ae403d35f0c.
Metadados em build/functional-results-2026-11-09/tcp-reassembly.json;
quadros em packet-index.json. Não houve nova execução de testes nessa
revisão, nem geração de screenshots (tshark/Wireshark ausentes). Falhas locais
continuam comprovadas por fixture/log, sem captura do runner. Não interpretar
o primeiro401 do desafio normal como senha inválida ou segmento loopback16KiB
como MTU WAN. LaTeX conferido estaticamente, sem compilador disponível.

## Resultados funcionais para o texto acadêmico — 2026-11-09

Verificado nesta sessão, HEAD f9df4a8, Rocky Linux8.10: rebuild incremental
de build/managed passou; suíte regular18/18 em37,20s e teste prolongado
QUIC/ICE/TURN1/1 em663,814s. Primeira tentativa sandbox foi impedida ao
criar sockets/namespaces; resultados válidos vieram da execução autorizada
fora dessa restrição, com NICE_DEBUG=all, G_MESSAGES_DEBUG=libnice e PCAP.
Não houve nova execução Ubuntu/GNS3 nem alteração funcional dos protocolos.

Logs confirmaram pares SRV-RFLX/SRV-RFLX nos quatro cenários ICE diretos e
RELAYED/RELAYED nos três TURN curtos. Remapeamento A54960→38576,
B47643→64027, troca íntegra. Seis túneis curtos passaram echo binário16KiB;
capturas locais contêm SYN/payload/FIN. Quatro testes locais de falha/regressão
confirmaram erro esperado e restauração da contagem de FDs. Prolongado:
67 blocos/1097728 bytes por sentido, permissões renovadas em241/481s,
allocations/canais em540s,438 no close dos dois peers seguido de retry e
remoção efetiva. Servidor confirmou TURN_RENEWED2/2 e TURN_RELEASED2.
221 PCAPs lidos sem erro pelo tcpdump; não é evidência de perdas induzidas
ou de isolamento da contribuição de cada mecanismo de keep-alive.

tem.tex recebeu as tabelas preenchidas e os limites das conclusões;
LaTeX conferido estaticamente, sem compilador disponível. Detalhes e vínculo
entre testes/tabelas em docs/analysis/functional-results-2026-11-09.md.
Logs/PCAPs arquivados em build/functional-results-2026-11-09/{short,long},
com observations.json e resumos. BUILD_LONG_TESTS foi temporariamente ON
no build/managed para reutilizar dependências, e restaurado OFF ao final.
Preservadas as alterações anteriores em coturn.cmake e nesta memória.

## Política de extração do coturn/libevent — 2026-11-09

Verificado no código: source/CMakeLists.txt redefine as políticas com mínimo3.16
e inclui novamente common_functions.cmake; funções capturam políticas na
definição. Assim a CMP0135 NEW da raiz não cobre coturn incluído por check_deps.
cmake_modules/coturn.cmake agora define CMP0135 NEW sob if(POLICY CMP0135),
como libnice, para usar timestamps de extração nas duas dependências URL.
Validação: CCACHE_DISABLE=1 cmake --fresh --preset all configurou/gerou sem
avisos no CMake3.26.5. A variável foi necessária porque o ccache tenta gravar
em ~/.ccache, fora do sandbox. Não executados build nem testes de rede.

## Correção do bootstrap de ferramentas — 2026-11-06

Após o usuário reproduzir falha no preset all, Meson e glib-mkenums deixaram de
ser requisitos externos obrigatórios. cmake_modules/build_tools.cmake reutiliza
instalações existentes; se ausentes, ExternalProject baixa Meson1.12.1 (wheel
oficial) e glib-mkenums da GLib2.56.4, ambos com SHA256 fixado, durante o build.
O launcher Python persistente e o gerador ficam em _deps/build-tools/bin; não
precisam de pip, PYTHONPATH, instalações no home ou wrappers em /tmp. Bootstrap
Meson requer Python>=3.10. GLib/headers, Ninja e patch continuam externos.
O hash dos caminhos das ferramentas integra o diretório da libnice, impedindo
reutilizar build.ninja que guardou o launcher Meson temporário anterior.

Verificado: comando exato cmake --preset all -DBUILD_PCAP_TOOLS=ON configurou
build/all sem ferramentas temporárias. Build/managed foi reconfigurado com
PATH=/usr/bin:/bin e sem PYTHONPATH; baixou/instalou as ferramentas e reconstruiu
libnice.18/18 passaram em31,85s, execução sem LD_LIBRARY_PATH/PYTHONPATH locais.
Incremental sem repetir etapas ExternalProject. GLib externa ainda fornecida
via PKG_CONFIG_PATH ~/.local/lib/pkgconfig. Logs /tmp/managed-tools-{configure,
build,ctest,incremental}.log. As referências abaixo aos wrappers na validação
inicial são históricas e explicam a falha reportada pelo usuário.

## Dependências gerenciadas pelo CMake — 2026-11-06

Implementado OpenSSL4.1.0 estático/PIC, coturn4.6.3 e libevent2.1.12-stable
via ExternalProject_Add, com versões/SHA256 fixados em build/<preset>/_deps.
Removidos os caminhos fixos ~/.local/openssl. A libnice0.1.21 com o patch TURN
existente agora usa o OpenSSL gerenciado, dispensando GnuTLS de desenvolvimento.
BUILD_PCAP_TOOLS=ON prepara libpcap1.10.5 e tcpdump4.99.5 sem troca de usuário.
PATCHBAY_PCAP continua controlando a captura na execução. CTest configura PATH
com ferramentas gerenciadas e diretórios dos executáveis externos encontrados.
Targets de rede individuais dependem de patchbay_runner, coturn e PCAP opcional;
Cargo continua responsável pelo crate Patchbay0.7.0 e Cargo.lock. GLib com headers,
glib-mkenums, Meson e demais ferramentas de desenvolvimento continuam externos.

Verificado em build/managed: compilação de todas as dependências a partir de
arquivos-fonte de cache conferidos por SHA256, URLs upstream respondendo200,
suíte18/18 em31,79s sem PATH/LD_LIBRARY_PATH de ~/.local. Capturas habilitadas:
CHOWNAT ICE TCP, ICE UDP direto/TURN e QUIC ICE TURN passaram4/4 em11,58s;
68 PCAPs lidos pelo tcpdump gerenciado, status COMPLETE. ldd confirmou ausência
de libssl/libcrypto/libevent/libpcap compartilhadas nos binários inspecionados.
GLib externa para o build foi fornecida via PKG_CONFIG_PATH ~/.local/lib/pkgconfig;
a afirmação de independência de ~/.local vale para as dependências gerenciadas.
Meson e glib-mkenums foram fornecidos por wrappers/arquivos de /tmp na validação.
Incremental de target QUIC TURN individual não repetiu etapas ExternalProject.
Configure sem testes/rede e configure long passaram; long não foi executado.
Logs: /tmp/managed-{build-native,ctest,pcap-ctest,final-incremental}.log.

Descobertas verificadas: coturn pelo CMake upstream compila/inicia mas não
responde à sondagem STUN nesta máquina (causa exata não isolada); configure/Make
nativo passou os testes. Patch em patches/coturn faz configure respeitar TMPDIR,
pois upstream fixa /var/tmp e falha no sandbox. Build usa diretório privado de
fontes como TMPDIR, compila/copia apenas turnserver e desabilita bancos/systemd/
Prometheus/SCTP. A descoberta de libpcap usa CMAKE_PREFIX_PATH explícito para
não escolher a versão do sistema; libnice recebe -ldl para seus executáveis com
OpenSSL estático. Detalhes de uso em docs/build-dependencies.md.

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

# Retomada atual — integração libnice consolidada em 2026-11-04

Ler primeiro a [memória completa da integração](libnice-integration.md).
Ela consolida todas as etapas, decisões substituídas, arquivos alterados,
commits, código ainda no working tree, ambientes, diagnósticos e resultados.
**Esse documento prevalece sobre os retratos cronológicos abaixo.**

- ATUALIZAÇÃO POSTERIOR: o usuário autorizou repetir o teste11min. Em
  HEAD17b4d0a, configure/build long e CTest1/1 passaram em664,11s, total664,12s.
  Ambos peers transferiram1097728 bytes/67 blocos íntegros por sentido, mesma
  sessão relay/relay. Permissões renovadas241/482s; allocations/canais540s.
  Close662s recebeu438 por peer, patch repetiu e obteve res0/lifetime0;
  servidor removeu ambas663s, TURN_RENEWED/TURN_RELEASED confirmados.
  Agora há aprovação longa completa após o patch. Artefatos atuais em
  build/long/sim/patchbay/artifacts/network_quic_ice_turn_long; log CTest em
  /tmp/quic-long-nonce-test.log. Nenhuma implementação alterada; somente memória
  e documentos de validação atualizados. As interrupções abaixo são históricas.
- Retomada posterior conferiu HEAD1a08804 na branch udp_transport e Git limpo
  antes dos ajustes documentais. O usuário commitou implementações/testes/patch
  descritos abaixo. Referências a HEADf56c49e e working tree pendente são da
  fotografia anterior ao commit. Conferidos código e12 hashes da memória;
  nenhuma execução de teste nesta retomada, incluindo long.
- A libnice corrigida JÁ está integrada ao build, não apenas em /tmp.
  `cmake_modules/libnice.cmake` baixa/confere tag0.1.21, aplica o patch em
  `patches/libnice`, compila Meson/Ninja e fornece `Libnice::Nice` estática.
  ICE e nice_local usam esse target, sem substituir bibliotecas do sistema.
- Patch final exige NONCE/REALM válidos, limita cópia e uma repetição do desafio
  438 na remoção. Usa lifetime zero e timers existentes; inicializa lifetime do
  log. Campo de controle privado, sem alteração de API pública da libnice.
- A regressão nova está no fonte compartilhado test_ice_failures, cenário
  turn_stale_nonce. Exige erro de conectividade ao peer ausente e, ao fechar,
  438 seguido de remoção efetiva no coturn, além de FD/contexto limpos.
- Validado: configure/build all e long; incremental sem reaplicar patch;
  regressão nonce passou4,21s; suíte all18/18 em33,36s. Um executável separado
  /tmp/test_ice_failures-unpatched, ligado à libnice original, falhou como
  controle negativo esperado. O projeto não voltou a usar libnice sem patch.
- Teste11min iniciado após patch foi INTERROMPIDO a pedido do usuário, CTest130.
  Confirmado sem processos CTest/runner/peer long restantes. Não repetir sem
  novo pedido. Dados/renovações da execução longa completa anterior foram
  exercitados, mas ela falhou no cleanup; não há long aprovado após o patch.
- Branch udp_transport, HEAD1a08804. Mudanças recentes e do usuário foram
  incorporadas pelo usuário; nenhum staging/commit criado pelo agente. Esta
  retomada é documental; não repetiu testes ou modificou implementação.

## Histórico: diagnóstico independente do nonce TURN — 2026-11-04

Esta seção registra o estágio ANTERIOR à integração do patch no build.
A memória consolidada acima prevalece. Quanto ao diagnóstico
do fechamento e às versões efetivamente testadas. Branch udp_transport,
HEAD f56c49e; alterações anteriores do usuário preservadas, sem staging/commit.
Relatório completo, fontes e reprodução:
[nonce expirado no fechamento](../analysis/libnice-turn-close-nonce-2026-11-03.md).

- CONFIRMADO sem HAL/QUIC/signaling/Patchbay: um único NiceAgent e coturn4.6.1,
  credenciais corretas, recepção ativa e contexto GLib vivo até a callback.
  libnice0.1.21 sistema e tag upstream compilada falham ao remover com nonce
  expirado; controles com nonce válido removem. Contextos padrão e privado
  reproduzem. Callback/GTask concluem com sucesso mesmo sem remoção remota.
- CONFIRMADO: cliente TURN Python independente recebe438, usa NONCE novo,
  repete Refresh lifetime0 e o servidor remove. MESSAGE-INTEGRITY validado
  nas respostas de sucesso. Coturn observado responde corretamente ao fluxo.
- CONFIRMADO por experimento causal: somente na cópia0.1.21 em /tmp, tratar438
  em priv_map_reply_to_relay_remove, copiar nonce e repetir pelo helper existente
  resolve os quatro cenários testados, incluindo ambos contextos e renovação
  anterior. Patch experimental NÃO aplicado ao projeto/sistema, não pronto
  para distribuição sem revisão de limites/erros/timers e regressões.
- Renovação normal0.1.21 com nonce expirado foi exercitada: recebe438, repete
  e funciona; fechamento depois recebe438 e desiste. Não confundir os caminhos.
- libnice0.1.24 agora foi testada em runtime, não apenas comparada estaticamente:
  também não remove com nonce expirado. Na reprodução gathered-only, remove
  streams antes de processar respostas, loga stream1destroyed e termina por
  timeout/retransmissões até com nonce válido (nesse controle servidor remove).
  Não extrapolar esse achado adicional para todas as integrações.
- História: condição problemática já presente no commit0556ec49 de2019-02-19.
  Teste TURN upstream examinado espera callback, sem forçar nonce expirado ou
  conferir remoção no servidor; não foi auditada toda a CI upstream.
- Tracker/API acessível nesta investigação: buscas em títulos/descrições não
  acharam relato confirmadamente idêntico; comentários retornam401. Não afirmar
  inexistência de issue. MR333 trata consistência/valgrind; MR283 final trata
  DNS/close-forced, não correção comprovada438. Nenhum contato externo feito.
- Teste longo completo continua REPROVADO: dados/renovações comprovados,
  remoção TURN falha. Nenhuma nova execução11min feita nesta investigação.
  Aprovar abordagem de correção da dependência antes de mudar produção;
  depois repetir longo. Não mascarar aumentando nonce ou relaxando asserts.
- Artefatos/scripts/logs/matrizes e builds isolados: /tmp/nice-nonce-deep/.
  Esta investigação só acrescentou relatório e atualizou estas notas no repo.

## Verificação QUIC/ICE — 2026-11-03

Adicionados `test_quic_ice` e variante `test_quic_ice_turn`, reutilizando
os serviços ISP e o padrão do teste CHOWNAT, sem alterar bibliotecas.
Após READY, o teste configura `udp_session.dst` com o candidato remoto
selecionado para o BIO de peer fixo; limite de payload permanece 1200.
Valida CONNECTED único, 16384 bytes por sentido, fechamento e deinit;
TURN exige relay/relay e confirmação da liberação das allocations.

Verificado nesta sessão: compilação dos dois targets e
`ctest --preset all -R '^network_quic_ice(_turn)?_tcp$'`: 2/2 passaram,
3,59 s direto e 4,06 s TURN. Logs em
`build/all/sim/patchbay/artifacts/network_quic_ice{,_turn}_tcp`.
Ambiente desta sessão: Ubuntu, libnice 0.1.21; coturn 4.6.1 extraído em
`/tmp/quic-coturn` com dependências, usando PATH/LD_LIBRARY_PATH temporários.
Execução Patchbay exigiu permissões fora do sandbox para namespaces.
A suíte completa não foi executada. Falhas/manutenção prolongada e GNS3
continuam pendentes.

# Memória para futuras sessões

Estado atual consolidado em 2026-11-03. Detalhes e evidências:
[contexto do repositório](../repository-context.md). Comandos:
[guia Patchbay](../../sim/patchbay/README.md).
Estas memórias são arquivos locais; não garantem persistência fora do repo.
O resumo abaixo prevalece sobre os relatos históricos posteriores quando houver
conflito. Conferir sempre Git e código antes de continuar.

## Ponto de retomada — 2026-11-03

### Resumo final verificado da sessão

- Branch udp_transport, HEAD07f7d91; implementação, testes e documentação da
  sessão ainda no working tree. Nenhum staging/commit feito pelo agente.
- ICE direto/STUN, remapeamento por XOR no .meta, TURN UDP obrigatório e
  túnel TCP CHOWNAT sobre ICE direto/TURN foram validados no Patchbay.
  HAL continua com wait específico por implementação, sem thread dedicada.
  Libnice possui suas sockets. force_relay é configuração específica ICE.
- Decisão definitiva desta discussão: prazo de controle CHOWNAT pertence ao
  chamador. chownat_recv_control foi aplicado nas cinco recepções de controle,
  com deadline monotônico, wait pelo restante e recv com timeout0 após
  prontidão. Eventos ICE/KAs curtos não renovam prazo nem esgotam tentativas.
  O protótipo que alterava ice_recv é somente histórico, não aplicar por engano.
- Novos testes CHOWNAT/ICE usam cinco tentativas e preservam handshake01→03→03.
  Consomem KA antecipado durante a espera por READY e validam16KiB binários
  nos dois sentidos, EOF/saídas zero/limpeza. TURN exige relay/relay e confirmação
  no servidor de Refresh lifetime0/fechamento de todas as allocations.
- Última validação: build all, sintaxe Wall/Wextra/Werror e diff-check passaram;
  CTest completo12/12 em26,50s. Novos CHOWNAT direto/TURN passaram6/6 repetições
  em17,08s. Harness do helper confirmou prazo200ms mesmo com eventos/KAs
  contínuos e propagação de erros. Namespaces executados fora do sandbox.
- Ambiente local Rocky8.10: libnice0.1.24 e coturn4.6.3 em ~/.local, usando
  OpenSSL4.1.0 local. Headers GLib/PCRE locais correspondem ao runtime do host;
  libevent2.1.12 estática para coturn. Shell/VS Code receberam PATH,
  PKG_CONFIG_PATH/LD_LIBRARY_PATH locais. Dependências de serviços exigidas
  no configure somente para os testes ICE de rede habilitados.
- Fatos e limites detalhados abaixo e na análise de timeout. A falha inicial
  de cinco tentativas foi reproduzida/instrumentada; a falha posterior de
  READY é registrada separadamente, sem afirmar causa interna totalmente
  isolada. Captura WAN permanece indisponível. KA explícito não prova
  agendamento por inatividade nem manutenção NAT/TURN prolongada.
- Próximo passo funcional: QUIC sobre ICE direto e TURN, verificando contrato
  do BIO (peer IPv4 fixo/payload1200), handshake, dados e shutdown com ICE vivo.
  Depois falhas/manutenção/recuperação e GNS3. Perda/reordenação, refresh longo,
  TURN TCP/TLS, restart e continuidade da mesma sessão não estão validados.
- Atualização final da memória apenas documental; nenhum teste repetido por
  esta consolidação. Resultados acima vêm das execuções já registradas.

### Instalação local da libnice — 2026-11-03

- A pedido do usuário, libnice0.1.24 compilada do tarball oficial e instalada
  em ~/.local (lib/libnice.so.10.16.0, include/nice e lib/pkgconfig/nice.pc).
  Backend crypto OpenSSL4.1.0 local; GStreamer, UPnP, introspection e docs
  desabilitados. Bibliotecas do sistema não foram substituídas.
- Host Rocky8.10 sem headers GLib nem Meson. Headers/arquivos pkg-config
  extraídos de RPMs oficiais Rocky glib2-devel2.56.4-169.el8_10 e
  pcre-devel8.42-6.el8, correspondentes aos runtimes instalados, para ~/.local.
  Symlinks locais de desenvolvimento apontam para as bibliotecas em /usr/lib64.
  Meson1.12.1 usado temporariamente em /tmp/libnice-local-install/meson.
- ~/.bashrc.d/libnice-local.sh acrescenta ~/.local/lib/pkgconfig ao
  PKG_CONFIG_PATH e ~/.local/lib ao LD_LIBRARY_PATH; carregar com source no
  shell já aberto. SHA256SUMS e install-log.txt em ~/.local/share/libnice-install.
- Verificado pkg-config nice=0.1.24/prefix ~/.local e ldd sem dependências
  ausentes, usando libcrypto.so.4 local. Cinco testes upstream passaram:
  address, credentials, drop-invalid, restart e fullmode. Teste nice_local do
  repo compilado com Wall/Wextra/Werror contra a instalação passou: ambos
  READY e mensagens corretas nas duas direções. Sockets fora do sandbox.
  Não reexecutada suíte NAT/TURN nem alterados fontes/build system do repo.
- Após erro de descoberta no IDE, confirmado que pkg-config sem
  PKG_CONFIG_PATH não encontra nice; com o caminho local encontra0.1.24.
  Acrescentados PKG_CONFIG_PATH e LD_LIBRARY_PATH locais ao cmake.environment
  em .vscode/settings.json (ignorado pelo Git), preservando Rust/configuração
  existente. Configure isolado em /tmp/libnice-cmake-discovery-check encontrou
  nice0.1.24 e gerou build com sucesso; não foi inspecionada a UI do VS Code.

### Plano de validação atualizado — 2026-11-03

- Decisão posterior do usuário supera proposta de corrigir ice_recv: prazo
  deve ser controlado no chamador. Aprovou/aplicou função privada
  chownat_recv_control em chownat.c e cinco substituições no handshake
  (cliente/servidor) e fechamento (envio/recepção). Deadline monotônico com
  prazo positivo fornecido, wait pelo restante e recv com timeout0 após
  prontidão. Ignora datagramas curtos e NOT_READY sem renovar prazo; retorna
  ERROR em falha. Ice_recv/HAL/recepção de payload não alterados nesta correção.
- Testes CHOWNAT/ICE agora têm conn_max_attempts/dconn_max_attempts5, cobrindo
  regressão anteriormente contornada com20. Primeiro repeat passou cinco
  execuções e falhou na terceira TURN antes do handshake: B não chegouREADY,
  A enviou KA e aguardou. Fixture ignorava payload durante wait_ice_ready,
  deixando slot pending ocupado. Pode impedir progresso ICE; não foi isolado
  o estado interno exato dessa execução. Ajustada fixture para consumir e
  registrar KA antecipado e contar esse recebimento na checagem posterior.
  Logs anteriores preservados; runner reprovou por55,79s, nunca sucesso.
- Após ajuste,6/6 execuções CHOWNAT/ICE com5 tentativas passaram em17,08s.
  Harness temporário que inclui fonte CHOWNAT ativo e usa HAL de fixture
  confirmou prazo200ms sob despertares5ms (~39 chamadas), também com KAs
  contínuos; NOT_READY transitório seguido de controle03 em~83ms; erros de
  wait/recv propagados. Sem resets de prazo. Não é teste de rede/NAT.
- Build all offline com RUSTC/RUSTDOC diretos passou. Wall/Wextra/Werror em
  CHOWNAT e dois modos de teste, diff-check e suíte completa12/12 em26,50s
  passaram fora do sandbox. CHOWNAT/ICE direto2,98s/TURN3,74s,16KiB nos dois
  sentidos, EOF/close e allocations liberadas. Snapshot/harness/logs finais e
  falha de READY em docs/experiments/chownat-ice-recv-timeout-2026-11-03/
  applied-chownat-helper (ignorado pelo Git). Análise/plano/contexto atualizados.
  Sem commit; manutenção prolongada/perda/reordenação ainda não validadas.
- Usuário pediu verificar a causa das cinco tentativas falhando. Diagnóstico
  confirmado em cópias instrumentadas, relatório
  [chownat-ice-recv-timeout-2026-11-03.md](../analysis/chownat-ice-recv-timeout-2026-11-03.md).
  Ice_recv faz uma única ice_wait e retorna NOT_READY ao vencer timer GLib
  sem payload. Servidor pediu1s mas recebeu NOT_READY após1–20ms; cinco
  tentativas acabaram em~80ms (duas falhas), antes do primeiro01 do cliente
  em~149ms, que aguardava accept TCP. Servidor enviou cinco KAs; cliente os
  consumiu como respostas não03 e também falhou.20 tentativas passou com oito
  retornos antecipados, portanto contorna a janela sem corrigir a espera.
- Medição ociosa real ICE/TURN pediu250ms: atual retornou19,148/19,119ms;
  cópia que mantém prazo total de recv retornou250,261/250,693ms. Alterando só
  essa espera na cópia, com5 tentativas, TCP passou3/3 em2,730/2,862/2,877s,
  com payload16KiB/EOF/saídas zero e allocations liberadas. Não identificado
  qual callback/tipo específico originou cada timer GLib. Sem nova validação
  de NULL/timeout enorme/perda/reordenação. Ice_wait público não foi alterado.
- Nenhuma correção aplicada no código ativo nesta investigação. Patch/sources/
  logs preservados em docs/experiments/chownat-ice-recv-timeout-2026-11-03
  (ignorado pelo Git), além de /tmp/chownat-ice-timeout-analysis. SHA-256 de76
  arquivos rastreados e comparação do teste novo com snapshot confirmaram
  preservação. Uma chamada da fixture ociosa colidiu caminho binário/diretório
  antes dos peers; execução corrigida concluiu normalmente. Próxima correção
  recomendada: preservar prazo solicitado em ice_recv antes de avançar QUIC.
- Usuário confirmou que HP CHOWNAT continua como handshake01→03→03 sobre
  o caminho ICE já estabelecido e autorizou teste similar a test_chownat com
  ice_ops. Criado source/tests/nice/test_chownat_ice (fonte/.meta) e variante
  test_chownat_ice_turn (CMake/.meta reutiliza fonte). Cenários tcp no Patchbay,
  nomes network_chownat_ice_tcp e network_chownat_ice_turn_tcp, sem regras NAT
  extras. Reutilizam ISP STUN ou TURN existente. Backends/runner/topologia não
  alterados nesta tarefa. Confirmado por leitura: chownat_connect só notifica
  CONNECTED; não pular chownat_hole_punching como sugerido incorretamente antes.
- Teste conecta signaling TCP, udp_conn_init com ice_ops, aguarda READY/par
  adequado (reflexivo direto ou relay/relay) e confere envio KA original do
  CHOWNAT e recepção de byte zero nos dois peers antes do handshake. Isso é
  validação explícita do datagrama, não do agendamento5s ou NAT prolongado.
  Executa udp_connection original e exige statusOK, um CONNECTED,16KiB em cada
  direção por callbacks, nenhum LOST_DATA e sessão fechada. Runner confere
  conteúdo binário/EOF do echo e saída zero. Deinit precisa liberar ICE e TCP;
  variante TURN mantém confirmação CLOSED/OK das allocations no signaling.
- Compilação inicial expôs macro size de chownat.h interferindo nos headers
  libnice. Fonte novo inclui chownat.h após headers externos, sem refatorar a
  macro global. Config inicial de5 tentativas passou direto e falhou TURN no
  handshake (status-5, CONNECTED0); alinhada a20, como test_chownat existente.
  Depois2/2 em6,04s e repeat until-fail:3 passou6/6 em17,43s. Hipótese para
  sensibilidade: ice_recv pode retornar NOT_READY após controle/timer antes do
  prazo pedido e HP conta tentativas/envia KA; não isolada por instrumentação.
  Não foi corrigido timeout/retry da biblioteca nesta tarefa nem testada perda.
- Configure all e build dos dois targets passaram. Wall/Wextra/Werror e
  diff-check passaram. Suíte completa12/12 em22,95s fora do sandbox, novos
  CHOWNAT direto2,82s e TURN3,31s; demais cenários preservados. Artefatos nos
  diretórios com esses nomes em build/all/sim/patchbay/artifacts. Logs confirmam
  KA, handshake,16KiB nos dois peers e TURN_RELEASED allocations=2/contador0.
  Documentação/plano atualizados: próximo passo QUIC sobre ICE. Teste UDP
  CHOWNAT isolado sobre ICE, manutenção longa, perda/reordenação, falhas e
  recuperação não foram acrescentados neste pedido. Sem commit do agente.
- Usuário autorizou criar/validar teste TURN. Novo target/test.meta/isp.sh em
  source/tests/nice/test_ice_udp_transport_turn reutiliza fonte dos peers com
  ICE_REQUIRE_RELAY e NAT nativo. Config ICE ganhou force_relay (defaultFALSE),
  exige turn_server se TRUE e aplica propriedade libnice antes de gathering.
  Contrato oficial conferido: https://libnice.freedesktop.org/libnice/NiceAgent.html#NiceAgent--force-relay.
  Edição preexistente do usuário em set_remote_credentials preservada.
- Coturn real no ISP com lt-cred-mech, realm/usuário ice-test e senha de
  laboratório ice-test-password, UDP3478 e relays49160–49200. Teste exige
  candidato relay na coleta e par selecionado relay/relay, READY, payload
  correto nos dois sentidos e close liberando HAL/agente/contexto.
- Signaling compartilha implementação anterior com opção --turn-log. Após
  close, peers enviam frame CLOSED e aguardam OK. Servidor consulta stdout
  do coturn via /proc/<PID-do-script>/fd/1 e exige, para todas as sessões new
  com lifetime>0 (mínimo2), refreshed lifetime0 e closed(2nd stage). Só então
  imprime TURN_RELEASED allocations=2 e confirma aos peers. Sem confirmação
  não há saída zero. Esse verificador depende do formato de log coturn4.6.3.
- Primeira execução levou payloads pelo relay, mas reprovou por faltar log
  verbose das sessões. Habilitado --verbose no ISP, sem relaxar o aceite;
  checagem reforçada para aguardar fechamento efetivo, além de Refresh0.
  CTest dos três cenários ICE passou3/3 em8,13s. Controle negativo em
  /tmp/ice-turn-no-relay-check usou servidor STUN-only: peer erro/exit1, nenhum
  READY/relay/fallback direto; encerramento não foi por watchdog.
- Configure all e build all passaram. Primeiro build total encontrou rustup
  tentando sincronizar1.83.0 em diretório protegido; repetido offline com
  RUSTC/RUSTDOC apontando diretamente ao toolchain stable já instalado. Nenhuma
  instalação Rust ou alteração de política/cache CMake necessária nesta etapa.
  Sintaxe Wall/Wextra/Werror passou no transporte e três variantes de teste;
  sh -n, AST Python e diff-check passaram.
- Suíte final10/10 em16,63s, fora do sandbox: nice_local0,04; unit2,02;
  CHOWNAT TCP1,24/UDP1,33/close1,28; QUIC TCP1,33/UDP1,15; ICE2,73;
  remap2,77; TURN2,70. Par TURN final10.0.0.1:49160↔10.0.0.1:49162; logs
  confirmam ambas as sessões fechadas e contador global de allocations0 antes
  de TURN_RELEASED. Captura WAN continua indisponível. Não valida refresh
  prolongado, TURN TCP/TLS, falhas/restart ou CHOWNAT/QUIC sobre ICE. Próxima
  etapa funcional: CHOWNAT sobre ICE. Sem commit criado pelo agente.
- Usuário autorizou novo cenário separado com regras no .meta. Criado
  source/tests/nice/test_ice_udp_transport_remap com CMakeLists próprio para
  recompilar o mesmo fonte do teste original e test.meta.cmake com libs,
  ICE_REQUIRE_PORT_REMAP, dependências coturn/python3, script ISP compartilhado
  e PATCHBAY_NAT_EXTRA_RULES. Nenhuma alteração no runner/topologia Rust.
- Regras: limpa mapa fullcone/postrouting; indexa DNAT por udp sport XOR0x4000
  e aplica a mesma expressão no SNAT UDP; SNAT TCP para ISP mantém signaling.
  Intervalo efêmero observado32768–60999 evita resultado zero. Cenário com
  um host por NAT, sem alocador para colisões/múltiplos hosts. Teste exige local
  selecionado srflx e porta pública != base privada em ambos os peers, além
  de READY/payloads/fechamento existentes; imprime base e endpoint público.
- Configure all e build dos dois executáveis passaram. CTest ICE2/2 em5,41s:
  original2,64; remap2,76. Novo teste confirmou A192.168.80.20:39241 para
  10.0.0.2:55625 e B192.168.80.30:50811 para10.0.0.3:34427, mensagens corretas
  e saídas zero. Snapshots nft confirmam XOR e TCP; captura WAN indisponível.
  Controle negativo em /tmp/ice-remap-negative-control executou novo binário
  sem regras e reprovou ambos por portas iguais, com runner não zero. Primeira
  asserção de fixture esperou acentos, mas locale gerou '?'; conferência dos
  logs existentes confirmou motivo correto, sem repetir a execução.
- Ambos os modos compilaram com fsyntax-only/Wall/Wextra/Werror; diff-check
  passou. Nove testes registrados; suíte inteira não repetida. README nice,
  guia Patchbay, contexto e plano atualizados. Fonte ice_udp_transport.c já
  modificado pelo usuário antes desta tarefa, preservado. TURN permanece a
  próxima etapa funcional; captura WAN do remapeamento ainda pendente.
- Verificação solicitada de remapeamento na topologia existente: logs normais
  da execução2,68s mostravam só endpoints públicos, sem portas privadas, e
  captura estava indisponível. Repetido somente network_ice_udp_transport com
  NICE_DEBUG=all/G_MESSAGES_DEBUG=all, sem alterar regras NAT ou fontes: passou
  1/1 em2,65s. Logs -c.stdout.log/-s.stdout.log nos artefatos registram candidatos
  host/reflexivo: A192.168.80.20:34484 ->10.0.0.2:34484;
  B192.168.80.30:54002 ->10.0.0.3:54002. READY e dados bidirecionais confirmados.
  Portanto nesta execução NAT traduziu IP e preservou as duas portas; não
  comprova porta pública diferente da privada. Logs anteriores copiados para
  /tmp/ice-port-verification/before. Nenhuma regra NAT alterada nesta análise.
- A pedido do usuário, test_ice_udp_transport/test.meta.cmake agora chama
  check_deps(DEPENDENCIES turnserver python3) quando BUILD_TESTING e
  BUILD_NETWORK_TESTS estão ON e o target ICE é configurado. README nice
  documenta falha no configure e PATH para instalação local. Sem dependência
  adicional para builds sem teste ICE de rede. Edição preexistente do usuário
  em ice_udp_transport.c preservada.
- Quatro configures novos isolados em /tmp/ice-configure-dependency-check-*
  validaram: PATH sem turnserver falha com Missing dependencies: turnserver;
  PATH com ~/.local/bin passa; sem rede ou com BUILD_TESTING=OFF passa sem
  turnserver. git diff --check passou. Sem recompilação ou novo teste de rede,
  pois a mudança é só checagem de dependências no configure.
- Coturn instalado posteriormente a pedido do usuário em ~/.local:
  turnserver4.6.3 e turnutils, libevent2.1.12 compilada estática localmente,
  usando OpenSSL4.1.0 local. Sem backends de banco de dados, systemd ou
  Prometheus; STUN/TURN com configuração por CLI disponível. Não foi criado
  serviço permanente. ~/.bashrc.d/coturn-local.sh acrescenta ~/.local/bin ao
  PATH; cmake.environment do VS Code recebeu o mesmo caminho.
- Compilação inicial CMake do coturn passou os vetores RFC5769, mas não
  respondeu ao probe STUN; strace temporário confirmou recepção sem resposta.
  Substituída pela compilação configure/make oficial da mesma versão, sem
  alteração dos fontes do repo. Ajustado apenas TMPDIR no configure temporário
  para /tmp devido à restrição de escrita em /var/tmp. Causa exata da diferença
  entre builds não isolada. Executáveis com permissão755 e turnadmin como link
  relativo para turnserver, sem depender de /tmp para execução.
- Validação final: CTest network_ice_udp_transport1/1 passou em2,68s com
  PATH/LD_LIBRARY_PATH locais, fora do sandbox para namespaces. Artefatos em
  build/all/sim/patchbay/artifacts/network_ice_udp_transport/. Não repetida a
  suíte inteira nem testado TURN relay. Manifesto/checksums e configuração em
  ~/.local/share/coturn-install; fontes/logs de build em /tmp/coturn-local-install.
- Diagnóstico local posterior à instalação libnice: execução CTest das17:49
  passou7/8; network_ice_udp_transport falhou em0,35s antes dos peers.
  isp.stderr.log informa turnserver não encontrado; result.txt registra ISP
  script exited before READY. command -v não encontrou turnserver e a cópia
  antiga /tmp/ice-coturn-runtime/root/usr/bin/turnserver não existe neste host.
  Coturn precisa ser disponibilizado no PATH para esse teste STUN. Diagnóstico
  por logs/consulta local; sem instalação coturn ou nova execução de testes.
- Usuário pediu consolidar as etapas pendentes e acrescentar reprodução no
  GNS3 como etapa final. Plano atual: (1) comprovar porta pública diferente
  da privada; (2) TURN forçado; (3) CHOWNAT sobre ICE; (4) QUIC sobre ICE;
  (5) falhas/manutenção/limpeza; (6) reproduzir casos aprovados no GNS3.
- Atualizados docs/plans/ice-quic-integration.md, docs/repository-context.md e
  source/tests/nice/README.md; criado sim/gns3/README.md com preparação,
  cenários, evidências e aceite. Plano original preservado como histórico.
- Estado comprovado continua ICE/STUN pela HAL no NAT nativo do Patchbay,
  com signaling e mensagens bidirecionais; suíte8/8 em11,74s registrada acima.
  Remapeamento real, relay, protocolos sobre ICE e GNS3 continuam pendentes.
  Atualização apenas documental, sem nova execução de testes ou mudança C/Rust.

### Lista de regras NAT por teste — 2026-11-03

- Preferência posterior do usuário: substituir a opção booleana por
  set(PATCHBAY_NAT_EXTRA_RULES "regra1" "regra2" ...) no .meta.cmake.
  Removidos PATCHBAY_MIRRORED_NAT/--mirrored-nat e bloco fixo do Rust.
  add_test_class zera a variável a cada teste e copia a lista ao target;
  registro CMake gera <target>.nat.nft e passa --nat-rules <arquivo>.
- Main lê o arquivo e passa conteúdo opcional à topologia. Regras aplicadas
  em ordem nos dois roteadores após NAT nativo, substituindo @PUBLIC_IP@,
  @WAN@ e @ISP_IP@ por valores do roteador/ISP. Lista ausente/vazia mantém
  o NAT nativo. Sintaxe de cada elemento é nftables; ponto e vírgula interno
  precisa de escape CMake \\; para não separar elementos da lista.
- Cinco .meta WebRTC agora declaram as oito regras antigas do mapeamento
  espelhado. ICE não declara lista e mantém ISP script/NAT nativo. Não há
  mais adição implícita de SNAT TCP no Rust: testes que sobrescrevam regras
  e usem signaling TCP devem incluí-lo na própria lista. README atualizado.
- Configure/build all e diff-check passaram. Conferidos arquivos gerados:
  oito linhas por lista, sem escapes CMake no nft final; argumentos CTest
  com --nat-rules só nos cinco diretos. CTest all com coturn temporário real
  fora do sandbox passou8/8 em11,74s: nice0,04; unit2,02;
  CHOWNAT TCP1,31/UDP1,42/close1,40; QUIC TCP1,42/UDP1,25; ICE/NAT2,87.
  Nenhum teste permanente novo nem commit criado pelo agente.

### Mapeamento espelhado opcional por teste — 2026-11-03

- Usuário pediu regras manuais como parâmetro opcional do build system.
  Propriedade PATCHBAY_MIRRORED_NAT ON no .meta.cmake acrescenta
  --mirrored-nat ao runner. Ausente/OFF usa NAT nativo. Main aceita flag e
  script opcionais e passa mirrored_nat bool a topology::Program. Bloco
  manual inteiro, inclusive SNAT TCP condicional para ISP, só roda com true.
- Os cinco testes diretos WebRTC CHOWNAT/QUIC habilitam a propriedade.
  ICE não a declara: permanece com isp.sh, signaling e Nat::Moderate nativo.
  Nós/enlaces e regras dentro do bloco manual não mudaram. README documenta
  opção e CLI; nenhuma alteração em bibliotecas/protocolos.
- Configure/build all e diff-check passaram. CTest gerado conferido: cinco
  cenários com flag, ICE com script sem flag. Primeira suíte7/8 em9,75s:
  nice_local, unit e cinco diretos passaram; ICE falhou antes de READY porque
  coturn temporário havia desaparecido. Dependências recuperadas por download
  e extração em /tmp/ice-coturn-runtime, sem instalação global. Repetido só
  o ICE com esse PATH/LD_LIBRARY_PATH: passou1/1 em2,83s (total2,84s).
- Snapshots router-a.nft confirmam opened_ports e :udp sport nos cinco
  diretos, ausentes no ICE; result.txt dos seis cenários de rede mostra PASS.
  Todos os oito testes foram validados nas duas execuções, não houve uma
  única execução final8/8. Não afirmar remapeamento de porta no ICE apenas
  por usar NAT nativo. Sem commit criado pelo agente.

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

## Revisão dos logs QUIC/ICE — 2026-11-03

Revisados stdout/stderr de ambos peers, serviços ISP, capturas UDP dos dois
roteadores, estado nft e resultado dos dois cenários. Direto: pares selecionados
recíprocos 10.0.0.2:53989/10.0.0.3:52841; captura confirma tráfego entre eles.
TURN: relay/relay 10.0.0.1:49192/49199; tráfego externo dos dois peers somente
com 10.0.0.1:3478. Cada peer envia 16 blocos de 1024 bytes; teste/runner
confirmam os 16384 bytes por sentido e EOF. Direto até 1200 bytes UDP;
TURN até 1204 bytes, compatível com encapsulamento ChannelData de 4 bytes.

Pontos para discutir antes de mudar código: `Should not be here` no cliente
(2 direto, 9 TURN) coincide com keep-alives de 1 byte enviados pelo próprio
servidor no hole punching; classificação ERROR é enganosa. Fase transiente
mantém 10 esperas de 50 ms mesmo após confirmação (aprox. 500 ms).
Coturn registra erro de SQLite inexistente nos dois cenários, embora o fluxo
com usuário estático funcione. TURN registra 3 Refresh lifetime=0 por allocation,
a intervalos compatíveis com retransmissão (0,5 s e 1 s), com respostas nas
capturas; causa da repetição ainda não identificada. Ambas allocations são
removidas e `TURN_RELEASED allocations=2` é confirmado. ISP captura zero
pacotes na interface ix; roteadores capturam 95/95 direto e 122/122 TURN,
sem drops. Capturas começam tarde para cobrir todo o gathering no direto.
Nenhuma correção funcional aplicada nesta revisão.

## Causa confirmada da retransmissão TURN no close — 2026-11-03

Investigação somente; implementação do projeto não alterada. Fonte upstream
libnice tag 0.1.21 consultado em agent/agent.c e agent/discovery.c (cópias
em /tmp/libnice-{agent,discovery}-0.1.21.c). Recepção usa API explícita;
ice_close só itera GMainContext, sem recv_messages nem callback de recepção.
No modo datagrama, iterar esse contexto sozinho não consome as respostas.

Experimento A/B no mesmo teste QUIC/TURN com NICE_DEBUG=all: baseline em
/tmp/quic-close-baseline-artifacts confirma envio, retransmissões +0,5/+1,5 s,
`TURN deallocate ... timed out` após ~2 s e só então liberação local.
Variante isolada /tmp/ice-close-probe.c e /tmp/test-quic-close-probe habilitou
nice_agent_attach_recv com callback de descarte exclusivamente no fechamento.
Logs /tmp/quic-close-probe-artifacts confirmam resposta TURN válida em ~1 ms,
priv_map_reply_to_relay_remove res=0/lifetime=0 e liberação sem retransmissão.
Ambos testes passaram; logo PASS anterior comprovava liberação no servidor,
mas não distinguia confirmação recebida do timeout interno da libnice.
Discutir correção com usuário; preservar leitura explícita durante operação.

Correção aplicada com autorização: ice_close habilita ice_discard_recv via
nice_agent_attach_recv somente no fechamento, antes de close_async.
Rebuild e CTest QUIC/ICE direto + TURN passaram 2/2 (3,71 s e 2,65 s).
Logs NICE_DEBUG confirmam resposta de remoção lifetime=0 processada;
sem retransmissão/timeout de deallocate e TURN_RELEASED allocations=2.

## Testes locais de falha ICE — 2026-11-03

Criado source/tests/nice/test_ice_failures com fonte único e três argumentos
CTest: signaling_unavailable, turn_bad_credentials, remote_absent.
Sem alteração de biblioteca e sem Patchbay: porta TCP reservada sem listener,
coturn loopback com senha errada/force_relay e candidato UDP reservado sem
respondedor. Exigem UDP_TRANSPORT_ERROR/FAILED, nunca READY, fechamento e
retorno da contagem de FDs ao baseline (depois de aquecer GLib/GTask).
TURN exige readiness STUN, log 401 e nenhuma allocation bem-sucedida.
Timers do agente reduzidos somente na fixture; limite 8 s não pode contar
como sucesso; CTest TIMEOUT 15 s. Três repetições de cada caso passaram
(9 execuções), cerca de 3,3 s no total. Não valida udp_conn/QUIC nem os
prazos default ou duração prolongada. README nice documenta comandos.

## Revisão geral da suíte atual — 2026-11-03

Rebuild all e CTest completo com NICE_DEBUG=all/G_MESSAGES_DEBUG=libnice:
17/17 passaram em 31,00 s. Revisão usou logs frescos dos 12 cenários network
registrados e LastTest.log dos 5 casos locais/unit; diretórios antigos não
registrados foram excluídos do diagnóstico atual. Três cenários TURN (HAL,
CHOWNAT e QUIC): uma resposta de remoção por peer, nenhuma retransmissão nem
timeout de deallocate; todas as allocations confirmadas liberadas.

Achados para discutir, não corrigidos: test_chownat_udp_close retorna zero
mesmo se udp_connection falhar, ignora retorno de send/disconnect/deinit e
não exige 20 mensagens/estado closed; test_chownat TCP também ignora erro da
conexão/deinit (runner ainda valida payload/EOF). Callback udp_close imprime
payload via %s sem respeitar nbytes; logs exibem bytes estranhos após strings,
compatível com leitura além do payload, não prova corrupção no transporte.
Aviso libnice 'Integrity check failed - bad request' surge uma vez por peer
em vários cenários ICE, para datagrama STUN 28 bytes vindo do peer; caminho
selecionado depois funciona. Origem exata ainda não comprovada; não descartar
como mero ruído de autenticação. Falhas de pares host-host 192.168.80.x entre
LANs isoladas são esperadas, com sucesso pelo par reflexivo. Coturn ainda
emite erro SQLite inexistente; usuários estáticos funcionam. Captura ISP ix
vazia, capturas WAN dos roteadores contêm tráfego; limita observabilidade.
Fase transiente QUIC conserva 10x50 ms. Nenhuma correção funcional nesta revisão.

Correção autorizada aplicada em test_chownat_udp_close: contadores próprios
CONNECTED/recepção (cliente 20, servidor 21), valida retorno de send e tamanho,
disconnect, udp_connection, closed antes de deinit, deinit e LOST_DATA.
Impressão limitada por nbytes via %.*s. Rebuild e CTest focado passaram
1/1 em 1,58 s; logs confirmam 20/21 mensagens, sem caracteres extras.

## Diagnóstico do aviso de integridade STUN — 2026-11-03

Reproduzido QUIC/ICE direto com NICE_DEBUG e wrapper /tmp/quic-stun-capture-bin/
tcpdump adicionando -XX; artefatos em /tmp/quic-stun-inspect, sem editar projeto.
Decodificado payload peer-peer exato de 28 bytes: tipo 0x0011 (Binding
Indication), body length 8, cookie 0x2112a442, único atributo 0x8028 FINGERPRINT
length 4. Não tem USERNAME/MESSAGE-INTEGRITY. Horário coincide com libnice
'resending STUN to keep the selected base address ... alive' no emissor e
'Integrity check failed - bad request' no receptor.
Fonte 0.1.21: conncheck.c chama stun_usage_bind_keepalive; usages/bind.c cria
indication sem chave. agent.c nice_agent_init_stun_agent usa SHORT_TERM_CREDENTIALS
sem NO_INDICATION_AUTH no modo RFC5245/RFC5389. stun/stunagent.c valida indications
com os requisitos de credenciais curtas e retorna UNAUTHORIZED_BAD_REQUEST;
conncheck.c emite o aviso. Origem é interna à libnice instalada, não senha ICE
errada nem pacote QUIC entregue por engano. RFC8445 seção11 exige indications
keepalive sem autenticação, recomenda só FINGERPRINT. Recepção não precisa
produzir payload/reply para esse keepalive; envio já preserva mapping NAT.
Nenhuma correção de biblioteca ou flags internos aplicada. Sessão prolongada
continua necessária para validar manutenção real; não concluir isso só deste
handshake curto.

## Teste prolongado separado — 2026-11-03

Criado source/tests/long/test_quic_ice_turn_long, fonte compartilhado QUIC/ICE
com ICE_LONG_SESSION. Classe BUILD_LONG_TESTS padrão OFF e explicitamente OFF
no preset all; novo preset long usa build/long, compila só teste/runner e CTest
filtra label long. Listagem confirmada: all 17 casos, long 1 caso selecionado.
Modo opcional tcp-long no runner: 67 rodadas de 16384 bytes/10s, mínimo 660s,
probe limite710s/CTest740s. Dados exatos, mesma conexão, fechamento normal.
ISP wrapper exige renovações por env ICE_TEST_REQUIRE_RENEWALS: Refresh positivo
por allocation e CHANNEL_BIND pelo menos duas vezes cada; depois release normal.
Fluxos curtos continuam sem essa exigência. Comandos em source/tests/long/README.md.

Build passou. Execução ctest --preset long falhou em11,38s com early eof;
não validou sessão de11min nem renovação. Reprodução GDB isolada em
/tmp/quic-long-gdb-artifacts confirma SIGSEGV em ossl_quic_write_flags ->
SSL_write_ex2 -> ossl_quic_udp_send_ka -> udp_connection nos dois peers.
Usuário adicionou SSL_write_ex(data->conn, "\0", 1, NULL); preservado, sem
correção não autorizada. Ponteiro written NULL e envio de byte no stream
precisam discussão; recv atual encaminha dados do stream ao túnel TCP.

Regressão dos dois testes curtos QUIC/ICE após runner tcp-long: 2/2 passaram
(3,47s direto, 2,60s TURN). Nenhuma execução de11min bem-sucedida nesta etapa.

Reexecução solicitada após usuário trocar written NULL por &written no send_ka:
rebuild long passou; CTest long falhou em11,42s com `payload mismatch in round 2`.
SIGSEGV anterior não se repetiu. O byte zero de keep-alive é enviado no mesmo
stream da aplicação e o recv atual o encaminha ao TCP; teste detecta interferência
no conteúdo depois da primeira pausa. Nenhuma correção adicional aplicada.

## Streams QUIC explícitos — implementação autorizada, 2026-11-03

ossl_quic_data_t agora guarda stream e control, ambos bidirecionais, criados
pelo cliente (IDs0/4) e aceitos pelo servidor por ID/tipo. Default stream
desabilitado; incoming policy ACCEPT. open_streams usa o deadline já existente
de5s e processa SSL_handle_events enquanto aguarda disponibilidade/aceitação.
Troca inicial de um byte D/C em cada stream é consumida antes de CONNECTED,
permitindo servidor enviar primeiro sem depender de payload TCP. Não usar
ordem de aceitação para identificar streams. Payload via data->stream;
send_ka via data->control, com written/erros verificados. process_events
consome controle sem callbacks de dados nem encaminhamento TCP. Streams
liberados antes da conexão no deinit. Keep-alives continuam acionados pelo
loop udp_conn, sem mudança de sua API.

Build all/long passou; quatro testes QUIC curtos passaram4/4 em8,99s
(TCP/UDP direct e TCP ICE direto/TURN). Teste long está em execução;
segunda rodada já passou sem mismatch. Aguardar resultado completo antes
de afirmar renovações/duração validadas.

Primeira execução completa com streams separados: dados íntegros por660s,
permissões renovadas aos241/481s, allocations e CHANNEL_BIND aos540s. CTest
falhou em661,85s porque signaling.py ainda esperava CLOSED com timeout5s;
ISP stderr confirma TimeoutError nesse read. Ajustado prazo para720s somente
quando ICE_TEST_REQUIRE_RENEWALS=1 (cenário long); negociação SDP e testes
curtos mantêm seus limites. Segunda execução completa em andamento.

Segunda execução long (signaling720s): falhou em664,10s, peer exit1.
Todos67 blocos/1097728 bytes por sentido transferidos íntegros na mesma sessão;
fechamento QUIC chegou a deinit sem erro de status. ISP confirmou permissões
renovadas242/482s, allocations lifetime600 e CHANNEL_BIND541s. No deinit ICE,
Coturn respondeu438 Stale nonce aos pedidos de remoção aos662s; nenhum
Refresh lifetime0/closed/TURN_RELEASED no log. Clientes falham esperando
confirmação da liberação. Logo separação de streams e duração/renovações foram
exercitadas, mas teste completo NÃO passou; liberação TURN após nonce expirar
é achado pendente a discutir. Não contornado/desabilitado no teste.
Artefatos atuais em build/long/sim/patchbay/artifacts/network_quic_ice_turn_long.

## Diagnóstico confirmado de 438 no deallocate TURN — 2026-11-03

Investigação somente, sem alterar bibliotecas/configuração do projeto.
Na libnice0.1.21, conncheck.c priv_map_reply_to_relay_refresh trata erro438,
copia resp (novo nonce) e repete Refresh normal. Já
priv_map_reply_to_relay_remove chama refresh_free para qualquer retorno
!=STUN_USAGE_TURN_RETURN_INVALID, inclusive RETURN_ERROR(2) de438. Assim
nice_agent_close_async completa localmente sem remoção efetiva no servidor.
Fonte0.1.24 contém a mesma lógica de remoção (comparação estática, não teste
runtime dessa versão). RFC8656 seção8.3 e RFC8489 exigem/recomendam repetir
com NONCE novo; o nonce não é a senha ICE/TURN.

Reprodução rápida isolada derivada da fixture local (credenciais corretas,
sem novo teste commitado): /tmp/ice-nonce-probe com coturn --stale-nonce=1,
espera2s antes de close, logs /tmp/ice-nonce-probe.log. Confirma438, mapping
remove res2, refresh_free imediato, nenhum lifetime0. Controle igual com
--stale-nonce=60, /tmp/ice-nonce-control.log: mapping res0 lifetime0,
servidor confirma Refresh lifetime0. Executáveis diagnósticos mantêm asserts
da fixture original de senha errada e saem1 intencionalmente neste cenário;
não contabilizar como testes CTest passados. Evidência são os logs da
transação isolada e código upstream. Não repetir11min para essa investigação.

Correção real pertence ao tratamento de remoção na libnice: reconhecer438,
armazenar novo nonce e reconstruir/repetir Refresh lifetime0 antes de liberar
o CandidateRefresh. Discutir patch/dependência com usuário; não contornar
simplesmente aumentando validade do nonce no teste.

## Busca por relatos upstream do 438 no fechamento — 2026-11-03

Buscas web por libnice close/stale nonce/deallocate/priv_map_reply_to_relay_remove
não localizaram relato indexado confirmadamente idêntico. Encontrada discussão
primária2019: https://groups.google.com/g/discuss-webrtc/c/qAuxJA1ETU4
sobre interrupção TURN após10min e438, com referência ao issue libnice64;
resposta trata necessidade de renovar ChannelBind, não comprova mesmo bug de
remoção. Encontrado commit relacionado ad3b2c89 / MR283: discovery Remove TURN
allocations in a consistent way (2025), descreve retransmissões/cleanup ao
podar candidatos; não há evidência consultada de correção438 no deallocate.
Fonte indexado: https://gitlab.com/libnice/libnice/blob/master/agent/conncheck.c
Acesso direto ao tracker/API GitLab freedesktop e GitLab.com indisponível pela
ferramenta web; não afirmar inexistência de issue ou que nunca foi reportado.
Nenhum issue aberto ou mensagem enviada nesta tarefa.

## Preparação da infraestrutura física — 2026-11-10

Relato do usuário: PC principal ligado a dois roteadores, um para hospedar
STUN/TURN/signaling mediante DMZ e outro para peer A; segundo PC atrás de
terceiro roteador para peer B. Endereços, WANs, rotas e disponibilidade de
configuração da DMZ ainda não informados/verificados.

Conferência estática no HEAD 08c29f8, working tree inicialmente limpo:
test_quic_ice e test_quic_ice_turn usam PATCHBAY_ISP_ADDRESS (IPv4 numérico,
STUN/TURN3478 e signaling5555) e PATCHBAY_LOCAL_ADDRESS. Executáveis podem
ser chamados manualmente, mas exigem exatamente16384 bytes por sentido,
echo TCP local no peer servidor e probe local no cliente; não são CLI genérica
para aplicações. Signaling TURN exige log real do coturn e confirma remoção.
Signaling conecta sem bind ao IP do peer: host com duas interfaces precisa
isolar processos/rotas para não contornar o NAT. Mesmo host também pode
resolver localmente o endereço do servidor, contornando o roteador.
Nenhum teste físico ou mudança de rede realizada nesta conversa.

Verificação local posterior: ip -br -4 addr confirma enp0s31f6 com
192.168.80.100/24 e wlp0s20f3 com10.0.0.108/24. Default Ethernet via
192.168.80.1 métrica100, Wi-Fi via10.0.0.1 métrica600. `ip -4 route get
10.0.0.108 from192.168.80.100` (com espaço após from na execução) retorna
local/dev lo/table local: confirmar que apenas bind não atravessa A.
Orientação: separar Ethernet em namespace peer-a; comandos de mudança de
rede fornecidos ao usuário, não executados. IP WAN de A e configuração do
PC2 ainda desconhecidos. STUN deve revelar WAN de A na rede10.0.0.0/24,
não necessariamente IP público da Internet. Sem teste de conectividade físico.

## Tentativa física ICE — 2026-11-10

Usuário relatou `signaling TCP connection refused`, depois `ICE não chegou a
READY`. Na inspeção, inicialmente não havia coturn nem signaling ouvindo; ambos
foram iniciados manualmente (coturn UDP3478, signaling TCP5555 em10.0.0.108).
Após a tentativa do outro PC, `ss` mostrou TIME-WAIT entre10.0.0.108:5555 e
10.0.0.40:55492, comprovando conexão TCP do peer remoto ao signaling, mas não
valida troca SDP/READY. No PC1 não havia namespace peer-a nem processo peer A;
não se conclui ainda a causa exata da falha ICE. O signaling foi reiniciado;
TIME-WAIT impediu bind imediato e motivou SO_REUSEADDR no listener. Confirmado
novo `READY` e LISTEN. Coturn permaneceu ativo. Falta executar ambos os peers
simultaneamente em redes isoladas e revisar saídas dos dois lados.

## Script de execução física do peer A — 2026-11-10

Criado `sim/physical/run-peer-a.sh` a pedido do usuário para iniciar em um só
comando (com sudo) coturn STUN/TURN, signaling, namespace da Ethernet, probe
TCP de16KiB e `test_quic_ice -c`. Aguarda conexão TCP do peer B ao signaling
para iniciar A, registra logs em /tmp/physical-ice-<timestamp> e restaura a
interface ao sair. Foram encerrados os serviços manuais anteriores para liberar
as portas. Validação estática: `bash -n`, `python3 -m py_compile` do signaling e
`git diff --check` passaram; portas3478/5555 livres após encerramento.
Não foi executado o script porque sudo exige senha interativa; logo nenhuma
nova validação física ICE/QUIC foi realizada nesta etapa.

Atualização: o script foi renomeado para `sim/physical/run-physical-ice.sh` e
passou a aceitar papéis `a` (PC1, sudo) e `b` (PC2, echo+peer B). PC1 aguarda
conexão TCP de B ao signaling antes de iniciar A. Comandos estáticos `bash -n`,
`git diff --check` e compilação Python do signaling passaram; não houve
execução root do script nem execução remota no PC2 nesta sessão.

## Recuperação do namespace físico — 2026-11-10

Usuário executou script A e recebeu `Device enp0s31f6 does not exist`.
Inspeção sem privilégios: host tem Wi-Fi10.0.0.108 e enp1s0 sem carrier;
`physical-peer-a` ainda existe, sem PIDs, enquanto enp0s31f6 não está no host.
Primeira execução do script deixou log signaling READY e rota do peer em
/tmp/physical-ice-20261110-190826. Conclusão provável: Ethernet ficou no
namespace após interrupção; conteúdo interno não pôde ser lido sem sudo.
Script atualizado para recuperar enp0s31f6 desse namespace vazio antes de
validar IP, e tratar SIGHUP na limpeza. Verificações `bash -n` e
`git diff --check` passaram. Recuperação efetiva ainda depende da execução
sudo do usuário; não afirmar que a interface já foi restaurada.

## Primeira negociação física direta — 2026-11-10

Execução do usuário registrada em /tmp/physical-ice-20261110-191408:
`peer-a.log` confirmou `ICE READY: 10.0.0.117:53129 -> 10.0.0.40:41209;
direto/reflexivo` e `Connection opened to remote end` após hole punching.
QUIC não conectou: `UDP_CONN_CONNECT_ERROR` (-6), connected=0,
0 bytes. Usuário relatou `connect Connection refused` no outro lado.
Código em udp_conn.c mostra que, no modo -s, essa mensagem é emitida se
connect() a tcp_session.local (127.0.0.1:1234 no test_quic_ice) falhar.
Isso é compatível com echo local ausente no PC2; falta log B para confirmar
formalmente. No PC1 após o teste, namespace não existe e Ethernet enp0s31f6
está de volta com192.168.80.100. Nenhum dado QUIC/TCP transferido ainda.

## PCAPs da infraestrutura física — 2026-11-10

A pedido do usuário, `sim/physical/run-physical-ice.sh` agora inicia tcpdump
antes dos serviços/tráfego: no papel A, Wi-Fi wlp0s20f3 do servidor e Ethernet
enp0s31f6 dentro do namespace; no papel B, interface LAN do PC2, desde que
o script B seja executado com sudo. Filtro `udp or tcp port 5555` captura
STUN/ICE/QUIC e signaling; `-Z root` evita troca de usuário do tcpdump do
sistema no namespace. Capturas são terminadas com SIGINT antes de restaurar a
Ethernet; script imprime caminho e contagem dos PCAPs em /tmp/physical-ice-*.
Verificação estática `bash -n` e `git diff --check` passaram; tcpdump4.99.4
presente no PC1. Ainda não houve nova execução física com PCAP habilitado;
não há captura retroativa da execução que passou.

## Artefatos físicos dentro do repositório — 2026-11-10

A pedido do usuário, o script físico agora salva logs e PCAPs em
`sim/physical/artifacts/a-<data-hora>/` no PC1 e
`sim/physical/artifacts/b-<data-hora>/` no PC2. O diretório contém
`.gitignore` que ignora as execuções, preservando o arquivo de ignore.
Ao encerrar, se executado via sudo, o script transfere propriedade dos
artefatos ao usuário original. `bash -n`, `git diff --check` e
`git check-ignore` para PCAP exemplo passaram. Capturas anteriores em /tmp
não foram movidas; nova localização vale para próximas execuções físicas.

Correção da decisão acima, a pedido do usuário: removido
`sim/physical/artifacts/.gitignore`. Capturas e logs de futuras execuções
permanecem no repositório e não são ignorados pelo Git; o usuário quer
preservá-los/versioná-los. `git check-ignore` não encontrou regra para PCAP
exemplo; `bash -n` e `git diff --check` passaram. Nenhuma captura nova foi
executada nesta alteração.

## Diagnóstico do PCAP físico direto — 2026-11-10

Na execução `sim/physical/artifacts/a-20261110-194913`, logs confirmam ICE READY
(10.0.0.117:36888 -> 10.0.0.40:39423), handshake QUIC,16384 bytes nos dois
sentidos e probe OK/EOF. PCAP server-wifi é legível, inclui STUN/signaling;
também contém SSDP de fundo porque filtro antigo capturava todo UDP.
O PCAP peer-a-lan original está corrompido: tcpdump escreveu uma linha de
status após o cabeçalho PCAP e estatísticas ao final, ambas misturadas aos
registros; `tcpdump -r` falha. Uma cópia reparada foi criada em
`sim/physical/artifacts/a-20261110-194913-peer-a-lan-repaired.pcap` removendo
91 bytes de prefixo e80 de sufixo, preservando156 registros válidos. Leitura
integral com tcpdump passou; contém106 datagramas UDP com10.0.0.40, incluindo
44 datagramas >500 bytes (vários de1200) no fluxo entre os peers. Isso apoia,
junto aos logs, o envio QUIC; tcpdump sozinho só classifica como UDP.
Script atualizado para filtros por IP do servidor/peers, excluindo SSDP de
fundo, uso de nsenter em vez de ip netns exec para tcpdump do peer A, e
validação integral de cada PCAP no fechamento. `bash -n`, `git diff --check`
e aplicação dos filtros aos PCAPs existentes passaram; a correção do novo
capturador ainda não foi validada numa nova execução física.

Correção a pedido do usuário: removida a restrição por host dos três filtros
físicos de captura. Voltaram a `udp or tcp port 5555`, portanto SSDP e outros
UDP de fundo podem aparecer nos PCAPs. Validação estática `bash -n` e
`git diff --check` passou; não houve nova execução de captura nesta mudança.

## Signaling órfão após captura física — 2026-11-10

Usuário recebeu `Porta 5555 ou 3478 ocupada` ao reiniciar script A. Conferência:
enp0s31f6 foi recuperada ao host (192.168.80.100), namespace ausente,
UDP3478 livre, TCP5555 ocupado por `python3 -u .../signaling.py` PID166701
iniciado às19:49:13, mesma hora da captura anterior. Sem sudo, não foi
encerrado pelo agente. Script agora, antes de reservar portas, procura apenas
signaling.py cujo stdout aponta para `sim/physical/artifacts/a-*/signaling.log`,
envia TERM, aguarda liberação breve e ainda aborta caso outro processo ocupe
as portas, imprimindo `ss -p`. `bash -n` e `git diff --check` passaram; a
limpeza automática depende de execução sudo do usuário e não foi validada aqui.

## Segunda captura física validada, com limpeza pendente — 2026-11-10

Artefatos atuais `sim/physical/artifacts/a-20261110-200138/`: peer-a.log confirma
ICE READY 10.0.0.117:60001 -> 10.0.0.40:40221, QUIC handshake,
16384 bytes por sentido e fechamento; probe.log confirma payload/EOF. Leitura
integral por tcpdump passou para peer-a-lan.pcap (147 pacotes) e
server-wifi.pcap (77); ambos logs tcpdump reportam zero drops. PCAP peer A
contém44 datagramas >500 bytes no fluxo com10.0.0.40 (vários1200 bytes).
Limpeza não terminou: namespace physical-peer-a ainda existe, enp0s31f6 não
está no host, e signaling.py PID173370 permanece TCP5555; TURN e tcpdump não
estão ativos. TURN log informa sinal2, compatível com interrupção manual;
causa exata da limpeza interrompida não comprovada. Script alterado para
encerrar capturas com SIGTERM em vez de SIGINT (background pode ignorar INT),
validar PCAPs e oferecer modo `cleanup` que recupera namespace e encerra
signaling antigo identificado pelo stdout nos artefatos. `bash -n` e
`git diff --check` passaram. Modo cleanup ainda precisa ser executado com
sudo pelo usuário; estado de rede atual não está restaurado.
