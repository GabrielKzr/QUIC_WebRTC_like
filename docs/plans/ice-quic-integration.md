# Validação da integração ICE, CHOWNAT e QUIC

Atualizado em 2026-11-03. As etapas abaixo são o plano atual de validação.
A execução de cada etapa será discutida com o usuário; este documento não
constitui autorização para implementar todas elas de uma vez.

## Estado validado

- [x] HAL e implementações direct/ICE separadas; libnice controla suas sockets.
- [x] ICE threadless com contexto GLib processado pelo wait e signaling síncrono.
- [x] Dois peers negociando ICE até READY e trocando datagramas pela HAL.
- [x] STUN real e signaling TCP no ISP do Patchbay, com um peer por NAT IPv4.
- [x] Seleção de candidato remoto reflexivo, conteúdo correto e fechamento.
- [x] Regras NAT opcionais por teste via PATCHBAY_NAT_EXTRA_RULES. Os cinco
  testes diretos usam regras de mapeamento espelhado; ICE original usa NAT nativo.
- [x] Cenário ICE separado com remapeamento por XOR, regras no .meta e
  checagem obrigatória de portas privada/pública diferentes nos dois peers.
- [x] TURN UDP obrigatório com force_relay, par relay/relay, payloads íntegros
  e confirmação servidor de Refresh lifetime zero/fechamento das allocations.
- [x] Configure/build all e suíte final10/10 em16,63s; TURN2,70s.
- [x] Túnel TCP CHOWNAT sobre ICE direto e TURN, com handshake original,
  datagrama KA,16KiB nos dois sentidos, EOF e fechamento dos peers/allocations.
- [x] Suíte completa após CHOWNAT/ICE:12/12 em22,95s.
- [x] Validação final após espera de controle no chamador:12/12 em26,50s;
  CHOWNAT/ICE com cinco tentativas6/6 em17,08s.
- [x] Suíte anterior passou 8/8 em 11,74s em 2026-11-03, com coturn real
  extraído em /tmp e disponível pelo PATH/LD_LIBRARY_PATH da execução.

O NAT nativo pode preservar a porta espontaneamente. O novo cenário comprovou
A39241→55625 e B50811→34427, com dados e fechamento; testes ICE original/novo
passaram2/2 em5,41s. Controle sem regras rejeitou portas preservadas nos dois
peers. Captura WAN indisponível nesta máquina; evidências em logs dos candidatos
selecionados e snapshots nft. TURN foi validado posteriormente: três cenários
ICE passaram3/3 em8,13s; o relay levou2,70s e liberou duas allocations antes de
aprovar. Controle com servidor STUN-only retornou erro sem fallback direto.
CHOWNAT TCP foi validado posteriormente nos dois caminhos:2/2 em6,04s e
seis execuções consecutivas em17,43s. O KA foi enviado e recebido explicitamente
antes do handshake; agendamento por inatividade/sessão prolongada não cobertos.
QUIC sobre ICE e GNS3 ainda pendentes.
O BIO próprio já funciona com QUIC sobre transporte direto;
a configuração atual usa peer IPv4 fixo e limite de payload de 1200 bytes.

## Etapas pendentes, em ordem

Tratamento de timers corrigido no chamador CHOWNAT, conforme decisão do usuário:
`chownat_recv_control()` aguarda o prazo total e lê sem bloquear após prontidão.
Aplicado nas cinco recepções de controle; ice_recv e HAL permanecem como estavam.
Cinco tentativas validadas6/6 execuções; suíte completa12/12 em26,50s.
Diagnóstico/protótipo histórico em
[análise de timeout](../analysis/chownat-ice-recv-timeout-2026-11-03.md).

### 1. Comprovar remapeamento de porta

- [x] Usar um cenário NAT coerente em que a porta pública difira da privada.
- [x] Registrar porta local, candidato STUN e par selecionado.
- [ ] Acrescentar captura WAN quando a ferramenta estiver disponível.
- [x] Exigir READY, dados íntegros nos dois sentidos e fechamento correto.

Aceite: evidência explícita da diferença de portas e da transferência completa.

### 2. Forçar TURN

