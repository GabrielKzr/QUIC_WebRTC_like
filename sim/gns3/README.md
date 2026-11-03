# Validação final da integração no GNS3

Estado em 2026-11-03: planejada, ainda não executada. Esta é a última etapa do
[plano de validação](../../docs/plans/ice-quic-integration.md), após os cenários
correspondentes passarem no Patchbay.

## Preparação

- Dois peers atrás de NATs separados e um ISP acessível pelos dois lados.
- STUN/TURN e signaling no ISP, mantendo negociação separada dos dados do túnel.
- Reutilizar binários e configurações dos testes aprovados; registrar versões,
  IPs, portas, regras NAT e configuração dos serviços.
- Distinguir os cenários com preservação de porta daqueles com remapeamento real.

## Casos a reproduzir

1. ICE direto com STUN, incluindo porta pública diferente da privada.
2. TURN forçado, comprovando que o par selecionado usa relay.
3. Fluxo completo de CHOWNAT sobre ICE, direto e relay.
4. Fluxo completo de QUIC sobre ICE: handshake, dados e shutdown.
5. Falhas e manutenção aplicáveis, com prazo de erro e limpeza dos recursos.

## Evidências e aceite

Salvar a topologia, configurações, comandos, versões, logs dos peers/serviços,
pares selecionados e capturas nos lados privado e público. Exigir conteúdo
íntegro nos dois sentidos, caminho esperado, encerramento normal e códigos de
saída corretos. Compilação ou conectividade parcial não bastam para aprovação.

Os arquivos antigos em chownat_basic/ e topology_config/ são referências do
laboratório anterior; não comprovam execução de CHOWNAT/QUIC sobre ICE.
