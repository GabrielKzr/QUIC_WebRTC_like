# Evidências dos resultados funcionais — 2026-11-09

Execução solicitada para preencher a seção de resultados em `tem.tex`.
Os fatos abaixo foram conferidos nesta sessão; não são resultados herdados
das notas históricas. A data usa o contexto do usuário (America/Sao_Paulo).

## Ambiente e escopo

- Rocky Linux 8.10, kernel `4.18.0-553.134.1.el8_10.x86_64`.
- HEAD: `f9df4a863c87254b64de151137ddab7403df7bdc`.
- Preservadas as alterações preexistentes em `cmake_modules/coturn.cmake`
  e `docs/memory/agent-notes.md`. Nenhuma alteração funcional nos protocolos.
- Build incremental de `build/managed` concluído antes dos testes.
- Dependências gerenciadas: OpenSSL 4.1.0, libnice 0.1.21 com o patch
  de remoção TURN após 438, coturn 4.6.3 e Patchbay 0.7.0.
- Não foi realizada nova execução em Ubuntu ou GNS3.

A primeira tentativa dentro do sandbox foi impedida por `Operation not
permitted` ao criar sockets/namespaces. Isso é uma limitação do ambiente,
não uma reprovação funcional do transporte. Os resultados documentados
abaixo vêm da execução subsequente com permissão para esses recursos.

## Comandos e artefatos

```sh
CCACHE_DIR=/tmp/quic-functional-ccache cmake --build build/managed -j2
NICE_DEBUG=all G_MESSAGES_DEBUG=libnice PATCHBAY_PCAP=1 \
  ctest --test-dir build/managed --output-on-failure \
  --output-junit /tmp/quic-functional-short.xml \
  --output-log /tmp/quic-functional-short.log
```

Os artefatos foram preservados antes da execução prolongada em
`functional-results-2026-11-09/`, na raiz do repositório, após cópia feita
pelo usuário para incluí-los no Git. A cópia dos 746 arquivos originais foi
conferida por SHA-256 antes dos ajustes dos caminhos nos índices; PCAPs e
logs brutos foram mantidos sem alterações:

- `short/LastTest.log`: saída completa dos testes locais e resumo CTest.
- `short/ctest.xml` e `short/ctest.log`: resultados e tempos do executor.
  O XML limita o tamanho da saída; consultar `LastTest.log` para os detalhes.
- `short/artifacts/<teste>/`: stdout/stderr dos peers e ISP, snapshots nft,
  captura PCAP e metadados de interfaces.
- `short/pcap-validation.json`: leitura das 204 capturas pelo tcpdump e
  presença de SYN, payload e FIN nos loopbacks dos seis cenários de túnel.
- `observations.json`: extração dos resultados e pares selecionados.

Os tempos CTest são duração do teste completo, incluindo preparação,
transferência e limpeza. Não são latências de conexão nem medidas de vazão.

## Suíte regular: fatos verificados

18/18 aprovados em 37,20 s: 12 cenários de rede, dois testes locais/unitários
e quatro casos de falha/regressão TURN. Uma execução por teste nesta coleta.

| Grupo no texto | Testes CTest e evidência |
| --- | --- |
| Chownat sem ICE | `network_chownat_tcp`, `network_chownat_udp`, `network_chownat_udp_close`; endpoints fixos e regras de mapeamento espelhado. |
| QUIC sem ICE | `network_openssl_quic_tcp`, `network_openssl_quic_udp`; comunicação direta pelos endpoints configurados. |
| ICE direto | `network_ice_udp_transport`; ambos os peers registram `SRV-RFLX` local e remoto. |
| Remapeamento | `network_ice_udp_transport_remap`; A: 54960 → 38576; B: 47643 → 64027; par `SRV-RFLX/SRV-RFLX`, troca bilateral conferida. |
| ICE via TURN | `network_ice_udp_transport_turn`; par `RELAYED/RELAYED`, duas allocations removidas. |
| Chownat/ICE | `network_chownat_ice_tcp` e `network_chownat_ice_turn_tcp`; respectivamente `SRV-RFLX/SRV-RFLX` e `RELAYED/RELAYED`. |
| QUIC/ICE | `network_quic_ice_tcp` e `network_quic_ice_turn_tcp`; respectivamente `SRV-RFLX/SRV-RFLX` e `RELAYED/RELAYED`. |

