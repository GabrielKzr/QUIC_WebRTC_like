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

## Estrutura do repositorio

- `chownat/`: implementacao original (C e Perl) usada para validacao inicial do hole punching.
- `gns3/`: cenarios de laboratorio e configuracoes de topologia/rede.
- `refactoring/`: base atual em C com CMake, separando biblioteca comum e testes.
- `modelando_protocolo/`: materiais de modelagem/estudo do protocolo.

## GNS3: configuracoes e laboratorio

O laboratorio em `gns3/` contem os arquivos para montar o ambiente de NAT, clientes e servicos de apoio:

- `gns3/topology_config/clients_cisco`: configuracao dos clientes em roteador Cisco.
- `gns3/topology_config/clients_debian`: configuracao dos clientes Debian.
- `gns3/topology_config/*_nftables.conf`: variacoes com nftables.
- `gns3/topology_config/dhcp*`: arquivos de suporte para DHCP.
- `gns3/chownat_basic/how_to_execute`: roteiro de execucao rapida do experimento basico.

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

Exemplo resumido, conforme `gns3/chownat_basic/how_to_execute`:

```bash
# servidor
perl chownat.pl -d -s 22 10.0.0.20 2222

# cliente
perl chownat.pl -d -c 44444 10.0.0.30 2222
```

No codigo C refatorado, os modulos centrais estao em `refactoring/src/chownat.c` e `refactoring/inc/chownat.h`, com logica de:

- inicializacao de socket UDP e timeouts,
- keep-alive,
- abertura/fechamento de sessao,
- callbacks de eventos de conexao.

## Inicio da implementacao QUIC para o cenario WebRTC + hole punching

No estado atual do `refactoring/`:

- Ja existe base da biblioteca comum (`quic_webrtc_lib`) com `udp_conn`, `chownat` e `quic_like`.
- A API QUIC-like ainda esta em fase inicial (headers, tipos e constantes em `refactoring/inc/quic_like.h`).
- A implementacao em `refactoring/src/quic_like.c` esta majoritariamente esqueleto/comentada, servindo como ponto de partida para pacotes long/short header, handshake e controle de stream.
- Ha tambem trilha opcional com OpenSSL QUIC (`refactoring/src/ossl_quic.c`), habilitada por `-DENABLE_OSSLQUIC=ON`, para testes comparativos.

## Build e testes (refactoring)

```bash
cd refactoring
cmake -S . -B build -DENABLE_OSSLQUIC=OFF
cmake --build build --target test_chownat
```

Para listar alvos disponiveis:

```bash
cmake --build build --target help
```

Observacao: o caminho de testes com OpenSSL QUIC ainda esta em ajuste no CMake dos testes e deve ser tratado como WIP.

## Roadmap sugerido

1. Consolidar cenario GNS3 estavel (Cisco + Debian) com reproducibilidade.
2. Fechar maquina de estados de conexao CHOWNAT e telemetria de falhas NAT.
3. Implementar handshake minimo QUIC-like sobre caminho UDP ja aberto.
4. Adicionar streams bidirecionais basicas e controle simples de retransmissao.
5. Integrar sinalizacao estilo WebRTC para automatizar descoberta e setup P2P.
6. Comparar desempenho entre trilha QUIC-like propria e trilha OpenSSL QUIC.

## Status atual

Projeto em pesquisa aplicada/prototipo. O foco atual e transformar a validacao de hole punching em uma pilha de comunicacao mais proxima de WebRTC, com suporte progressivo a recursos de QUIC.
