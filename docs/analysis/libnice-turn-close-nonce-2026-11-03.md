# TURN: nonce expirado no fechamento da libnice

**Atualização posterior — 2026-11-04:** a correção foi autorizada e integrada
em `patches/libnice`, com build CMake/Meson e regressão de nonce. Suíte18/18
passou; teste longo posterior foi interrompido a pedido do usuário.
A [memória consolidada](../memory/libnice-integration.md) registra a aplicação,
os controles e o estado atual. O texto abaixo preserva a investigação antes
dessa implementação; “sem alterar dependências” refere-se àquela etapa.

Investigação iniciada em 2026-11-03 e consolidada em 2026-11-04,
branch `udp_transport`, HEAD `f56c49e`.
O working tree contém alterações anteriores; esta investigação não mudou
código de produção, dependências instaladas ou configuração dos testes.

## Conclusão verificada

A libnice 0.1.21 não repete o pedido de remoção da allocation TURN quando
recebe `438 Stale nonce`. Ela libera seu estado local e conclui
`nice_agent_close_async()`, embora a allocation não tenha sido removida no
servidor. Isso foi reproduzido sem HAL, QUIC, signaling ou Patchbay.

O coturn 4.6.1 forneceu um nonce novo e aceitou a remoção quando o cliente
repetiu corretamente o pedido. Uma alteração experimental somente na cópia
da libnice em `/tmp` também resolveu a reprodução. A conclusão sobre esse
fluxo específico é falha no tratamento de remoção da libnice, não falha de
autenticação da aplicação ou do servidor observado.

## Relação com a sessão longa

O teste completo `network_quic_ice_turn_long` continua **reprovado**.
Sua última execução transferiu os 67 blocos, totalizando 1.097.728 bytes
íntegros por sentido, durante aproximadamente 660 segundos. Permissões,
allocations e canais foram renovados. No fechamento, aos 662 segundos,
o coturn respondeu `438`, sem confirmar `Refresh lifetime=0` ou remoção.

Esses resultados separam manutenção de sessão e liberação do relay:
o defeito encontrado no fechamento não demonstra falha da renovação normal.
Artefatos dessa execução foram copiados para
`/tmp/quic-turn-long-before-nonce-patch` antes da tentativa posterior.
`build/long/sim/patchbay/artifacts/network_quic_ice_turn_long` foi reutilizado
pela execução interrompida e não deve ser confundido com aquela evidência.

## Reprodução independente

Ambiente medido: Ubuntu, libnice do sistema 0.1.21, GLib/GIO 2.80.0,
coturn 4.6.1 extraído em `/tmp/quic-coturn`. Também foram compiladas as tags
upstream 0.1.21 e 0.1.24, sem instalar nenhuma delas no sistema.

O programa `/tmp/nice-nonce-deep/nice_only.c` usa somente NiceAgent e GLib/GIO:

1. Cria um contexto GLib, um agente e um stream com um componente.
2. Configura TURN UDP em loopback, credenciais válidas, relay obrigatório,
   ICE TCP e UPnP desabilitados; mantém recepção por callback ativa.
3. Aguarda gathering e exige um candidato relay.
4. Espera 2,2 segundos e chama `nice_agent_close_async()`.
5. Continua processando o contexto e mantém o agente vivo até a callback.

O servidor usa `--stale-nonce=1` para antecipar a expiração. O controle idêntico
usa `--stale-nonce=60`. Foram usados tanto contexto padrão quanto privado,
com o contexto correto definido como thread-default. Não foi necessário
outro peer, signaling, transferência de dados ou thread de aplicação.

Compilação da reprodução:

```sh
cc -Wall -Wextra -Werror /tmp/nice-nonce-deep/nice_only.c \
    -o /tmp/nice-nonce-deep/nice_only $(pkg-config --cflags --libs nice)
python3 /tmp/nice-nonce-deep/run_matrix.py
```

O runner precisa do coturn e suas dependências nos caminhos registrados acima.
As bibliotecas compiladas foram selecionadas exclusivamente por
`LD_LIBRARY_PATH`; o caminho carregado foi conferido com `ldd`.

### Resultados principais