O verificador Rust compara o echo binário de 16384 bytes byte a byte e exige
EOF/conclusão do echo nos seis testes de túnel. Nos quatro testes de túnel
com ICE, as fixtures C exigem ainda um evento CONNECTED, 16384 bytes enviados
e recebidos por peer, estado fechado e deinit bem-sucedido. O exemplo Chownat
TCP sem ICE não confere todos esses retornos internos; sua evidência é o echo
validado pelo runner, os logs de fechamento e as capturas TCP locais.

Chownat UDP e QUIC UDP sem túnel validaram três rodadas por peer e fechamento.
O teste adicional Chownat UDP close exige 20 mensagens recebidas no cliente,
21 no servidor, fechamento e liberação sem erro. No transporte ICE puro,
conteúdo e tamanho das mensagens são comparados nas duas direções.

Nos três cenários TURN curtos, os logs do ISP contêm
`TURN_RELEASED allocations=2`; os logs dos peers contêm
`priv_map_reply_to_relay_remove ... res 0 with lifetime 0`.

### Falhas locais

`LastTest.log` registra os seguintes resultados. Todos os quatro casos
exigem `UDP_TRANSPORT_ERROR`, estado FAILED e retorno da contagem de FDs
ao baseline após fechamento, com aquecimento prévio da GLib/GTask.

| Caso | Evidência observada |
| --- | --- |
| `ice_failure_signaling_unavailable` | Conexão recusada, uma chamada da callback, falha e cleanup confirmados. |
| `ice_failure_turn_bad_credentials` | Coturn responde 401, nenhuma allocation criada, nenhum candidato relay, falha e cleanup confirmados. |
| `ice_failure_remote_absent` | Candidato remoto configurado sem respondedor, falha e cleanup confirmados antes do limite de 8 s da fixture. |
| `ice_failure_turn_stale_nonce` | Allocation obtida com credenciais válidas; espera de 2,2 s com nonce de validade 1 s; erro 438, remoção com lifetime zero e exclusão no servidor confirmados. |

Esses casos usam temporizadores ICE reduzidos apenas na fixture. Não
representam interrupção de sessão QUIC/Chownat estabelecida nem ensaio
com os prazos padrão de falha da libnice. TURN forçado não prova fallback
automático após falha direta. As capturas não provam identidade TLS,
e a coleta não introduziu perda/reordenação controladas.

## Teste prolongado: fatos verificados

Habilitado temporariamente no mesmo diretório de build para reutilizar
as dependências e o runner já compilados:

```sh
CCACHE_DIR=/tmp/quic-functional-ccache \
  cmake -S . -B build/managed -DBUILD_LONG_TESTS=ON
CCACHE_DIR=/tmp/quic-functional-ccache \
  cmake --build build/managed --target test_quic_ice_turn_long patchbay_runner -j2
NICE_DEBUG=all G_MESSAGES_DEBUG=libnice PATCHBAY_PCAP=1 \
  ctest --test-dir build/managed -R '^network_quic_ice_turn_long$' \
  --output-on-failure --output-junit /tmp/quic-functional-long.xml \
  --output-log /tmp/quic-functional-long.log
CCACHE_DIR=/tmp/quic-functional-ccache \
  cmake -S . -B build/managed -DBUILD_LONG_TESTS=OFF
```

`network_quic_ice_turn_long` aprovado em 663,814 s (tempo do caso CTest;
663,83 s para o comando completo). O cache foi restaurado para
`BUILD_LONG_TESTS=OFF` após preservar os artefatos.

- Par `relay/relay` nos dois peers.
- 67 blocos de 16384 bytes comparados pelo runner, enviados com intervalo
  de 10 s a partir do segundo bloco: 1097728 bytes por sentido.
