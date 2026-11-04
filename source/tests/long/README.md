# Sessão prolongada QUIC sobre ICE/TURN

Este teste não é compilado nem registrado pelo preset `all`.
Execução explícita:

```sh
cmake --preset long
cmake --build --preset long -j4
ctest --preset long
```

Requer as mesmas dependências e permissões de namespaces dos testes Patchbay.
`BUILD_LONG_TESTS` é OFF por padrão. O preset `long` usa build/long e filtra
somente o label long; os testes curtos continuam em build/all.

O cenário exige relay/relay, mantém a mesma conexão QUIC/TCP por pelo menos
11 minutos e troca 67 blocos binários de 16 KiB nos dois sentidos, com pausas
de 10 segundos entre blocos. Conteúdo deve ser idêntico: keep-alives não podem
aparecer como bytes adicionais no túnel. A libnice continua processando eventos
durante as pausas por meio de udp_connection/udp_transport_wait.

No servidor TURN, exigir Refresh com lifetime positivo para cada allocation,
renovação de CHANNEL_BIND (pelo menos dois sucessos por allocation), e depois
Refresh lifetime=0 e remoção de todas as allocations. O fechamento QUIC deve
ocorrer antes da liberação ICE. Limite do probe: 710 s; CTest: 740 s.
Logs ficam em build/long/sim/patchbay/artifacts/network_quic_ice_turn_long.

O teste não faz reconexão, restart ICE nem simula perda/alteração de NAT.

## Estado da validação

Com streams QUIC separados de dados e controle, os 67 blocos (1097728 bytes
por sentido) passaram íntegros durante 11 minutos. Logs confirmaram renovação
de permissões, allocations e canais TURN. Os quatro testes QUIC curtos passam.

Antes do patch, o teste prolongado completo falhou no fechamento: Coturn respondeu
`438 Stale nonce` aos pedidos de remoção após a sessão prolongada, e a liberação
das allocations não foi confirmada. Essa falha não foi contornada no teste.
O patch da libnice agora é aplicado pelo build e a regressão curta de nonce
passou, assim como a suíte all18/18. A nova tentativa longa foi interrompida
a pedido do usuário; não existe aprovação longa completa após o patch.
Não repetir esse teste sem novo pedido. Estado detalhado na
[memória da integração](../../../docs/memory/libnice-integration.md).
O prazo de espera por CLOSED no signaling é 720 s somente neste cenário;
os testes curtos preservam o prazo anterior.
