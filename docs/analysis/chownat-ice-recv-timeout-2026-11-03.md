# Diagnóstico: tentativas CHOWNAT esgotadas por espera ICE antecipada

Verificado em 2026-11-03, a pedido do usuário. Resultado: a hipótese de retorno
antecipado está confirmada. A investigação abaixo foi feita em cópias; depois
o usuário escolheu preservar o prazo no chamador e autorizou aplicar a função
privada `chownat_recv_control()` no protocolo, em vez de modificar ice_recv.
O protótipo anterior em ice_recv fica preservado apenas como experimento.

## Causa observada

`ice_recv()` chama `ice_wait()` uma única vez. A espera usa o menor prazo
GLib/chamador e pode terminar após consumir um controle ou timer, sem payload.
Quando não há `UDP_TRANSPORT_READABLE`, recv retorna `NOT_READY` imediatamente.
Isso faz a recepção síncrona entregar o fim de um timer interno como ausência
de dados ao CHOWNAT antes de esgotar o prazo de recepção solicitado.

`chownat_hole_punching()` pede 1 segundo por recepção, mas conta cada retorno
diferente de OK como tentativa. No servidor, também envia KA após esse retorno.
O laço termina rápido, e os KAs acumulados podem consumir as tentativas do
cliente como respostas diferentes de03. `udp_transport_wait()` continuar
retornando após um evento interno é coerente com seu contrato; o problema
investigado é repassar isso diretamente pela recepção com timeout.

## Evidência do handshake real com TURN

Instrumentação em cópias temporárias registrou clocks monotônicos, retorno,
tamanho/conteúdo dos controles, timeout pedido e resultado de `g_poll`.
GNU ld `--wrap` observou as chamadas públicas; cópia de ice_wait registrou
timeout GLib, prontidão, descritores e payload pendente. Sem threads novas.

| Variante | Resultado | Evidência do servidor |
| --- | --- | --- |
| Atual,5 tentativas,execução1 | Falha explícita | Cinco NOT_READY em79,993ms |
| Atual,5 tentativas,execução2 | Falha explícita | Cinco NOT_READY em80,095ms |
| Atual,20 tentativas | Passa | Oito NOT_READY antes de completar em150,354ms |
| Cópia com prazo total,5 tentativas | Passa3/3 | Handshake152,636/153,427/194,371ms |

Na primeira falha, o servidor pediu1000000us em cada recv, mas esperou
19125,1115,19102,20103 e20121us. Todos retornaram NOT_READY, zero bytes.
O trace da primeira chamada registra timeout de poll19ms, retorno0,
nenhum evento público e nenhum payload pendente. A fonte exata desses timers
GLib não foi identificada; não atribuir cada um a um controle TURN específico.

Servidor entrou às6827041054658us e encerrou às6827041134651us. Cliente entrou
às6827041054605us, mas estava no accept TCP; enviou o primeiro01 somente às
6827041203285us, cerca de149ms após entrar. Portanto o servidor já havia
desistido antes da primeira sondagem do cliente. O cliente recebeu cinco KAs
de byte00 em poucos microssegundos e também encerrou com status-5/CONNECTED0.
Runner retornou erro por reset/saída de peer; watchdog não foi usado como aceite.

## Medição independente de recv sem dados

Dois peers ICE/TURN reais trocaram payloads, chegaram a READY e pediram
recepção ociosa de250ms, sem novos datagramas de aplicação:

| Implementação | Peer A | Peer B | Retorno |
| --- | --- | --- | --- |
| Atual |19,148ms |19,119ms |NOT_READY,zero bytes |
| Protótipo com prazo total |250,261ms |250,693ms |NOT_READY,zero bytes |

As duas fixtures terminaram com dados/fechamento e allocations liberadas.
Uma chamada inicial da fixture corrigida falhou antes dos peers por usar o
mesmo caminho para binário/diretório; nomes corrigidos na execução final.

## Protótipo testado, sem aplicação

Esta seção descreve o protótipo histórico em ice_recv. A aplicação escolhida
pelo usuário foi posterior e fica no CHOWNAT, conforme seção seguinte.

