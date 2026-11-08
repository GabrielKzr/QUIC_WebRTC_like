# Sugestões para a avaliação e os resultados

Data: 8 de novembro de 2026.

A introdução do capítulo e suas seções já foram adicionadas ao LaTeX pelo autor. Este documento reúne as sugestões para preencher essas seções; não registra novos experimentos nem resultados obtidos.

## Objetivo e metodologia dos testes

O resultado principal esperado é demonstrar que a infraestrutura estabelece comunicação e transporta corretamente os dados entre aplicações TCP associadas a gateways em redes distintas. As medições de desempenho complementam essa demonstração, sem exigir uma comparação de superioridade entre QUIC e TCP.

A avaliação pode responder a três perguntas:

1. A solução estabelece a comunicação nas topologias escolhidas?
2. Os dados chegam corretamente, inclusive após períodos de inatividade?
3. Quanto tempo é necessário para estabelecer e utilizar a comunicação?

Definir previamente, para cada teste, configuração, carga, número de execuções, prazo máximo e critérios de aprovação. Encerramento por timeout, limpeza forçada pelo runner ou simples compilação não demonstram sucesso funcional.

## Ambiente e configuração experimental

Registrar:

- Commit da implementação avaliada.
- Hosts: processador, memória, sistema operacional e papéis na execução.
- Roteadores: modelos, versões, interfaces, regras de NAT e filtragem.
- Versões e configurações das bibliotecas utilizadas.
- Localização dos serviços de sinalização, STUN e TURN.
- Topologia, endereçamento, velocidade dos enlaces e condições de rede impostas.
- Aplicações geradoras e receptoras, parâmetros da carga e método de coleta.
- Configuração de certificados e validação de identidade utilizada nos testes.

Separar resultados obtidos com namespaces dos obtidos com hosts e roteadores físicos. Os namespaces podem sustentar testes controlados e de regressão; a avaliação física deve ser identificada como outro ambiente. Não afirmar utilização de equipamentos antes de executar os respectivos testes.

## Estabelecimento da comunicação e caminhos selecionados

| Cenário | Procedimento sugerido | Evidências e critérios |
|---|---|---|
| QUIC sobre ICE direto | Transferir dados entre aplicações TCP nos gateways atrás de NATs distintos | Túnel pronto, par selecionado registrado, conteúdo correto e encerramento normal |
| QUIC sobre TURN | Repetir com relay obrigatório | Par relay/relay, conteúdo correto e remoção das allocations confirmada no servidor |
| ICE com remapeamento de portas | Utilizar o cenário controlado de tradução de portas | Porta pública diferente da privada e comunicação bem-sucedida |