- [x] Configurar TURN real e impedir que o teste aprove um caminho direto.
- [x] Exigir candidato selecionado relay e registrar o par utilizado.
- [x] Verificar dados bidirecionais e fechamento com liberação da allocation.

Aceite: tráfego comprovadamente pelo relay, sem aprovação por fallback direto.

### 3. Validar CHOWNAT sobre ICE

- [x] Adaptar um teste específico para CHOWNAT com ice_ops.
- [x] Validar estabelecimento, transferência íntegra, datagrama keep-alive e encerramento.
- [x] Exercitar caminho direto negociado e TURN, preservando as regressões diretas.

Os novos cenários são network_chownat_ice_tcp e network_chownat_ice_turn_tcp.
Manutenção prolongada e agendamento de KA por inatividade ficam na etapa5.

Aceite: fluxo completo de CHOWNAT usando ICE, com erros propagados na saída.

### 4. Validar QUIC sobre ICE

- [ ] Conferir o contrato de peer/endereço e limite de datagrama do BIO atual.
- [ ] Validar cliente e listener, handshake, dados íntegros e shutdown pela HAL.
- [ ] Exercitar caminho direto e relay, mantendo ICE ativo até encerrar QUIC.

Aceite: transferência completa e shutdown correto, sem disputar leitura de socket.

### 5. Validar falhas, manutenção e limpeza

- [ ] Signaling indisponível, credenciais inválidas e negociação ICE falhando.
- [ ] Timeouts de gathering/checks, handshake e shutdown, com recursos liberados.
- [ ] Perda do caminho, consent perdido e indisponibilidade de STUN/TURN.
- [ ] Execução longa para observar manutenção NAT e refresh TURN.
- [ ] Buffer insuficiente, perda/reordenação e contrapressão de envio.
- [ ] Quando implementados, validar candidatos adicionais/restart e recuperação;
  distinguir reconexão de continuidade da mesma sessão QUIC.

Aceite: falhas limitadas por prazo, códigos de erro corretos e limpeza verificável.
Um watchdog encerrando processos não constitui sucesso.

### 6. Reproduzir a validação no GNS3 — etapa final

- [ ] Montar dois peers atrás de NATs separados e ISP com STUN/TURN e signaling.
- [ ] Reproduzir os casos aprovados no Patchbay: remapeamento, direto e relay.
- [ ] Executar CHOWNAT sobre ICE e QUIC sobre ICE, incluindo encerramento.
- [ ] Repetir os casos de falha/manutenção aplicáveis.
- [ ] Salvar versões, configurações, capturas, pares selecionados e resultados.

Aceite: mesmos critérios de integridade, caminho e fechamento usados no Patchbay,
com evidências produzidas no GNS3. Uma execução anterior de CHOWNAT direto no
GNS3 não comprova essa integração. Guia: [sim/gns3/README.md](../../sim/gns3/README.md).

## Histórico do plano original — 2026-10-29

As seções seguintes preservam alternativas e checklists da proposta original;
não representam o estado atual. A sequência e as decisões acima prevalecem.
[Análise e fontes originais](../analysis/ice-quic-integration-2026-10-29.md).

## 1. Resolver ownership antes da implementação

- [ ] Fazer prova mínima da interface de I/O, sem alterar backends ativos.
- [ ] Rota A: libnice cria sockets; infraestrutura configura endereço/portas e
  transporta datagramas QUIC pela API libnice. Mesmo socket físico é possível.
- [ ] Rota B: infraestrutura cria FD; PJNATH `pj_ice_sess`/STUN/TURN baixos usam
  callbacks de envio e entrada de pacotes. Registrar custos e lacunas restantes.
- [ ] Escolher rota conforme prioridade de FD, manutenção, licença e recuperação.
  Libnice pública não deve ser anunciada como capaz de adotar FD arbitrário.

Saída: contrato de posse, um leitor por socket, ciclo de vida e versão fixada.
Não iniciar com fork libnice sem antes comparar A/B.

## 2. Provar ICE persistente com datagramas opacos

- [ ] Dois peers, IPv4, um componente, transporte de datagramas sem PseudoTCP.
- [ ] Signaling de laboratório separado, com credenciais/candidatos/gerações.
- [ ] STUN e TURN reais no laboratório; primeiro direto, depois relay forçado.
- [ ] No FD próprio, provar que STUN e dados usam o socket previsto e que o
  dispatcher único entrega cada pacote ao consumidor correto.
