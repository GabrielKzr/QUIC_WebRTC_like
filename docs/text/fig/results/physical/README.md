# Figura da topologia física

`physical_topology.svg` incorpora os paths dos ícones e contém um retângulo
branco opaco cobrindo todo o fundo. Não depende de imagens por URL, fontes
remotas ou da cor de fundo do visualizador Mermaid.

`physical_topology.png` é a versão raster com fundo branco, e
`physical_topology.pdf` preserva os elementos vetoriais para inclusão no LaTeX.

```latex
\begin{figure}[htbp]
    \centering
    \includegraphics[width=\textwidth]{fig/results/physical/physical_topology.pdf}
    \caption{Topologia da infraestrutura física utilizada nos testes.}
    \label{fig:physical_topology}
\end{figure}
```

Para regenerar com Python 3 e librsvg:

```sh
python3 docs/text/fig/results/physical/draw_physical_topology.py
rsvg-convert -w 2400 -h 1800 -o docs/text/fig/results/physical/physical_topology.png docs/text/fig/results/physical/physical_topology.svg
rsvg-convert -f pdf -o docs/text/fig/results/physical/physical_topology.pdf docs/text/fig/results/physical/physical_topology.svg
```

Endereços: logs/PCAPs A201935/B201944 e esclarecimentos do usuário sobre
roteador base e gateway LAN B. `eth0`/`eth1` são nomes ilustrativos.

Ícones: Material Design Icons, projeto Pictogrammers/Templarian,
https://github.com/Templarian/MaterialDesign. Fontes em `icons/`, com a
licença Apache 2.0 correspondente em `icons/LICENSE`.
