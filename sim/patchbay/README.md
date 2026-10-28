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
runner <peer-binary> <test-kind> <source-dir> <artifacts-dir>
```

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
A base usa o preset NAT Moderate com APDF nativo e regras UDP próprias:
mapa inicialmente vazio, SNAT pela porta de origem e entrada não aberta
descartada antes do conntrack por `inbound_guard`/`opened_ports`. Sem zonas.
A guarda foi restaurada a pedido em 2026-10-12, sem nova validação. Esta base
passou anteriormente 5/5 mais três repetições por teste; sua remoção falhou5/5.
Endpoints externos conhecidos; ICE e STUN não estão implementados.

É uma política específica do laboratório, não uma reprodução universal de NAT.
Vários hosts disputando a mesma porta pública e expiração/renovação do mapa
não estão cobertos; as entradas dinâmicas têm timeout de 300 s.

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
`topology.txt`, `test.txt`, resultado e stdout/stderr dos peers; também salva regras nft e capturas
`router-{a,b}.udp.log`. São sobrescritos a cada execução.
O diretório de trabalho dos peers é `source/` para localizar os certificados QUIC.

User namespaces devem ser permitidos pelo kernel e pela política do ambiente.
O runner não usa sudo; em sandbox restrito pode ser necessário executar fora
dele. Não desativar globalmente AppArmor nem alterar sysctls para essa finalidade.
O tcpdump é opcional e usa `-Z root` dentro do user namespace, sem exigir root
no host. Falhas ao iniciar a captura geram aviso e são registradas em
`capture-status.txt`, sem impedir as verificações dos protocolos. Os arquivos
`*.tcpdump.log` guardam o stderr das capturas. A inspeção das regras nft e as
verificações de dados e encerramento continuam obrigatórias.

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
