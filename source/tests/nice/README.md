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
O configure do CMake exige esses executáveis quando o cenário de rede está
habilitado e falha indicando as dependências ausentes. Para instalação local,
inclua `~/.local/bin` no PATH do processo que configura e executa os testes.

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

## ICE com remapeamento de porta

`network_ice_udp_transport_remap` reutiliza o mesmo fonte em outro target.
As regras ficam em `test_ice_udp_transport_remap/test.meta.cmake`: a porta
UDP pública é `porta privada XOR 0x4000`, com mapa DNAT indexado pela porta
traduzida. O SNAT TCP para o ISP mantém o signaling acessível. O cenário
original continua usando NAT nativo, sem essa lista de regras.

Além dos critérios anteriores, cada peer exige candidato local selecionado
server-reflexive, com porta pública diferente da porta base privada, e imprime
os dois endpoints. Preservar a porta reprova o teste, mesmo com dados corretos.
A regra usa o intervalo efêmero Linux do laboratório (32768–60999), evitando
traduzir para porta zero. Não é um alocador NAT geral para múltiplos hosts.

```sh
cmake --preset all
cmake --build --preset all --target test_ice_udp_transport_remap -j2
ctest --preset all -R '^network_ice_udp_transport_remap$'
```

## TURN obrigatório

`network_ice_udp_transport_turn` reutiliza o fonte dos peers em outro target,
com configuração TURN UDP e `force_relay=TRUE`. Essa opção da implementação
ICE requer servidor TURN e é aplicada antes da coleta. O teste exige candidatos
locais e remotos selecionados do tipo relay, além de READY e dados corretos
nos dois sentidos. Falta de relay reprova o cenário, sem fallback direto.

O `.meta.cmake` associa um `isp.sh` próprio: coturn com autenticação de longo
prazo, realm `ice-test`, usuário `ice-test`, senha de laboratório
`ice-test-password` e portas relay49160–49200. Usa NAT nativo; não modifica as
regras dos cenários anteriores. Credenciais servem apenas ao teste isolado.

Após fechar a HAL e liberar agente/contexto, cada peer informa `CLOSED` pelo
signaling. Nesse modo, `signaling.py --turn-log` consulta o stdout do coturn
(`isp.stdout.log`) e exige lifetime zero no Refresh e fechamento das sessões
de todas as allocations criadas, no mínimo duas. Só então responde `OK` aos
peers e registra `TURN_RELEASED allocations=2`. Ausência dessa confirmação
reprova o teste; encerrar coturn na limpeza do runner não aprova allocations.
Essa checagem usa o formato de log do coturn4.6.3 validado nesta máquina.

```sh
cmake --preset all
cmake --build --preset all --target test_ice_udp_transport_turn -j2
ctest --preset all -R '^network_ice_udp_transport_turn$'
```

## Túnel TCP CHOWNAT sobre ICE

`test_chownat_ice` segue o fluxo de `test_chownat`, usando `ice_ops` e a
configuração ICE no lugar do transporte direto. Aguarda gathering, signaling
e READY, exige par remoto reflexivo e executa `udp_connection()` com
`chownat_api` original. A troca01→03→03 é mantida como handshake CHOWNAT sobre
o caminho já negociado pelo ICE. Não há alteração nos backends ou no runner.

`network_chownat_ice_tcp` usa STUN no ISP e NAT nativo. A variante
`network_chownat_ice_turn_tcp` recompila o mesmo fonte com TURN obrigatório,
exigindo par relay/relay e confirmação de liberação das allocations pelo
servidor, como no teste TURN de datagramas. Os dois `.meta.cmake` configuram
`PATCHBAY_MODE tcp` e reutilizam os serviços ISP existentes.

Antes do handshake, cada peer chama o envio KA original do CHOWNAT e confere
recepção do datagrama de um byte zero do outro peer. Isso valida a passagem
do controle por ICE/TURN; não mede agendamento por inatividade ou manutenção
prolongada de NAT. Os novos testes usam cinco tentativas para cobrir a regressão
de timeout antecipado; os exemplos diretos existentes continuam com vinte.

As esperas de controle ficam em `chownat_recv_control()`: prazo monotônico
calculado uma vez, `udp_transport_wait()` com o restante e `recv()` com timeout
zero quando há payload. Controles internos ICE e keep-alives curtos não
consomem tentativas nem reiniciam o prazo. A função é usada no handshake e
fechamento dos dois modos; `ice_recv()` permanece com sua implementação atual.

O runner exige16KiB binários íntegros e EOF no echo TCP. Cada peer exige um
CONNECTED,16KiB enviados e recebidos, nenhum LOST_DATA, sessão fechada e
deinit sem erro, com descritores TCP invalidados e ICE liberado. Erros são
propagados no código de saída. O runner encerrar peers por timeout é falha.

```sh
cmake --preset all
cmake --build --preset all --target test_chownat_ice test_chownat_ice_turn -j2
ctest --preset all -R '^network_chownat_ice(_turn)?_tcp$'
```

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

Os cenários comprovam ICE/STUN no NAT nativo, com remapeamento controlado de
porta, via TURN obrigatório e com túnel TCP CHOWNAT no Patchbay. Ainda faltam,
nesta ordem: QUIC sobre ICE, falhas/manutenção/limpeza e reprodução final no GNS3.
Critérios de aceite e estado estão no
[plano de integração](../../../docs/plans/ice-quic-integration.md).
