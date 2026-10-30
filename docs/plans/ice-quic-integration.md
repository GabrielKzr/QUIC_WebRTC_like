# Plano inicial: ICE persistente e QUIC

Data: 2026-10-29. Estado: proposta para revisão, **não implementação autorizada
como decisão definitiva**. [Análise e fontes](../analysis/ice-quic-integration-2026-10-29.md).
Prioridade do usuário: ICE persistente, relay/novos candidatos, mesmo socket,
preferencialmente FD criado pela infraestrutura e demultiplexação central.

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
