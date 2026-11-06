# Regressões com Patchbay

Runner Rust/Patchbay 0.7.0 que cria peers e roteadores em namespaces Linux,
executa os programas C e verifica comunicação real. Contexto, correções e
pendências estão em [docs/repository-context.md](../../docs/repository-context.md).

## Preparação e execução

Requer Linux, Rust/Cargo estável, `ip`, `nft`, `tc`, `stdbuf` e OpenSSL
com as APIs QUIC usadas no projeto. Instalação Rust: https://rustup.rs/.
CMake procura Cargo também em `~/.cargo/bin` e compila com `cargo build --locked`.
O preset exige CMake >= 3.21. Na raiz:

```sh
cmake --preset all
cmake --build --preset all -j2
ctest --preset all
```

Para detalhar ou selecionar testes:

```sh
ctest --preset all -V
ctest --preset all -L openssl_quic
ctest --preset all -L chownat
ctest --preset all -L baseline
ctest --preset all -L program
```

CTest não recompila os binários. Sem presets, continua disponível:

```sh
cmake -S . -B build -DBUILD_NETWORK_TESTS=ON
cmake --build build -j2
ctest --test-dir build -L network --output-on-failure
```

Sem testes de rede, o build C não precisa de Rust. O build direto por `source/`
não registra esta suíte; use pasta nova ao mudar a raiz de configuração CMake.

Os executáveis são criados por `add_test_class()` em `source/tests/`. Cada
`test.meta.cmake` declara propriedades do target:

```cmake
set_target_properties(test_chownat_udp PROPERTIES
    PATCHBAY_MODE udp
    PATCHBAY_TIMEOUT 20
    PATCHBAY_TEST_NAME network_chownat_udp
    PATCHBAY_LABELS "network;chownat;baseline"
)
```

`PATCHBAY_MODE` habilita o registro e aceita `tcp`/`udp`; `PATCHBAY_TIMEOUT` é
obrigatório e positivo (segundos). Nome e labels são opcionais. O registro em
`sim/patchbay/program_tests.cmake` percorre os targets dos diretórios de `source/`
já configurados, inclusive subdiretórios; executáveis sem essas propriedades
não são executados pelo Patchbay. Não há lista manual de protocolos.

Cada `.meta.cmake` também pode declarar `set(PATCHBAY_NAT_EXTRA_RULES ...)`,
com uma instrução nftables por elemento. O helper guarda a lista no target;
o registro gera `<target>.nat.nft` no build e passa `--nat-rules <arquivo>`
ao runner. As regras são aplicadas em ordem nos dois roteadores NAT, depois
da configuração nativa. Lista ausente ou vazia mantém o NAT nativo.

Marcadores são substituídos pelo runner: `@PUBLIC_IP@` é o IPv4 público do
roteador atual, `@WAN@` é sua interface WAN e `@ISP_IP@` é o gateway downstream
do ISP. Dentro de uma regra, escreva `\;` para preservar ponto e vírgula na
lista CMake e `\"` para aspas. O arquivo gerado contém a sintaxe nft normal.
Veja a lista completa de mapeamento espelhado em
[test_chownat_udp/test.meta.cmake](../../source/tests/WebRTC/test_chownat_udp/test.meta.cmake).
Os cinco testes diretos declaram essa lista. O teste ICE original não declara
regras; `test_ice_udp_transport_remap/test.meta.cmake` declara a tradução de
porta UDP por XOR e o mapa de retorno, além do SNAT TCP para o signaling.

O CMake do Patchbay carrega esse registro após criar o runner. A raiz apenas
adiciona `source/` e `sim/patchbay/`. Executor e verificação Rust ficam juntos
em `sim/patchbay/src/`; `main.rs` usa `mod patchbay`. A sondagem antiga do
ambiente foi removida; a suíte ativa executa os testes dos programas.

## Topologia e testes

A rede é definida uma única vez em `src/topology.rs`, na função `build()`:

```text
peer_a 192.168.80.20/24 -- router-a -- ISP LAN 10.0.0.0/24 -- router-b -- peer_b 192.168.80.30/24
```

Sub-redes dos roteadores são definidas por `downstream_cidr`; os IPs dos peers,
por `IfaceConfig::routed(router.id()).addr("IP/prefixo".parse()?)` no próprio
`add_device`. Edite ali os nós, conexões e endereços. Os testes consultam os
objetos criados para obter os IPs, sem repetir endereços ou usar `ip addr add`.
Os IPs públicos são alocados pelo Patchbay e consultados com `uplink_ip()`.
Cada execução cria uma instância isolada dessa mesma topologia.

`topology::build(&Program { ... })` recebe o caminho do executável, diretório
de trabalho/logs e a seleção dos argumentos dos peers. Os comandos de cliente
e servidor são declarados na topologia; o serviço echo não faz parte dela.
`Topology` contém uma coleção de devices por nome, roteadores e processos;
adicionar um nó não exige criar um campo fixo na struct nem editar a main.

