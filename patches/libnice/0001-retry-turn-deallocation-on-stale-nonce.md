# libnice: remoção TURN interrompida por nonce expirado

Este documento explica o patch
[`0001-retry-turn-deallocation-on-stale-nonce.patch`](0001-retry-turn-deallocation-on-stale-nonce.patch),
aplicado pelo projeto à **libnice 0.1.21**.

## O problema

Ao encerrar uma sessão, a libnice solicita a remoção da allocation no servidor
TURN enviando uma requisição `Refresh` com `LIFETIME=0`.

Essa requisição usa autenticação de longo prazo, incluindo um nonce fornecido
pelo servidor. Quando o nonce expirou, o servidor responde `438 Stale Nonce`
com um nonce novo. O cliente deve reconstruir a requisição autenticada e
repeti-la usando o novo valor.

Na libnice 0.1.21, esse tratamento existe para a renovação normal da allocation,
mas não para sua remoção: o handler de fechamento libera o estado local ao
receber o erro, sem repetir a requisição. A callback de fechamento conclui,
embora o servidor ainda não tenha removido a allocation.

## Para que serve o nonce

O nonce é um desafio temporário gerado pelo servidor. Ele participa da
autenticação e permite ao servidor limitar a validade daquele desafio,
reduzindo a janela de reutilização de requisições autenticadas.

Não é a senha TURN nem o par ufrag/password do ICE. Sua expiração é uma
situação prevista pelo protocolo e não significa, por si só, senha incorreta.
O servidor informa um novo valor justamente para permitir outra tentativa.

## Fluxo esperado e comportamento sem o patch

```text
Cliente                                     Servidor TURN
   | Refresh: LIFETIME=0, nonce antigo           |
   |------------------------------------------>|
   |          438 Stale Nonce, nonce novo       |
   |<------------------------------------------|
   | Refresh: LIFETIME=0, autenticação refeita   |
   |------------------------------------------>|
   |             sucesso, LIFETIME=0            |
   |<------------------------------------------|
```

Sem o patch, a libnice encerra o tratamento após receber o `438`. A aplicação
pode terminar normalmente, mas a allocation fica sujeita ao prazo restante
no servidor. A conclusão da callback de `nice_agent_close_async()` não serve
como confirmação de remoção remota nesse caso.

As regras estão na [RFC 8489, seção 9.2.5](https://www.rfc-editor.org/rfc/rfc8489.html#section-9.2.5)
e na [RFC 8656, seção 8](https://www.rfc-editor.org/rfc/rfc8656.html#section-8).

## Onde estava a falha

Em `agent/conncheck.c`, a função `priv_map_reply_to_relay_remove()` chama
`stun_usage_turn_refresh_process()` e libera o `CandidateRefresh` para qualquer
resultado diferente de `STUN_USAGE_TURN_RETURN_INVALID`.

O desafio `438` produz `STUN_USAGE_TURN_RETURN_ERROR`, que também satisfaz essa
condição. Assim, uma resposta reconhecida como erro encerra o estado de remoção
antes de executar a negociação do nonce novo.

O caminho de renovação normal, `priv_map_reply_to_relay_refresh()`, já possui
tratamento específico para nonce expirado. O defeito documentado aqui está
no caminho separado de remoção.

## O que o patch altera

O patch modifica dois arquivos internos da libnice:

- `agent/discovery.h`: acrescenta `remove_nonce_retried` ao `CandidateRefresh`
  para limitar a uma repetição do desafio por remoção.
- `agent/conncheck.c`: reconhece o erro `438` no handler de remoção, exige
  NONCE e REALM presentes e não vazios, confere a capacidade do buffer, copia
  a resposta e reconstrói a requisição com o nonce novo.

A repetição reutiliza `priv_turn_allocate_refresh_tick_unlocked()`. Como o
candidato continua marcado como `disposing`, o construtor mantém `LIFETIME=0`;
ele não transforma a remoção em renovação da allocation.

O mecanismo existente controla as retransmissões da nova requisição. Um novo
desafio depois da repetição permitida, ou os demais erros, segue o cleanup
existente. Isso limita a repetição de desafios, sem garantir remoção remota
quando há outras falhas de rede ou autenticação.

O patch também inicializa o `lifetime` usado pelo log do handler: respostas
de erro não preenchem esse valor. Não altera a API pública da libnice.

## Por que não enviar a remoção diretamente pela aplicação

A libnice já envia o pedido de remoção. A etapa ausente é reagir ao `438`,
usar o novo nonce e recalcular a autenticação antes de repetir.

Fazer isso externamente exigiria enviar pela mesma associação de transporte
TURN — mesmos endpoints e protocolo — e acessar credenciais, nonce e estado
da transação. O patch mantém esse tratamento dentro da libnice, que já possui
essas informações e controla a socket.

## Como a causa e a correção foram verificadas

- Um exemplo usando somente NiceAgent e coturn reproduziu a falha, sem HAL,
  QUIC, signaling ou Patchbay. Nonce válido removia a allocation; nonce
  expirado fazia a callback concluir sem remoção no servidor.
- Um cliente TURN independente repetiu a requisição usando o nonce novo e
  obteve remoção, confirmando o comportamento esperado do coturn observado.
- A cópia modificada da libnice resolveu a reprodução. No projeto, o teste
  `ice_failure_turn_stale_nonce` exige um `438` seguido de Refresh com lifetime
  zero e remoção confirmada no log do servidor, além da limpeza local.
- O mesmo teste ligado separadamente à libnice original falhou, como esperado.
  Esse controle foi um executável em `/tmp`; o projeto continuou usando o patch.
- A suíte do projeto passou **18/18 em 33,36 s** com a dependência corrigida.

Uma primeira tentativa longa foi interrompida a pedido do usuário. Após nova
autorização, em2026-11-04, o teste completo passou **1/1 em664,11s**:67 blocos,
1097728 bytes íntegros por sentido, renovação das permissões/allocations/canais
e fechamento. Os dois pedidos de remoção receberam438, foram repetidos com
o nonce novo e resultaram em lifetime zero e remoção confirmada das duas
allocations. Logs em
[`network_quic_ice_turn_long`](../../build/long/sim/patchbay/artifacts/network_quic_ice_turn_long/).

## Aplicação no projeto e referências

O [módulo CMake](../../cmake_modules/libnice.cmake) obtém a versão fixa,
confere seu SHA-256 e aplica o patch antes de compilar com Meson/Ninja.
Os consumidores usam a biblioteca estática pelo target `Libnice::Nice`,
sem substituir a instalação do sistema.

Instruções de build e regressão: [README desta pasta](README.md).
Evidências e limites da investigação:
[relatório do nonce TURN](../../docs/analysis/libnice-turn-close-nonce-2026-11-03.md).
