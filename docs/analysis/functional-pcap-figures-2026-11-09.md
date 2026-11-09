# Guia das figuras de comprovação — resultados funcionais

Os seis placeholders estão em `tem.tex`, junto do argumento que cada figura
sustenta. Não é necessário colocar imagens das 221 capturas. Escolher esses
recortes fornece uma sequência de evidências: descoberta → caminho direto
ou relay → entrega à aplicação → renovação → remoção.

Todos os caminhos abaixo são relativos à raiz do repositório. Abra os PCAPs
originais no Wireshark. Os números indicados são os números de quadro desses
arquivos; salvar somente os pacotes filtrados em outro PCAP pode renumerá-los.
Os filtros `frame.number` não dependem de o dissector reconhecer o protocolo.
Para a imagem, mostrar `No.`, `Time`, `Source`, `Destination`, `Protocol` e
`Info`; portas podem ser mostradas em colunas adicionais ou na árvore UDP/TCP.
Manter números de quadro e endereços legíveis no tamanho final do TCC.

Na análise inicial, os números, tipos STUN, atributos e payloads foram
conferidos diretamente nos registros PCAP Ethernet/IPv4. Posteriormente,
Wireshark/tshark 4.6.9 foram instalados via Flatpak no usuário desta máquina;
tshark confirmou os quadros 22/23 da Figura 1 e a GUI abriu essa captura.
As imagens das seis figuras ainda precisam ser produzidas pelo usuário.
`packet-index.json` preserva a extração na raiz da coleta.

A coleta foi copiada pelo usuário para `functional-results-2026-11-09/`,
fora do build ignorado, e a cópia dos 746 arquivos originais foi conferida
por SHA-256. Os PCAPs mantêm seus bytes e números de quadro originais.
Ver o README da coleta para validar a integridade e abrir os arquivos em casa.

## Figura 1 — porta privada, tradução NAT e descoberta STUN

Placeholder: `fig:result_stun_remap`, após a descrição da tradução 54960 → 38576.

Painel (a):

```text
functional-results-2026-11-09/short/artifacts/network_ice_udp_transport_remap/pcap/peer_a/eth0.pcap
```

Painel (b):

```text
functional-results-2026-11-09/short/artifacts/network_ice_udp_transport_remap/pcap/router-a/wan.pcap
```

Filtro em ambos:

```text
frame.number == 22 || frame.number == 23
```

- No quadro 22 da LAN: expandir UDP, mostrando origem `192.168.80.20:54960`
  e destino `10.0.0.1:3478`.
- No quadro 22 da WAN: a origem é `10.0.0.2:38576`.
- No quadro 23: expandir STUN → `XOR-MAPPED-ADDRESS`, mostrando
  `10.0.0.2:38576`.
- Destacar o mesmo Transaction ID `332ea69f478d618c323a166b` nos dois
  pedidos/respostas. Isso correlaciona as observações antes/depois do NAT.

Se a árvore de detalhes não couber em um único recorte, usar o quadro 22
selecionado na LAN e o 23 selecionado na WAN. A lista de pacotes deve mostrar
ambos em cada painel. A figura sustenta a tradução efetiva e a descoberta
do endpoint, não apenas a existência de uma regra configurada.

Para um recorte opcional dos checks do **mesmo teste**, usar WAN 28–29:
Request com USE-CANDIDATE e Success, Transaction ID
`cb807ac009c5e2e36c7585f6`, destino `10.0.0.3:64027`.
As portas do outro peer foram corroboradas por `-s.stdout.log` desse teste.

## Figura 2 — checks ICE seguidos pelo estabelecimento QUIC direto

Placeholder: `fig:result_ice_quic_direct`, após a descrição dos pacotes Initial.

```text
functional-results-2026-11-09/short/artifacts/network_quic_ice_tcp/pcap/router-a/wan.pcap
```

Filtro:

```text
(frame.number >= 25 && frame.number <= 32) || (frame.number >= 59 && frame.number <= 73)
```

Dividir em dois painéis:

1. Checks: selecionar quadro 26 e expandir STUN → atributos USERNAME,
   MESSAGE-INTEGRITY e USE-CANDIDATE. O quadro 27 responde à mesma transação
   `beb4ef3887495899d54b6074`. Quadros 30–31 mostram o check no outro sentido.
2. QUIC: selecionar quadro 59 e mostrar UDP Length 1208, correspondente
   a **1200 bytes de payload UDP**, e o cabeçalho QUIC Initial. Há Retry no
   quadro 61 e Handshake nos quadros 70 e 73. Se necessário, utilizar
   Analyze → Decode As → QUIC para as portas 49381/38368. O reconhecimento
   automático pode depender da versão/configuração do Wireshark.