`main.rs` recebe o tipo de verificação (`tcp`, `udp`) e coordena a
execução. Os requisitos específicos do teste, o serviço echo e a verificação
ficam em `sim/patchbay/src/patchbay.rs`. O teste atual requer os devices `peer_a`
e `peer_b`; um teste que use outro conjunto de nós deve declarar seus próprios
 requisitos, sem alterar a main. A CLI continua:

```text
runner <peer-binary> <test-kind> <source-dir> <artifacts-dir> [isp-script] [--nat-rules <file>]
```

### Script opcional no ISP

Um teste pode declarar um script próprio para iniciar serviços no namespace do
`isp`, antes dos peers, usando a propriedade opcional do target:

```cmake
PATCHBAY_ISP_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/isp.sh"
```

O caminho deve ser absoluto e apontar para um arquivo existente. O CMake o
repassa como quinto argumento opcional do runner, após o diretório de artefatos.
Testes sem essa propriedade mantêm a execução anterior.

Para testes com hook, os peers recebem `PATCHBAY_ISP_ADDRESS` (gateway da
rede downstream do ISP) e `PATCHBAY_LOCAL_ADDRESS` (IPv4 do próprio peer).
Esses valores vêm dos objetos da topologia. O hook usa o NAT configurado para
o teste: se a lista substituir o postrouting, ela também deve fornecer o SNAT
TCP necessário ao signaling. Sem regras extras, TCP e UDP usam o NAT nativo.

O runner executa `sh <script>` com diretório de trabalho `source/`, stdin fechado
e logs em `isp.stdout.log`/`isp.stderr.log`. O script deve imprimir uma linha
exatamente `READY` depois de confirmar que seus serviços estão prontos. Há um
limite de 5 segundos para esse anúncio; saída antecipada ou timeout reprova o
teste antes de iniciar os peers.

O script deve permanecer em primeiro plano e manter os serviços no mesmo grupo
de processos, sem daemonização ou `setsid`. Ele é acompanhado separadamente dos
peers; sua saída durante a espera pelos peers reprova o teste. No encerramento,
o runner envia SIGKILL ao grupo inteiro e aguarda o script. A limpeza do grupo
também ocorre em falhas de preparação, sem depender de traps do shell.

| Testes | Verificação |
| --- | --- |
| `network_chownat_tcp`, `network_openssl_quic_tcp` | Echo TCP binário de 16 KiB |
| `network_chownat_udp` | Aguarda ambos os peers terminarem com saída zero |
| `network_chownat_udp_close` | Executável UDP de encerramento, também exige saída zero |
| `network_openssl_quic_udp` | Executável QUIC sem túnel, também exige saída zero |

Todos usam a topologia com dois NATs; foram removidas as variantes antigas
`public`, `nat` e `gns3`. Não há teste sem NAT nesta configuração atual.
O laboratório manual em `sim/gns3/` permanece separado.

As duas LANs usam `192.168.80.0/24`, isoladas pelos roteadores NAT. Endereços
WAN de A/B são alocados automaticamente na rede do ISP e consultados ao iniciar
os peers. peer_a executa o papel cliente (-c), peer_b o servidor (-s) dos
programas atuais. Isso não define um serviço no ISP. A imagem topology.png
anterior ainda mostra a rede antiga; a definição atual é topology.rs.
Validação desta rede em 2026-10-12: build all e suíte5/5 em6,71s.

O echo escuta em TCP 1234; a entrada TCP cliente é 44444 e o hole punching usa
UDP 2222. Essas portas pertencem à execução dos programas, não aos IPs da rede.
O runner inicia ambos os peers e conecta à entrada TCP cliente.
A base usa `Nat::Moderate`. As listas dos testes diretos substituem o
postrouting, criam o mapa de retorno no envio, mantêm a porta pública igual
à privada e usam `inbound_guard`/`opened_ports` para bloquear portas ainda
não abertas. A filtragem nativa de encaminhamento permanece. Essas regras
ficam nos `.meta.cmake`, sem bloco fixo de mapeamento espelhado no Rust.
No ICE, a lista é ausente e o NAT nativo escolhe a porta externa. Isso não
força remapeamento: a porta pode ser preservada quando disponível.

CHOWNAT/QUIC diretos usam endpoints externos conhecidos. O cenário
`network_ice_udp_transport` descobre candidatos com STUN no ISP e negocia ICE.
Vários hosts disputando a mesma porta pública e expiração/renovação do mapa
não estão cobertos por esse cenário simples.

## Contrato de sucesso

UDP: os executáveis CHOWNAT e QUIC sem túnel fazem três rodadas request/response,
verificando conteúdo e sequência; o cliente inicia o disconnect ao concluir.
Ambos os peers retornam zero apenas com troca completa e sessão encerrada.
QUIC usa mensagens delimitadas por newline e acumula bytes para suportar
leituras fragmentadas/agrupadas do stream. O runner aguarda os códigos de saída;
não procura mensagens nos logs.

