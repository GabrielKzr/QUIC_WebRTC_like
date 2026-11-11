# Texto sugerido para o capítulo de implementação

Substituir a subseção provisória “Infraestrutura controlada” em
`docs/text/src/5_implementation.tex` pelo trecho abaixo.

```latex
\subsection{Infraestrutura física controlada}
\label{sec:physical_infrastructure}

Além do ambiente baseado no PatchBay, foi preparada uma
infraestrutura com dois computadores e três roteadores físicos.
O objetivo dessa montagem é exercitar a implementação utilizando
interfaces e equipamentos reais de encaminhamento e tradução
de endereços, mantendo a organização lógica da topologia ICE
descrita na Seção~\ref{sec:ice_topology}.

Um roteador base, com endereço \texttt{10.0.0.1}, fornece a rede
\texttt{10.0.0.0/24}, à qual estão conectados o servidor de
serviços auxiliares e as interfaces WAN dos dois roteadores NAT.
O roteador A utiliza o endereço WAN \texttt{10.0.0.117}, enquanto
o roteador B utiliza \texttt{10.0.0.40}. Ambos disponibilizam
uma LAN com prefixo \texttt{192.168.80.0/24} e endereço de
gateway \texttt{192.168.80.1}. Essas LANs são segmentos separados,
apesar de utilizarem o mesmo endereçamento. Os peers A e B
utilizam, respectivamente, os endereços
\texttt{192.168.80.100} e \texttt{192.168.80.101}.

% Inserir aqui a figura da topologia criada pelo usuário.
% Usar \label{fig:physical_topology} imediatamente após a \caption.
% Se a figura utilizar eth0/eth1 como nomes genéricos de interfaces,
% informar na legenda que esses nomes são ilustrativos.

O primeiro computador acumula duas funções: executar o peer A
e hospedar os serviços auxiliares. Sua interface Ethernet
conecta o peer A à LAN do roteador A, enquanto a interface
Wi-Fi conecta o servidor à rede do roteador base, com endereço
\texttt{10.0.0.108}. Nesse servidor, o \texttt{coturn}
disponibiliza STUN/TURN por UDP na porta 3478, e o serviço
de sinalização utiliza TCP na porta 5555. O segundo computador
executa o peer B, conectado à LAN do roteador B.

Para separar as funções executadas no primeiro computador,
a interface Ethernet do peer A é movida para um
\textit{network namespace} próprio. Esse namespace recebe
uma rota padrão pelo roteador A. A separação impede que o
acesso ao servidor, hospedado no mesmo computador, seja
resolvido diretamente pela pilha de rede local, contornando
o roteador NAT. Diferentemente do ambiente PatchBay, nesse
caso o namespace isola uma interface física, enquanto o
encaminhamento e a tradução de endereços são realizados
pelos roteadores externos.

A execução é organizada por um script com papéis distintos
para os dois computadores. No papel A, são preparados os
serviços auxiliares, o namespace e a aplicação geradora de
dados. No papel B, são iniciados o peer remoto e um serviço
TCP de echo. Esse serviço permite verificar a integração
com aplicações TCP locais sem exigir alterações na
implementação do transporte.

O script também inicia capturas nas interfaces utilizadas
pelo peer A, pelo servidor e pelo peer B. Os arquivos PCAP
e os registros dos processos são armazenados por execução,
permitindo relacionar a negociação ICE, o estabelecimento
QUIC e a comunicação entre os gateways. Os resultados
obtidos nesse ambiente são apresentados na
Seção~\ref{sec:physical_results}.
```

## Referência no capítulo de resultados

Em `docs/text/src/6_results.tex`, acrescentar o label logo após o título
da seção já existente:

```latex
\section{Avaliação em infraestrutura física controlada}
\label{sec:physical_results}
```

## Complemento opcional

Se disponíveis, acrescentar os modelos e firmwares dos roteadores em uma
tabela de equipamentos. Não atribuir a classificação `Moderate` do PatchBay
aos roteadores físicos sem uma verificação específica desse comportamento.

O texto descreve a montagem e a execução. A transferência verificada de
dados, os endpoints selecionados, a análise dos pacotes e os limites das
capturas devem ser apresentados no capítulo de resultados.

# Proposta para a seção de resultados da infraestrutura física

