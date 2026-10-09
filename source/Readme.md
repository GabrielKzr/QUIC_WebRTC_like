# Código C

O build principal fica na raiz do repositório:

```sh
cmake -S . -B build -DTEST_NAME=test_chownat
cmake --build build -j 2
```

Também é possível compilar esta pasta isoladamente com `cmake -S source -B build-source`.
As bibliotecas ficam em `common/` e `libs/protocols/`; executáveis em `tests/`.
Selecione um exemplo por `TEST_NAME` ou a classe por `BUILD_WEBRTC_TESTS=ON`.
OpenSSL é uma dependência obrigatória da configuração atual.

Para regressões automatizadas, veja [Patchbay](../sim/patchbay/README.md).
