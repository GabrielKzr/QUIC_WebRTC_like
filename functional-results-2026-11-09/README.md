# Coleta funcional — 9 de novembro de 2026

Cópia das evidências usadas na seção de resultados de `../tem.tex`.
Este diretório está fora de `build/` para acompanhar o repositório no Git.
Não contém os executáveis nem as dependências compiladas dos testes.

## Retomar a análise em outra máquina

Não é necessário recompilar ou executar testes para abrir estes PCAPs.
Instale/abra Wireshark e use **File → Open**. Na raiz do repositório,
com Wireshark instalado via Flatpak, a primeira figura pode ser aberta com:

```sh
flatpak run org.wireshark.Wireshark \
  -r functional-results-2026-11-09/short/artifacts/network_ice_udp_transport_remap/pcap/peer_a/eth0.pcap \
  -Y 'frame.number == 22 || frame.number == 23'
```

Com instalação nativa, substituir `flatpak run org.wireshark.Wireshark`
por `wireshark`. A instalação Flatpak efetuada na máquina do laboratório
é local ao usuário e não é transferida pelo Git.

Os quadros 22/23 representam o Binding Request e sua resposta. Os números
são posições no arquivo original, não identificadores universais de pacotes.
Para as seis figuras, seguir o
[guia dos recortes](../docs/analysis/functional-pcap-figures-2026-11-09.md).

## Conteúdo e origem

- `short/`: suíte regular aprovada, 18/18 testes em 37,20 s.
- `long/`: teste QUIC/ICE/TURN aprovado em 663,814 s.
- `short/artifacts/` e `long/artifacts/`: logs dos peers/servidor,
  snapshots de regras e capturas; 221 PCAPs no total.
- `observations.json`: extração dos resultados e pares selecionados.
- `packet-index.json`: quadros usados na análise das figuras.
- `tcp-reassembly.json`: comparação dos quatro fluxos TCP locais
  reconstruídos, de 16384 bytes cada, com o payload de referência.
- `inspect_pcaps.py`: leitor auxiliar dos cabeçalhos Ethernet/IPv4/STUN/TCP
  usados nessa análise; localiza a coleta pela própria posição do script.
- `SHA256SUMS`: hashes para conferir esta cópia.

O [relatório de execução](../docs/analysis/functional-results-2026-11-09.md)
registra ambiente, comandos e limites das conclusões. Os caminhos absolutos
presentes nos logs brutos são registros históricos da máquina que executou
os testes; não precisam existir no computador usado para ler as capturas.

O usuário copiou a coleta de `build/functional-results-2026-11-09/` para
este diretório. Os 746 arquivos originais foram conferidos por SHA-256.
Depois foram ajustados os caminhos dos índices e a localização da coleta
no script auxiliar; os logs e PCAPs brutos permaneceram byte a byte iguais.

## Integridade e novas execuções

Para conferir os arquivos desta cópia:

```sh
cd functional-results-2026-11-09
sha256sum -c SHA256SUMS
```

O manifesto não inclui a si próprio. Se editar um arquivo listado ou
regenerar o índice, sua soma pode mudar; não alterar os PCAPs usados no TCC.

Uma nova execução pode mudar portas, identificadores, tempos e números
de quadro. Os guias e valores em `tem.tex` referem-se a esta coleta,
não aos arquivos gerados futuramente em `build/`. A presença da variável
`PATCHBAY_PCAP` habilita novas capturas, que substituem os artefatos do
respectivo teste no diretório de build, sem alterar esta cópia da raiz.