Sugestão: usar três subseções. A configuração deve apenas identificar as
condições desta avaliação e remeter à montagem descrita na implementação.
Depois, apresentar a evidência de estabelecimento e transferência. Por fim,
comparar o comportamento funcional com o caso QUIC/ICE direto do PatchBay.

O exemplo abaixo usa uma única execução pareada:

- A: `sim/physical/artifacts/a-20261110-200717/`.
- B: `sim/physical/artifacts/b-20261110-200730/`.
- Endpoints selecionados: A `10.0.0.117:35538`, B `10.0.0.40:52875`.

Os horários dos diretórios diferem porque cada computador iniciou seu script
separadamente. O pareamento é sustentado pelos endpoints recíprocos dos logs,
e não apenas pela proximidade dos horários.

Não misturar as portas dessa execução com as do par A201935/B201944 usado
em exemplos anteriores. Os IPs da topologia são os mesmos, mas as portas mudam.

## Texto sugerido

Substituir a seção física provisória em `docs/text/src/6_results.tex` pelo
trecho abaixo. Manter uma única definição de `sec:physical_results` no TCC.

```latex
\section{Avaliação em infraestrutura física controlada}
\label{sec:physical_results}

A avaliação física examinou o estabelecimento de uma conexão
QUIC sobre ICE e a transferência de dados de uma aplicação TCP
entre computadores conectados por roteadores NAT físicos.
O objetivo foi verificar, nesse ambiente, a sequência funcional
exercitada pelo caso C10 do PatchBay: descoberta de endpoints,
seleção de caminho direto, estabelecimento QUIC e entrega
dos dados ao serviço remoto. A análise utiliza os registros
dos dois gateways e as capturas realizadas durante a execução.

\subsection{Configuração e identificação da execução}

A montagem segue a Seção~\ref{sec:physical_infrastructure},
com os dois gateways em LANs distintas e os serviços auxiliares
acessíveis pela rede do roteador base. O namespace do gateway A
separa sua interface Ethernet da interface utilizada pelo servidor,
de modo que o acesso aos serviços auxiliares percorra o roteador A.
O gateway B executa em outro computador, conectado ao roteador B.

O experimento utiliza o mesmo padrão binário de 16384 bytes
do teste de tunelamento TCP no PatchBay. Uma aplicação local
no gateway A envia os dados à entrada do túnel; no gateway B,
o serviço TCP de echo devolve o conteúdo recebido. Assim,
a comparação mantém o critério de integridade dos dados,
embora a infraestrutura de encaminhamento seja diferente.

% Se disponíveis, registrar aqui modelos/firmwares dos roteadores e
% sistemas operacionais dos dois computadores. Não copiar o ambiente
% Rocky Linux da coleta PatchBay sem conferir os equipamentos físicos.
% A figura da montagem já deve aparecer na implementação; referenciá-la
% aqui caso necessário, evitando inserir a mesma imagem novamente.

Para a análise detalhada, foram selecionados os registros
\texttt{a-20261110-200717} e \texttt{b-20261110-200730}.
Os endpoints informados em cada gateway são recíprocos,
permitindo associar os dois conjuntos de artefatos à mesma
comunicação. A Tabela~\ref{tab:physical_selected_endpoints}
apresenta os endereços locais e os endpoints reflexivos
selecionados nessa execução.

\begin{table}[htbp]
    \centering
    \caption{Endereçamento e endpoints selecionados na execução física analisada.}
    \label{tab:physical_selected_endpoints}
    \begin{tabular}{@{}p{0.20\textwidth}p{0.32\textwidth}p{\dimexpr0.48\textwidth-4\tabcolsep\relax}@{}}
        \toprule
        \textbf{Gateway} & \textbf{IP local} & \textbf{Endpoint reflexivo selecionado} \\
        \midrule
        A & \texttt{192.168.80.100} & \texttt{10.0.0.117:35538} \\
        B & \texttt{192.168.80.101} & \texttt{10.0.0.40:52875} \\
        \bottomrule
    \end{tabular}
\end{table}

Esses endpoints pertencem à rede externa às LANs dos gateways,
mas continuam sendo endereços privados da infraestrutura
controlada. Portanto, o experimento não representa uma
comunicação entre redes independentes pela Internet pública.

\subsection{Estabelecimento do caminho e transferência de dados}

O log do gateway A registra o estado ICE READY com o par
\texttt{10.0.0.117:35538} para \texttt{10.0.0.40:52875};
o gateway B registra os mesmos endpoints em ordem inversa.
Ambos identificam o caminho como direto/reflexivo.
Esses registros demonstram que o caminho foi selecionado
utilizando os endpoints externos aos dois NATs.

% LOGS: seleção do caminho ICE e estabelecimento QUIC.
% Arquivo: sim/physical/artifacts/a-20261110-200717/peer-a.log
% Linha 2: Peer A ICE READY: 10.0.0.117:35538 -> 10.0.0.40:52875; direto/reflexivo.
% Linha 12: [DEBUG] UDP transport BIO attached to OpenSSL QUIC
% Linha 13: [DEBUG] Connected to QUIC server
% Linha 14: [DEBUG] QUIC data stream 0 and control stream 4 established
%
% Arquivo: sim/physical/artifacts/b-20261110-200730/peer-b.log
% Linha 2: Peer B ICE READY: 10.0.0.40:52875 -> 10.0.0.117:35538; direto/reflexivo.
% Linha 12: [DEBUG] UDP transport BIO attached to OpenSSL QUIC
% Linha 13: [DEBUG] QUIC server listening
% Linha 14: [DEBUG] Connected to QUIC client
% Linha 15: [DEBUG] QUIC data stream 0 and control stream 4 established
% Os endpoints recíprocos associam os registros dos dois gateways.
% Os registros de conexão/streams corroboram o estabelecimento QUIC;
% apenas ICE READY não comprovaria a conclusão dessa etapa.

Nas capturas das interfaces LAN, os pacotes de cada gateway
são enviados ao endpoint traduzido do outro. Esse tráfego
inclui as verificações STUN entre os gateways e o estabelecimento
QUIC posterior. A observação nos dois extremos permite
relacionar o par escolhido pelo ICE à comunicação que
efetivamente atravessou os roteadores físicos.

% FIGURA FÍSICA 1: inserir aqui dois painéis, um por gateway.
% A: sim/physical/artifacts/a-20261110-200717/peer-a-lan.pcap
% B: sim/physical/artifacts/b-20261110-200730/peer-b-lan.pcap
% Filtro em ambos: udp.port == 35538 || udp.port == 52875
% Mostrar as verificações STUN e o início da conexão QUIC.
% Na LAN A, a origem local é 192.168.80.100 e o remoto é 10.0.0.40.
% Na LAN B, a origem local é 192.168.80.101 e o remoto é 10.0.0.117.
% Selecionar o início da conexão e destacar portas, origem/destino e
% identificação dos pacotes. Não atribuir as portas de outra execução.
% Sugestão de label: fig:physical_ice_quic.

Após o estabelecimento QUIC, os dois gateways registraram
16384 bytes enviados e recebidos e a conclusão do fechamento.
O gerador local também informou que a resposta do echo era
idêntica ao conteúdo enviado, seguida de EOF. A evidência
de integridade vem dessa comparação byte a byte, enquanto
as capturas permitem identificar o percurso e a sequência
de protocolos utilizados.

% LOGS: exemplo de encaminhamento dos dados TCP pelo backend QUIC.
% Arquivo: sim/physical/artifacts/a-20261110-200717/peer-a.log
% Linha 16: [DEBUG] Received 1024 bytes from TCP tunnel
% Linha 17: [DEBUG] Sending 1024 bytes through QUIC
% Linha 18: [DEBUG] Sent 1024 bytes through QUIC
% Essas linhas ilustram um bloco; o total é conferido no resumo final
% e a igualdade integral dos bytes é verificada pelo probe.
%
% LOGS: totais e fechamento nos dois gateways.
% Arquivo: sim/physical/artifacts/a-20261110-200717/peer-a.log
% Linha 96: [DEBUG] Received 0 bytes from TCP tunnel
% Linha 98: [DEBUG] ossl_quic_deinit()
% Linha 99: Peer A QUIC/ICE: handshake, 16384 bytes nos dois sentidos e fechamento completos.
%
% Arquivo: sim/physical/artifacts/b-20261110-200730/peer-b.log
% Linha 85: [DEBUG] ossl_quic_deinit()
% Linha 86: Peer B QUIC/ICE: handshake, 16384 bytes nos dois sentidos e fechamento completos.
%
% LOGS: comparação integral do payload e conclusão do serviço echo.
% Arquivo: sim/physical/artifacts/a-20261110-200717/probe.log
% Linha 1: OK: 16384 bytes íntegros e EOF
%
% Arquivo: sim/physical/artifacts/b-20261110-200730/echo.log
% Linha 1: Echo pronto
% Linha 2: Echo encerrado
% O probe compara a resposta byte a byte e verifica EOF. O resumo
% de cada peer corrobora os contadores e o fechamento da conexão.

\begin{quote}
\begin{verbatim}
OK: 16384 bytes íntegros e EOF
\end{verbatim}
\end{quote}

% Comprovação do conteúdo: probe.log do diretório A selecionado.
% Complementar com a última linha de peer-a.log e peer-b.log,
% que registra handshake, bytes nos dois sentidos e fechamento.
% A listagem acima já apresenta o resultado do probe; uma captura de
% terminal é opcional. Não alegar leitura do payload cifrado na PCAP.

A conclusão do serviço echo no gateway B e o EOF observado
pelo gerador no gateway A mostram que a verificação abrangeu
a entrega aos serviços TCP locais. Assim, a avaliação não se
limitou à obtenção do estado ICE READY ou à abertura da conexão
QUIC: incluiu o transporte e o retorno do conteúdo de aplicação.

\subsection{Comparação com o ambiente PatchBay e alcance dos resultados}

A Tabela~\ref{tab:physical_patchbay_comparison} relaciona as
verificações do caso C10 às observações do ambiente físico.
A comparação considera o comportamento funcional da integração,
mantendo o conteúdo transferido e o uso de um caminho direto
como pontos comuns entre os ambientes.

\begin{table}[htbp]
    \centering
    \small
    \caption{Correspondência funcional entre o caso C10 do PatchBay e a execução física analisada.}
    \label{tab:physical_patchbay_comparison}
    \begin{tabular}{@{}p{0.27\textwidth}p{0.34\textwidth}p{\dimexpr0.39\textwidth-4\tabcolsep\relax}@{}}
        \toprule
        \textbf{Aspecto} & \textbf{PatchBay -- C10} & \textbf{Infraestrutura física} \\
        \midrule
        Encaminhamento & Roteadores configurados em namespaces. & Roteadores e enlaces físicos. \\
        Negociação ICE & Seleção de caminho direto com candidatos reflexivos. & Seleção de caminho direto com endpoints externos aos dois NATs. \\
        Estabelecimento & Conexão QUIC sobre o caminho ICE selecionado. & Handshake QUIC concluído após a negociação ICE. \\
        Dados da aplicação & Echo binário de 16384 bytes comparado byte a byte. & Mesmo padrão e tamanho, com resposta íntegra e EOF. \\
        Encerramento & Conclusão do túnel e fechamento dos peers. & Fechamento registrado pelos dois gateways e conclusão do echo. \\
        \bottomrule
    \end{tabular}
\end{table}

Os resultados físicos mostram que o funcionamento observado
no caso direto do PatchBay também foi obtido com tradução e
encaminhamento realizados pelos equipamentos externos.
A mesma implementação integrou a negociação ICE, o transporte
QUIC e as aplicações TCP locais nesse arranjo.

O alcance dessa observação é restrito à topologia e aos equipamentos
utilizados. A coleta física analisada não exercita TURN obrigatório,
sessão prolongada, perdas induzidas ou mudança de rede.
Também não caracteriza sistematicamente o comportamento dos NATs
nem estabelece diferenças de latência ou vazão em relação ao
PatchBay. Esses aspectos exigiriam procedimentos e medições
específicos, além da verificação funcional realizada.
```

## O que acrescentar como comprovação

1. **Uma figura com dois painéis de PCAP**, logo após a descrição da
   negociação: uma captura da LAN A e outra da LAN B, usando somente
   a execução escolhida. Destacar STUN, endpoints e estabelecimento QUIC.
2. **O resultado do probe e dos dois peers**, após a descrição da transferência.
   O trecho `OK: 16384 bytes íntegros e EOF` já está no texto; pode permanecer
   como listagem. Os dois logs corroboram bytes e fechamento em ambos os lados.
3. **A tabela comparativa**, que relaciona as verificações ao C10 em vez
   de repetir apenas “funcionou”. Não usar a contagem de pacotes ou a duração
   total das execuções como comparação de desempenho.

Não é necessário inserir outra figura da topologia nesta seção se ela já
estiver no capítulo de implementação. Evitar usar as capturas menores do
gateway B de outras rodadas como se representassem esta execução.