Os endpoints são `10.0.0.2:49381` e `10.0.0.3:38368`. Confrontar com
`-c.stdout.log`, linhas que contêm `Local selected pair`,
`Remote selected pair` e `Peer A ICE READY`.

O filtro faz um recorte de duas etapas: existem pacotes entre os intervalos
mostrados. Nos quadros 35–58 aparecem controles da descoberta do backend,
incluindo o payload `30 31 0a` (`01\n`). Não interpretar o intervalo do
recorte como uma medida isolada de latência ICE ou QUIC.

A figura demonstra a ordem dos protocolos e os endpoints utilizados.
Sem chaves da sessão, não usar os pacotes Handshake/1-RTT para alegar leitura
de certificados ou do conteúdo de aplicação. Initial pode ser decodificado
por ferramentas compatíveis; isso não torna o restante da sessão legível.

## Figura 3 — caminho via relay e encapsulamento TURN

Placeholder: `fig:result_turn_data_path`, depois da análise de ChannelData.

```text
functional-results-2026-11-09/short/artifacts/network_quic_ice_turn_tcp/pcap/router-a/wan.pcap
```

Filtro:

```text
(frame.number >= 11 && frame.number <= 14) || frame.number == 30 || frame.number == 31 || frame.number == 64
```

Recortar em painéis para os detalhes não ficarem ilegíveis:

- Allocate: 11 é o pedido inicial, 12 o erro/desafio 401, 13 o novo pedido
  autenticado e 14 o sucesso. No 14, expandir `XOR-RELAYED-ADDRESS`
  (`10.0.0.1:49191`), `XOR-MAPPED-ADDRESS` (`10.0.0.2:47158`) e LIFETIME 600.
- ChannelBind: 30 é o pedido e 31 o sucesso. Expandir CHANNEL-NUMBER e
  XOR-PEER-ADDRESS, cujo destino é `10.0.0.1:49172`.
- ChannelData: 64 contém canal `0x4000`, Length 1200 e payload UDP total
  de 1204 bytes (cabeçalho UDP excluído). Se o Wireshark decodificar o
  conteúdo encapsulado, expandir também o QUIC Initial; os quatro bytes
  do cabeçalho ChannelData e o endereço do servidor já demonstram TURN.

A conexão WAN observada é `10.0.0.2:47158` ↔ `10.0.0.1:3478`, enquanto os
candidatos selecionados são os endereços relay. Confrontar com
`Peer A ICE READY ... relay/relay` e `TURN_RELEASED allocations=2` nos logs.

O primeiro 401 **não é evidência de credenciais incorretas**: aqui ele é
seguido por Allocate Success. Nos testes negativos, a evidência vem das
assertivas da fixture e da ausência de qualquer allocation bem-sucedida.

## Figura 4 — duas conexões TCP locais e os mesmos dados

Placeholder: `fig:result_tcp_application`, depois da descrição dos segmentos.

Painel (a):

```text
functional-results-2026-11-09/short/artifacts/network_quic_ice_tcp/pcap/peer_a/lo.pcap
tcp.port == 56538 && tcp.port == 44444
```

Selecionar quadro **86**: 16384 bytes de payload TCP; prefixo hexadecimal
`00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f`.
Esse filtro remove as tentativas anteriores recusadas antes de o listener
ficar disponível. Os FIN desse fluxo são os quadros 120 e 122.

Painel (b):

```text
functional-results-2026-11-09/short/artifacts/network_quic_ice_tcp/pcap/peer_b/lo.pcap
tcp.port == 52972 && tcp.port == 1234
```

Selecionar quadro **4**: 1024 bytes e o mesmo prefixo; o serviço devolve
o início do conteúdo no quadro 8. Os FIN são 39 e 40.

Em cada painel, usar Follow → TCP Stream e selecionar a exibição hexadecimal
para mostrar que há bytes nulos e que o conteúdo retorna no outro sentido.
Pode-se usar duas imagens dos detalhes dos segmentos e um recorte menor
do fluxo reconstituído. Não é necessário mostrar os 16 KiB na página.

### Comprovação da integridade total

Uma coincidência do prefixo visual não prova igualdade de todos os bytes.
Foi feita reconstrução dos quatro fluxos a partir dos números de sequência,
sem lacunas, com verificação de igualdade dos bytes sobrepostos. Todos
possuem 16384 bytes e são iguais ao padrão `bytes(i % 251 for i in range(16384))`.

```text
SHA-256: 4348e3b98e8a327b34ced39c1da9e67cdb4cd5e48e4d7960607a3ae403d35f0c
```