Registrar o par de candidatos selecionado e usar PCAPs para confirmar o caminho dos dados. Apenas estabelecer a conexão não comprova que o transporte ocorreu diretamente ou por TURN. O par selecionado é o caminho utilizado para os dados pelo ICE, conforme o [RFC 8445](https://www.rfc-editor.org/rfc/rfc8445.html).

No cenário de remapeamento, distinguir a validação da interface ICE da validação do túnel QUIC completo. Não atribuir à integração inteira uma evidência obtida apenas em uma camada.

Para observar a variação no estabelecimento, uma proposta inicial é realizar 30 execuções independentes por cenário. Essa quantidade é uma escolha prática a confirmar, não uma exigência normativa. Registrar o número de sucessos sobre o total de tentativas e os motivos das falhas.

## Transferência de dados e integração com aplicações TCP

Utilizar duas cargas simples:

| Carga | Finalidade | Verificação |
|---|---|---|
| Transferência binária de conteúdo conhecido | Verificar preservação dos dados pelo túnel | Comparação exata ou hash do conteúdo completo, quantidade de bytes e EOF |
| Mensagens periódicas de requisição e resposta | Observar comunicação ao longo do tempo e medir resposta | Tamanho e intervalo definidos, número de sequência e conteúdo esperado |

Os testes podem usar tráfego sintético para avaliar o transporte entre gateways, sem sensores físicos. Para caracterizar o perfil como representativo de uma aplicação IoT específica, justificar seus parâmetros com base nessa aplicação.

Definir tamanho das mensagens, intervalo de envio, direção dos dados, duração e delimitação das mensagens no fluxo de bytes. Não presumir que cada chamada de envio corresponde a uma recepção completa.

O teste existente de 16 KiB serve como evidência funcional; sozinho, não deve ser apresentado como ensaio de vazão sustentada.

## Manutenção e encerramento da sessão

Executar uma sessão prolongada alternando transferências e pausas na mesma conexão. Verificar:

- Conteúdo preservado antes e depois das pausas.
- Continuidade da mesma conexão, sem reconexão implícita.
- Ausência de dados de controle misturados ao conteúdo da aplicação.
- Renovação dos recursos TURN durante a sessão, quando esse caminho for utilizado.
- Encerramento normal do túnel e liberação dos recursos.
- Remoção das allocations no servidor TURN, inclusive no caso de nonce expirado coberto pelo patch da libnice.

O código atual separa dados e controle em duas streams QUIC bidirecionais, IDs 0 e 4. Isso não equivale a uma stream por sensor ou a multiplexação de múltiplas aplicações.

Os registros do repositório relatam aprovação histórica do teste prolongado após o patch. Para utilizá-la no TCC, recuperar os artefatos, identificar a versão e descrever as condições daquela execução. Nenhum teste foi reexecutado para elaborar estas sugestões.

## Comportamento diante de falhas

| Falha | Comportamento esperado |
|---|---|
| Sinalização indisponível | Falha identificada, sem declarar comunicação pronta |
| Credenciais TURN inválidas com relay obrigatório | Falha sem aceitar caminho direto como substituto |
| Peer ausente | Negociação sem sucesso, com estado de falha e recursos liberados |

Identificar o nível coberto pelos testes existentes: as fixtures locais de falha ICE não validam automaticamente o tratamento completo de falhas em udp_conn e QUIC.

Não apresentar esses testes como prova de recuperação automática, disponibilidade geral do serviço ou resistência a ataques.

## Medições de estabelecimento, resposta e vazão

| Medida | Definição proposta | Apresentação |
|---|---|---|
| Sucesso de estabelecimento | Túneis prontos dentro do prazo dividido pelo total de tentativas | Contagem explícita por cenário, como 30/30 |
| Tempo de estabelecimento | Início da negociação até o túnel pronto para transportar dados | Mediana e dispersão entre execuções independentes |
| Tempo de resposta | Envio da requisição até recebimento da resposta no mesmo host | Mediana e percentil 95, com tamanho da amostra |
| Correção da transferência | Comparação do conteúdo completo enviado e recebido | Bytes, mensagens completas e divergências |
| Vazão útil | Bytes de aplicação recebidos divididos pelo intervalo de transferência | Resultado por cenário, duração e condições do ensaio |

Definir os marcos exatos dos intervalos. O tempo de estabelecimento da implementação pode incluir sinalização, ICE, fase transiente, handshake QUIC e abertura das streams; não chamar esse intervalo inteiro de handshake QUIC.

Centenas de mensagens na mesma conexão não equivalem a centenas de estabelecimentos independentes. Para medições de resposta, realizar múltiplas execuções e informar tanto a quantidade de conexões quanto a de mensagens.

Comparar direto e TURN com a mesma carga e condições controladas, descrevendo as diferenças de caminho. Não atribuir automaticamente toda diferença ao protocolo ou ao relay.

A vazão útil já aparece nos objetivos específicos do manuscrito: medi-la ou revisar esse objetivo se a avaliação não a contemplar. Um ensaio de vazão deve ter duração suficiente para distinguir estabelecimento e transferência sustentada.

## Discussão e limitações

Apresentar os resultados acompanhados de sua interpretação, e não apenas como uma lista de testes aprovados. Exemplo de redação a preencher somente após obter as evidências:

> No cenário com retransmissão obrigatória, os agentes selecionaram candidatos do tipo relay em ambos os extremos. A aplicação receptora recebeu o conteúdo integral enviado, sem divergências na comparação. Ao término da sessão, os registros do servidor confirmaram a remoção das allocations TURN. Esses resultados demonstram o funcionamento do túnel TCP sobre QUIC/ICE nesse cenário.

Relacionar cada conclusão à configuração e à evidência correspondente. Não generalizar o funcionamento observado para todos os NATs, aplicações IoT, escalas ou condições de rede.

Não apresentar 0-RTT e migração QUIC/ICE como funcionalidades comprovadas: a leitura do backend atual não identificou implementação explícita desses fluxos, e o BIO trabalha com um peer IPv4 fixo. A presença dos recursos no protocolo não demonstra suporte na integração.

## Prioridade de execução

1. Recuperar os artefatos dos testes funcionais existentes e relacioná-los às versões avaliadas.
2. Consolidar evidências de estabelecimento, transferência, manutenção, falha e encerramento.
3. Acrescentar repetições e medições de estabelecimento e resposta; incluir vazão conforme os objetivos.
4. Reproduzir os cenários escolhidos no laboratório físico e registrar sua configuração.
5. Redigir a discussão e as limitações com base nos resultados efetivamente obtidos.
