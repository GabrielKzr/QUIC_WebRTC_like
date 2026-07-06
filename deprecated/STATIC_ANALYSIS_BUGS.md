# Análise Estática - Bugs Encontrados

**Data**: 2026-07-06  
**Escopo**: Análise estática do código-fonte (sem testes dinâmicos)  
**Nota**: Esta análise não requer múltiplos peers para validação

---

## 🔴 BUGS CRÍTICOS

### 1. **Exit chamado em callback de erro** 
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L100](source/libs/protocols/chownat/src/chownat.c#L100)

```c
int sent = sendto(udp_session->socket_fd, "\0", 1, 0, ...);
if(sent < 0) {
    DEBUG_PRINT("[ERROR] send %s\n", strerror(errno));
    exit(errno);  // ❌ BUG: Exit de emergência em biblioteca
}
```

**Problema**: Função de biblioteca chama `exit()` diretamente  
**Impacto**: Qualquer erro de send termina todo o programa abruptamente  
**Severidade**: CRÍTICA  
**Solução**: Retornar código de erro e deixar caller lidar com a situação

**Localizações adicionais**:
- [chownat.c#L528](source/libs/protocols/chownat/src/chownat.c#L528): `exit(errno)` em `chownat_tcp_bind()`

---

### 2. **Buffer Overflow - Inteiro não inicializado** 
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L128](source/libs/protocols/chownat/src/chownat.c#L128)

```c
static char buffer[4];  // ⚠️ Static, não inicializado
if(recv(session->socket_fd, buffer, 3, 0) < 0) { // recebe 3 bytes
    attempts++;
    continue;
}
buffer[3] = 0;  // Escreve no índice 3, válido
```

**Problema**: Buffer estático não inicializado pode conter lixo na memória  
**Impacto**: Comparações `strcmp(buffer, "03\n")` podem ler valores indefinidos  
**Severidade**: ALTA  
**Solução**: `static char buffer[4] = {0};`

**Localizações adicionais**:
- [chownat.c#L171](source/libs/protocols/chownat/src/chownat.c#L171): `char buffer[4];` (não inicializado)
- [chownat.c#L271](source/libs/protocols/chownat/src/chownat.c#L271): `char buffer[4];` (não inicializado)
- [ossl_quic.c#L248](source/libs/protocols/ossl_quic/src/ossl_quic.c#L248): `char buffer[4] = {0};` (OK)

---

### 3. **Static Buffer Reutilizado Sem Sincronização**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L442](source/libs/protocols/chownat/src/chownat.c#L442)

```c
static udp_conn_status_t chownat_tcp_recv(const udp_conn_t* conn) {
    static char msg[size-3];  // ⚠️ Static, compartilhado entre chamadas
    
    int recvd = recv(tcp_session->accepted_sock, msg, size-3, 0);
    
    // callback pode chamar udp_conn_recv que pode chamar tcp_recv novamente
    conn->udp_conn_callback(conn, CHOWNAT_TCP_DATA_SENT, msg, recvd);  // Buffer pode ser sobrescrito
```

**Problema**: Buffer estático pode ser sobrescrito se callback chamar recursivamente  
**Impacto**: Data corruption durante processamento de callbacks  
**Severidade**: CRÍTICA  
**Solução**: Usar buffer alocado na stack ou heap por chamada

---

### 4. **String não terminada - Off-by-one**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L130](source/libs/protocols/chownat/src/chownat.c#L130)

```c
static char buffer[4];
if(recv(session->socket_fd, buffer, 3, 0) < 0) { }
buffer[3] = 0;  // Correto
if(strcmp(buffer, "03\n") == 0) {  // Comparando 3 bytes + null
```

**Problema**: Recebe "03\n" (3 bytes) e compara com strcmp - pode ler além do buffer  
**Impacto**: Comportamento indefinido em strcmp  
**Severidade**: ALTA  
**Solução**: Usar `strncmp(buffer, "03\n", 3) == 0`

---

### 5. **Infinite Loop no Servidor - Sem Timeout**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L169](source/libs/protocols/chownat/src/chownat.c#L169)

```c
else if(ctrl->mode == 's') {
    DEBUG_PRINT("[DEBUG] Waiting a connection from the remote end\n"); 
    char buffer[4];
    
    while (1)  // ❌ Infinite loop sem proteção
    {
        if(recv(session->socket_fd, buffer, 3, 0) < 0) {
            chownat_udp_send_ka(conn);
            continue;  // Volta ao loop infinito
        }
        // ...
    }
}
```

**Problema**: Loop infinito sem escape seguro em hole_punching do servidor  
**Impacto**: Servidor pode travar esperando por conexão que nunca vem  
**Severidade**: ALTA  
**Solução**: Implementar contador de tentativas ou timeout

---

## 🟠 BUGS DE SEGURANÇA / MEMORY LEAKS

### 6. **Memory Leak - Contexto SSL não liberado em erro**
**Arquivo**: [source/libs/protocols/ossl_quic/src/ossl_quic.c#L91](source/libs/protocols/ossl_quic/src/ossl_quic.c#L91)

```c
data->ctx = SSL_CTX_new(OSSL_QUIC_client_method());
if(data->ctx == NULL) {
    DEBUG_PRINT("[ERROR] Error while creating ssl context\n");
    return UDP_CONN_ERR;  // ❌ Contexto anterior não liberado se reinit
}
```

**Problema**: Se `ossl_quic_init()` é chamado 2x sem `deinit()`, memory leak  
**Impacto**: Vazamento de SSL_CTX  
**Severidade**: MÉDIA  
**Solução**: Verificar e liberar contextos anteriores antes de criar novo

---

### 7. **File Descriptor Leak - TCP Session**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L69](source/libs/protocols/chownat/src/chownat.c#L69)

```c
static udp_conn_status_t chownat_deinit(const udp_conn_t* conn) {
    close(udp_session->socket_fd);
    if(tcp_session)
        close(tcp_session->socket_fd);  // Fecha apenas socket, não accepted_sock
    // ❌ tcp_session->accepted_sock não é fechado
```

**Problema**: `accepted_sock` não é fechado em deinit  
**Impacto**: Vazamento de file descriptor  
**Severidade**: ALTA  
**Solução**: 
```c
if(tcp_session->accepted_sock > 0)
    close(tcp_session->accepted_sock);
if(tcp_session->socket_fd > 0)
    close(tcp_session->socket_fd);
```

---

### 8. **Falta de Validação de Tamanho - Buffer Overflow Potencial**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L418](source/libs/protocols/chownat/src/chownat.c#L418)

```c
static udp_conn_status_t chownat_udp_send(const udp_conn_t* conn, void* buf, size_t nbytes) {
    if(nbytes > size-3)  // size = 1024, so limite é 1021
        return UDP_CONN_ERR;
    
    char outbuf[size];
    outbuf[0] = '0';
    outbuf[1] = '9';
    outbuf[2] = data->id;
    
    memcpy(&outbuf[3], data_in, nbytes);  // ✅ Seguro após validação
```

**Problema**: Validação correta, mas falta validação no `chownat_tcp_recv()` que usa `size-3` sem limite  
**Impacto**: TCP pode receber mais de 1021 bytes e causar buffer overflow  
**Severidade**: ALTA  
**Solução**: Adicionar validação em `recv()` do TCP

#### TODO: Analisar, mas não é pra dar problema


---

### 9. **Dupla Liberação Potencial - TCP Session**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L325](source/libs/protocols/chownat/src/chownat.c#L325)

```c
// em chownat_disconnect_send():
if(tcp_session) {
    close(tcp_session->socket_fd);
    tcp_session->socket_fd = -1;
    close(tcp_session->accepted_sock);
    tcp_session->accepted_sock = -1;  // ✅ Definido como -1
}

// Depois em chownat_deinit():
close(tcp_session->socket_fd);  // ⚠️ Se foi -1 antes, fecha -1 (EBADF)
```

**Problema**: Fechar FD já fechado retorna erro mas não trava  
**Impacto**: Erro silencioso, EBADF não é verificado  
**Severidade**: BAIXA  
**Solução**: Verificar `if(tcp_session->socket_fd > 0)` antes de close()

---

## 🟡 PROBLEMAS DE LÓGICA / DESIGN

### 10. **Race Condition - FD_SET em Multi-thread**
**Arquivo**: [source/libs/protocols/udp_conn/src/udp_conn.c#L51](source/libs/protocols/udp_conn/src/udp_conn.c#L51)

```c
FD_ZERO(&ctrl->read_fds);
FD_SET(udp_fd, &ctrl->read_fds);
FD_SET(tcp_fd, &ctrl->read_fds);

ready = select(max(udp_fd, tcp_fd)+1, &ctrl->read_fds, NULL, NULL, &ka_timeout);
```

**Problema**: `ctrl->read_fds` é compartilhado; se múltiplas threads acessarem, race condition  
**Impacto**: Select pode usar fd_set corrompido  
**Severidade**: MÉDIA  
**Solução**: Usar fd_set local ou adicionar mutex

---

### 11. **Variável Estática Compartilhada - Estado Global**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L128](source/libs/protocols/chownat/src/chownat.c#L128)

```c
static char buffer[4];  // Compartilhada entre múltiplas chamadas
```

**Problema**: Mesma variável estática usada em múltiplos contextos sem sincronização  
**Impacto**: Dados podem ser sobrescritos se múltiplos eventos ocorrem simultaneamente  
**Severidade**: MÉDIA  

---

### 12. **Falha na Validação de Retorno - setsockopt**
**Arquivo**: [source/libs/protocols/ossl_quic/src/ossl_quic.c#L156](source/libs/protocols/ossl_quic/src/ossl_quic.c#L156)

```c
if(setsockopt(udp_session->socket_fd, SOL_SOCKET, SO_REUSEADDR, &config->reuse, sizeof(config->reuse)) < 0) {
    perror("Erro ao configurar setsockopt\n");  // ❌ Não retorna erro
    close(udp_session->socket_fd);
    return UDP_CONN_ERR;  // OK aqui
}
```

**Problema**: `perror()` é usado mas deveria ser `DEBUG_PRINT()` para consistência  
**Impacto**: Mensagens de erro não respeitam configuração de debug  
**Severidade**: BAIXA

---

### 13. **Callback pode ser NULL**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L253](source/libs/protocols/chownat/src/chownat.c#L253)

```c
conn->udp_conn_callback(conn, CHOWNAT_UDP_CONNECTED, NULL, 0);  // ❌ Sem verificação de NULL
```

**Problema**: Se `udp_conn_callback` for NULL, segmentation fault  
**Impacto**: Crash da aplicação  
**Severidade**: ALTA  
**Solução**:
```c
if(conn->udp_conn_callback)
    conn->udp_conn_callback(conn, CHOWNAT_UDP_CONNECTED, NULL, 0);
```

---

### 14. **Infinite Loop Sem Saída - SSL Write**
**Arquivo**: [source/libs/protocols/ossl_quic/src/ossl_quic.c#L433](source/libs/protocols/ossl_quic/src/ossl_quic.c#L433)

```c
while (1);  // ❌ Loop infinito proposital?
```

**Problema**: Loop infinito após enviar mensagem no servidor QUIC  
**Impacto**: Servidor fica travado  
**Severidade**: CRÍTICA  
**Solução**: Remover ou implementar lógica adequada

---

### 15. **Falta de Proteção contra recv() negativo**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L362](source/libs/protocols/chownat/src/chownat.c#L362)

```c
int recvd = recv(udp_session->socket_fd, msg, size, 0);

if(recvd < 0) {
    DEBUG_PRINT("[ERROR] recv %s\n", strerror(errno));
    data->closed = 1;
    return UDP_CONN_ERR;
} else if(recvd < 3) {  // ❌ Pode ser 0 ou 1 ou 2
    DEBUG_PRINT("[DEBUG] Received keep-alive\n");
}
// ... depois acessa msg[2] sem verificação
else if(strncmp(msg, "02\n", 3) == 0) {  // ⚠️ Pode ler lixo se recvd < 3
```

**Problema**: Acessa dados além do que foi recebido  
**Impacto**: Leitura de memória não inicializada / buffer overread  
**Severidade**: ALTA

---

### 16. **Patch Length Mismatch - Lenght em Protocol**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L131](source/libs/protocols/chownat/src/chownat.c#L131)

```c
char* msg = "01\n";
sendto(session->socket_fd, msg, strlen(msg), 0, ...)  // strlen("01\n") = 3
// mas depois:
sendto(session->socket_fd, "03\n", strlen(msg), 0, ...)  // ⚠️ strlen(msg) = 3, "03\n" = 3, OK mas confuso
```

**Problema**: Usar `strlen(msg)` quando msg aponta para "01\n" mas envia "03\n" - funciona mas é error-prone  
**Impacto**: Fácil cometer erros em refatoração  
**Severidade**: BAIXA  
**Solução**: Usar constantes: `sendto(..., "03\n", 3, ...)`

---

## 🟠 PROBLEMAS DE ROBUSTEZ

### 17. **Sem Tratamento de Interrupção de Sinal**
**Arquivo**: [source/libs/protocols/udp_conn/src/udp_conn.c#L56](source/libs/protocols/udp_conn/src/udp_conn.c#L56)

```c
ready = select(max(udp_fd, tcp_fd)+1, &ctrl->read_fds, NULL, NULL, &ka_timeout);

if(ready < 0) {
    DEBUG_PRINT("[ERROR] select %s\n", strerror(errno));
    exit(errno);  // ❌ Exit em EINTR (signal interrupção)
}
```

**Problema**: EINTR (signal interruption) causa exit ao invés de retry  
**Impacto**: Sinais (SIGINT, etc) terminam programa  
**Severidade**: MÉDIA

---

### 18. **Falta de Inicialização - data->closed**
**Arquivo**: [source/libs/protocols/chownat/src/chownat.c#L47](source/libs/protocols/chownat/src/chownat.c#L47)

```c
chownat_init() {
    // ...
    data->closed = 1;  // Inicializa como TRUE
    return UDP_CONN_OK;
}
```

**Problema**: Logo após init, `is_closed()` retorna CLOSED, o que é confuso  
**Impacto**: Lógica contra-intuitiva: inicializado = fechado?  
**Severidade**: BAIXA  

---

### 19. **Arquivo Hardcoded - Certificados**
**Arquivo**: [source/libs/protocols/ossl_quic/src/ossl_quic.c#L120](source/libs/protocols/ossl_quic/src/ossl_quic.c#L120)

```c
if(SSL_CTX_use_certificate_file(data->ctx, "../common/server.crt", SSL_FILETYPE_PEM) <= 0) {
```

**Problema**: Caminho relativo hardcoded, não portável  
**Impacto**: Falha se executado de diretório diferente  
**Severidade**: MÉDIA  
**Solução**: Usar caminho absoluto ou variável de ambiente

---

### 20. **Sem Limpeza de Erro OpenSSL**
**Arquivo**: [source/libs/protocols/ossl_quic/src/ossl_quic.c#L406](source/libs/protocols/ossl_quic/src/ossl_quic.c#L406)

```c
if(!SSL_connect(data->conn)) {
    DEBUG_PRINT("[ERROR] Error trying to connect\n");
    ERR_print_errors_fp(stderr);  // Printa mas não limpa
    return UDP_CONN_ERR;
}
```

**Problema**: Fila de erros do OpenSSL não é limpa, pode acumular  
**Impacto**: Mensagens de erro antigas aparecem em próximas falhas  
**Severidade**: BAIXA  
**Solução**: `ERR_clear_error()` após tratar o erro

---

## 📊 RESUMO

| Severidade | Quantidade |
|------------|-----------|
| CRÍTICA    | 4         |
| ALTA       | 8         |
| MÉDIA      | 5         |
| BAIXA      | 3         |
| **TOTAL**  | **20**    |

---

## 🛠️ Recomendações Imediatas

1. **Remover `exit()` de todas as funções de biblioteca** - Deixar caller tratar erros
2. **Inicializar todos os buffers estáticos** - `static char buf[N] = {0};`
3. **Remover loops infinitos** - Adicionar contadores de tentativas
4. **Validar callbacks antes de chamar** - `if(callback) callback(...)`
5. **Adicionar sincronização** - Se usar threads, proteger `fd_set` e buffers estáticos
6. **Validar tamanhos em recv()** - Especialmente TCP
7. **Fechar FDs corretamente** - Verificar se `> 0` antes de close()

---

## 📝 Notas

- Análise feita **sem testes dinâmicos** (conforme solicitado)
- Baseada em inspeção de código estático
- Alguns bugs só manifestam em condições específicas (race conditions, edge cases)
- Recomenda-se uso de ferramentas: `cppcheck`, `clang-analyzer`, `valgrind`
