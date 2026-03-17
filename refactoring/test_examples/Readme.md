Esta pasta é reservada para testes

# MBEDTLS

    -> A mbedtls foi uma tentativa de uso para implementação manual do QUIC, ela implementa TLS de forma simples e de fácil controle por callbacks, mas tem limitações em relação a aplicação do TLS (1.2 e 1.3) com sockets UDP, tornando impossível utilizar-lo apenas para a troca de chaves inicial do QUIC

# OPENSSL

    -> A OpenSSL tem uma implementação completa do QUIC que será utilizada para testes com HolePunching, a limitação existente é apenas por ser uma biblioteca muito pesada e com muitas informações adicionais, não sendo necessariamente ideal para um cenário de sistemas embarcados

    -> A alternativa mais interessante seria o sistema conectado a um PC ou roteador rodando o cliente udp_conn e transmitindo dados via interface tun/tap (ideias a serem desenvolvidas posteriormente
    )