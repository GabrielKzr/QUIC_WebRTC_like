# Coturn gerenciado

O CMake compila coturn4.6.3 pelo `configure`/Make nativo, com OpenSSL e libevent
estáticos produzidos pelo próprio build. O binário criado pelo CMake upstream
foi rejeitado pela sondagem STUN dos testes neste ambiente; a causa exata dessa
diferença não foi isolada. O build nativo deve ser validado pelos testes de
protocolo, além da compilação.

`0001-respect-tmpdir.patch` faz o configure respeitar TMPDIR em vez de selecionar
`/var/tmp` incondicionalmente. O ExternalProject aponta TMPDIR para seu diretório
de fontes, gravável dentro do build. Isso permite configurar em sandbox sem
escrever fora da árvore gerenciada. O hash do patch faz parte do diretório da
fonte para que alterações não sejam reaplicadas sobre uma árvore já corrigida.

Somente `bin/turnserver` é construído e copiado para o prefixo privado. Não há
instalação de serviço, configuração de sistema ou chamada ao install upstream,
que também executaria ações desnecessárias ao laboratório. Bancos de dados,
Prometheus, systemd e SCTP ficam desabilitados. Os testes usam usuários definidos
na linha de comando.