TCP: além da comparação integral do payload, o probe deve concluir, ambos os
peers devem terminar com saída zero e o echo deve receber EOF e retornar sucesso.
Depois da resposta, o probe fecha a escrita TCP. O runner aguarda peers e echo
concorrentemente. Saída não zero, sinal, erro de tarefa ou timeout fazem o teste
falhar; processos restantes são encerrados como limpeza, nunca como sucesso.
A sondagem de transferência mantém seu limite de 55 s.

## Logs, limites e ambiente

Os cinco testes rodam em série. Túneis de 16 KiB têm limite de transferência de
55 s para transferência e mais 10 s para encerramento de peers/echo; CTest usa
75 s. UDP aguarda os peers por 10 s e CTest usa 20 s. Esses limites não são a
duração esperada dos testes.
O runner encerra e aguarda os peers e libera o laboratório.

Artefatos ficam em `build/all/sim/patchbay/artifacts/<teste>/` com o preset,
ou em `build/sim/patchbay/artifacts/<teste>/` no fluxo manual acima. Incluem
`topology.txt`, `test.txt`, resultado e stdout/stderr dos peers, além das regras
nft de cada roteador. O diretório de trabalho dos peers é `source/` para
localizar os certificados QUIC.

Para habilitar capturas de pacotes:

```sh
PATCHBAY_PCAP=1 ctest --preset all
```

A existência de `PATCHBAY_PCAP` habilita a captura, inclusive com valor vazio ou
`0`. Remova a variável para desabilitar. Sem ela, tcpdump não é executado nem
exigido; a inspeção nft continua obrigatória.

Cada interface dos peers e roteadores do laboratório (incluindo ISP, bridges e
loopback) gera `pcap/<nó>/<interface>.pcap` dentro dos artefatos do teste.
Não há filtro de protocolo nem truncamento dos pacotes. As capturas começam
antes dos serviços ISP e dos peers, abrangendo estabelecimento da conexão,
dados e encerramento. Interfaces sem tráfego geram arquivos apenas com cabeçalho.
Cada nó também recebe `interfaces.txt` e `addresses.txt`; o stderr fica em
`<interface>.tcpdump.log`. Os arquivos podem ser abertos no Wireshark.

O runner encerra tcpdump com SIGINT e aguarda a gravação dos arquivos.
`capture-status.txt` registra DISABLED, COMPLETE, FAILED ou ABORTED. Com captura
habilitada, falha ao iniciar, saída prematura ou falha na finalização faz o teste
falhar. Ao habilitar novamente, o subdiretório `pcap/` é substituído; execuções
sem a variável preservam capturas anteriores, identificadas pelo status DISABLED.

User namespaces devem ser permitidos pelo kernel e pela política do ambiente.
O runner não usa sudo; em sandbox restrito pode ser necessário executar fora
dele. Não desativar globalmente AppArmor nem alterar sysctls para essa finalidade.
Com `-DBUILD_PCAP_TOOLS=ON`, o CMake prepara tcpdump/libpcap e configura o PATH
dos testes automaticamente; veja [dependências do build](../../docs/build-dependencies.md).
Sem essa opção, o tcpdump encontrado no PATH precisa funcionar nesses namespaces.
Nesta máquina, o tcpdump do sistema tenta trocar para o usuário `tcpdump` e
falha com `Operation not permitted`. Foi instalado em `~/.local/bin/tcpdump`
o upstream 4.99.5, compilado com `WITH_USER` vazio, `WITH_CAP_NG=OFF` e
`WITH_CAPSICUM=OFF`; mantenha `~/.local/bin` antes dos diretórios do sistema
no PATH ao habilitar capturas. Isso não altera a política do host.

O script `legacy/netns_virtual_env.sh` permanece apenas como referência manual.
A suíte verifica integridade e conclusão; cobertura de falhas, desempenho e
encerramento ainda precisa ser ampliada. Última validação em 2026-10-11:
Após tornar também o exemplo QUIC UDP finito, **5/5 passaram em 13,37 s**:
QUIC UDP 1,11 s; CHOWNAT TCP/UDP/UDP close também passaram. QUIC TCP passou em
6,24 s nesta execução, embora tenha passado em ~1,3 s antes com o mesmo helper:
a instrumentação posterior localizou a espera no helper de shutdown: eventos
concluem o fechamento após a última consulta, mas poll aguarda o restante do
limite de5s sem timer pendente. Reordenação foi aplicada pelo usuário e a espera trocada para select
em2026-10-12; build eQUIC2/2 passaram em2,58s. Detalhes no contexto. Não atribuir apenas a build antigo.
Payload inválido em cópia temporária do teste QUIC UDP produziu falha no runner;
harness temporário confirmou mensagens divididas, agrupadas e sequência inválida.

O backend QUIC conclui shutdown usando socket/timers OpenSSL e limite de 5 s,
reconhece fechamento remoto e mantém erros separados do estado fechado. Deinit
libera recursos mesmo em falha; o teste C propaga erros no código de saída.
Fixtures temporárias verificaram erro remoto7 e shutdown sem progresso: ambos
foram rejeitados pelo runner. As falhas anteriores foram superadas nos casos
validados; detalhes e limites estão no contexto do repositório.
