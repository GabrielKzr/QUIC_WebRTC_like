# Teste local da libnice

Dois agentes no mesmo processo usam somente UDP em `127.0.0.1`, com portas
automáticas. Credenciais e candidatos são trocados em memória. Após ambos
chegarem a `READY`, cada peer envia um datagrama e verifica o conteúdo recebido.
O programa retorna erro se houver falha ou se o prazo de 5 segundos expirar.

```sh
cmake -S . -B build/nice -DBUILD_TESTING=ON -DBUILD_NICE_TESTS=ON -DBUILD_NETWORK_TESTS=OFF
cmake --build build/nice --target test_nice_local -j2
ctest --test-dir build/nice -R '^nice_local$' --output-on-failure -V
```

Executar os comandos na raiz do repositório. O teste usa diretamente a libnice,
sem Patchbay, STUN/TURN externos ou integração com `ice_udp_transport`/QUIC.
Este cenário valida apenas o caminho local, sem NAT.

## Teste da interface ice_udp_transport através de NAT

`test_ice_udp_transport` é executado pelo Patchbay, com um peer em cada
namespace, atrás dos dois NATs IPv4 existentes. O `.meta.cmake` associa
`isp.sh` ao hook `PATCHBAY_ISP_SCRIPT`, iniciando coturn em modo STUN-only
(UDP/3478) e `signaling.py` (TCP/5555) no ISP. Requer `turnserver` do coturn
e `python3` no PATH, além das dependências do runner Patchbay.

O runner fornece `PATCHBAY_ISP_ADDRESS` e `PATCHBAY_LOCAL_ADDRESS` a partir
dos objetos da topologia. Cada execução representa um peer, sem fork. A
configuração restringe a coleta ao IPv4 do peer para evitar que uma conexão
IPv6 direta contorne o NAT testado. A callback troca SDPs com o ISP por TCP,
com timeout de 3 segundos. As sockets UDP continuam sob controle da libnice.

Cada peer usa `open`, `wait`, `send`, `recv` e `close` da interface pública.
O teste exige candidato local server-reflexive descoberto por STUN, signaling
concluído, estado `READY`, mensagem correta nos dois sentidos, candidato remoto
selecionado server-reflexive ou peer-reflexive e liberação do agente/contexto
no fechamento. O par selecionado é registrado no stdout de cada peer. O
progresso tem prazo de 5 segundos e o CTest limita a execução a 20 segundos.

```sh
cmake --preset all
cmake --build --preset all -j2
ctest --test-dir build/all -R '^network_ice_udp_transport$' --output-on-failure -V
```

O cenário é registrado quando `BUILD_NICE_TESTS`, `BUILD_TESTING` e
`BUILD_NETWORK_TESTS` estão habilitados. Sem testes de rede, o executável pode
ser compilado, mas esse cenário não é registrado. Não testa CHOWNAT/QUIC
sobre ICE nem usa TURN; o exemplo `nice_local` continua independente.

## Serviços do ISP

O signaling verifica uma resposta STUN local antes de anunciar `READY`. Cada
par de conexões TCP envia uma mensagem com prefixo de 4 bytes de tamanho em
ordem de rede, seguido do SDP (1 a 65536 bytes). O servidor troca os SDPs entre
as duas conexões e aguarda o próximo par. Leituras dos clientes têm timeout
de 5 segundos. O canal serve somente à negociação ICE, sem carregar os dados
do túnel CHOWNAT/QUIC ou substituir echo/SSH.

O script permanece em primeiro plano e falha se algum serviço sair. Os dois
serviços herdam o grupo de processos encerrado pelo runner ao terminar.

## Próximas validações

O cenário atual comprova ICE/STUN no NAT nativo do Patchbay. Ainda faltam, nesta
ordem: remapeamento de porta comprovado, TURN forçado, CHOWNAT sobre ICE, QUIC
sobre ICE, falhas/manutenção/limpeza e reprodução final no GNS3.
Critérios de aceite e estado estão no
[plano de integração](../../../docs/plans/ice-quic-integration.md).