| Biblioteca e condição | Callback concluiu | Servidor aceitou lifetime zero e removeu |
| --- | --- | --- |
| Sistema 0.1.21, nonce válido, contexto padrão/privado | Sim | Sim, ambos |
| Sistema 0.1.21, nonce expirado, contexto padrão/privado | Sim | Não, ambos |
| Upstream 0.1.21 compilada sem alterações, nonce válido | Sim | Sim |
| Upstream 0.1.21 compilada sem alterações, nonce expirado | Sim | Não |
| Upstream 0.1.21 com alteração experimental, nonce válido | Sim | Sim |
| Upstream 0.1.21 com alteração experimental, nonce expirado, ambos contextos | Sim | Sim, ambos |

O código de saída zero da reprodução informa candidato relay e callback
concluída. **Não comprova liberação no servidor.** O runner verifica
separadamente o log do coturn para `refreshed ... lifetime=0` e `delete`.
Nos casos sem remoção, essa ausência foi observada durante a janela do teste;
não se aguardaram os 600 segundos de expiração natural da allocation.

Também foi antecipada a primeira renovação normal, com
`--max-allocate-lifetime=6`, nonce de um segundo e fechamento aos 7,5 segundos.
Na 0.1.21 original, a renovação recebeu `438`, repetiu com nonce novo e obteve
sucesso; o fechamento posterior recebeu outro `438` e desistiu. Com a alteração
experimental, ambos os desafios foram resolvidos e a allocation foi removida.
O coturn registrou lifetime 600 após renovar; esse experimento não mede uma
allocation com duração efetiva de seis segundos.

## Prova separada do comportamento do servidor

Um cliente Python independente, sem libnice, criou uma allocation usando
autenticação de longo prazo e executou esta sequência:

```text
Allocate sem autenticação          -> 401 com REALM/NONCE
Allocate autenticado               -> sucesso, lifetime=600
espera de 2,2 segundos
Refresh lifetime=0 com nonce antigo -> 438 com novo NONCE
Refresh lifetime=0 com nonce novo   -> sucesso, lifetime=0
log do coturn                      -> allocation removida
```