A única alteração de comportamento da cópia é em ice_recv: repetir esperas
internas usando o restante de um prazo monotônico total, até payload, erro
ou prazo esgotado. Timeout zero continua uma consulta. A espera pública não
foi alterada, nem o protocolo CHOWNAT. Essa cópia manteve5 tentativas e passou
o túnel TCP completo3/3:2,730/2,862/2,877s no runner, com16KiB/EOF/saídas zero
e confirmação de liberação das duas allocations TURN.

Isso isola o retorno antecipado como causa da falha reproduzida.20 tentativas
contornam a janela observada, sem corrigir o prazo da recepção. Perda/reordenação,
timeout NULL, timeout muito grande e manutenção prolongada não foram validados
adicionalmente no protótipo; patch não deve ser descrito como integrado.

## Aplicação escolhida: prazo no CHOWNAT

Após revisar o esboço, usuário exigiu que a espera ficasse no chamador e
autorizou uma função privada reutilizável. Aplicada chownat_recv_control nas
cinco leituras de controle: espera pelo03 do cliente, pelo01/03 do servidor
e nas duas rotinas de fechamento. A função mantém deadline monotônico para
o prazo positivo já fornecido por essas chamadas, repete wait com o restante
e só chama recv com timeout zero após prontidão. Datagramas menores que3
bytes não encerram a tentativa nem reiniciam o prazo. Ice_recv, espera pública
e recepção comum de payload permanecem como estavam.

Os dois testes CHOWNAT/ICE passaram a usar cinco tentativas como regressão.
Um harness temporário compilou o fonte ativo e injetou despertares a cada5ms:
timeout200ms terminou em~200ms sem dados e também sob KAs contínuos; após
dois recv NOT_READY, controle03 foi aceito em~83ms; erros de wait e recv foram
propagados. Isso verifica a lógica de prazo, sem representar uma rede real.

Na primeira repetição dos testes reais, a terceira execução TURN falhou antes
do handshake porque um peer não chegou a ICE READY. O outro enviou KA e
aguardou resposta. A fixture deixava payload pendente sem consumo enquanto
esperava READY; como ice_read_pending preserva um único datagrama, isso pode
interromper progresso dos controles. Corrigida a fixture para consumir e
registrar o KA antecipado e reaproveitá-lo na checagem posterior. Essa falha
levou o runner a reprovar por55s, sem ser usada como sucesso. Logs anteriores
em /tmp/chownat-control-deadline-ice-ready-failure/artifacts.

Após esse ajuste, as seis execuções reais de CHOWNAT/ICE direto/TURN com cinco
tentativas passaram em17,08s, com16KiB íntegros/EOF e allocations liberadas.
Teste direto registrou KA recebido durante a espera por READY. Não foram
alteradas política de posse, timers do ICE nem topologia para esse resultado.
Build all, Wall/Wextra/Werror e diff-check passaram. Suíte completa12/12 em
26,50s, incluindo regressões CHOWNAT direto/UDP/close e QUIC existentes.
Harness, snapshot do CHOWNAT, logs finais e falha anterior ao ajuste da fixture
preservados no subdiretório applied-chownat-helper do experimento local.

## Artefatos e preservação

Fontes de instrumentação, scripts, patch revisável e logs preservados em
[experimento local](../experiments/chownat-ice-recv-timeout-2026-11-03/).
Essa pasta é ignorada pelo Git. `recv-deadline.patch` contém somente a proposta
para ice_recv; `summary.json` e `results.json` registram medições e saídas.
`build.py`/`build-fixed.py` pressupõem o build atual do repo e o diretório
`/tmp/chownat-ice-timeout-analysis`; os fontes preservados permitem revisão
sem depender dos binários temporários. Ambiente: libnice0.1.24, GLib2.56.4,
coturn4.6.3, OpenSSL4.1.0 local; namespaces fora do sandbox, mesma topologia
e mesmos serviços ISP dos testes ativos, sem alterar regras NAT.

SHA-256 confirmou76 arquivos rastreados de source/runner inalterados;
o teste novo não rastreado também permaneceu byte a byte igual ao snapshot
inicial com20 tentativas. Não foram alterados fontes/build/limites ativos.