- [ ] Testar controles TURN vs QUIC conforme RFC 9443 e validar pacotes inválidos.

Aceite: payloads e datagramas íntegros em ambos os sentidos, par/relay registrados,
timers progridem e fechamento normal de todos os componentes.

## 3. Provar QUIC sobre o transporte ICE fixo

- [ ] Adaptador BIO datagrama, preferencialmente par BIO no primeiro ensaio.
- [ ] Adaptar cliente e listener desde o início; sem SSL_set_fd concorrente.
- [ ] Handshake não bloqueante com deadline total e filas limitadas.
- [ ] Dados de três rodadas e túnel TCP com conteúdo/EOF/status verificados.
- [ ] Caminho direto e TURN forçado, primeiro sobre UDP até o relay.
- [ ] Shutdown QUIC com ICE ainda ativo; depois release TURN/ICE e sockets.
  Aguardar fechamento assíncrono do agente quando aplicável, em vez de apenas
  destruir objetos e presumir que as allocations foram liberadas.

Aceite: nenhuma disputa de recv, truncamento, busy loop ou fechamento duplo.

## 4. Validar manutenção e falhas

- [ ] Pausa de aplicação maior que timeout NAT; verificar tráfego de manutenção.
- [ ] Pausa maior que idle timeout QUIC; definir comportamento esperado da sessão.
- [ ] TURN em execução longa suficiente para observar refresh real.
- [ ] Remapeamento de porta, perda, reordenação, filas cheias e contrapressão.
- [ ] Consent perdido, credenciais inválidas, TURN/signaling indisponíveis.
- [ ] Timeout/cancelamento em gathering, checks, handshake e shutdown.

Aceite: erro limitado por prazo, limpeza verificável e nenhuma falsa aprovação
por watchdog. Separar disponibilidade do caminho ICE da disponibilidade QUIC.

## 5. Recuperação: testar duas garantias separadas

- [ ] Direto cai com relay já coletado: observar falha, renegociação e retomada.
- [ ] TURN adicionado após READY: medir custo e passos necessários.
- [ ] Candidato remoto adicional e restart com geração/credenciais novas.
- [ ] Nova interface/IP: verificar necessidade de nova stream/agente e recolher.
- [ ] Medir primeiro reconexão QUIC explícita; depois continuidade da mesma sessão.
- [ ] Comparar endpoint físico vs lógico estável no BIO; registrar impacto
  em RTT, MTU, congestionamento e tempo máximo de indisponibilidade.

Aceite de reconexão não significa aceite de migração transparente. Só afirmar
continuidade se o mesmo QUIC/streams sobreviver e conteúdo permanecer íntegro.

## 6. Integrar à infraestrutura depois da prova

- [ ] Extrair contrato datagrama com modos UDP direto e ICE, sem refatoração ampla.
- [ ] Separar configuração/ownership de socket de configuração do QUIC.
- [ ] Rota libnice: integrar GLib ao dispatcher ou filas + FD de notificação.
  Rota PJNATH: integrar timer heap e callbacks/ioqueue ao dispatcher escolhido.
- [ ] Priorizar execução sem thread dedicada, conforme discussão posterior:
  integrar GMainContext ao laço existente por prepare/query/check/dispatch ou
  registrar TCP e timers QUIC no próprio contexto GLib. Na primeira opção,
  contrato de espera precisa fornecer vários descritores/eventos e timeout;
  um único FD de notificação não substitui o progresso do ICE na mesma thread.
- [ ] Adaptar estabelecimento, entrega, espera, erros e shutdown, não só HP.
- [ ] Definir manutenção por transporte, sem inferi-la de process_events.
- [ ] Preservar regressões CHOWNAT/QUIC existentes e acrescentar matriz ICE/TURN.
- [ ] Preservar fontes, patches, versões e logs dos experimentos no repositório.

Decisões abertas: FD próprio obrigatório ou preferencial; reconexão aceitável
no primeiro marco; modelo de eventos; versão/licença; política de relay prévio;
autenticação de peers e formato de signaling. Não exigem alterar código agora.
