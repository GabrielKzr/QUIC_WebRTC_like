# Dependências do build

O CMake baixa fontes com versões fixadas e SHA256, compila e instala dentro de
`build/<preset>/_deps/`. Não instala no sistema nem em `~/.local`.

| Dependência | Versão | Uso |
| --- | --- | --- |
| OpenSSL | 4.1.0 | QUIC, libnice e coturn; bibliotecas estáticas com PIC |
| libnice | 0.1.21 + patch do repo | ICE e remoção TURN após stale nonce |
| libevent | 2.1.12-stable | Dependência estática do coturn |
| coturn | 4.6.3 | STUN/TURN dos testes; credenciais em memória |
| libpcap | 1.10.5 | Opcional, captura de pacotes |
| tcpdump | 4.99.5 | Opcional, sem troca de usuário nos namespaces |
| Meson | 1.12.1 | Preparado em `_deps` se ausente no PATH |
| glib-mkenums | GLib 2.56.4 | Ferramenta Python isolada, preparada se ausente |
| Patchbay | crate 0.7.0 | Cargo compila o runner com Cargo.lock |

OpenSSL e libnice são preparados para os consumidores das bibliotecas.
Coturn é preparado quando algum teste declara `turnserver` como dependência.
Ele usa configure/Make nativo com um [patch para TMPDIR](../patches/coturn/README.md).
O CTest coloca os executáveis gerenciados antes do PATH herdado, incluindo nos
scripts ISP e nos testes de falha locais. Os testes de rede dependem do runner
Rust e das ferramentas selecionadas, inclusive ao compilar um target individual.
`ctest` executa testes; execute o build antes dele.

```sh
cmake --preset all
cmake --build --preset all -j4
ctest --preset all
```

Para construir também as ferramentas PCAP:

```sh
cmake --preset all -DBUILD_PCAP_TOOLS=ON
cmake --build --preset all -j4
PATCHBAY_PCAP=1 ctest --preset all
```

`BUILD_PCAP_TOOLS` seleciona a construção; a presença de `PATCHBAY_PCAP` seleciona
a captura durante a execução. Sem a opção de build, uma captura explicitamente
solicitada ainda pode usar um tcpdump compatível do PATH.

Continuam externos: compiladores C/C++, Make, Perl, Python3, Rust/Cargo,
Ninja, patch, pkg-config, GLib/GIO/GThread >=2.54 com headers e dependências
transitivas, e ferramentas Linux ip/nft/tc/stdbuf. PCAP requer também Flex e Bison.
Meson e glib-mkenums existentes no PATH são reutilizados. Se ausentes, o build
baixa as versões fixadas e verificadas por hash, sem pip nem instalação no home.
O bootstrap do Meson requer Python >=3.10. Não é necessário configurar PYTHONPATH.
Os downloads dessas ferramentas ocorrem no build, depois do configure.
GnuTLS não é exigida pela libnice: ela usa o OpenSSL gerenciado. A execução de
rede exige suporte a user/network namespaces no host.

Requisitos externos ausentes são apontados no configure. Fontes dos projetos
externos são baixadas e compiladas durante o build; falhas dessas etapas são
erros do build, com logs em `_deps/<dependência>/src/*-stamp/`. Downloads ficam
em `_deps/downloads/` e podem ser reutilizados; o SHA256 é verificado também
para arquivos já presentes. Cargo mantém seu cache próprio e usa `--locked`.
A opção `DEPENDENCY_BUILD_JOBS` limita os jobs por dependência (padrão4);
ela pode ser ajustada no configure conforme os recursos da máquina.
O primeiro build precisa de acesso às fontes/crates ou caches previamente
populados; execuções incrementais reutilizam as dependências construídas.

O preset `long` usa o mesmo gerenciamento, em seu próprio diretório de build.
Compilar esse preset não executa o teste de 11 minutos.