- Ambos os peers confirmaram os contadores e o fechamento.
- Coturn registrou seis CREATE_PERMISSION bem-sucedidos: dois iniciais,
  duas renovações em 241 s e duas em 481 s.
- Em 540 s, duas allocations foram renovadas com lifetime 600 e dois
  CHANNEL_BIND adicionais foram processados com sucesso.
- Servidor confirmou `TURN_RENEWED allocations=2 channels=2` e
  `TURN_RELEASED allocations=2`.
- No fechamento, ambas as sessões receberam 438 Stale nonce. Em cada peer,
  `priv_map_reply_to_relay_remove` registrou `res 2` seguido de `res 0`,
  e o servidor confirmou refresh lifetime zero e remoção.
- Logs de cada peer contêm 28 mensagens de envio STUN de manutenção.
  Isso não isola o efeito dos keep-alives da sessão, ICE e QUIC.
- As 17 capturas do prolongado também foram lidas pelo tcpdump sem erro.
  Total da coleta: 221 PCAPs (204 curtos + 17 prolongado).

Artefatos adicionais em `functional-results-2026-11-09/long/`:
`LastTest.log`, `ctest.xml`, `ctest.log`, `summary.json` e
`artifacts/network_quic_ice_turn_long/`. Os resultados extraídos também
foram incorporados a `observations.json` na raiz da coleta.

As oito tabelas em `tem.tex` usam `tabular`, sem tabularx. Ambientes,
chaves, unicidade dos labels e número de colunas foram conferidos
estaticamente. O documento LaTeX não foi compilado: não há compilador
LaTeX disponível nesta sessão.

## Revisão da análise e preparação das figuras

A pedido do usuário, `tem.tex` foi reescrito para relacionar os resultados
às transações e ao conteúdo das capturas, substituindo as tabelas repetitivas
de aprovação por análise dos endpoints, dos fluxos TCP e do ciclo TURN.
Não houve nova execução dos testes nessa revisão editorial.

Foram conferidos nos PCAPs os números dos quadros, os Transaction IDs,
XOR-MAPPED/RELAYED/PEER-ADDRESS, USE-CANDIDATE, LIFETIME, código 438 e mudança
do NONCE. Seis placeholders compiláveis foram inseridos junto dos argumentos
que as imagens devem sustentar. Os arquivos, filtros e campos para as imagens
estão em `docs/analysis/functional-pcap-figures-2026-11-09.md`.

Além da comparação original do runner, os quatro fluxos unidirecionais das
duas conexões TCP locais de `network_quic_ice_tcp` foram reconstruídos pelas
sequências TCP das capturas de loopback. Todos contêm 16384 bytes sem lacunas,
com sobreposições consistentes, e são idênticos ao padrão `i % 251` usado
pelo gerador. SHA-256 comum:

```text
4348e3b98e8a327b34ced39c1da9e67cdb4cd5e48e4d7960607a3ae403d35f0c
```

Resultados em `functional-results-2026-11-09/tcp-reassembly.json`.
`packet-index.json` contém a extração dos quadros usados nas figuras;
`inspect_pcaps.py` preserva o leitor usado para inspecionar os registros
Ethernet/IPv4, STUN e TCP. Na análise inicial não havia tshark/Wireshark;
posteriormente, a versão 4.6.9 foi instalada via Flatpak no usuário e
validou os quadros 22/23 da primeira figura. Não foram geradas imagens
para o TCC. Os filtros por
`frame.number` referenciam os PCAPs originais, sem exportação/renumeração.

A revisão distingue o desafio TURN 401 normal do teste de senha inválida,
o tamanho do segmento de loopback da MTU WAN e a visualização do prefixo
do payload da comparação de todos os bytes. Os testes locais negativos
não possuem PCAP produzida pelo runner; sua evidência permanece nos logs
e nas assertivas das fixtures. Os seis placeholders não exigem arquivos
de imagem para compilar; a estrutura LaTeX foi conferida estaticamente.