O cliente validou MESSAGE-INTEGRITY nas respostas de sucesso. As respostas
de desafio observadas não continham esse atributo; não se afirma que sua
integridade foi verificada. O protocolo admite o desafio com nonce novo.
Fontes normativas: [RFC 8489, seção 9.2.4/9.2.5](https://www.rfc-editor.org/rfc/rfc8489.html#section-9.2.5)
e [RFC 8656, seção 8](https://www.rfc-editor.org/rfc/rfc8656.html#section-8).

## Causa no código e experimento de correção

Em `agent/conncheck.c`, `priv_map_reply_to_relay_refresh()` trata o erro 438:
copia a resposta contendo o novo nonce e reconstrói o Refresh. Entretanto,
`priv_map_reply_to_relay_remove()` chama `refresh_free()` para qualquer
resultado diferente de `STUN_USAGE_TURN_RETURN_INVALID`, inclusive
`STUN_USAGE_TURN_RETURN_ERROR` retornado para o desafio 438.

A cópia experimental da 0.1.21 reconhece esse erro na remoção, copia o novo
nonce e reutiliza `priv_turn_allocate_refresh_tick_unlocked()`. Esse helper
já preserva lifetime zero quando `cand->disposing` é verdadeiro.
Quatro cenários experimentais confirmaram remoção no servidor: controle
válido, nonce expirado nos dois contextos e renovação seguida de fechamento.

Esse patch é uma prova causal, não uma correção pronta para distribuição.
Ainda requer revisão de limites de tentativas, desafios malformados,
temporizadores, demais erros e cobertura de regressão upstream.
Ele existe somente em `/tmp/nice-nonce-deep/nonce-close-experiment.patch`;
não foi aplicado à biblioteca do sistema nem ao projeto.

A [documentação de nice_agent_close_async](https://libnice.freedesktop.org/libnice/NiceAgent.html#nice-agent-close-async)
permite ignorar o resultado assíncrono, pois a operação não reporta falha.
No experimento, até o resultado booleano interno do GTask foi verdadeiro
quando a remoção remota falhou. Conferir somente a callback mascara o caso.

## Versão 0.1.24: atualização simples não resolveu

A tag 0.1.24 foi compilada e executada com a mesma reprodução, nos dois
contextos. Com nonce expirado, o servidor respondeu 438 repetidamente e a
allocation não foi removida. O handler de remoção ainda contém a condição
problemática; ela também aparece no `master` consultado.

Houve uma diferença adicional nessa reprodução: `nice_agent_close_async()`
remove os streams, e `component_io_cb()` encontra o stream ausente e encerra
a source de recepção (`stream 1 destroyed`). A libnice então retransmite
e encerra por timeout. Com nonce válido, o servidor remove a allocation,
mas o cliente também termina por timeout interno. Com nonce expirado,
retransmite o pedido com o nonce antigo, sem conseguir remover.

Esse comportamento adicional foi observado no exemplo de um agente que
apenas fez gathering; não foi validado em todas as formas de integração.
Não se deve descrevê-lo como o mesmo caminho interno da 0.1.21, que recebe
o 438 e libera imediatamente seu estado.

## Histórico e relatos públicos

A condição que libera o estado diante de qualquer resposta reconhecida já
está no [commit 0556ec49, de 2019-02-19](https://github.com/libnice/libnice/commit/0556ec49f497b1046c74a9b1e7fdb0bd4b501372),
que introduziu o handler de remoção assíncrona. O suporte a nonce expirado
em outros caminhos é anterior: commits de 2008/2014 tratam renovação e
ChannelBind. Isso não garante tratamento equivalente no fechamento.

Na tag 0.1.24, `tests/test-turn.c` espera a callback de fechamento, mas não
confere liberação no servidor nem força expiração de nonce. Essa cobertura
não detecta o defeito demonstrado aqui; não foi auditada toda a CI upstream.

Foi possível consultar títulos e descrições de issues/MRs abertos e fechados
pela API do GitLab freedesktop. Buscas por `nonce`, `438`, `stale`,
`allocation`, `TURN`, `close` e `deallocat` não localizaram relato
confirmadamente idêntico. Os endpoints de comentários retornaram HTTP 401;
isso impede concluir que nunca foi discutido ou reportado.

Relatos relacionados, com escopos distintos:

- [Issue 64](https://gitlab.freedesktop.org/libnice/libnice/-/work_items/64):
  renovação TURN durante sessão, não remoção com nonce expirado.
- [Issue 84](https://gitlab.freedesktop.org/libnice/libnice/-/work_items/84):
  timeout/callback da remoção assíncrona, sem descrição explícita de 438.
- [MR 51](https://gitlab.freedesktop.org/libnice/libnice/-/merge_requests/51):
  introdução da remoção assíncrona com resposta/retransmissão.
- [MR 333](https://gitlab.freedesktop.org/libnice/libnice/-/merge_requests/333):
  consistência da remoção e correção de memória; não corrige esse handler.
- [MR 283](https://gitlab.freedesktop.org/libnice/libnice/-/merge_requests/283):
  resultado final trata espera por DNS no fechamento e opção `close-forced`;
  parte do trabalho de remoção foi separada na MR 333. A associação anterior
  nas notas não deve ser entendida como correção deste 438.

Nenhum issue foi aberto e nenhuma mensagem foi enviada a terceiros.

## Impacto e próximo passo sugerido

O encerramento local pode concluir sem liberar imediatamente o relay.
A allocation fica sujeita ao seu prazo restante no servidor. No teste longo,
a última renovação foi aos 541 segundos com lifetime 600; aos 662 segundos
restariam aproximadamente 479 segundos, se não houvesse outra remoção.
Essa duração é uma estimativa pelos logs, não uma espera observada até expirar.

A correção adequada deve tratar o desafio no fechamento da libnice e ter
um teste que confira o servidor. Aumentar o prazo do nonce ou desabilitar
a conferência do teste esconderia esse caso. A escolha de manter um patch,
trocar a dependência ou preparar um relato upstream ainda precisa ser
discutida com o usuário. Depois de uma correção aprovada, repetir o teste
longo para validar todo o fluxo; as reproduções curtas não o substituem.

## Artefatos da investigação

Diretório `/tmp/nice-nonce-deep/`:

- `nice_only.c`, `run_matrix.py`: reprodução sem código do projeto.
- `matrix-system.json`, `matrix-0.1.21-source.json`: controles originais.
- `matrix-system-refresh-438.json`: renovação normal resolve 438, close falha.
- `raw_turn.py`, `raw-client.log`, `raw-client-packets.json`: cliente separado.
- `matrix-0.1.24.json`: execução da versão 0.1.24.
- `nonce-close-experiment.patch`, `matrix-patched-0.1.21.json`: prova causal.
- `*.client.log`, `*.turn.log`: logs dos dois lados de cada cenário.
- `tracker-search-results.json`, `commit-*.json`: pesquisa upstream/histórico.

Os diretórios de build e as duas árvores de código upstream também ficaram
nesse diretório temporário. Nenhuma biblioteca foi instalada pelo experimento.
