# Regressões com Patchbay

Runner Rust/Patchbay 0.7.0 que cria peers e roteadores em namespaces Linux,
executa os programas C e verifica comunicação real. Contexto, correções e
pendências estão em [docs/repository-context.md](../../docs/repository-context.md).

## Preparação e execução

Requer Linux, Rust/Cargo estável, `ip`, `nft`, `tc`, `tcpdump`, `stdbuf` e OpenSSL
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
```

CTest não recompila os binários. Sem presets, continua disponível:

```sh
cmake -S . -B build -DBUILD_NETWORK_TESTS=ON
cmake --build build -j2
ctest --test-dir build -L network --output-on-failure
```

Sem testes de rede, o build C não precisa de Rust. O build direto por `source/`
não registra esta suíte; use pasta nova ao mudar a raiz de configuração CMake.

## Cenários registrados

| Testes | Verificação |
| --- | --- |
| `network_{chownat,openssl_quic}_public` | Echo TCP binário de 16 KiB, rede roteada sem NAT |
| `network_{chownat,openssl_quic}_nat` | Mesmo echo, dois NATs com tradução fixa UDP 2222 |
| `network_environment_nat` | UDP independente dos protocolos, verifica o laboratório |
| `network_chownat_gns3` | Echo de 14 bytes, cliente 10.0.0.20 e servidor 10.0.0.30 |
| `network_chownat_udp-gns3`, `network_chownat_udp-nat` | Mensagens UDP recebidas nos dois sentidos |

O echo escuta em TCP 1234; a entrada TCP cliente é 44444 e o hole punching usa
UDP 2222. O runner inicia ambos os peers e conecta à entrada TCP cliente.
O NAT é Open, com firewall permissivo, mapa público:2222 ↔ privado:2222
pré-instalado e SNAT fixo. Endpoints externos são informados diretamente;
ICE, STUN e tradução dinâmica não fazem parte da suíte atual. `gns3` é uma
referência de endpoints diretos, sem reprodução dos roteadores Cisco.

## Logs, limites e ambiente

Os oito testes rodam em série. Túneis public/nat têm limite de transferência de
55 s e CTest de 75 s; cenários baseline usam limite CTest de 20 s, assim como
a verificação de ambiente. Esses limites não são a duração esperada dos testes.
O runner encerra e aguarda os peers e libera o laboratório.

Artefatos ficam em `build/all/sim/patchbay/artifacts/<teste>/` com o preset,
ou em `build/sim/patchbay/artifacts/<teste>/` no fluxo manual acima. Incluem
resultado e stdout/stderr dos peers; NAT também salva regras nft e capturas
`router-{a,b}.udp.log`. São sobrescritos a cada execução.
O diretório de trabalho dos peers é `source/` para localizar os certificados QUIC.

User namespaces devem ser permitidos pelo kernel e pela política do ambiente.
O runner não usa sudo; em sandbox restrito pode ser necessário executar fora
dele. Não desativar globalmente AppArmor nem alterar sysctls para essa finalidade.
O tcpdump usa `-Z root` dentro do user namespace, sem exigir root no host.

O script `legacy/netns_virtual_env.sh` permanece apenas como referência manual.
A suíte verifica integridade e conclusão; cobertura de falhas, desempenho e
encerramento ainda precisa ser ampliada. Última validação registrada: 8/8 testes
passaram; detalhes e limites no contexto do repositório.
