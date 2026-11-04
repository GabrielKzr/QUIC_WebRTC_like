# libnice corrigida no build

O projeto compila libnice **0.1.21** com
`0001-retry-turn-deallocation-on-stale-nonce.patch`. A tag upstream é baixada
do espelho oficial GitHub e conferida pelo SHA-256 definido em
`cmake_modules/libnice.cmake`.

Requer Meson, Ninja, GNU patch, pkg-config e headers/bibliotecas de
GLib/GIO/GThread >= 2.54 e GnuTLS >= 2.12. A libnice instalada no sistema
não é necessária. Meson é chamado pelo CMake; os comandos continuam:

```sh
cmake --preset all
cmake --build --preset all -j4
ctest --preset all
```

O primeiro build precisa acessar a URL do fonte. Download, extração, aplicação
do patch, compilação e instalação ocorrem em `build/<preset>/_deps/`.
O hash do patch faz parte do diretório da dependência: alterar seu conteúdo
provoca nova extração/compilação, sem reaplicar sobre uma árvore já corrigida.
Um build incremental sem alterações reutiliza a cópia pronta.

`Libnice::Nice` disponibiliza os headers instalados e o arquivo estático
`libnice.a`. Tanto `ice_udp_transport` quanto o teste direto `nice_local`
usam esse target; não carregam uma libnice sem patch por `LD_LIBRARY_PATH`.
GLib e GnuTLS continuam sendo dependências do ambiente. Nenhuma instalação
do sistema é modificada.

## Correção e limites

`priv_map_reply_to_relay_remove()` passa a repetir o Refresh com lifetime zero
quando recebe `438 Stale nonce`. Exige NONCE/REALM presentes e não vazios,
confere o limite da cópia e permite uma única repetição do desafio por
remoção. O construtor e o mecanismo de retransmissão existentes são reutilizados.
Demais erros e esgotamento das tentativas seguem a limpeza existente.

O campo de controle é privado à libnice; não altera sua API pública.
O patch também inicializa o lifetime usado pelo log desse handler, pois uma
resposta de erro não preenche esse valor. O sucesso da callback continua
sem garantir confirmação remota em todos os tipos de falha da rede.

A base 0.1.21 corresponde à reprodução validada. A 0.1.24 também falhou no
experimento independente; apenas atualizar para ela não resolveu o caso.
Histórico e evidências estão na
[investigação](../../docs/analysis/libnice-turn-close-nonce-2026-11-03.md).

## Regressão

```sh
ctest --preset all -R '^ice_failure_turn_stale_nonce$' -V
```

Esse caso local reutiliza `test_ice_failures`: cria uma allocation com
credenciais válidas, configura um peer ausente e expira o nonce antes do close.
Exige candidato relay, falha esperada de conectividade, limpeza local sem
descritores extras, resposta 438 e, no servidor, Refresh lifetime zero seguido
da remoção da allocation. O coturn é encerrado somente após essas verificações.

O teste de sessão prolongada permanece separado do preset `all`:

```sh
cmake --preset long
cmake --build --preset long -j4
ctest --preset long
```