Metadados de cada fluxo e quadros de dados:
`functional-results-2026-11-09/tcp-reassembly.json`.
O runner também compara o conteúdo do echo byte a byte. No TCC, relatar
essa comparação no texto; a imagem ilustra a arquitetura e o padrão binário.

O quadro de 16 KiB foi capturado no **loopback**. Seu tamanho não deve ser
interpretado como MTU ou tamanho de datagrama na WAN. As duas conexões
usam números de sequência/portas locais próprios; não são um único TCP
fim a fim entre os gateways. Usar a Figura 2 para o trecho externo QUIC.

## Figura 5 — manutenção durante a sessão longa

Placeholder: `fig:result_turn_renewals`, após a tabela de transações.

```text
functional-results-2026-11-09/long/artifacts/network_quic_ice_turn_long/pcap/peer_a/eth0.pcap
```

Filtro:

```text
(frame.number >= 1312 && frame.number <= 1313) || (frame.number >= 2757 && frame.number <= 2760)
```

- 1312–1313: CreatePermission e sucesso, aproximadamente 241,29 s.
- 2757/2759: Refresh e sucesso, aproximadamente 540,54 s. Expandir
  LIFETIME 600 na resposta 2759.
- 2758/2760: ChannelBind e sucesso, na mesma etapa de renovação.

Se quiser mostrar a segunda renovação de permissão, acrescentar 2456–2457
(481,73 s), já descrita na tabela. Os tempos são relativos ao primeiro
registro **desse arquivo** e não exatamente iguais ao contador inteiro
de segundos do coturn. LIFETIME 600 é o prazo concedido pela resposta,
não a duração medida do experimento.

Abaixo ou ao lado da captura, pode ser usado este trecho de
`long/artifacts/network_quic_ice_turn_long/isp.stdout.log`:

```text
TURN_RENEWED allocations=2 channels=2
TURN_RELEASED allocations=2
```

O texto da seção já inclui esse trecho como `verbatim`: não é necessário
adicionar uma captura de terminal redundante. Continuidade dos dados
é sustentada pelas 67 comparações do runner e pelo encerramento dos peers
com 1097728 bytes enviados/recebidos em cada um.

## Figura 6 — erro 438, nonce atualizado e remoção efetiva

Placeholder: `fig:result_turn_stale_nonce`, depois da análise do fechamento.

Usar o **mesmo PCAP da Figura 5**:

```text
frame.number >= 3371 && frame.number <= 3374
```

| Quadro | Mensagem | Campo a destacar |
| --- | --- | --- |
| 3371 | Refresh Request | LIFETIME 0 e NONCE anterior. |
| 3372 | Refresh Error Response | ERROR-CODE 438 e NONCE devolvido pelo servidor. |
| 3373 | Novo Refresh Request | LIFETIME 0 e NONCE igual ao do quadro 3372. |
| 3374 | Refresh Success Response | LIFETIME 0. |

Transaction ID do primeiro par: `4127ab6b9d69af4eeb2ad43d`.
Transaction ID do segundo par: `c564076ffa9e24560e8400c6`.
Foi conferido nos bytes dos atributos que o NONCE de 3373 difere do de
3371 e coincide com o de 3372. A figura pode usar um painel com a lista dos
quatro pacotes e dois painéis menores com o erro e a resposta de sucesso.

Corroborar com `isp.stdout.log`: em aproximadamente 662 s, as duas sessões
registram `error 438: Stale nonce`, `refreshed ... lifetime=0`, exclusão
e `TURN_RELEASED allocations=2`. Nos logs dos peers, procurar
`priv_map_reply_to_relay_remove`: `res 2` seguido de `res 0`.

Isso comprova que o procedimento continuou após o erro e concluiu a remoção;
um retorno local de close sem essa transação não teria a mesma evidência.

## Falhas locais — utilizar os registros, sem inventar uma PCAP

Essas fixtures não usam o runner PatchBay e não foram incluídas na captura
por interface. A fonte é:

```text
functional-results-2026-11-09/short/LastTest.log
```

Procurar `signaling_unavailable:`, `turn_bad_credentials:`, `remote_absent:`
e `TURN stale nonce: server removal confirmed`. A mensagem
`expected failure and cleanup confirmed` só é emitida após conferir
estado FAILED, erro de transporte e restauração da contagem de FDs;
os critérios específicos de cada falha também são exigidos no código.

Para o TCC, a tabela de falhas e a explicação dos critérios são suficientes.
Se desejar uma listagem adicional, inserir um extrato desses resultados
logo após a tabela, removendo o ruído de debug e identificando que se trata
de **saída de teste**, sem apresentá-lo como análise de pacotes. Não usar
o desafio 401 do teste bem-sucedido da Figura 3 para ilustrar senha inválida.
