# QUIC_WebRTC_like

Projeto experimental para estudar comunicacao P2P atraves de NAT com UDP hole punching, com duas frentes tecnicas:

- CHOWNAT: mecanismo leve para abrir/manter caminho UDP e tunelar dados.
- QUIC/WebRTC-like: evolucao para uma camada de transporte segura e multiplexada, alinhada com o modelo usado em WebRTC.

## Visao geral da proposta

O objetivo e validar, em laboratorio, um caminho de comunicacao entre dois peers atras de NAT sem depender de relay de dados. A arquitetura foi separada em fases:

1. Estabelecer conectividade P2P com hole punching usando CHOWNAT.
2. Transportar dados de aplicacao de forma estavel via UDP tunelado.
3. Evoluir para um protocolo QUIC-like, com foco em handshake, estados de conexao e streams.
4. Integrar esse fluxo com o padrao de sinalizacao e negociacao esperado em cenarios WebRTC.

Contexto técnico, estado validado e pendências: [docs/repository-context.md](docs/repository-context.md).

## Estrutura do repositorio

- `chownat/`: implementacao original (C e Perl) usada para validacao inicial do hole punching.
- `sim/gns3/`: cenarios de laboratorio e configuracoes de topologia/rede.
- `source/`: base atual em C com CMake, separando biblioteca comum e testes.
- `modelando_protocolo/`: materiais de modelagem/estudo do protocolo.

## GNS3: configuracoes e laboratorio

O laboratorio em `sim/gns3/` contem os arquivos para montar o ambiente de NAT, clientes e servicos de apoio:

- `sim/gns3/topology_config/clients_cisco`: configuracao dos clientes em roteador Cisco.
- `sim/gns3/topology_config/clients_debian`: configuracao dos clientes Debian.
- `sim/gns3/topology_config/*_nftables.conf`: variacoes com nftables.
- `sim/gns3/topology_config/dhcp*`: arquivos de suporte para DHCP.
- `sim/gns3/chownat_basic/how_to_execute`: roteiro de execucao rapida do experimento basico.

Observacao pratica ja registrada no projeto: o cenario com Cisco foi validado com sucesso no fluxo basico de CHOWNAT, enquanto variacoes com nftables ainda apresentam instabilidades e exigem ajuste fino.

## Hole punching + WebRTC (proposta de arquitetura)

### 1) Descoberta e sinalizacao

- Um canal de sinalizacao troca IP:porta observados e parametros de sessao entre os peers.
- A sinalizacao nao carrega midia/dados finais; serve apenas para coordenar abertura de caminho.

### 2) Abertura de caminho UDP (hole punching)

- Cada peer envia trafego UDP de keep-alive para o endpoint remoto anunciado.
- O NAT de cada lado cria estado de traducao e permite trafego de retorno.
- O CHOWNAT implementa essa fase com mensagens de controle e tentativas repetidas.

### 3) Sessao de dados estilo WebRTC

- Com o caminho aberto, os peers passam a trocar dados de aplicacao.
- O alvo e aproximar o comportamento de um DataChannel (baixa latencia, resiliencia a perda, reconexao/retentativa).

### 4) Evolucao para QUIC

- Sobre o caminho UDP aberto, a camada QUIC-like fornece negociacao, framing e multiplexacao por stream.
- Esse modelo aproxima a pilha do que e usado em transportes modernos para comunicacao P2P segura.

## Uso do CHOWNAT

No fluxo basico (script Perl em laboratorio), a ideia e:

1. Subir servico TCP no host servidor (ex.: SSH).
2. Rodar CHOWNAT em modo servidor expondo o destino interno via UDP.
3. Rodar CHOWNAT em modo cliente apontando para o endpoint remoto.
4. Validar conexao fim a fim (ex.: cliente SSH atraves do tunel).

Exemplo resumido, conforme `sim/gns3/chownat_basic/how_to_execute`:

```bash
# servidor
perl chownat.pl -d -s 22 10.0.0.20 2222

# cliente
perl chownat.pl -d -c 44444 10.0.0.30 2222
```

No codigo C refatorado, os modulos centrais estao em `source/libs/protocols/chownat/src/chownat.c` e `source/libs/protocols/chownat/inc/chownat.h`, com logica de:

- inicializacao de socket UDP e timeouts,
- keep-alive,
- abertura/fechamento de sessao,
- callbacks de eventos de conexao.

## Inicio da implementacao QUIC para o cenario WebRTC + hole punching

No estado atual do `source/`:

- A base atual separa `udp_conn`, `chownat`, `ossl_quic` e `quic_like` em bibliotecas, com o agregador `protocols`.
- A API QUIC-like ainda esta em fase inicial (headers, tipos e constantes em `source/libs/protocols/quic_like/inc/quic_like.h`).
- A implementacao em `source/libs/protocols/quic_like/src/quic_like.c` esta majoritariamente esqueleto/comentada, servindo como ponto de partida para pacotes long/short header, handshake e controle de stream.
- A integração OpenSSL QUIC está em `source/libs/protocols/ossl_quic/` e participa do build atual.

## Build e testes

Para configurar todas as classes de testes atuais, compilar todos os alvos e
executar toda a suíte registrada no CTest (CMake 3.21 ou superior):

```sh
cmake --preset all
cmake --build --preset all -j 2
ctest --preset all
```

O preset usa `build/all/` e inclui as dependências de rede descritas em
[sim/patchbay](sim/patchbay/README.md). Para saída detalhada, use
`ctest --preset all -V`. Os programas de exemplo também são compilados;
o CTest executa os cenários automatizados registrados, que coordenam os peers.

Para compilar apenas um executável:

```sh
cmake -S . -B build -DTEST_NAME=test_chownat
cmake --build build -j 2
```

As bibliotecas atuais são `utils`, `udp_conn`, `chownat`, `ossl_quic`,
`quic_like` e o agregador `protocols`. OpenSSL é requerido pelo CMake atual;
`ENABLE_OSSLQUIC` e `quic_webrtc_lib` pertencem à configuração antiga.

Para regressões de rede, veja [sim/patchbay](sim/patchbay/README.md).

## Roadmap sugerido

1. Consolidar cenario GNS3 estavel (Cisco + Debian) com reproducibilidade.
2. Fechar maquina de estados de conexao CHOWNAT e telemetria de falhas NAT.
3. Implementar handshake minimo QUIC-like sobre caminho UDP ja aberto.
4. Adicionar streams bidirecionais basicas e controle simples de retransmissao.
5. Integrar sinalizacao estilo WebRTC para automatizar descoberta e setup P2P.
6. Comparar desempenho entre trilha QUIC-like propria e trilha OpenSSL QUIC.

## Status atual

Projeto em pesquisa aplicada/prototipo. O foco atual e transformar a validacao de hole punching em uma pilha de comunicacao mais proxima de WebRTC, com suporte progressivo a recursos de QUIC.
