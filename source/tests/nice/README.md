# Teste local da libnice

Dois agentes no mesmo processo usam somente UDP em `127.0.0.1`, com portas
automáticas. Credenciais e candidatos são trocados em memória. Após ambos
chegarem a `READY`, cada peer envia um datagrama e verifica o conteúdo recebido.
O programa retorna erro se houver falha ou se o prazo de 5 segundos expirar.

```sh
cmake -S . -B build/nice -DBUILD_TESTING=ON -DBUILD_NICE_TESTS=ON -DBUILD_NETWORK_TESTS=OFF
cmake --build build/nice --target test_nice_local -j2
ctest --test-dir build/nice -R '^nice_local$' --output-on-failure -V
```

Executar os comandos na raiz do repositório. O teste usa diretamente a libnice,
sem Patchbay, STUN/TURN externos ou integração com `ice_udp_transport`/QUIC.
Este cenário valida apenas o caminho local, sem NAT.
